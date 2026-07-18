#!/usr/bin/env bash
# Fetches and lays out the SDL2 MinGW "devel" packages this project's MinGW
# build path (see Makefile) needs, plus the compat-include shim headers that
# let this project's macOS-framework-style #include <SDL2_image/SDL_image.h>
# resolve against the official packages' actual layout. Idempotent: safe to
# re-run, skips whatever is already present. Used by both a fresh local dev
# setup and CI (.github/workflows/mingw-validation.yml) -- one script, not
# duplicated logic in each.
#
# Run from the repository root: ./scripts/setup-mingw-sdl2.sh
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

VENDOR_ROOT="vendor/SDL2-mingw"
TRIPLET="x86_64-w64-mingw32"

# Single source of truth for version numbers lives in the Makefile -- parse
# `make print-mingw-versions` rather than hardcoding a second copy here.
eval "$(make print-mingw-versions)"

mkdir -p "$VENDOR_ROOT"

# libsdl-org repo names: SDL, SDL_image, SDL_ttf, SDL_mixer -- not simply
# derived from the package/tarball prefix, so spelled out explicitly rather
# than string-manipulated.
fetch_one() {
    local repo="$1" tarball_name="$2" version="$3"
    local dest_dir="$VENDOR_ROOT/${tarball_name}-${version}"
    local asset="${tarball_name}-devel-${version}-mingw.tar.gz"
    local url="https://github.com/libsdl-org/${repo}/releases/download/release-${version}/${asset}"

    if [ -d "$dest_dir/$TRIPLET" ]; then
        echo "setup-mingw-sdl2: ${tarball_name} ${version} already present, skipping"
        return
    fi

    echo "setup-mingw-sdl2: fetching ${asset}"
    curl -fLsS -o "/tmp/${asset}" "$url"
    tar -xzf "/tmp/${asset}" -C "$VENDOR_ROOT"
    rm -f "/tmp/${asset}"
}

fetch_one "SDL"       "SDL2"        "$SDL2_VERSION"
fetch_one "SDL_image" "SDL2_image"  "$SDL2_IMAGE_VERSION"
fetch_one "SDL_ttf"   "SDL2_ttf"    "$SDL2_TTF_VERSION"
fetch_one "SDL_mixer" "SDL2_mixer"  "$SDL2_MIXER_VERSION"

# compat-include shims: the official packages expose SDL2_image/SDL_image.h
# et al. under include/SDL2/SDL_image.h, not the macOS-framework-style path
# this project's inc/header.h uses. Recreated verbatim every run (cheap,
# always correct) rather than skipped when present.
mkdir -p "$VENDOR_ROOT/compat-include/SDL2_image"
mkdir -p "$VENDOR_ROOT/compat-include/SDL2_ttf"
mkdir -p "$VENDOR_ROOT/compat-include/SDL2_mixer"

cat > "$VENDOR_ROOT/compat-include/SDL2_image/SDL_image.h" <<'EOF'
#pragma once
#include <SDL2/SDL_image.h>
EOF

cat > "$VENDOR_ROOT/compat-include/SDL2_ttf/SDL_ttf.h" <<'EOF'
#pragma once
#include <SDL2/SDL_ttf.h>
EOF

cat > "$VENDOR_ROOT/compat-include/SDL2_mixer/SDL_mixer.h" <<'EOF'
#pragma once
#include <SDL2/SDL_mixer.h>
EOF

echo "setup-mingw-sdl2: done"
