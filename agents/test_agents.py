"""Tests for the agent API, the town generator and the demo.

    python -m unittest discover agents

The engine tests need the built engine (`python build.py build`) and are
skipped without it. Nothing here calls the real Claude API.
"""

import json
import os
import sys
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent))

import demo  # noqa: E402
from minds import Action, ClaudeMind, ScriptedMind, Situation, summarize_memory  # noqa: E402
from town import DEFAULT_BINARY, Town, TownError  # noqa: E402
from towngen import generate_town, place_resident_npcs  # noqa: E402

needs_engine = unittest.skipUnless(DEFAULT_BINARY.exists(), "the engine isn't built")


@needs_engine
class TownTest(unittest.TestCase):
    def setUp(self):
        self.town = Town()

    def tearDown(self):
        self.town.close()

    def test_placing_checks_the_tiles(self):
        self.assertTrue(self.town.place("building", 0, 0, floors=2))
        self.assertFalse(self.town.place("building", 2, 2))
        self.assertTrue(self.town.place("prop", 10, 10, id="bench"))
        objects = self.town.objects()
        self.assertEqual([o["type"] for o in objects], ["building", "prop"])
        self.assertEqual(objects[1]["id"], "bench")

    def test_unknown_types_are_errors(self):
        with self.assertRaises(TownError):
            self.town.place("spaceship", 0, 0)

    def test_agents_walk_around_buildings(self):
        self.town.place("building", 0, 0)
        ada = self.town.spawn("Ada", 0, -6)
        with self.assertRaises(TownError):
            self.town.spawn("Ben", 0, 0)  # Inside the building.

        steps = self.town.go(ada, 0, 6)
        self.assertGreater(steps, 12)  # Longer than the straight line.
        state = self.town.step(steps)
        self.assertEqual(state["tick"], steps)
        self.assertEqual((state["agents"][0]["x"], state["agents"][0]["y"]), (0, 6))
        self.assertFalse(state["agents"][0]["moving"])

    def test_agents_see_what_is_near(self):
        self.town.place("tree", 3, 0)
        self.town.place("tree", 40, 0)
        ada = self.town.spawn("Ada", 0, 0)
        self.town.spawn("Ben", 2, 2)
        self.town.spawn("Cleo", 30, 0)

        seen = self.town.observe(ada, radius=10)
        self.assertEqual([o["x"] for o in seen["objects"]], [3])
        self.assertEqual([a["name"] for a in seen["agents"]], ["Ben"])
        self.assertAlmostEqual(seen["agents"][0]["distance"], 2.8, places=1)

    def test_walled_in_tiles_are_unreachable(self):
        for x, y in [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1)]:
            self.town.place("wall", x, y)
        ada = self.town.spawn("Ada", 5, 5)
        with self.assertRaises(TownError):
            self.town.go(ada, 0, 0)

    def test_saving_and_exporting(self):
        self.town.place("road", 0, 0)
        self.town.place("building", 10, 10, floors=3)
        with tempfile.TemporaryDirectory() as directory:
            self.town.save(Path(directory) / "world.dat")
            self.town.export_gltf(Path(directory) / "world.glb")
            self.assertEqual((Path(directory) / "world.glb").read_bytes()[:4], b"glTF")

            objects = self.town.objects()
            self.town.new_world()
            self.assertEqual(self.town.objects(), [])
            self.town.load(Path(directory) / "world.dat")
            self.assertEqual(self.town.objects(), objects)

        with self.assertRaises(TownError):
            self.town.load("/no/such/world.dat")


@needs_engine
class TownGenTest(unittest.TestCase):
    def generate(self, seed):
        with Town() as town:
            plan = generate_town(town, ["Ada", "Ben"], seed=seed)
            return plan, town.objects()

    def test_same_seed_same_town(self):
        self.assertEqual(self.generate(3)[1], self.generate(3)[1])
        self.assertNotEqual(self.generate(3)[1], self.generate(4)[1])

    def test_every_place_can_be_reached(self):
        with Town() as town:
            plan = generate_town(town, ["Ada", "Ben"], seed=1)
            self.assertEqual(set(plan.homes), {"Ada", "Ben"})
            self.assertIn("Fountain Square", [l.name for l in plan.locations])

            start = plan.homes["Ada"].entrance
            walker = town.spawn("Walker", *start)
            for location in plan.locations:
                with self.subTest(location.name):
                    self.assertGreaterEqual(town.go(walker, *location.entrance), 0)


