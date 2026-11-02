#!/usr/bin/env python3
"""Build a self-contained interactive Educore demo showcase in agents/out/.

Runs the headless C++ agent server simulation (5 residents, 15 turns) to
produce:
  - agents/out/town.glb
  - agents/out/town.dat
  - agents/out/replay.json
  - agents/out/town_life.html
  - agents/out/showcase.html
"""

import argparse
import base64
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "agents"))

import demo  # noqa: E402


def parse_glb_summary(glb_path: Path) -> dict:
    """Extract live glTF 2.0 statistics from a binary .glb file."""
    raw = glb_path.read_bytes()
    if len(raw) < 20 or raw[:4] != b"glTF":
        return {"size_kb": round(len(raw) / 1024, 1)}

    json_len, json_type = struct.unpack_from("<II", raw, 12)
    if json_type != 0x4E4F534A:
        return {"size_kb": round(len(raw) / 1024, 1)}

    gltf = json.loads(raw[20 : 20 + json_len].decode("utf-8"))
    nodes = gltf.get("nodes", [])
    meshes = gltf.get("meshes", [])
    materials = gltf.get("materials", [])
    scenes = gltf.get("scenes", [])
    lights = (
        gltf.get("extensions", {})
        .get("KHR_lights_punctual", {})
        .get("lights", [])
    )

    type_counts = {}
    named_entities = []
    for node in nodes:
        extras = node.get("extras") or {}
        etype = extras.get("educore_type") or extras.get("type")
        if etype:
            type_counts[etype] = type_counts.get(etype, 0) + 1
            if extras.get("id") or etype in ("building", "castle", "npc", "item", "prop"):
                named_entities.append(
                    {
                        "name": node.get("name", etype),
                        "type": etype,
                        "id": extras.get("id", ""),
                        "floors": extras.get("floors"),
                        "translation": node.get("translation", [0, 0, 0]),
                    }
                )

    return {
        "size_kb": round(len(raw) / 1024, 1),
        "scenes": len(scenes),
        "nodes": len(nodes),
        "meshes": len(meshes),
        "materials": len(materials),
        "lights": len(lights),
        "extensions_used": gltf.get("extensionsUsed", []),
        "world_extras": (scenes[0].get("extras") if scenes else {}) or {},
        "type_counts": type_counts,
        "sample_entities": named_entities[:24],
    }


