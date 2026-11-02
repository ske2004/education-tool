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
  {"type": "npc", "x": 4, "y": 4, "floors": 1, "id": "mayor"}
  ```

  `x` and `y` are the editor's grid coordinates. `id` is emitted for props and
  any named object (`obj.id[0] != '\0'`, such as named NPCs and items).
  `interior_scene` is the index of the scene holding the building's interior.
  When `single_scene_interiors` is enabled, `interior_node` is the node index of
  the interior root in scene 0. Blender shows extras as custom properties, and
  Godot imports them as node metadata.
- **Meshes and materials.** Geometry is shared: by default, every box is one cube
  mesh instanced with a transform, and each source model is stored once. When
  `merge_object_meshes` is enabled, an object's box primitives are grouped by
  material and merged into a single mesh node per color/material. Textures are
  embedded as PNGs. Object colors become `baseColorFactor`, converted from sRGB
  to linear as glTF requires.

## Export Options

The exporter accepts optional configuration via `GltfExportOptions`:

```cpp
struct GltfExportOptions
{
    bool single_scene_interiors = false;
    bool merge_object_meshes = false;
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
- **`merge_object_meshes`**: When set to `true`, all box draws within an object
  (`cube`, `cube_shaded`, `cube_window`) that share the same material/color are
  transformed by their local `Matrix4` and combined into a single merged mesh
  node per color/material (while non-box 3D models remain regular child nodes).
  This significantly reduces the total glTF node count and draw calls for
  complex procedural objects like multi-floor buildings and castles.

## Engine Import Helper Scripts

Ready-to-use post-import scripts are provided in `scripts/` to automatically
reconstruct physics colliders, door triggers, spawn points, and lighting from
the exported `.glb`:

### Godot 4 (`scripts/godot_import_educore.gd`)

1. Copy `assets/world.glb` and `scripts/godot_import_educore.gd` into your
   Godot 4 project.
2. Select `world.glb` in the FileSystem dock, open the **Import** dock, set
   **Import Script -> Path** to `res://godot_import_educore.gd`, and click
   **Reimport**.
3. The script (`@tool extends EditorScenePostImport`) automatically:
   - Reads scene `extras` (`time_of_day`, `sun_light`, `script`), attaches them
     as scene metadata, and creates a `DirectionalLight3D` (`EducoreSunLight`).
   - Reads each object node's `extras` (`type`, `x`, `y`, `floors`, `id`,
     `interior_scene`, `interior_node`).
   - Creates `StaticBody3D` + `CollisionShape3D` (`BoxShape3D`) for `building`,
     `castle`, `wall`, `tree`, `water`, `prop`, and `ground`.
   - Creates `Area3D` door triggers (`DoorTrigger` in group `"educore_door"`)
     at building entrances storing `interior_scene` and `interior_node` metadata.
   - Adds a `Marker3D` (`PlayerSpawn` in group `"player_spawn"`) on `player`
     nodes and `Area3D` interaction triggers on `npc` and `item` nodes.

### Unity (`scripts/UnityEducoreImporter.cs`)

1. Place `scripts/UnityEducoreImporter.cs` inside an `Assets/Editor/` folder in
   your Unity project (alongside a glTF 2.0 importer such as glTFast or
   UnityGLTF).
2. When `world.glb` is imported, `UnityEducoreImporter` (`AssetPostprocessor`):
   - Configures or creates a directional `Light` (`EducoreSunLight`) with soft
     shadows.
   - Adds sized `BoxCollider` components to buildings, castles, walls, trees,
     water, props, and ground nodes.
   - Creates `DoorTrigger` child objects with trigger `BoxCollider` and
     `EducoreDoorTrigger` components wired to `targetInteriorRoot`.
   - Attaches `EducoreSpawnPoint` (tagged `"Respawn"`) to `player` nodes and
     `EducoreObjectMetadata` + trigger colliders to `npc` and `item` nodes.

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