@needs_engine
class DemoTest(unittest.TestCase):
    def test_offline_morning(self):
        with Town() as town, tempfile.TemporaryDirectory() as directory:
            personas = demo.PERSONAS[:3]
            plan = generate_town(town, [p.name for p in personas], seed=2)
            simulation = demo.Simulation(town, plan, personas, ScriptedMind(2))
            simulation.run(6)

            self.assertEqual(len(simulation.frames), 6 * demo.TICKS_PER_TURN + 1)
            kinds = {event["kind"] for event in simulation.events}
            self.assertTrue({"go", "arrive", "thought"} <= kinds)

            page = demo.write_outputs(town, simulation, Path(directory), "scripted")
            html = page.read_text(encoding="utf-8")
            self.assertNotIn("__REPLAY_JSON__", html)
            self.assertNotIn("__GLB_BASE64__", html)
            data = html.split('id="replay-data" type="application/json">', 1)[1].split("</script>", 1)[0]
            self.assertEqual(len(json.loads(data)["frames"]), len(simulation.frames))

    def test_speech_reaches_people_nearby(self):
        with Town() as town:
            personas = demo.PERSONAS[:3]
            plan = generate_town(town, [p.name for p in personas], seed=2)
            simulation = demo.Simulation(town, plan, personas, ScriptedMind(0))
            ada, ben, cleo = simulation.residents
            ben.x, ben.y = ada.x + 2, ada.y
            cleo.x, cleo.y = ada.x + 40, ada.y

            simulation.act(ada, Action(say="Morning!"))
            self.assertEqual(ben.heard, [("Ada", "Morning!")])
            self.assertEqual(cleo.heard, [])


def make_situation():
    return Situation(
        clock="08:00",
        text="It's 08:00.",
        places=["Café", "Library"],
        people=["Ben"],
        nearby=[],
        heard=[],
    )


def make_persona():
    return demo.PERSONAS[0]


class FakeMessages:
    def __init__(self, stop_reason="end_turn", text=None):
        self.requests = []
        self.stop_reason = stop_reason
        self.text = text or json.dumps({"thought": "Coffee first.", "say": " Morning! ", "go_to": "Café"})

    def create(self, **request):
        self.requests.append(request)
        return SimpleNamespace(
            stop_reason=self.stop_reason,
            content=[SimpleNamespace(type="text", text=self.text)],
            usage=SimpleNamespace(input_tokens=100, output_tokens=20),
        )


def fake_client(messages):
    return SimpleNamespace(messages=messages, beta=SimpleNamespace(messages=messages))


class ClaudeMindTest(unittest.TestCase):
    def test_haiku_by_default(self):
        messages = FakeMessages()
        mind = ClaudeMind(client=fake_client(messages))
        action = mind.decide(make_persona(), make_situation())

        self.assertEqual(action, Action("Coffee first.", "Morning!", "Café"))
        request = messages.requests[0]
        self.assertEqual(request["model"], "claude-haiku-4-5")
        # Haiku 4.5 takes neither an effort setting nor server-side fallbacks.
        self.assertNotIn("effort", request["output_config"])
        self.assertNotIn("fallbacks", request)
        schema = request["output_config"]["format"]["schema"]
        self.assertEqual(schema["properties"]["go_to"]["enum"], ["", "Café", "Library", "Ben"])
        self.assertIn("You are Ada", request["system"][0]["text"])
        self.assertEqual(request["system"][0]["cache_control"], {"type": "ephemeral"})
        self.assertEqual(
            mind.usage(), "100 input and 20 output tokens (0 cache read, 0 cache write)"
        )

    def test_opus_gets_effort_and_fallbacks(self):
        messages = FakeMessages()
        mind = ClaudeMind("claude-opus-5-5", "low", client=fake_client(messages))
        mind.decide(make_persona(), make_situation())

        request = messages.requests[0]
        self.assertEqual(request["output_config"]["effort"], "low")
        self.assertEqual(request["fallbacks"], "default")
        self.assertEqual(request["betas"], ["server-side-fallback-2026-07-01"])

    def test_refusals_do_nothing(self):
        messages = FakeMessages(stop_reason="refusal", text="")
        mind = ClaudeMind(client=fake_client(messages))
        action = mind.decide(make_persona(), make_situation())
        self.assertEqual((action.say, action.go_to), ("", ""))


