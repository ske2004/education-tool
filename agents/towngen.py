"""Generates a small town through the agent server.

The town is a grid of streets. Each block holds two buildings whose doors open
onto the street south of them, with a backyard of trees and grass behind. One
block is a park with a fountain. Every placement goes through the engine, so
its placement rules hold, and the result can be saved and opened in the editor.
"""

import random
from dataclasses import dataclass, field

BLOCK = 24  # Tiles between street centers.
ROAD = 4  # Roads are 4x4 tiles, centered on their position.

PUBLIC_PLACES = ["Café", "Library", "Town Hall", "Bakery", "Market", "Workshop", "School", "Clinic"]


@dataclass
class Location:
    name: str
    kind: str  # "home", "public" or "park".
    entrance: tuple  # The walkable tile agents go to, e.g. outside a door.
    center: tuple  # Where the building or park is, for labels.
    floors: int = 0


@dataclass
class TownPlan:
    seed: int
    locations: list
    bounds: tuple  # (min_x, min_y, max_x, max_y) in tiles.
    homes: dict = field(default_factory=dict)  # Resident name -> Location.

    def location(self, name):
        return next(location for location in self.locations if location.name == name)


def generate_town(town, residents, seed=0, blocks_x=3, blocks_y=2, spawn_npcs=False):
    """Builds a town in an empty world, with a home for each resident."""
    rng = random.Random(seed)
    building_blocks = blocks_x * blocks_y - 1
    if len(residents) + 1 > building_blocks * 2:
        raise ValueError("the town is too small for that many residents")

    # Center the town on the world's origin.
    x0 = -blocks_x * BLOCK // 2
    y0 = -blocks_y * BLOCK // 2

    roads = set()
    for j in range(blocks_y + 1):
        for x in range(x0, x0 + blocks_x * BLOCK + 1, ROAD):
            roads.add((x, y0 + j * BLOCK))
    for i in range(blocks_x + 1):
        for y in range(y0, y0 + blocks_y * BLOCK + 1, ROAD):
            roads.add((x0 + i * BLOCK, y))
    for x, y in sorted(roads):
        town.place("road", x, y)

    blocks = [(x0 + i * BLOCK, y0 + j * BLOCK) for j in range(blocks_y) for i in range(blocks_x)]
    park = blocks[len(blocks) // 2]

    # Two building slots per block, on its south side. A building's tiles
    # start 4 tiles before its position, and its door faces south, so the
    # tile in front of the door is on the street.
    slots = []
    for bx, by in blocks:
        if (bx, by) != park:
            slots += [(bx + 7, by + 6), (bx + 17, by + 6)]
    rng.shuffle(slots)

    locations = []
    homes = {}
    names = [f"{resident}'s home" for resident in residents] + PUBLIC_PLACES
    for name, (x, y) in zip(names, slots):
        is_home = name.endswith("'s home")
        floors = rng.randint(1, 2) if is_home else rng.randint(2, 4)
        obj_type = "castle" if name == "Town Hall" else "building"
        town.place(obj_type, x, y, floors=floors)
        location = Location(name, "home" if is_home else "public", (x, y - 5), (x, y), floors)
        locations.append(location)
        if is_home:
            homes[name[: -len("'s home")]] = location

    for bx, by in blocks:
        if (bx, by) == park:
            _place_park(town, rng, bx, by)
        else:
            _place_backyard(town, rng, bx, by)
        for corner in [(bx + 2, by + 2), (bx + 21, by + 2), (bx + 2, by + 21), (bx + 21, by + 21)]:
            town.place("prop", *corner, id="street_light")

    px, py = park
    locations.append(Location("Fountain Square", "park", (px + 12, py + 9), (px + 12, py + 12)))

    bounds = (x0 - ROAD // 2, y0 - ROAD // 2, x0 + blocks_x * BLOCK + ROAD // 2, y0 + blocks_y * BLOCK + ROAD // 2)
    plan = TownPlan(seed, locations, bounds, homes)
    if spawn_npcs:
        place_resident_npcs(town, plan)
    return plan


def place_resident_npcs(town, plan):
    """Places npc objects at home and public locations for the editor."""
    placed = []
    for loc in plan.locations:
        if loc.kind in ("home", "public"):
            cx, cy = loc.center
            candidates = [
                (cx - 5, cy),
                (cx + 4, cy),
                (cx - 5, cy + 2),
                (cx + 4, cy + 2),
                (cx, cy + 4),
                (cx - 3, cy + 4),
                (cx + 3, cy + 4),
            ]
            for tx, ty in candidates:
                if town.place("npc", tx, ty):
                    placed.append((loc.name, (tx, ty)))
                    break
    return placed


def _place_backyard(town, rng, bx, by):
    # Behind the buildings: rows y in [by + 10, by + 22).
    if rng.random() < 0.4:
        px = rng.randint(bx + 6, bx + 18)
        py = rng.randint(by + 14, by + 18)
        for dx in range(2):
            for dy in range(2):
                town.place("water", px + dx, py + dy)

    for _ in range(5):
        town.place("tree", rng.randint(bx + 4, bx + 20), rng.randint(by + 12, by + 20))
    for _ in range(8):
        town.place("high_grass", rng.randint(bx + 3, bx + 20), rng.randint(by + 11, by + 20))
    town.place("prop", rng.randint(bx + 4, bx + 19), by + 11, id=rng.choice(["bench", "flower_pot", "barrel"]))


def _place_park(town, rng, bx, by):
    cx, cy = bx + 12, by + 12
    town.place("prop", cx, cy, id="fountain")
    for dx, dy in [(-3, 0), (3, 0), (0, 3)]:
        town.place("prop", cx + dx, cy + dy, id="bench")
    for dx, dy in [(-3, -3), (3, -3), (-3, 3), (3, 3)]:
        town.place("prop", cx + dx, cy + dy, id="flower_pot")
    for x, y in [(bx + 5, by + 5), (bx + 19, by + 5), (bx + 5, by + 19), (bx + 19, by + 19), (bx + 12, by + 19)]:
        town.place("tree", x, y)
    for _ in range(6):
        town.place("high_grass", rng.randint(bx + 3, bx + 20), rng.randint(by + 15, by + 20))

    # Small pond next to the fountain with a decorative bridge
    pond_x = bx + 6
    pond_y = by + 15
    for dx in range(3):
        for dy in range(3):
            wx, wy = pond_x + dx, pond_y + dy
            if dx == 1 and dy == 1:
                town.place("bridge", wx, wy)
            else:
                town.place("water", wx, wy)

