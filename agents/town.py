"""Python client for the engine's headless agent server.

The engine owns the world: placement rules, which tiles can be walked on,
pathfinding, movement and perception. Python decides what the agents do.

    with Town() as town:
        town.place("road", 0, 0)
        ada = town.spawn("Ada", 0, 0)
        town.go(ada, 6, 0)
        town.step(6)
        print(town.observe(ada))
"""

import json
import os
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BINARY = REPO_ROOT / "bin" / ("educore.exe" if os.name == "nt" else "educore")

# The error the engine gives when an object overlaps another one.
TILES_TAKEN = "the tiles are taken"


class TownError(RuntimeError):
    """The agent server rejected a command."""


def _plain(value):
    # The engine's JSON reader doesn't handle escapes in strings.
    value = str(value)
    if '"' in value or "\\" in value:
        raise ValueError(f"{value!r} can't contain quotes or backslashes")
    return value


class Town:
    """A running `educore --agent-server` process holding one world."""

    def __init__(self, binary=None, show_engine_log=False):
        binary = Path(binary or os.environ.get("EDUCORE_BIN") or DEFAULT_BINARY)
        if not binary.exists():
            raise FileNotFoundError(
                f"{binary} doesn't exist, build the engine with `python build.py build`"
            )

        # The engine loads assets relative to the repository root.
        self._process = subprocess.Popen(
            [str(binary), "--agent-server"],
            cwd=REPO_ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=None if show_engine_log else subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            bufsize=1,
        )

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def close(self):
        if self._process.poll() is None:
            try:
                self._call("quit")
            except (TownError, OSError):
                pass
            self._process.stdin.close()
            self._process.wait(timeout=10)
        self._process.stdout.close()

    def _call(self, cmd, **fields):
        fields = {key: value for key, value in fields.items() if value is not None}
        self._process.stdin.write(json.dumps({"cmd": cmd, **fields}) + "\n")
        self._process.stdin.flush()

        line = self._process.stdout.readline()
        if not line:
            raise TownError(f"the agent server exited during '{cmd}'")
        reply = json.loads(line)
        if not reply.get("ok"):
            raise TownError(reply.get("error", "unknown error"))
        return reply

    # World

    def new_world(self):
        """Replaces the world with an empty one and removes every agent."""
        self._call("new")

    def load(self, path):
        """Loads a world saved by the editor or `save`, removing every agent."""
        self._call("load", path=_plain(Path(path).resolve().as_posix()))

    def save(self, path):
        """Saves the world in the editor's format, so the editor can open it."""
        self._call("save", path=_plain(Path(path).resolve().as_posix()))

    def export_gltf(self, path):
        """Exports the world as a binary glTF file (.glb)."""
        self._call("export", path=_plain(Path(path).resolve().as_posix()))

    def place(self, type, x, y, floors=None, id=None):
        """Places an outdoor object. Returns False if its tiles are taken.

        `type` is an object type such as "road", "building", "tree" or "prop";
        `id` names the prop model, e.g. "bench" or "barrel".
        """
        try:
            self._call(
                "place",
                type=_plain(type),
                x=x,
                y=y,
                floors=floors,
                id=None if id is None else _plain(id),
            )
            return True
        except TownError as error:
            if str(error) == TILES_TAKEN:
                return False
            raise

    def objects(self):
        """Every outdoor object, as dicts with type, x, y, floors and id."""
        return self._call("objects")["objects"]

    # Agents

    def spawn(self, name, x, y):
        """Adds an agent standing on a walkable tile. Returns its ID."""
        return self._call("spawn", name=_plain(name), x=x, y=y)["agent"]["agent"]

    def go(self, agent, x, y):
        """Sends an agent walking to a tile. Returns the number of steps."""
        return self._call("go", agent=agent, x=x, y=y)["steps"]

    def stop(self, agent):
        self._call("stop", agent=agent)

    def step(self, ticks=1):
        """Advances time. Walking agents move one tile per tick.

        Returns {"tick": ..., "agents": [{"agent", "name", "x", "y", "moving"}]}.
        """
        reply = self._call("step", ticks=ticks)
        return {"tick": reply["tick"], "agents": reply["agents"]}

    def agents(self):
        reply = self._call("agents")
        return {"tick": reply["tick"], "agents": reply["agents"]}

    def say(self, agent, text):
        """Records an agent speaking out loud."""
        self._call("say", agent=agent, text=_plain(text))

    def speech(self):
        """Recent speech events recorded on the server."""
        return self._call("speech")["events"]

    def observe(self, agent, radius=10):
        """What an agent perceives: itself, and the objects and agents nearby.

        Returns {"tick", "self", "objects", "agents"}; objects and agents carry
        their distance, nearest first.
        """
        reply = self._call("observe", agent=agent, radius=radius)
        res = {key: reply[key] for key in ("tick", "self", "objects", "agents")}
        if "speech" in reply:
            res["speech"] = reply["speech"]
        return res

