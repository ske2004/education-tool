# Agent sandbox (experiment)

A minimal Python API for running agents inside the engine's world, and a demo
of a few Claude-driven residents spending a morning in a generated town.

The engine owns the world: placement rules, walkable tiles, pathfinding,
movement, perception and saving. Python owns the minds and the conversation.
The two talk over a pipe to a headless engine process, so nothing is rendered
while the simulation runs and it steps deterministically.

## Quick start

```sh
python build.py build                 # builds bin/educore
python agents/demo.py --offline       # scripted residents, no API calls
```

To let Claude decide what the residents do, install the SDK and give it
credentials (`ant auth login`, or set `ANTHROPIC_API_KEY`):

```sh
pip install anthropic
python agents/demo.py                 # 3 residents, 12 turns, claude-haiku-4-5
python agents/demo.py --agents 5 --turns 18 --seed 3
python agents/demo.py --model claude-opus-5-5 --effort low
```

The demo writes into `agents/out/`:

- `town_life.html`: the morning replayed in 3D, with speech bubbles and a
  log. It's a single file with the town embedded, so it can be shared; it
  loads three.js from a CDN.
- `town.glb`: the town as glTF (see [docs/gltf_export.md](../docs/gltf_export.md)).
- `town.dat`: the town in the editor's format. Copy it to `assets/world.dat`
  to open it in the editor.
- `replay.json`: every position and event, for analysis.

Tests: `python -m unittest discover agents`. They use the built engine and
never call the real API.

## How the demo works

Each turn is 10 minutes of town time. Every resident, in order:

1. **Perceives.** The engine reports what's within 14 tiles: buildings
   (named, e.g. the Café), props, trees, and other residents with their
   distance. Python adds where the resident is, what it heard since its last
   turn and its last 12 memories.
2. **Decides.** One Claude request per resident turn returns a structured
   action: a private `thought`, something to `say` out loud, and where to
   `go_to`. The destination is limited by the JSON schema to real places and
   people, so it can't invent somewhere to go.
3. **Acts.** Words reach everyone within 6 tiles and land in their memory.
   Walking goes through the engine's pathfinding.

Then the engine advances 10 ticks, one tile of walking per tick. Residents
who reach a building go inside until they leave.

The residents have personas and goals that need each other (a librarian
starting a book club, a painter looking for somewhere to exhibit, a clerk
recruiting festival volunteers...), see `PERSONAS` in `demo.py`. The town is
generated per seed: a grid of streets, two buildings per block with doors on
the street, backyards, street lights and a fountain park.

**Cost.** Requests use `claude-haiku-4-5` by default, which is fast and cheap
for short in-character decisions. A turn's prompt is around a thousand
tokens, so the default run (36 decisions) should cost a few cents; the demo
prints the tokens it used. `--model` picks another model, and `--effort` sets
its effort (Haiku 4.5 doesn't take one). With Claude Opus 5.5, Opus 5, Sonnet
5.5 or Fable 5.1, a request declined by a safety classifier is retried on a
fallback model (`fallbacks: "default"`). On any model, a decline that still
stands makes the resident do nothing that turn.

## Python API

`town.py` starts `bin/educore --agent-server` and wraps its commands:

```python
from town import Town
from towngen import generate_town

with Town() as town:
    plan = generate_town(town, ["Ada", "Ben"], seed=1)
    ada = town.spawn("Ada", *plan.homes["Ada"].entrance)
    steps = town.go(ada, *plan.location("Library").entrance)
    town.step(steps)
    print(town.observe(ada, radius=10))
    town.save("my_town.dat")
```

| Method | What it does |
| --- | --- |
| `new_world()`, `load(path)`, `save(path)` | Replace, load or save the world (editor format). |
| `export_gltf(path)` | Export the world as `.glb`. |
| `place(type, x, y, floors=, id=)` | Place an outdoor object; `False` when its tiles are taken. Types are the editor's: `road`, `building`, `tree`, `prop` (with `id`, e.g. `bench`), ... |
| `objects()` | Every outdoor object. |
| `spawn(name, x, y)` | Add an agent on a walkable tile; returns its ID. |
| `go(agent, x, y)`, `stop(agent)` | Start or stop walking; `go` returns the number of steps. |
| `step(ticks=1)` | Advance time; returns every agent's position. |
| `observe(agent, radius=10)` | Nearby objects and agents, nearest first, with distances. |

Commands that fail raise `TownError`.

### Protocol

Any language can drive the server: run `bin/educore --agent-server` from the
repository root, write one JSON object per line to its stdin and read one JSON
line back per command. Every reply has `"ok"`, and an `"error"` when it's
false.

```
> {"cmd": "place", "type": "building", "x": 10, "y": 6, "floors": 2}
< {"ok":true}
> {"cmd": "spawn", "name": "Ada", "x": -5, "y": -5}
< {"ok":true,"agent":{"agent":0,"name":"Ada","x":-5,"y":-5,"moving":false}}
> {"cmd": "go", "agent": 0, "x": 10, "y": 1}
< {"ok":true,"steps":21}
> {"cmd": "step", "ticks": 3}
< {"ok":true,"tick":3,"agents":[{"agent":0,"name":"Ada","x":-2,"y":-5,"moving":true}]}
```

Commands: `new`, `load`, `save`, `export` (all but `new` take `path`),
`place`, `objects`, `spawn`, `go`, `stop`, `step`, `observe`, `agents` and
`quit`. Tiles are the editor's grid coordinates; `y` is the editor's depth
axis. The command reader is deliberately small: string values can't contain
quotes or backslashes.

## Limits

- **Outdoors only.** Agents walk the outdoor place; "inside" a building is
  tracked by the demo, not simulated. Interiors exist in the world format but
  agents can't walk them yet.
- **Agents don't collide** with each other, and props, grass and water don't
  block walking (the same rules as the playtest's NPCs).
- **Speech lives in Python.** The engine doesn't know who said what; hearing is
  a distance check in `demo.py`.
- **Turns are concurrent by default.** Residents decide at the same time, so a
  turn takes about as long as its slowest request. Nobody hears what another
  resident says in the same turn; they hear it at the start of their next turn.
  Actions still apply in resident order. `--sequential` decides one resident
  at a time, so later residents hear earlier ones in the same turn. `--offline`
  always decides sequentially.
- **Short memory.** Each resident sees its last 12 events, with no
  summarising or reflection, so long runs forget the morning.

## Next steps

1. **Interiors.** Let agents enter buildings and walk their interior places,
   reusing the playtest's door logic.
2. **Gym-style wrapper.** `reset()` / `step(actions)` returning observations,
   for reinforcement learning and benchmarks.
3. **Memory and reflection.** Summarise each resident's day into longer-term
   memories, as in the Generative Agents paper.
4. **Parallel decisions.** Decide all residents at once per turn, and handle
   overheard speech on the next turn, to cut wall-clock time.
5. **Engine-side events.** Move speech and interactions (picking up items,
   opening doors) into the engine so other clients see them too.
6. **Live view.** Stream positions to the editor or a browser while the
   simulation runs instead of replaying afterwards.
