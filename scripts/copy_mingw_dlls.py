#!/usr/bin/env python3
"""Copy the SDL MinGW runtime DLLs next to a validation executable.

The helper intentionally uses only the Python standard library so the same
Makefile target works from PowerShell, CMD, MSYS2, and Linux.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import sys


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mingw-root", required=True, type=Path)
    parser.add_argument("--triplet", required=True)
    parser.add_argument("--sdl2-version", required=True)
    parser.add_argument("--sdl2-image-version", required=True)
    parser.add_argument("--sdl2-ttf-version", required=True)
    parser.add_argument("--sdl2-mixer-version", required=True)
    parser.add_argument("--destination", required=True, type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    packages = (
        (f"SDL2-{args.sdl2_version}", "SDL2.dll"),
        (f"SDL2_image-{args.sdl2_image_version}", "SDL2_image.dll"),
        (f"SDL2_ttf-{args.sdl2_ttf_version}", "SDL2_ttf.dll"),
        (f"SDL2_mixer-{args.sdl2_mixer_version}", "SDL2_mixer.dll"),
    )
    copies = [
        (args.mingw_root / package / args.triplet / "bin" / dll, args.destination / dll)
        for package, dll in packages
    ]
    missing = [source for source, _ in copies if not source.is_file()]
    if missing:
        for source in missing:
            print(f"missing MinGW runtime DLL: {source}", file=sys.stderr)
        return 1

    args.destination.mkdir(parents=True, exist_ok=True)
    for source, destination in copies:
        shutil.copy2(source, destination)
        print(f"copied {source} -> {destination}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