def render_showcase_html(
    out_dir: Path,
    glb_summary: dict,
    replay: dict,
) -> str:
    glb_b64 = base64.b64encode((out_dir / "town.glb").read_bytes()).decode("ascii")
    dat_b64 = base64.b64encode((out_dir / "town.dat").read_bytes()).decode("ascii")
    replay_json_str = json.dumps(replay, ensure_ascii=False).replace("</", "<\\/")
    summary_json_str = json.dumps(glb_summary, ensure_ascii=False).replace("</", "<\\/")

    say_events = sum(1 for e in replay.get("events", []) if e.get("kind") == "say")
    move_events = sum(1 for e in replay.get("events", []) if e.get("kind") == "go")

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Educore — 3D World Studio, Agent Simulation &amp; glTF 2.0 Pipeline</title>
  <style>
    :root {{
      --bg: #0b0f19;
      --panel: #111827;
      --panel-alt: #1e293b;
      --border: rgba(255, 255, 255, 0.1);
      --text: #f3f4f6;
      --muted: #9ca3af;
      --accent: #38bdf8;
      --emerald: #10b981;
      --amber: #f59e0b;
      --purple: #a855f7;
    }}
    * {{ box-sizing: border-box; }}
    html, body {{
      margin: 0;
      padding: 0;
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Inter, sans-serif;
      line-height: 1.5;
    }}
    a {{ color: var(--accent); text-decoration: none; }}
    .container {{
      max-width: 1360px;
      margin: 0 auto;
      padding: 24px;
    }}
    .hero {{
      background: linear-gradient(135deg, rgba(17, 24, 39, 0.96), rgba(15, 23, 42, 0.92));
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 28px 32px;
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 20px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.45);
      margin-bottom: 24px;
    }}
    .hero-title {{
      max-width: 720px;
    }}
    .badge-row {{
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-bottom: 10px;
    }}
    .badge {{
      font-size: 11.5px;
      font-weight: 600;
      padding: 3px 10px;
      border-radius: 999px;
      background: rgba(56, 189, 248, 0.14);
      color: var(--accent);
      border: 1px solid rgba(56, 189, 248, 0.3);
    }}
    .badge.emerald {{
      background: rgba(16, 185, 129, 0.14);
      color: #34d399;
      border-color: rgba(16, 185, 129, 0.3);
    }}
    .badge.amber {{
      background: rgba(245, 158, 11, 0.14);
      color: #fbbf24;
      border-color: rgba(245, 158, 11, 0.3);
    }}
    .hero h1 {{
      margin: 0 0 8px;
      font-size: 28px;
      letter-spacing: -0.02em;
    }}
    .hero p {{
      margin: 0;
      color: var(--muted);
      font-size: 14.5px;
    }}
    .hero-actions {{
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }}
    .btn {{
      appearance: none;
      border: 1px solid var(--border);
      background: var(--panel-alt);
      color: var(--text);
      font-size: 13px;
      font-weight: 600;
      padding: 9px 15px;
      border-radius: 8px;
      cursor: pointer;
      transition: background 0.15s, border-color 0.15s, transform 0.1s;
    }}
    .btn:hover {{
      background: rgba(255, 255, 255, 0.12);
      border-color: rgba(255, 255, 255, 0.25);
    }}
    .btn-primary {{
      background: linear-gradient(135deg, #0284c7, #059669);
      border-color: rgba(56, 189, 248, 0.45);
      color: #fff;
    }}
    .stats-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
      gap: 14px;
      margin-bottom: 24px;
    }}
    .stat-card {{
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 16px 18px;
    }}
    .stat-label {{
      font-size: 12px;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.04em;
    }}
    .stat-value {{
      font-size: 24px;
      font-weight: 700;
      margin-top: 4px;
      color: var(--text);
    }}
    .stat-sub {{
      font-size: 12px;
      color: var(--accent);
      margin-top: 2px;
    }}
    .viewer-shell {{
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 16px;
      overflow: hidden;
      margin-bottom: 28px;
      box-shadow: 0 18px 40px rgba(0, 0, 0, 0.35);
    }}
    .viewer-header {{
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 12px 18px;
      background: rgba(15, 23, 42, 0.9);
      border-bottom: 1px solid var(--border);
    }}
    .viewer-header h2 {{
      margin: 0;
      font-size: 15px;
      font-weight: 600;
    }}
    .viewer-frame {{
      width: 100%;
      height: 640px;
      border: 0;
      display: block;
      background: #17181b;
    }}
    .cards-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(380px, 1fr));
      gap: 20px;
      margin-bottom: 28px;
    }}
    .card {{
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 14px;
      padding: 22px;
    }}
    .card h3 {{
      margin: 0 0 8px;
      font-size: 17px;
      display: flex;
      align-items: center;
      gap: 8px;
    }}
    .card p {{
      margin: 0 0 14px;
      color: var(--muted);
      font-size: 13.5px;
    }}
    pre {{
      margin: 0;
      padding: 12px 14px;
      background: #090d16;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 8px;
      color: #e2e8f0;
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
      font-size: 12px;
      overflow-x: auto;
    }}
    table {{
      width: 100%;
      border-collapse: collapse;
      font-size: 12.5px;
    }}
    th, td {{
      text-align: left;
      padding: 7px 10px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.07);
    }}
    th {{
      color: var(--muted);
      font-weight: 600;
    }}
    .type-pill {{
      display: inline-block;
      padding: 1px 7px;
      border-radius: 999px;
      font-size: 11px;
      font-weight: 600;
      background: rgba(56, 189, 248, 0.14);
      color: var(--accent);
    }}
  </style>
