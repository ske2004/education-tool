"""Python client for the engine's headless viewport server.

Spawns `educore --viewport-server` and communicates with it to render offscreen
frames and capture RGBA viewport images without opening an interactive GUI window.

Example usage:

    with Viewport(width=640, height=480) as vp:
        frame = vp.render(pos=(0, 10, 10), look_at=(0, 0, 0), fov=60)
        frame.save_png("screenshot.png")
"""

import json
import os
import struct
import subprocess
import zlib
from pathlib import Path
from typing import Optional, Sequence, Tuple, Union

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BINARY = REPO_ROOT / "bin" / ("educore.exe" if os.name == "nt" else "educore")


class ViewportError(RuntimeError):
    """The viewport client or server encountered an error."""


def write_png(width: int, height: int, rgba: bytes) -> bytes:
    """Encodes raw RGBA bytes into PNG format using standard library zlib and struct.

    Args:
        width: Image width in pixels.
        height: Image height in pixels.
        rgba: Raw bytes in RGBA order (width * height * 4 bytes).

    Returns:
        bytes: Complete valid PNG file content.
    """
    expected_len = width * height * 4
    if len(rgba) != expected_len:
        raise ValueError(
            f"Expected {expected_len} RGBA bytes ({width}x{height}x4), got {len(rgba)}"
        )

    def _chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    # 8-byte PNG signature
    header = b"\x89PNG\r\n\x1a\n"

    # IHDR: width (4), height (4), bit depth (1: 8), color type (1: 6 = RGBA),
    # compression (1: 0 = deflate), filter (1: 0), interlace (1: 0 = no interlace)
    ihdr_payload = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    ihdr = _chunk(b"IHDR", ihdr_payload)

    # IDAT: filter byte 0 (None) before each scanline row
    row_bytes = width * 4
    raw_scanlines = bytearray((row_bytes + 1) * height)
    for y in range(height):
        dest_idx = y * (row_bytes + 1)
        raw_scanlines[dest_idx] = 0
        src_idx = y * row_bytes
        raw_scanlines[dest_idx + 1 : dest_idx + 1 + row_bytes] = rgba[
            src_idx : src_idx + row_bytes
        ]

    compressed = zlib.compress(bytes(raw_scanlines))
    idat = _chunk(b"IDAT", compressed)

    # IEND
    iend = _chunk(b"IEND", b"")

    return header + ihdr + idat + iend


def save_png(path: Union[str, Path], width: int, height: int, rgba: bytes) -> None:
    """Saves raw RGBA bytes as a PNG image file."""
    png_data = write_png(width, height, rgba)
    Path(path).write_bytes(png_data)


class Frame:
    """A single captured RGBA frame from the viewport server."""

    def __init__(self, width: int, height: int, data: bytes):
        self.width = width
        self.height = height
        self.data = data

    def to_png(self) -> bytes:
        """Encodes this frame into PNG bytes."""
        return write_png(self.width, self.height, self.data)

    def save_png(self, path: Union[str, Path]) -> None:
        """Saves this frame to disk as a PNG file."""
        save_png(path, self.width, self.height, self.data)

    def __len__(self) -> int:
        return len(self.data)

    def __repr__(self) -> str:
        return f"<Frame {self.width}x{self.height} ({len(self.data)} bytes)>"


