"""A few agents living a morning in a generated town.

    python agents/demo.py             # Claude Haiku 4.5 decides what everyone does
    python agents/demo.py --offline   # scripted agents, no API calls

Writes town.glb, town.dat (open it in the editor), replay.json and
town_life.html, a page that replays the morning in 3D, into agents/out/.
"""

import argparse
import base64
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from minds import (  # noqa: E402
    DEFAULT_EFFORT,
    DEFAULT_MODEL,
    Action,
    ClaudeMind,
    Persona,
    ScriptedMind,
    Situation,
    summarize_memory,
    supports_effort,
)
from town import Town, TownError  # noqa: E402
from towngen import generate_town  # noqa: E402

HERE = Path(__file__).resolve().parent

TICKS_PER_TURN = 10  # Agents walk one tile per tick.
MINUTES_PER_TICK = 1
START_MINUTES = 8 * 60
HEARING = 6  # Tiles within which people hear each other.
SIGHT = 14  # Tiles within which agents see things.
AT_PLACE = 2  # Tiles from an entrance that count as being at the place.
MEMORY = 12  # Remembered events shown to the agent each turn.

PERSONAS = [
    Persona(
        "Ada",
        "the town baker",
        "Warm and chatty, up since dawn, knows everyone's business.",
        "Find someone to help with tomorrow's early bake, and win support for "
        "a Saturday bread stall at the Market.",
        routine=["Bakery", "Café", "Market", "Fountain Square"],
        lines=["Fresh rolls at the Bakery today.", "Know anyone who'd help with the early bake?"],
    ),
    Persona(
        "Ben",
        "the librarian",
        "Quiet and precise, with a dry sense of humour.",
        "Start a book club: get at least two people to agree to meet at the "
        "Library on Friday evening.",
        routine=["Library", "Café", "Fountain Square", "Library"],
        lines=["We're starting a book club on Friday.", "Read anything good lately?"],
    ),
    Persona(
        "Cleo",
        "a painter who moved to town last week",
        "Curious, a little shy at first, always noticing colours and light.",
        "Meet people and find somewhere in town to show your paintings.",
        routine=["Fountain Square", "Café", "Town Hall", "Library"],
        lines=["I'm new here, I paint.", "Is there anywhere in town that shows art?"],
    ),
    Persona(
        "Dan",
        "a clerk at the Town Hall",
        "Organised and earnest, slightly overworked.",
        "Recruit volunteers for next weekend's fountain festival.",
        routine=["Town Hall", "Fountain Square", "Market", "Café"],
        lines=["The fountain festival needs volunteers!", "Could you help out next weekend?"],
    ),
    Persona(
        "Eve",
        "the owner of the Café",
        "Sharp and generous, loves a good story.",
        "Find out what everyone is up to, and get people to hold something at the Café.",
        routine=["Café", "Bakery", "Fountain Square", "Café"],
        lines=["Coffee's on me if you stop by the Café.", "What's the news today?"],
    ),
]

COLORS = ["#e4572e", "#2e86ab", "#f3a712", "#7b2cbf", "#2a9d8f"]


@dataclass
class Resident:
    id: int
    persona: Persona
    color: str
    x: int = 0
    y: int = 0
    moving: bool = False
    destination: str = None  # The place or person it's walking to.
    inside: str = None  # The building it's inside.
    memory: list = field(default_factory=list)
    heard: list = field(default_factory=list)

    @property
    def name(self):
        return self.persona.name


