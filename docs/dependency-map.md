# Dependency Map — Ucode_Endgame

Documents the intended and actual dependency direction as of Phase 9 (commit `e967001`, tag
`refactor-pre-solid-gof`, before this phase's edits). Companion to `docs/solid-gof-audit.md`.

## Intended direction

```
main
  -> app / scene            (src/app.c, src/scene.c — process lifecycle, scene routing)
  -> mode frame orchestration (src/frame.c — arcade_frame / runner_frame)
  -> mode simulation / collision / rendering
       (src/process.c, src/collisionDetect.c, src/doRender.c, src/processEvents.c,
        src/runner_death.c — and, as of this phase, src/entity_spawn.c, src/input_command.c)
  -> low-level SDL adapters and resources
       (src/asset_loader.c, src/loadGame.c, src/load_menu.c)
```

Rules (unchanged by this phase, verified still true after it):
- Asset loaders (`asset_loader.c`, `loadGame.c`) must not change scenes — verified: neither file
  calls `app_change_scene()` anywhere; `scene.c`'s entry hooks call *into* the asset loaders
  (`arcade_assets_load`/`runner_assets_load`), not the reverse.
- Renderers (`doRender.c` and friends) must not reset sessions — verified: no `doRender*` function
  calls `arcade_session_reset`/`runner_session_reset` (render-purity fixed in Phase 5, unchanged).
- Collision code must not load textures — verified: `collisionDetect.c` calls only
  `collide2d()` and `runner_trigger_death()`, no loader functions.
- Low-level SDL adapters (`asset_loader.c`) must not know game rules — verified: `load_texture`/
  `load_music`/`load_font` take only generic parameters (renderer/path/pointer-out), no
  `GameState` knowledge at all.
- Tests may depend on public modules; production must not depend on tests — verified: nothing
  under `src/` includes anything from `docs/verification/`.

## Before (Phase 8 baseline, commit `e967001`)

```
main.c ---------------------------------------------------------------+
  |-> app.c (app_init/app_shutdown)                                   |
  |-> scene.c (app_change_scene)  --calls-->  loadGame.c (*_assets_load) |
  |-> frame.c (arcade_frame/runner_frame)                              |
  |     |-> process.c (process/process2)         <-- inline spawn logic (10 sites)
  |     |-> collisionDetect.c (collisionDetect/collisionDetect2)       |
  |     |-> doRender.c (doRender/doRender2)                            |
  |     |-> runner_death.c (runner_update_death)                      |
  |     `-> processEvents.c (processEvents/processEvents2)             |
  |           <-- keycode-duplicated quit/pause/jump bodies (2-4 files)|
  |-> menu_events.c, pause_events.c  --calls-->  scene.c               |
  `-> load_menu.c  --calls-->  raw IMG_Load/Mix_LoadMUS (bypasses asset_loader.c)
```
Every `.c` file (except `app.c`, via `app.h`) depends on the single `inc/header.h` for every
declaration it needs, regardless of which module actually owns that declaration (ISP violation,
§2 of the audit).

## After (Phase 9)

```
main.c
  |-> app.c (app_init/app_shutdown)              [includes: app.h]
  |-> scene.c (app_change_scene)                 [includes: scene.h, header.h]
  |     `-calls->  loadGame.c (*_assets_load)
  |-> frame.c (arcade_frame/runner_frame)         [includes: frame.h, header.h]
  |     |-> process.c (process/process2)
  |     |     `-calls->  entity_spawn.c (enemy_spawn/smart_enemy_spawn/boss_spawn)  [NEW]
  |     |-> collisionDetect.c
  |     |-> doRender.c
  |     |-> runner_death.c
  |     `-> processEvents.c (processEvents/processEvents2)
  |           `-calls->  input_command.c (translate_arcade_command/translate_runner_command) [NEW]
  |-> menu_events.c, pause_events.c
  |     `-calls->  input_command.c (translate_menu_command)  [NEW]                 [NEW dependency]
  `-> load_menu.c  --calls-->  asset_loader.c (load_texture)   [FIXED: no longer bypasses it]
```

**New dependency edges**: `process.c -> entity_spawn.c`, `processEvents.c -> input_command.c`,
`menu_events.c -> input_command.c`, `pause_events.c -> input_command.c`. All four are
low-risk/downward (a mode-simulation or input-handling module depending on a lower-level, stateless
helper module — consistent with the intended direction above, nothing added in the reverse
direction). `entity_spawn.c` and `input_command.c` both depend only on `header.h` (for `GameState`/
`Enemies`/`SDL_Keycode`) — neither depends on `scene.c`, `frame.c`, or any render/collision module,
keeping them at the same low level as `asset_loader.c`.

**Removed dependency edge**: `load_menu.c -> raw SDL IMG_Load/Mix_LoadMUS` becomes
`load_menu.c -> asset_loader.c (load_texture)`, matching every other texture-load call site in the
codebase (closing the one DIP inconsistency documented in the audit's §5.5/§7.4).

**Header dependency change**: `scene.c`, `frame.c`, and `main.c` now include the new focused
`inc/scene.h`/`inc/frame.h` for those specific declarations, in addition to `header.h` for
everything else they still need (no file's total dependency surface increases; `header.h` is
trimmed by exactly the declarations that moved). No other file's includes change. No circular
include was introduced — verified: neither `scene.h` nor `frame.h` includes the other, and neither
`entity_spawn.h` nor `input_command.h` includes `scene.h`/`frame.h`/`app.h`.

## Layers not enforced this phase

`process.c`'s bullet-pool management still lives alongside its simulation loops (both depend on
`header.h`, no new boundary drawn between them); `processEvents.c`'s input-polling still lives
alongside its win/loss rule checks. See `docs/solid-gof-audit.md` §9 ("Deferred") — introducing an
artificial wrapper layer with no behavioral value just to "enforce" a boundary here was rejected,
per the task's own instruction not to enforce layers through no-value wrapper functions.
