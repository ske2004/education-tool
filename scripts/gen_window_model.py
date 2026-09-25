# Generates assets/models/cube_window.{png,bin,gltf}. Run from the repo root.
import struct, json
from PIL import Image

# --- Texture: symmetric under horizontal/vertical flips so face orientation doesn't matter
N = 64
FRAME = (0xEC, 0xEF, 0xF1, 255)
FRAME_SHADOW = (0xB0, 0xBE, 0xC5, 255)
GLASS = (0x81, 0xD4, 0xFA, 255)
GLASS_EDGE = (0x4F, 0xA3, 0xD1, 255)

img = Image.new("RGBA", (N, N))
px = img.load()
BORDER, MULLION = 6, 4
mid_lo, mid_hi = N // 2 - MULLION // 2, N // 2 + MULLION // 2
for y in range(N):
    for x in range(N):
        def dist_to_frame(c):
            # distance (in px) from coordinate c to the nearest frame bar along that axis
            if c < BORDER or c >= N - BORDER or mid_lo <= c < mid_hi:
                return -1
            return min(c - BORDER, N - BORDER - 1 - c, abs(c - (mid_lo - 1)), abs(c - mid_hi))
        dx, dy = dist_to_frame(x), dist_to_frame(y)
        if dx < 0 or dy < 0:
            # frame; darken its inner edge next to the glass
            fx = x in (BORDER - 1, N - BORDER, mid_lo, mid_hi - 1)
            fy = y in (BORDER - 1, N - BORDER, mid_lo, mid_hi - 1)
            inner = (fx and dx < 0 and dy >= 0) or (fy and dy < 0 and dx >= 0)
            px[x, y] = FRAME_SHADOW if inner else FRAME
        else:
            d = min(dx, dy)
            if d == 0:
                px[x, y] = GLASS_EDGE
            else:
                px[x, y] = GLASS
img.save("assets/models/cube_window.png")

# --- Mesh: unit cube, each face mapped to the full 0..1 UV square
faces = [  # normal, right, up  (right x up == normal, so CCW from outside)
    ((0, 0, 1), (1, 0, 0), (0, 1, 0)),
    ((0, 0, -1), (-1, 0, 0), (0, 1, 0)),
    ((1, 0, 0), (0, 0, -1), (0, 1, 0)),
    ((-1, 0, 0), (0, 0, 1), (0, 1, 0)),
    ((0, 1, 0), (1, 0, 0), (0, 0, -1)),
    ((0, -1, 0), (1, 0, 0), (0, 0, 1)),
]
pos, nrm, uv, idx = [], [], [], []
for n, r, u in faces:
    base = len(pos)
    for s, t in ((-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5)):
        pos.append(tuple(n[i] * 0.5 + r[i] * s + u[i] * t for i in range(3)))
        nrm.append(n)
        uv.append((s + 0.5, 0.5 - t))  # glTF v points down
    idx += [base, base + 1, base + 2, base, base + 2, base + 3]

buf = b"".join(struct.pack("<3f", *p) for p in pos)
buf += b"".join(struct.pack("<3f", *p) for p in nrm)
buf += b"".join(struct.pack("<2f", *p) for p in uv)
buf += struct.pack(f"<{len(idx)}H", *idx)
assert len(buf) == 840
open("assets/models/cube_window.bin", "wb").write(buf)

g = json.load(open("assets/models/cube.gltf"))
g["images"][0]["name"] = "cube_window"
g["images"][0]["uri"] = "cube_window.png"
g["buffers"][0]["uri"] = "cube_window.bin"
g["asset"]["generator"] = "scripts/gen_window_model.py"
out = json.dumps(g, indent="\t", separators=(",", ":"))
open("assets/models/cube_window.gltf", "w").write(out + "\n")
