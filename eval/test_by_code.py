#!/usr/bin/env python3
"""Build and run the renderer, then compare its PNG with the checked-in reference."""
from __future__ import annotations

import json
import shutil
import struct
import subprocess
import sys
from pathlib import Path
import zlib

ROOT = Path(__file__).resolve().parents[1]
WORKSPACE = ROOT / "workspace"
EVAL = ROOT / "eval"
BUILD = EVAL / "build"
REFERENCE = ROOT / "test_files" / "data" / "reference.png"
OUTPUT = BUILD / "raytracing_test.png"
RESULT = EVAL / "code_result.json"


def run(command: list[str]) -> None:
    print("$", " ".join(command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def read_png(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG")
    offset = 8
    width = height = None
    bit_depth = color_type = interlace = None
    chunks = []
    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filt, interlace = struct.unpack(">IIBBBBB", payload)
            if (bit_depth, color_type, compression, filt, interlace) != (8, 6, 0, 0, 0):
                raise ValueError("only non-interlaced RGBA8 PNGs are supported")
        elif kind == b"IDAT":
            chunks.append(payload)
        elif kind == b"IEND":
            break
    if width is None or height is None:
        raise ValueError("PNG has no IHDR")
    raw = zlib.decompress(b"".join(chunks))
    row_bytes = width * 4
    if len(raw) != height * (row_bytes + 1):
        raise ValueError("unexpected PNG data size")
    pixels = bytearray(height * row_bytes)
    previous = bytearray(row_bytes)
    cursor = 0
    for y in range(height):
        filter_type = raw[cursor]
        cursor += 1
        row = bytearray(raw[cursor : cursor + row_bytes])
        cursor += row_bytes
        for x in range(row_bytes):
            left = row[x - 4] if x >= 4 else 0
            up = previous[x]
            up_left = previous[x - 4] if x >= 4 else 0
            if filter_type == 1:
                row[x] = (row[x] + left) & 255
            elif filter_type == 2:
                row[x] = (row[x] + up) & 255
            elif filter_type == 3:
                row[x] = (row[x] + ((left + up) // 2)) & 255
            elif filter_type == 4:
                p = left + up - up_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - up_left)
                predictor = left if pa <= pb and pa <= pc else up if pb <= pc else up_left
                row[x] = (row[x] + predictor) & 255
            elif filter_type != 0:
                raise ValueError(f"unsupported PNG filter {filter_type}")
        pixels[y * row_bytes : (y + 1) * row_bytes] = row
        previous = row
    return width, height, bytes(pixels)


def write_result(resolved: bool, score: float, reason: str) -> int:
    RESULT.write_text(json.dumps({"resolved": resolved, "score": score, "reason": reason}, ensure_ascii=False) + "\n")
    return 0 if resolved else 1


def main() -> int:
    try:
        BUILD.mkdir(parents=True, exist_ok=True)
        # A pre-existing build directory may contain a cache generated with a
        # different toolchain. Reset only that incompatible CMake cache; all
        # compatible object files remain available for incremental builds.
        expected_toolchain = (WORKSPACE / "external" / "vcpkg" / "scripts" /
                              "buildsystems" / "vcpkg.cmake").resolve()
        cache = BUILD / "CMakeCache.txt"
        if cache.exists():
            cached_toolchain = None
            for line in cache.read_text(errors="ignore").splitlines():
                if line.startswith("CMAKE_TOOLCHAIN_FILE:FILEPATH="):
                    cached_toolchain = Path(line.split("=", 1)[1]).resolve()
                    break
            system_files = list((BUILD / "CMakeFiles").glob("*/CMakeSystem.cmake"))
            stale_system = any(str(expected_toolchain) not in system.read_text(errors="ignore")
                               for system in system_files)
            if cached_toolchain != expected_toolchain or stale_system:
                cache.unlink()
                shutil.rmtree(BUILD / "CMakeFiles", ignore_errors=True)
        run(["cmake", "-S", str(WORKSPACE), "-B", str(BUILD), "-DCMAKE_BUILD_TYPE=Release"])
        run(["cmake", "--build", str(BUILD), "--config", "Release", "--parallel"])
        executable = BUILD / "src" / "simple_raytracer"
        if not executable.exists():
            candidates = list(BUILD.rglob("simple_raytracer"))
            if not candidates:
                return write_result(False, 0.0, "compiled executable was not found")
            executable = candidates[0]
        run([str(executable), str(OUTPUT), "1280", "720"])
        actual_w, actual_h, actual = read_png(OUTPUT)
        ref_w, ref_h, reference = read_png(REFERENCE)
        if (actual_w, actual_h) != (ref_w, ref_h) or len(actual) != len(reference):
            return write_result(False, 0.0, "output dimensions do not match reference")
        # Match origin/3-raytracing/test/main.cpp: compare RGB channels with a
        # per-channel tolerance of 4 and require more than 99% matching pixels.
        pixel_count = actual_w * actual_h
        match_count = 0
        for pixel in range(pixel_count):
            offset = pixel * 4
            if all(abs(actual[offset + channel] - reference[offset + channel]) <= 4
                   for channel in range(3)):
                match_count += 1
        match_rate = match_count / pixel_count
        if match_rate <= 0.99:
            return write_result(False, round(match_rate, 4),
                                f"image match rate {match_rate:.4f} is below 0.99")
        return write_result(True, round(match_rate, 4),
                            "build, render, and image comparison passed")
    except (subprocess.CalledProcessError, OSError, ValueError) as exc:
        return write_result(False, 0.0, f"test failed: {exc}")


if __name__ == "__main__":
    sys.exit(main())