class Viewport:
    """A running `educore --viewport-server` process context manager."""

    def __init__(
        self,
        width: int = 400,
        height: int = 300,
        binary: Optional[Union[str, Path]] = None,
        show_engine_log: bool = False,
    ):
        self.width = width
        self.height = height

        binary_path = Path(binary or os.environ.get("EDUCORE_BIN") or DEFAULT_BINARY)
        if not binary_path.exists():
            raise FileNotFoundError(
                f"{binary_path} doesn't exist, build the engine first."
            )

        cmd = [
            str(binary_path),
            "--viewport-server",
            "--width",
            str(width),
            "--height",
            str(height),
        ]

        self._process = subprocess.Popen(
            cmd,
            cwd=REPO_ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=None if show_engine_log else subprocess.DEVNULL,
            bufsize=0,
        )

    def __enter__(self) -> "Viewport":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def close(self) -> None:
        """Closes the viewport server process."""
        if self._process.poll() is None:
            try:
                self._send({"cmd": "quit"})
            except (OSError, BrokenPipeError):
                pass

            if self._process.stdin:
                try:
                    self._process.stdin.close()
                except OSError:
                    pass

            try:
                self._process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._process.terminate()
                try:
                    self._process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    self._process.kill()

        if self._process.stdout:
            try:
                self._process.stdout.close()
            except OSError:
                pass

    def _send(self, payload: dict) -> None:
        """Sends a JSON command line to the server via stdin."""
        if self._process.poll() is not None:
            raise ViewportError("Viewport server is not running")

        line = json.dumps(payload) + "\n"
        try:
            self._process.stdin.write(line.encode("utf-8"))
            self._process.stdin.flush()
        except (BrokenPipeError, OSError) as err:
            raise ViewportError(f"Failed to send command to viewport server: {err}")

    def _read_exact(self, n: int) -> bytes:
        """Reads exactly n bytes from the server stdout binary stream."""
        buf = bytearray()
        while len(buf) < n:
            chunk = self._process.stdout.read(n - len(buf))
            if not chunk:
                if not buf:
                    raise EOFError("Viewport server closed stdout pipe")
                raise EOFError(
                    f"Unexpected EOF reading frame: got {len(buf)} of {n} bytes"
                )
            buf.extend(chunk)
        return bytes(buf)

    def read_frame(self) -> Frame:
        """Reads the next binary frame from the server stdout stream.

        Supports both standard EVPF magic header (`EVPF` + width + height + RGBA)
        and binary header ([u32 width][u32 height][u32 size][RGBA]) formats.
        """
        first_4 = self._read_exact(4)
        if first_4 == b"EVPF":
            # EVPF magic header format
            dim_bytes = self._read_exact(8)
            width, height = struct.unpack("<II", dim_bytes)
            expected_pixels = width * height * 4

            # Check if an explicit u32 size field follows
            next_4 = self._read_exact(4)
            val = struct.unpack("<I", next_4)[0]
            if val == expected_pixels:
                # Size field was present, read full pixel data
                pixels = self._read_exact(expected_pixels)
            else:
                # next_4 is the first 4 bytes of pixel data
                rest = self._read_exact(expected_pixels - 4)
                pixels = next_4 + rest
        else:
            # Binary format: [u32 width][u32 height][u32 size][pixels]
            width = struct.unpack("<I", first_4)[0]
            rest_hdr = self._read_exact(8)
            height, size = struct.unpack("<II", rest_hdr)
            expected_pixels = width * height * 4
            pixel_bytes = size if size > 0 else expected_pixels
            pixels = self._read_exact(pixel_bytes)

        self.width = width
        self.height = height
        return Frame(width=width, height=height, data=pixels)

    def render(
        self,
        pos: Optional[Sequence[float]] = None,
        look_at: Optional[Sequence[float]] = None,
        fov: float = 60.0,
    ) -> Frame:
        """Sends camera parameters and returns the rendered frame."""
        cmd: dict = {"cmd": "render", "fov": fov}
        if pos is not None:
            cmd["pos"] = [float(v) for v in pos]
        if look_at is not None:
            cmd["look_at"] = [float(v) for v in look_at]

        self._send(cmd)
        return self.read_frame()

    def resize(self, width: int, height: int) -> None:
        """Resizes the offscreen render target."""
        self._send({"cmd": "resize", "w": float(width), "h": float(height)})
        self.width = width
        self.height = height

    def mouse_motion(
        self,
        dx: float,
        dy: float,
        middle: bool = False,
        right: bool = False,
    ) -> None:
        """Sends mouse motion for viewport navigation (pan/orbit)."""
        self._send(
            {
                "cmd": "mouse_motion",
                "dx": float(dx),
                "dy": float(dy),
                "middle": bool(middle),
                "right": bool(right),
            }
        )

    def mouse_wheel(self, delta: float) -> None:
        """Sends mouse wheel delta for zooming."""
        self._send({"cmd": "mouse_wheel", "delta": float(delta)})

    def save_png(
        self, path: Union[str, Path], frame: Optional[Frame] = None
    ) -> None:
        """Convenience method to save either a given frame or a new frame as PNG."""
        if frame is None:
            frame = self.read_frame()
        frame.save_png(path)
