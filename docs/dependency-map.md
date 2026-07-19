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
  |-> menu_events.c (menu0_events)
  |     `-calls->  input_command.c (translate_menu_command)  [NEW]
  |-> menu_events.c (menu_events), pause_events.c   -- unchanged this phase (audit sections 7.2/9)
  `-> load_menu.c  --calls-->  asset_loader.c (load_texture)   [FIXED: no longer bypasses it]
```

**New dependency edges**: `process.c -> entity_spawn.c`, `loadGame.c -> entity_spawn.c`,
`processEvents.c -> input_command.c`, `menu_events.c -> input_command.c` (only `menu0_events()`;
`menu_events()` and `pause_events.c` are left unchanged this phase, see audit §7.2). All are
low-risk/downward (a mode-simulation or input-handling module depending on a lower-level, stateless
helper module — consistent with the intended direction above, nothing added in the reverse
direction). `entity_spawn.c` and `input_command.c` both depend only on `header.h` (for `GameState`/
`Enemies`/`SDL_Keycode`) — neither depends on `scene.c`, `frame.c`, or any render/collision module,
keeping them at the same low level as `asset_loader.c`.

**Removed dependency edge**: `load_menu.c -> raw SDL IMG_Load/Mix_LoadMUS` becomes
`load_menu.c -> asset_loader.c (load_texture)`, matching every other texture-load call site in the
codebase (closing the one DIP inconsistency documented in the audit's §5.5/§7.4).

**Header dependency change**: `frame.c` and `main.c` now include the new `inc/frame.h`, and
`inc/header.h` is trimmed of the `arcade_frame`/`runner_frame` prototypes (verified: those two
functions have exactly one definer and one caller in the whole codebase, so removing them from
the compatibility header breaks nothing). `scene.c` and `main.c` additionally include the new
`inc/scene.h` for `app_change_scene()` — but that prototype is **kept** in `inc/header.h` too
(legal duplicate declaration), because `app_change_scene()` has 5 real call sites
(`scene.c`, `main.c`, `menu_events.c`, `pause_events.c`, `processEvents.c`); the task's own
"migrate gradually, don't replace every include in one commit" guidance applies directly here —
the 3 unrelated call sites keep compiling unchanged via `header.h`; `scene.h` is available as a
real, standalone-sufficient header for whichever module cares about scene routing specifically.
`AppScene`'s enum definition stays in `header.h` (see `docs/solid-gof-audit.md` §7.3 for why it
cannot move without a circular include, given `GameState.scene`'s own dependency on it). No other
file's includes change. No circular include was introduced — verified: neither `scene.h` nor
`frame.h` includes the other, and neither `entity_spawn.h` nor `input_command.h` includes
`scene.h`/`frame.h`/`app.h`.

## Layers not enforced this phase

`process.c`'s bullet-pool management still lives alongside its simulation loops (both depend on
`header.h`, no new boundary drawn between them); `processEvents.c`'s input-polling still lives
alongside its win/loss rule checks. See `docs/solid-gof-audit.md` §9 ("Deferred") — introducing an
artificial wrapper layer with no behavioral value just to "enforce" a boundary here was rejected,
per the task's own instruction not to enforce layers through no-value wrapper functions.

## Phase 10 update

`app_change_scene()`'s duplicate declaration is now resolved: `menu_events.c`, `pause_events.c`,
and `processEvents.c` (the 3 callers left on `header.h` above) now include `scene.h` directly, and
`header.h`'s copy of the prototype is removed — `scene.h` is the single authoritative source.
`arcade_frame`/`runner_frame`'s prototypes remain exclusively in `frame.h` as before (unchanged
this phase). An unrelated, accidental duplicate of `doRender`'s prototype (twice within
`header.h` itself, pre-existing, unrelated to any header split) was also found and removed — see
`docs/gamestate-decomposition.md` §5.

`GameState` gains two nested structs this phase, both defined in `header.h` (not a new top-level
header, per the task's own caution against a `header.h`-in-disguise `game_types.h`):
`AppContext { renderer; scene; }` (field `app`) and `AssetLifecycleFlags { arcadeAssetsLoaded;
runnerAssetsLoaded; sharedAssetsLoaded; }` (field `assetFlags`). No new dependency edges are
introduced by this — every file that already depended on `GameState` (via `header.h`) continues
to; only the field-access path changed (`game->renderer` → `game->app.renderer`, `game->scene` →
`game->app.scene`, `game->arcadeAssetsLoaded` → `game->assetFlags.arcadeAssetsLoaded`, etc.),
across `app.c`, `scene.c`, `loadGame.c`, `load_menu.c`, `kills_score.c`, `X_score.c`,
`x_list_leader.c`, `status.c`, `draw_lifes.c`, `main.c`, `menu_events.c`, `pause_events.c`,
`processEvents.c`, and 5 `docs/verification/*.c` test files. `window` remains outside `GameState`
entirely (it was never a field — see `docs/gamestate-decomposition.md` §3), so no new field was
added, only two existing ones regrouped.

New static check: `scripts/audit_repository_usage.py` now scans every `inc/*.h` file (not just
`header.h`) for duplicate function-prototype declarations, guarding against reintroducing an
accidental duplicate like the `doRender` one found this phase.