class Simulation:
    def __init__(self, town, plan, personas, mind, on_event=None, parallel=False):
        self.town = town
        self.plan = plan
        self.mind = mind
        self.on_event = on_event
        self.parallel = parallel
        self.places = {location.name: location for location in plan.locations}
        self.buildings = {location.center: location.name for location in plan.locations if location.kind != "park"}
        self.tick = 0
        self.events = []

        self.residents = []
        for i, persona in enumerate(personas):
            home = plan.homes[persona.name]
            agent = town.spawn(persona.name, *home.entrance)
            resident = Resident(agent, persona, COLORS[i % len(COLORS)], *home.entrance)
            resident.inside = home.name
            self.residents.append(resident)
        self.frames = [self._frame()]

    def clock(self, tick=None):
        minutes = START_MINUTES + (self.tick if tick is None else tick) * MINUTES_PER_TICK
        return f"{minutes // 60 % 24:02d}:{minutes % 60:02d}"

    def run(self, turns):
        for _ in range(turns):
            self.turn()

    def _remember(self, resident, entry):
        resident.memory.append(entry)
        if len(resident.memory) > 24:
            resident.memory = self.mind.summarize_memory(resident.memory)

    def turn(self):
        if self.parallel:
            import concurrent.futures

            situations = [self.situation(resident) for resident in self.residents]
            for resident in self.residents:
                resident.heard = []

            with concurrent.futures.ThreadPoolExecutor() as executor:
                futures = [
                    executor.submit(self.mind.decide, r.persona, sit)
                    for r, sit in zip(self.residents, situations)
                ]
                decisions = [f.result() for f in futures]

            for resident, decision in zip(self.residents, decisions):
                self.act(resident, decision)
        else:
            # Agents decide in order, so later agents hear what earlier ones said.
            for resident in self.residents:
                situation = self.situation(resident)
                resident.heard = []
                self.act(resident, self.mind.decide(resident.persona, situation))

        for _ in range(TICKS_PER_TURN):
            self.step()

    def step(self):
        state = self.town.step(1)
        self.tick = state["tick"]
        for resident, agent in zip(self.residents, state["agents"]):
            arrived = resident.moving and not agent["moving"]
            resident.x, resident.y, resident.moving = agent["x"], agent["y"], agent["moving"]
            if arrived and resident.destination:
                place = self.places.get(resident.destination)
                if place and place.kind != "park":
                    resident.inside = place.name
                self._event(resident, "arrive", resident.destination)
                self._remember(resident, f"{self.clock()} You arrived at {resident.destination}.")
                resident.destination = None
        self.frames.append(self._frame())

    def situation(self, resident):
        seen = self.town.observe(resident.id, radius=SIGHT)
        at = self.place_at(resident)
        others = [r for r in self.residents if r is not resident]
        distances = {agent["name"]: agent["distance"] for agent in seen["agents"]}
        nearby = [name for name, distance in distances.items() if distance <= HEARING]

        lines = [f"It's {self.clock()}."]
        if resident.inside:
            lines.append(f"You are inside {resident.inside}.")
        elif at:
            lines.append(f"You are at {at}.")
        else:
            lines.append(f"You are out in the street, at tile ({resident.x}, {resident.y}).")
        if resident.moving and resident.destination:
            lines.append(f"You are walking to {resident.destination}.")

        people = []
        for other in others:
            if other.name not in distances:
                continue
            where = f"inside {other.inside}" if other.inside else (f"at {self.place_at(other)}" if self.place_at(other) else "in the street")
            hearing = "close enough to talk" if other.name in nearby else "too far to talk"
            walking = f", walking to {other.destination}" if other.moving and other.destination else ""
            people.append(f"- {other.name}, {distances[other.name]:.0f} tiles away, {where}{walking} ({hearing}).")
        lines.append("People you can see:\n" + "\n".join(people) if people else "Nobody else is in sight.")

        things = _describe_objects(seen["objects"], self.buildings)
        if things:
            lines.append(f"Around you: {things}.")

        if resident.heard:
            lines.append("Since your last turn you heard:\n" + "\n".join(f'- {speaker}: "{words}"' for speaker, words in resident.heard))
        if resident.memory:
            if len(resident.memory) > MEMORY and resident.memory[0].startswith("Summary"):
                shown = [resident.memory[0]] + resident.memory[-(MEMORY - 1):]
            else:
                shown = resident.memory[-MEMORY:]
            lines.append("What you remember from earlier:\n" + "\n".join(f"- {entry}" for entry in shown))

        home = self.plan.homes[resident.name].name
        lines.append("Places in town: " + ", ".join(f"{name} (your home)" if name == home else name for name in self.places) + ".")
        lines.append("Decide what to do this turn.")

        return Situation(
            clock=self.clock(),
            text="\n\n".join(lines),
            places=list(self.places),
            people=[other.name for other in others],
            nearby=nearby,
            heard=list(resident.heard),
            at=resident.inside or at,
            moving=resident.moving,
        )

    def place_at(self, resident):
        for location in self.plan.locations:
            ex, ey = location.entrance
            if abs(ex - resident.x) <= AT_PLACE and abs(ey - resident.y) <= AT_PLACE:
                return location.name
        return None

    def act(self, resident, action: Action):
        if action.thought:
            self._event(resident, "thought", action.thought)

        if action.say:
            self._event(resident, "say", action.say)
            self._remember(resident, f'{self.clock()} You said: "{action.say}"')
            for other in self.residents:
                if other is not resident and _distance(resident, other) <= HEARING:
                    other.heard.append((resident.name, action.say))
                    self._remember(other, f'{self.clock()} {resident.name} said: "{action.say}"')

        target = action.go_to
        if not target or target == resident.name:
            return
        if target == resident.destination and resident.moving:
            return
        if target in self.places:
            if (resident.inside or self.place_at(resident)) == target and not resident.moving:
                return
            tile = self.places[target].entrance
        else:
            other = next(r for r in self.residents if r.name == target)
            tile = (other.x, other.y)

        try:
            steps = self.town.go(resident.id, *tile)
        except TownError:
            self._remember(resident, f"{self.clock()} You couldn't find a way to {target}.")
            return
        resident.destination = target
        resident.inside = None
        resident.moving = steps > 0
        self._event(resident, "go", target)
        if steps == 0:
            resident.destination = None

    def replay(self, mind_name):
        return {
            "title": "Town life",
            "mind": mind_name,
            "seed": self.plan.seed,
            "startMinutes": START_MINUTES,
            "minutesPerTick": MINUTES_PER_TICK,
            "ticksPerTurn": TICKS_PER_TURN,
            "hearing": HEARING,
            "bounds": self.plan.bounds,
            "locations": [
                {"name": l.name, "kind": l.kind, "entrance": l.entrance, "center": l.center, "floors": l.floors}
                for l in self.plan.locations
            ],
            "agents": [
                {"name": r.name, "color": r.color, "role": r.persona.role, "goal": r.persona.goal}
                for r in self.residents
            ],
            "frames": self.frames,
            "events": self.events,
        }

    def _frame(self):
        return [[r.x, r.y, 1 if r.inside else 0] for r in self.residents]

    def _event(self, resident, kind, text):
        event = {"tick": self.tick, "agent": resident.id, "kind": kind, "text": text}
        self.events.append(event)
        if self.on_event:
            self.on_event(self, resident, event)


