# Educore Stories (Godot)

A storytelling engine for teachers, built on Godot 4.7. Teachers make a story
out of places, people, items and conversations in a simple editor; students
play it in 3D: they walk around, talk to people, pick things up and go through
doors until one of their choices ends the story.

This is an early version of moving Educore to Godot. It doesn't use the C++
engine.

## Running it

Open this folder in Godot 4.7 and press F5, or:

```sh
godot --path godot                                   # menu
godot --path godot -- --play res://stories/water_cycle.json
godot --path godot -- --edit res://stories/water_cycle.json
```

The menu can play or edit the sample story ("Where Does the Water Go?"),
create a new story, or open a story file.

## In a browser

Classrooms can use the web version, with nothing to install:

```sh
godot --headless --path godot --export-release Web ../build/web/index.html
python3 -m http.server -d build/web 8000       # then open http://localhost:8000
```

The Pages workflow publishes it at `<pages site>/stories/`. In the browser:

- **Copy student link** in the editor copies a link with the whole story
  packed into it (after `#story=`). Students open it and start playing; the
  story isn't uploaded anywhere. The link plays the story as it was when it
  was copied, so copy a new one after changing the story.
- **Download** saves the story as a JSON file, and **Open...** loads one.
- The editor keeps a draft in the browser as you work, and the menu offers
  to continue it.

The export doesn't use threads, so any static file host works without
special headers.

## Making a story

- **Places** are outdoor areas or rooms. Place props (houses, trees, shelves...)
  with the tools above the 3D view: click to place, drag to move, R to turn,
  Delete to remove. Right-drag turns the camera, the wheel zooms, WASD moves.
  Ctrl+Z undoes and Ctrl+Shift+Z (or Ctrl+Y) redoes.
- **Water**: ponds, fountains, and river sections 8 m long that join up when
  placed 8 m apart. Rivers stop students; put a bridge across one to let
  them over.
- **Weather**: outdoor places can be clear, cloudy or rainy.
- **Doors** take students to another place. Students arrive in front of the
  door that leads back, so give each place a door back.
- **People** say lines. A conversation starts at the first line marked "Can
  start a conversation" whose conditions hold. Each choice leads to another
  line or ends the talk, and can give or take an item, set a fact, or end the
  story with an ending text.
- **Items** can lie in a place for students to pick up, or be given in a
  conversation. What students read when they pick one up is a good place for
  the lesson itself.
- **Conditions** ("Only if they have" / "Not if they have") check items and
  facts, so people can say different things as the story goes on.

Students walk with WASD or the arrow keys, jump with Space, and press E (or
Enter) to talk, pick things up, go through doors and sit on benches; moving,
jumping or E again stands them up. Space also moves a conversation on. Holding
the right mouse button and dragging looks around. On a tablet, students walk
with an on-screen joystick, jump with the round Jump button, drag to look
around, pinch to zoom and tap to talk.

The left panel lists problems to fix before playing, such as doors that lead
nowhere or a story with no ending. Play tries the story straight from the
editor.

## Story files

Stories are JSON (`stories/water_cycle.json` is a full example):

```json
{
  "title": "...", "intro": "...", "goal": "...",
  "start_place": "village", "start_pos": [0, 7],
  "places": [{"id": "village", "name": "Village square", "indoor": false, "size": 40,
              "ground": "#86b85e", "sky": "#8ec5ff",
              "props": [{"kind": "well", "pos": [0, -3], "rot": 0, "color": "#a9a297"}],
              "doors": [{"pos": [11, -5.4], "rot": 0, "to": "library", "label": "Library"}]}],
  "characters": [{"id": "mayor", "name": "Mayor Lena", "place": "village", "pos": [-2, -0.5],
                  "lines": [{"text": "...", "opens": true, "requires": "", "unless": "learned",
                             "choices": [{"text": "...", "next": 1, "requires": "", "unless": "",
                                          "gives": "", "takes": "", "sets": "quest", "ending": ""}]}]}],
  "items": [{"id": "notebook", "name": "Cloud notebook", "place": "library", "pos": [0, 0],
             "description": "..."}]
}
```

Positions are `[x, z]` in metres on the ground, with `z` towards the camera.
`next` is a line index, or -1 to stop talking. Missing fields get defaults.

## Code

| File | What it does |
| --- | --- |
| `scripts/story.gd` | Story data: defaults, loading, saving, problems, IDs |
| `scripts/story_state.gd` | A student's progress: place, backpack, facts, ending |
| `scripts/world_builder.gd` | Builds a place's 3D scene from simple shapes |
| `scripts/play_mode.gd` | Playing: walking, talking, items, doors, HUD |
| `scripts/editor_mode.gd` | The teacher's editor |
| `scripts/camera_rig.gd`, `player.gd`, `actor.gd` | Camera, student, animation |
| `scripts/dialogue_box.gd`, `ui_theme.gd` | Dialogue UI and the app's look |
| `scripts/main.gd` | Menu and switching between screens |
| `scripts/story_files.gd` | Opening and saving stories; browser drafts and story links |
| `scripts/touch_stick.gd` | The on-screen joystick for tablets |
| `scripts/drifter.gd`, `rain.gd`, `shaders/water.gdshader` | Clouds, rain and water |

## Tests

```sh
godot --headless --path godot -s res://tests/run_tests.gd
```

They check the story logic, a full walkthrough of the sample, building every
place, and drive play mode and the editor without a window.