</head>
<body>
  <div class="container">
    <section class="hero">
      <div class="hero-title">
        <div class="badge-row">
          <span class="badge">C++17 / Sokol 3D Engine</span>
          <span class="badge emerald">Headless Agent Server</span>
          <span class="badge amber">glTF 2.0 + KHR_lights_punctual</span>
          <span class="badge">Umka Visual Scripting</span>
          <span class="badge emerald">Playable STEM Quest</span>
        </div>
        <h1>Educore Interactive Demo Showcase</h1>
        <p>
          Build procedural 3D towns in the native or WebAssembly studio, playtest STEM quests with NPC dialogue and inventory, drive autonomous AI residents over the headless JSON server, and export standard binary <code>.glb</code> scenes to Three.js, Godot 4, and Unity.
        </p>
      </div>
      <div class="hero-actions">
        <button class="btn btn-primary" type="button" onclick="downloadAsset('town.glb', GLB_B64, 'model/gltf-binary')">Download town.glb ({glb_summary.get("size_kb", 0)} KB)</button>
        <button class="btn" type="button" onclick="downloadAsset('town.dat', DAT_B64, 'application/octet-stream')">Download town.dat</button>
        <button class="btn" type="button" onclick="downloadJson('replay.json', REPLAY_DATA)">Download replay.json</button>
        <a class="btn" href="town_life.html" target="_blank" rel="noopener">Open Fullscreen 3D Replay ↗</a>
      </div>
    </section>

    <section class="stats-grid">
      <div class="stat-card">
        <div class="stat-label">Binary glTF 2.0 Output</div>
        <div class="stat-value">{glb_summary.get("size_kb", 0)} KB</div>
        <div class="stat-sub">{glb_summary.get("nodes", 0)} nodes · {glb_summary.get("meshes", 0)} merged meshes</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">PBR &amp; Punctual Lighting</div>
        <div class="stat-value">{glb_summary.get("materials", 0)} Materials</div>
        <div class="stat-sub">{glb_summary.get("lights", 0)} KHR_lights_punctual directional sun</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">Simulated Residents</div>
        <div class="stat-value">{len(replay.get("agents", []))} Agents</div>
        <div class="stat-sub">{len(replay.get("frames", []))} ticks · {say_events} dialogues · {move_events} trips</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">Town Locations</div>
        <div class="stat-value">{len(replay.get("locations", []))} Places</div>
        <div class="stat-sub">{glb_summary.get("scenes", 1)} scenes (Exterior + Interiors)</div>
      </div>
    </section>

    <section class="viewer-shell">
      <div class="viewer-header">
        <h2>Interactive 3D Town Simulation — Live glTF 2.0 Scene + Multi-Agent Replay</h2>
        <span style="font-size: 12px; color: var(--muted);">Drag to orbit · Scroll to zoom · Scrub timeline or toggle thoughts below</span>
      </div>
      <iframe class="viewer-frame" id="replay-iframe" src="town_life.html" title="Educore 3D Town Life Replay"></iframe>
    </section>

    <section class="cards-grid">
      <div class="card">
        <h3>1. C++17 / Sokol 3D Editor &amp; STEM Quest</h3>
        <p>
          Create buildings, castles, bridges, roads, rivers, trees, animals, NPCs, items, and 18 procedural prop types. Press <code>Q</code> / <code>E</code> to cycle NPC, Item, and Prop presets, or click <strong>Sample</strong> to play the built-in Solar Observatory STEM restoration quest.
        </p>
        <pre># Launch the interactive 3D editor or WebAssembly build
./bin/debug/catedu
./bin/debug/catedu --export-gltf assets/world.glb