def _distance(a, b):
    return ((a.x - b.x) ** 2 + (a.y - b.y) ** 2) ** 0.5


def _describe_objects(objects, buildings):
    """Summarises what an agent sees, naming the buildings it recognises."""
    counts = {}
    for obj in objects:
        if obj["type"] == "road":
            continue
        name = buildings.get((obj["x"], obj["y"])) if obj["type"] in ("building", "castle") else None
        name = name or (obj.get("id") or obj["type"]).replace("_", " ")
        counts[name] = counts.get(name, 0) + 1

    def plural(name, count):
        if count == 1:
            return name if name[0].isupper() else f"a {name}"
        return f"{count} {name}{'es' if name.endswith('s') else 's'}"

    return ", ".join(plural(name, count) for name, count in counts.items())


def write_outputs(town, simulation, out, mind_name):
    """Writes the town and the replay, and returns the replay page's path."""
    out.mkdir(parents=True, exist_ok=True)
    town.export_gltf(out / "town.glb")
    town.save(out / "town.dat")

    replay = simulation.replay(mind_name)
    (out / "replay.json").write_text(json.dumps(replay, indent=1, ensure_ascii=False), encoding="utf-8")

    glb = base64.b64encode((out / "town.glb").read_bytes()).decode("ascii")
    # Keep "</script>" in the data from ending the script element early.
    data = json.dumps(replay, ensure_ascii=False).replace("</", "<\\/")
    page = (HERE / "viewer.html").read_text(encoding="utf-8")
    page = page.replace("__REPLAY_JSON__", data).replace("__GLB_BASE64__", glb)
    path = out / "town_life.html"
    path.write_text(page, encoding="utf-8")
    return path


def print_event(simulation, resident, event):
    clock = simulation.clock(event["tick"])
    kind, text = event["kind"], event["text"]
    if kind == "say":
        print(f'{clock}  {resident.name}: "{text}"')
    elif kind == "thought":
        print(f"{clock}  {resident.name} thinks: {text}")
    elif kind == "go":
        print(f"{clock}  {resident.name} heads to {text}")
    elif kind == "arrive":
        print(f"{clock}  {resident.name} arrives at {text}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--offline", action="store_true", help="use scripted agents instead of Claude")
    parser.add_argument("--agents", type=int, default=3, choices=range(1, len(PERSONAS) + 1), help="how many residents (default 3)")
    parser.add_argument("--turns", type=int, default=12, help="10-minute turns to simulate (default 12)")
    parser.add_argument("--seed", type=int, default=7, help="town layout seed")
    parser.add_argument("--model", default=DEFAULT_MODEL, help=f"Claude model (default {DEFAULT_MODEL})")
    parser.add_argument("--effort", default=DEFAULT_EFFORT, choices=["low", "medium", "high", "xhigh", "max"], help="Claude effort (default: the model's own; Haiku doesn't take one)")
    parser.add_argument("--out", type=Path, default=HERE / "out", help="output directory (default agents/out)")
    parser.add_argument("--quiet", action="store_true", help="don't print what happens")
    parser.add_argument("--parallel", action="store_true", help="run agent decisions concurrently in each turn")
    args = parser.parse_args(argv)

    personas = PERSONAS[: args.agents]
    if args.effort and not args.offline and not supports_effort(args.model):
        parser.error(f"{args.model} doesn't support --effort")
    if args.offline:
        mind, mind_name = ScriptedMind(args.seed), "scripted"
    else:
        try:
            mind, mind_name = ClaudeMind(args.model, args.effort, HEARING), args.model
        except ImportError:
            parser.error("the anthropic package is missing: pip install anthropic, or use --offline")

    with Town() as town:
        plan = generate_town(town, [p.name for p in personas], seed=args.seed)
        simulation = Simulation(town, plan, personas, mind, None if args.quiet else print_event, parallel=args.parallel)
        print(f"{len(personas)} residents, {len(plan.locations)} places, {args.turns} turns, decided by {mind_name}.")
        simulation.run(args.turns)
        page = write_outputs(town, simulation, args.out, mind_name)

    print(f"Used {mind.usage()}.")
    print(f"Replay: {page}")


if __name__ == "__main__":
    main()
