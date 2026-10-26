# Exporting to glTF

The editor can export the world as a binary glTF file (`.glb`) to use in
Blender, Godot, Unity, three.js or any other tool that reads glTF 2.0.

## Usage

Click the **Export** button in the editor toolbar (the tray with an up arrow),
or press **Ctrl+E**. The world is written to `assets/world.glb` and a popup
says whether the export succeeded.

Programmatic export is available via `export_world_gltf` in
`catedu/scene/gltf_export.hpp`.

## What's in the file

- **Scenes.** The outdoor world is the default scene (scene 0). Every interior
  gets a scene of its own, named after its building, e.g.
  `building interior (8, -4)`.
- **Directional Light (`KHR_lights_punctual`).** The root glTF declares the
  `KHR_lights_punctual` extension containing a directional sun light matching
  the world's `time_of_day` and sun angle.
- **Scene Extras.** Scene 0 carries world metadata in its `extras` object:
  - `time_of_day`: float value representing the hour of the day `[0..24)`.
  - `sun_light`: contains `direction` `[x, y, z]` (mirrored for glTF coordinates)
    and linear RGB `color` `[r, g, b]` matching the day/night/sunset palette.
  - `script`: serialized array of script graph nodes, each specifying:
    - `type`: node type (`event`, `say`, `yesno`, `teleport`, `give_item`)
    - `event`: trigger event (`start`, `enter`, `yes`, `no`, or `none`)
    - `text`: dialogue text or question prompt

  Example scene `extras`:
  ```json
  {
    "time_of_day": 14.5,
    "sun_light": {
      "direction": [0.2588, 0.9659, -0.5],
      "color": [0.9, 0.9, 0.8]
    },
    "script": [
      {"type": "event", "event": "start", "text": ""},
      {"type": "say", "event": "none", "text": "Welcome to Greenville!"}
    ]
  }
  ```

- **Nodes.** Each scene has a `ground` node and one node per placed object,
  named after its type (`building`, `road`, `tree`, ...) or, for props, after
  the prop ID (`barrel`, `bench`, ...). The object node holds the object's
  position; its children are the boxes and models it's drawn with.
- **Object Extras.** Object nodes carry the editor properties in `extras`, so the
  game logic can be rebuilt in another engine:

  ```json
  {"type": "building", "x": 8, "y": -4, "floors": 3, "interior_scene": 1, "interior_node": 7}
  {"type": "prop", "x": 2, "y": 5, "floors": 0, "id": "barrel"}
  ```

  `x` and `y` are the editor's grid coordinates. `interior_scene` is the index
  of the scene holding the building's interior. When `single_scene_interiors` is
  enabled, `interior_node` is the node index of the interior root in scene 0.
  Blender shows extras as custom properties, and Godot imports them as node
  metadata.
- **Meshes and materials.** Geometry is shared: every box is one cube mesh
  instanced with a transform, and each source model is stored once. Textures
  are embedded as PNGs. Object colors become `baseColorFactor`, converted from
  sRGB to linear as glTF requires.

## Export Options

The exporter accepts optional configuration via `GltfExportOptions`:

```cpp
struct GltfExportOptions
{
    bool single_scene_interiors = false;
};

bool export_world_gltf(const char *path, World &world,
                       const GenResources &resources,
                       GltfExportOptions options = {});
```

- **`single_scene_interiors`**: When set to `true`, each building interior is
  placed as an offset root node directly in scene 0 (translated along X by
  `interior_index * 64.0f` and Y by `-32.0f`). This allows single-scene
  importers like Godot and Unity to load all interior models and geometry from
  the default scene without needing multi-scene import support.

## Coordinate system

The engine's world is left-handed (looking down +Z, +X is to the right), while
glTF is right-handed. The exporter mirrors the Z axis and flips the triangle
winding, so the exported scene looks the same as in the editor, not mirrored.
A tile at editor position `(x, y)` ends up at glTF `(x, height, -y)`.

## Implementation

The exporter (`catedu/scene/gltf_export.cpp`) doesn't reimplement how objects
look. Rendering goes through `GenSink` (`catedu/genobj/render.hpp`): the
functions that describe objects emit `(model, matrix, color)` draws, and the
renderer draws them while the exporter records them as nodes. New object types
show up in the export without changes to the exporter.

Source geometry and textures are read from the model's glTF file, found with
`find_model_source` (`catedu/resources/resources.hpp`). No GPU data is read
back, which is why the unit tests in `tests/unit/test_gltf_export.inl` can
export without a graphics context.

## Limits

- **Fixed output path.** The editor UI export always goes to `assets/world.glb` and
  overwrites the previous export.
- **Web build.** In the WASM build the file is written to the browser's
  in-memory filesystem and can't be downloaded directly yet.
- **Textures must be PNG.** Other image formats are skipped with a warning on
  stderr; the model is still exported, untextured.
- **One primitive per model.** Like the renderer, the exporter only reads the
  first primitive of the first mesh of each source model.

## Next steps

1. **Download in the web build.** Offer the `.glb` as a browser download
   instead of writing it to the in-memory filesystem.
2. **Choose the output path.** A save dialog on desktop, or at least a
   timestamped file name so exports don't overwrite each other.
3. **Godot and Unity import helpers.** A small import script per engine that
   reads `extras` and swaps object nodes for engine prefabs (spawn points for
   `player`, NPCs with their dialogue, doors that load `interior_scene` /
   `interior_node`).
4. **Merge static geometry.** An option to merge each object's boxes into one
   mesh per material, for engines that handle many small nodes poorly.
