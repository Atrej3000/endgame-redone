# Phase 30 — Centralized asset and audio loading

Audio assets are now owned by `GameState.audio` (`AudioAssets`) rather than
being loaded from event, physics, projectile, or consequence paths.

## Ownership

- Application startup loads shared menu music, UI selection, and jump audio.
- `arcade_assets_load()` loads Arcade music plus shoot, damage, and kick
  effects before a game can start.
- `runner_assets_load()` loads Runner music before a game can start.
- The matching unload functions release only their matching asset group; app
  shutdown releases all groups after halting playback.

`load_chunk()` gives WAV files the same checked, no-overwrite loading contract
already used for textures, music, and fonts. Failure to load a mode asset
causes the existing scene-entry fallback rather than a later runtime hitch or
missing-audio failure.

## Runtime behavior

`src/menu_events.c`, `src/pause_events.c`, `src/processEvents.c`,
`src/process.c`, and `src/game_events.c` only play preloaded pointers. They no
longer contain `Mix_LoadWAV`/`Mix_LoadMUS` calls or free audio on scene exits.
This keeps a mode's loaded flag and audio lifetime consistent across repeated
sessions.

## Verification

The existing lifecycle test now asserts shared, Arcade, and Runner audio
loading/unloading alongside texture ownership. `audio_assets.h` is included in
the header self-containment test. The normal smoke test verifies that menu
music remains stable across repeated menu loads.