try:
    import anthropic
except ImportError:
    anthropic = None


@unittest.skipUnless(anthropic, "the anthropic package isn't installed")
class ClaudeMindSdkTest(unittest.TestCase):
    """Sends a decision through the real SDK to a local stand-in for the API."""

    def test_round_trip(self):
        received = []

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def do_POST(self):
                received.append((self.path, dict(self.headers), json.loads(self.rfile.read(int(self.headers["content-length"])))))
                body = json.dumps({
                    "id": "msg_test", "type": "message", "role": "assistant", "model": "claude-haiku-4-5",
                    "content": [{"type": "text", "text": json.dumps({"thought": "Hi.", "say": "Hello", "go_to": "Library"})}],
                    "stop_reason": "end_turn", "stop_sequence": None,
                    "usage": {"input_tokens": 10, "output_tokens": 5},
                }).encode()
                self.send_response(200)
                self.send_header("content-type", "application/json")
                self.send_header("content-length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        server = HTTPServer(("127.0.0.1", 0), Handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            client = anthropic.Anthropic(api_key="test", base_url=f"http://127.0.0.1:{server.server_port}", max_retries=0)
            action = ClaudeMind(client=client).decide(make_persona(), make_situation())
        finally:
            server.shutdown()

        self.assertEqual(action, Action("Hi.", "Hello", "Library"))
        path, headers, body = received[0]
        self.assertEqual(path, "/v1/messages")
        self.assertIsNone(headers.get("anthropic-beta"))
        self.assertEqual(body["model"], "claude-haiku-4-5")
        self.assertNotIn("fallbacks", body)
        self.assertEqual(body["output_config"]["format"]["type"], "json_schema")


class MemoryCompactionTest(unittest.TestCase):
    def test_under_threshold_unmodified(self):
        memories = [f"Event {i}" for i in range(24)]
        self.assertEqual(summarize_memory(memories), memories)

    def test_over_threshold_compacted(self):
        memories = [f"08:{i:02d} You arrived at Place{i}." for i in range(30)]
        compacted = summarize_memory(memories)
        self.assertEqual(len(compacted), 13)
        self.assertTrue(compacted[0].startswith("Summary"))
        self.assertIn("visited", compacted[0])
        self.assertEqual(compacted[1:], memories[-12:])

    def test_summarize_speakers_and_places(self):
        memories = [
            "08:00 You arrived at Bakery.",
            '08:05 Ada said: "Good morning!"',
            "08:10 You arrived at Café.",
            '08:15 Ben said: "Hello there."',
        ] + [f"08:{i:02d} Walking in town." for i in range(20, 50)]
        compacted = summarize_memory(memories)
        summary = compacted[0]
        self.assertIn("Bakery", summary)
        self.assertIn("Café", summary)
        self.assertIn("Ada", summary)
        self.assertIn("Ben", summary)

    def test_mind_memories_compacted(self):
        mind = ScriptedMind()
        mind.memories = [f"Memory {i}" for i in range(30)]
        compacted = mind.summarize_memory()
        self.assertEqual(len(compacted), 13)
        self.assertTrue(compacted[0].startswith("Summary"))
        self.assertEqual(len(mind.memories), 13)

    def test_mind_auto_compaction_on_decide(self):
        mind = ScriptedMind()
        mind.memories = [f"Memory {i}" for i in range(30)]
        mind.decide(make_persona(), make_situation())
        self.assertEqual(len(mind.memories), 13)
        self.assertTrue(mind.memories[0].startswith("Summary"))


class MockTownForGen:
    def __init__(self):
        self.placed = []

    def place(self, type, x, y, floors=None, id=None):
        self.placed.append({"type": type, "x": x, "y": y, "floors": floors, "id": id})
        return True


class TownGenEnhancementsTest(unittest.TestCase):
    def test_water_bridge_and_castle_generated(self):
        town = MockTownForGen()
        plan = generate_town(town, ["Ada", "Ben"], seed=1)
        types = [o["type"] for o in town.placed]
        self.assertIn("water", types)
        self.assertIn("bridge", types)
        self.assertIn("castle", types)

    def test_spawn_npcs_flag(self):
        town_no_npc = MockTownForGen()
        generate_town(town_no_npc, ["Ada", "Ben"], seed=1, spawn_npcs=False)
        self.assertNotIn("npc", [o["type"] for o in town_no_npc.placed])

        town_with_npc = MockTownForGen()
        generate_town(town_with_npc, ["Ada", "Ben"], seed=1, spawn_npcs=True)
        self.assertIn("npc", [o["type"] for o in town_with_npc.placed])

    def test_place_resident_npcs_helper(self):
        town = MockTownForGen()
        plan = generate_town(town, ["Ada", "Ben"], seed=1, spawn_npcs=False)
        placed_npcs = place_resident_npcs(town, plan)
        self.assertGreater(len(placed_npcs), 0)
        self.assertIn("npc", [o["type"] for o in town.placed])


class ParallelSimulationTest(unittest.TestCase):
    def test_parallel_turn_execution(self):
        class MockSimTown:
            def spawn(self, name, x, y):
                return len(name)

            def observe(self, agent, radius=10):
                return {"tick": 0, "self": {"agent": agent, "name": "Agent", "x": 0, "y": 0, "moving": False}, "objects": [], "agents": []}

            def step(self, ticks=1):
                return {"tick": 1, "agents": [
                    {"agent": 3, "name": "Ada", "x": 0, "y": 0, "moving": False},
                    {"agent": 3, "name": "Ben", "x": 2, "y": 2, "moving": False},
                    {"agent": 4, "name": "Cleo", "x": 4, "y": 4, "moving": False},
                ]}

            def go(self, agent, x, y):
                return 0

            def place(self, type, x, y, floors=None, id=None):
                return True

        mock_town = MockSimTown()
        plan = generate_town(MockTownForGen(), [p.name for p in demo.PERSONAS[:3]], seed=1)
        sim = demo.Simulation(mock_town, plan, demo.PERSONAS[:3], ScriptedMind(1), parallel=True)
        self.assertTrue(sim.parallel)
        sim.turn()
        self.assertEqual(len(sim.frames), demo.TICKS_PER_TURN + 1)


class TownSayTest(unittest.TestCase):
    def test_say_and_speech(self):
        town = Town.__new__(Town)
        calls = []

        def fake_call(cmd, **fields):
            calls.append((cmd, fields))
            if cmd == "speech":
                return {"ok": True, "events": [{"tick": 1, "agent": 0, "name": "Ada", "text": "Hello"}]}
            if cmd == "observe":
                return {
                    "ok": True,
                    "tick": 1,
                    "self": {"agent": 0, "name": "Ada", "x": 0, "y": 0, "moving": False},
                    "objects": [],
                    "agents": [],
                    "speech": [{"tick": 1, "agent": 1, "name": "Ben", "text": "Hi!", "distance": 2.0}],
                }
            return {"ok": True}

        town._call = fake_call

        town.say(0, "Good morning!")
        self.assertEqual(calls[0], ("say", {"agent": 0, "text": "Good morning!"}))

        events = town.speech()
        self.assertEqual(len(events), 1)
        self.assertEqual(events[0]["text"], "Hello")

        obs = town.observe(0)
        self.assertIn("speech", obs)
        self.assertEqual(obs["speech"][0]["text"], "Hi!")
        self.assertEqual(obs["tick"], 1)
        self.assertEqual(obs["objects"], [])


class TownProcessFailureTest(unittest.TestCase):
    def test_exited_engine_is_a_clear_error(self):
        # Python rejects the unknown `--agent-server` option and exits at once.
        town = Town(binary=sys.executable)
        try:
            with self.assertRaisesRegex(RuntimeError, "engine exited with code"):
                town.objects()
        finally:
            town.close()

    @unittest.skipIf(os.name == "nt", "needs a shebang script")
    def test_silent_engine_times_out(self):
        with tempfile.TemporaryDirectory() as directory:
            script = Path(directory) / "silent"
            script.write_text(f"#!{sys.executable}\nimport time\ntime.sleep(60)\n", encoding="utf-8")
            script.chmod(0o755)
            town = Town(binary=script, timeout=0.5)
            try:
                with self.assertRaisesRegex(TimeoutError, "'objects'"):
                    town.objects()
            finally:
                town.close()


if __name__ == "__main__":
    unittest.main()