# Or run the automated headless verification suite
python3 build.py test</pre>
      </div>

      <div class="card">
        <h3>2. Headless Agent Server &amp; Python Town SDK</h3>
        <p>
          Run <code>catedu --agent-server</code> for a zero-GUI simulation server speaking line-delimited JSON over stdin/stdout with A* road/bridge pathfinding, spatial observation queries, and instant <code>.glb</code> / <code>.dat</code> snapshots.
        </p>
        <pre>{{"op": "spawn", "name": "Ada", "x": 4, "y": 2}}
{{"op": "observe", "agent": 1, "radius": 14}}
{{"op": "go", "agent": 1, "x": 12, "y": 8}}
{{"op": "step", "ticks": 10}}
{{"op": "export_gltf", "path": "agents/out/town.glb"}}</pre>
      </div>

      <div class="card">
        <h3>3. glTF 2.0 Metadata &amp; Godot / Unity Importers</h3>
        <p>
          Every exported node carries structured <code>extras</code> (<code>educore_type</code>, <code>id</code>, <code>floors</code>, <code>grid</code>, plus world-level <code>script</code> and <code>time_of_day</code>). Drop the included import scripts into Godot 4 or Unity to auto-attach colliders and gameplay tags.
        </p>
        <table>
          <thead>
            <tr><th>Node Name</th><th>Type</th><th>ID / Role</th><th>Position (glTF)</th></tr>
          </thead>
          <tbody id="entity-table-body"></tbody>
        </table>
      </div>
    </section>
  </div>

  <script>
    const GLB_B64 = "{glb_b64}";
    const DAT_B64 = "{dat_b64}";
    const REPLAY_DATA = {replay_json_str};
    const GLB_SUMMARY = {summary_json_str};

    function downloadAsset(filename, b64, mime) {{
      const bytes = Uint8Array.from(atob(b64), c => c.charCodeAt(0));
      const blob = new Blob([bytes], {{ type: mime }});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      setTimeout(() => URL.revokeObjectURL(url), 2000);
    }}

    function downloadJson(filename, data) {{
      const blob = new Blob([JSON.stringify(data, null, 2)], {{ type: 'application/json' }});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      setTimeout(() => URL.revokeObjectURL(url), 2000);
    }}

    const tbody = document.getElementById('entity-table-body');
    (GLB_SUMMARY.sample_entities || []).slice(0, 8).forEach(ent => {{
      const tr = document.createElement('tr');
      const pos = (ent.translation || [0, 0, 0]).map(v => Number(v).toFixed(1)).join(', ');
      const detail = ent.id || (ent.floors ? ent.floors + ' floors' : '—');
      tr.innerHTML = `<td><code>${{ent.name}}</code></td><td><span class="type-pill">${{ent.type}}</span></td><td>${{detail}}</td><td>(${{pos}})</td>`;
      tbody.appendChild(tr);
    }});
  </script>
</body>
</html>
"""


def ensure_normal_binary() -> None:
    """Ensure bin/educore is built in NORMAL mode (not UNITTEST mode)."""
    import subprocess

    binary = ROOT / "bin" / "educore"
    cache = ROOT / "bin" / "CMakeCache.txt"
    needs_build = not binary.exists()
    if cache.exists() and "ENV_TYPE:UNINITIALIZED=UNITTEST" in cache.read_text(encoding="utf-8", errors="ignore"):
        needs_build = True
    if needs_build:
        subprocess.run([sys.executable, str(ROOT / "build.py"), "build"], cwd=ROOT, check=True)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--agents", type=int, default=5, help="number of town residents (default 5)")
    parser.add_argument("--turns", type=int, default=15, help="number of turns to simulate (default 15)")
    parser.add_argument("--seed", type=int, default=7, help="town layout seed (default 7)")
    parser.add_argument("--out", type=Path, default=ROOT / "agents" / "out", help="output directory")
    parser.add_argument(
        "--firefox",
        "--open",
        dest="open_browser",
        action="store_true",
        help="open the generated showcase.html in Firefox",
    )
    args = parser.parse_args(argv)

    ensure_normal_binary()

    out_dir = args.out.resolve()
    demo.main(
        [
            "--offline",
            "--agents",
            str(args.agents),
            "--turns",
            str(args.turns),
            "--seed",
            str(args.seed),
            "--out",
            str(out_dir),
            "--quiet",
        ]
    )

    glb_path = out_dir / "town.glb"
    replay_path = out_dir / "replay.json"
    glb_summary = parse_glb_summary(glb_path)
    replay = json.loads(replay_path.read_text(encoding="utf-8"))

    showcase_path = out_dir / "showcase.html"
    showcase_path.write_text(
        render_showcase_html(out_dir, glb_summary, replay),
        encoding="utf-8",
    )

    print(
        f"Showcase ready: {showcase_path} "
        f"({glb_summary.get('nodes', 0)} glTF nodes, "
        f"{glb_summary.get('meshes', 0)} meshes, "
        f"{len(replay.get('events', []))} agent events)"
    )
    if args.open_browser:
        demo.open_in_firefox(showcase_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
