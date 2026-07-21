# Refactor Plan — Ucode_Endgame

Guiding constraints (restated, non-negotiable for every pass): no clean rewrite, no ECS, no
engine/language/framework migration, no full architectural replacement. Small, reviewable,
behavior-preserving diffs. Existing code is the source of truth. Do not attempt every priority
in one pass — a smaller, stable, verified refactor beats a large unfinished redesign.

## Phase index

| Phase | Objective | Status |
|---|---|---|
| 0 | Documentation baseline (this file + audit + log) | **Done — Pass 1** |
| 1 | Priority-1 correctness fixes (OOB array read, event-union guards, uninitialized ledges, leaderboard bound check, dead-code arithmetic fix) | **Done — Pass 1** |
| 2 | Deterministic initialization: `GameState` `malloc` → `calloc` | **Done — Pass 1** |
| 3 | Eliminate per-frame asset reloading (textures/font/music/SFX) via NULL-guarded one-time loads; fix companion free-without-null bug; fix HUD-texture leak | **Done — Pass 1** |
| 4 | Raw scene-state ints → descriptive enums (`menu_status`, `menu0_status`) | **Done — Pass 3** (`AppScene` enum, `GameState.scene`, `app_change_scene()`) |
| 5 | `doRender2` gameplay-state mutation removed from render path | **Done — Pass 5** (relocated to `runner_resolve_death()`, called from `runner_frame()` after `doRender2()` returns — not merged into `process2()`, see Pass 5 notes below) |
| 6 | `collisionDetect.c` enemy-vs-enemy self-collision guard | Deferred |
| 7 | `IMG_Load`/NULL-check ordering normalization (~10 sites) | Deferred (residual gap in Phase 3b's asset-lifecycle flags — see Pass 2 notes below) |
| 8 | Full shutdown-path reconstruction (texture/font/chunk/music destruction, `Mix_CloseAudio`, `IMG_Quit`, `IMG_Init`) | **Done — Pass 2** (`src/app.c`) |
| 9 | `mx_*` dead-code removal (16 files) | Deferred |
| 10 | Large paired-function deduplication (`process`/`process2`, etc.) | Deferred (explicitly out of scope until a build-verified baseline exists) |
| 11 | Delta-time conversion | Deferred (explicitly out of scope until a stable, comparable baseline exists) |
| 12 | Build system / README modernization | Partially done — Pass 2 (additive Windows/MinGW `make` targets only; macOS `all` target and README's gameplay documentation untouched) |
| 13 | Git-protected rollback baseline | **Done — Pass 2** (`git init` + commit `cfb7ca7`) |
| 14 | Real compiler validation (strict warnings) | **Done — Pass 2** (MinGW-w64 GCC; 0 errors, 169 pre-existing warnings, 0 introduced) |
| 15 | Sanitizer validation (ASan/UBSan) | Attempted — **unavailable in this environment** (two MinGW toolchains both lack `libasan`/`libubsan`; target left in place for a future capable toolchain) |
| 16 | Asset-lifecycle sentinel hardening (implicit texture-pointer guards → explicit `bool` flags) | **Done — Pass 2** |
| 17 | Non-interactive runtime smoke test (`app_init`→asset-guard→`app_shutdown`→idempotency) | **Done — Pass 2**, 3 consecutive passing runs |
| 18 | New defect found during mandated nested-loop sweep: `process.c:515` OOB write | **Found and fixed — Pass 2** |
| 19 | Top-level main-loop simplification (two nested loops → one flat loop with scene dispatch) | **Done — Pass 3** |
| 20 | Non-interactive scene-transition test | **Done — Pass 3**, 16 checks, 3 consecutive passing runs |
| 21 | `doRender2` gameplay-state mutation (Phase 5, listed above as item 5) | **Done — Pass 5** |
| 22 | Separate asset loading from session reset (`loadGame()`/`loadGame2()` split) | **Done — Pass 4** (`shared_assets_load/unload`, `arcade_assets_load/unload`, `runner_assets_load/unload`, `arcade_session_reset`, `runner_session_reset`) |
| 23 | Checked SDL asset-loading helpers (`load_texture`/`load_music`/`load_chunk`/`load_font`) | **Done — Pass 4** (`src/asset_loader.c`) |
| 24 | Fix confirmed `multiPlayer` clobber bug (`loadGame.c:407` hardcoded `=1` every call) | **Found and fixed — Pass 4** |
| 25 | Fix confirmed bullet-array memory leak (direct `NULL` assignment instead of `removeBullet`/`removeSecondBullet`) | **Found and fixed — Pass 4** |
| 26 | Fix confirmed shared-texture leak (mult/leaders/pause/brick/death/font loaded independently by both modes) | **Found and fixed — Pass 4** (`shared_assets_load`/`shared_assets_unload`) |
| 27 | `multiPlayer` `int` → `GameMode` enum migration | **Done — Pass 4** (deferred by Pass 3, completed once its sole remaining writer was centralized) |
| 28 | Non-interactive asset/session lifecycle test | **Done — Pass 4**, 55 checks, caught and led to fixing one real gap in the new code before landing |
| 29 | Frame-pipeline map for all 10 scenes (`docs/frame-pipeline-map.md`) | **Done — Pass 5** |
| 30 | Explicit named per-mode frame wrappers (`arcade_frame`/`runner_frame`, `src/frame.c`) | **Done — Pass 5** |
| 31 | `doRender2` gameplay mutation removed from render path (see Phase 5 above) | **Done — Pass 5** |
| 32 | Fix confirmed double scene-transition risk (`SDLK_q` falling through into a stale game-over check) | **Found and fixed — Pass 5** |
| 33 | Fix confirmed `manFrames[0]` mis-documentation/duplication (both modes load the identical file) | **Found and fixed — Pass 5** (`shared_assets_load`/`shared_assets_unload`) |
| 34 | Non-interactive frame-pipeline test (render purity, relocated mutation, transition guard, animation wrap/idle, pause purity) | **Done — Pass 5**, 38 checks |
| 35 | Runner death-state lifecycle map (`docs/runner-death-lifecycle.md`) | **Done — Pass 6** |
| 36 | Fix confirmed severe defect: `process2()`'s `deathCountdown` never decremented, causing unbounded `gameLives` drain and a missed game-over the instant `isDead` became reachable | **Found and fixed — Pass 6** (`src/runner_death.c`) |
| 37 | Wire Runner's star-collision/fall-death triggers into a real respawn sequence (previously an instant, unconditional `gameLives=0` regardless of lives remaining) | **Done — Pass 6** — confirmed product decision, see Pass 6 notes |
| 38 | Fix confirmed respawn-position gap (found during design validation): left-edge falls and multiplayer deaths would otherwise re-trigger indefinitely once movement freezes during the death animation | **Found and fixed — Pass 6** |
| 39 | Non-interactive Runner death-lifecycle test | **Done — Pass 6**, 39 checks |
| 40 | Reproducible GitHub Actions CI (MinGW/Windows, matching the validated local path) | **Done — Pass 6** — Phase 5's PR merged with no CI run at all; this closes that gap |
| 41 | Unused-code/asset audit (`docs/unused-code-assets-audit.md`) | **Done — Pass 7** |
| 42 | Remove 18 unreachable source files (16 `mx_*` cluster, `leader_events.c`, `doRender_multiplayer.c`) and 4 dead helper functions | **Found and fixed — Pass 7** |
| 43 | Remove 13 dead `GameState`/`Man` struct fields, 2 dead macros, 1 orphaned prototype, 2 obsolete commented-out implementation blocks | **Found and fixed — Pass 7** |
| 44 | Remove 22 zero-reference asset files (~7.68MB) | **Found and fixed — Pass 7** |
| 45 | Repository usage integrity check (`scripts/audit_repository_usage.py`) — asset-path-with-case + dangling-prototype checks, wired into CI | **Done — Pass 7** |
| 46 | Case-sensitive asset path mismatch map (`docs/asset-path-portability.md`) | **Done — Pass 8** |
| 47 | Fix confirmed case-mismatch defects (`Sunset_front.png`/`brick_block.png`/`copper_block.png`, Phase 7 item 8) | **Found and fixed — Pass 8** |
| 48 | Strengthen `audit_repository_usage.py` (tiered errors/known-exceptions/informational output, regular-file check, case-colliding-sibling check, backslash check, duplicate-alias detection) | **Done — Pass 8** |
| 49 | Case-sensitive Linux CI job (`linux-asset-validation`) | **Done — Pass 8** |
| 50 | Additive Linux build target (`make linux`/`linux-smoketest`, best-effort, not locally verified) | Attempted — Pass 8, CI result is the deciding signal |
| 51 | Evidence-based SOLID/GoF architecture audit (`docs/solid-gof-audit.md`, `docs/dependency-map.md`) | **Done — Pass 9** |
| 52 | Simple Factory-style creation functions (not GoF Factory Method — see Pass 10 terminology correction): centralize enemy/smart-enemy/boss spawn invariants (`src/entity_spawn.c`) | **Done — Pass 9** |
| 53 | ISP: extract focused `inc/scene.h`/`inc/frame.h` headers | **Done — Pass 9** |
| 54 | Command: discrete-action input translation (`src/input_command.c`) for `processEvents`/`processEvents2`/`menu0_events` | **Done — Pass 9** |
| 55 | DIP consistency fix: route `load_menu.c` through the checked `load_texture()` loader | **Done — Pass 9** |
| 56 | GameState/header ownership audit (`docs/gamestate-decomposition.md`) — memory-layout safety verification + field-by-field group selection | **Done — Pass 10** |
| 57 | Terminology correction: entity-spawn functions relabeled Simple Factory-style, not GoF Factory Method | **Done — Pass 10** |
| 58 | Complete `app_change_scene()` header migration (single authoritative declaration in `scene.h`); remove accidental `doRender` prototype duplicate | **Done — Pass 10** |
| 59 | Header self-containment tests (`mingw-headertest`) for all 5 focused headers | **Done — Pass 10** |
| 60 | Static duplicate-declaration check added to `audit_repository_usage.py` | **Done — Pass 10** |
| 61 | `GameState` nested structs: `AppContext` (renderer+scene) and `AssetLifecycleFlags` | **Done — Pass 10** |
| 62 | GameState grouping lifecycle test (`mingw-groupingtest`) | **Done — Pass 10** |

## Phases executed in Pass 1

### Phase 0 — Documentation baseline
- Files: `docs/refactor-audit.md`, `docs/refactor-plan.md` (this file), `docs/refactor-log.md`
- Risk: none (documentation only)
- Validation: cross-checked every file:line reference and struct/array size against the live
  files during writing
- Rollback: delete `docs/` if unwanted; no code affected
- Completion criteria: all three docs exist and reflect the verified baseline — met

### Phase 1 — Priority-1 correctness fixes
- Files: `src/processEvents.c`, `src/menu_events.c`, `src/pause_events.c`, `src/loadGame.c`,
  `src/mx_strsplit.c`
- Risk: item 1 (`boss[i]`→`boss[j]`) is a **deliberate behavior fix** — boss-vs-player collision
  now checks real boss positions instead of aliased adjacent memory. Items 2-5 are
  behavior-preserving for the overwhelming common case (see audit table for per-item reasoning);
  item 3 (`ledges[96..98]`) involves one judgment call (pattern extension vs. zeroing), logged
  as an assumption.
- Validation: manual line-by-line review of each changed region; struct/array sizes
  cross-checked against `inc/header.h`; `mx_strsplit` confirmed dead via grep before fixing
- Rollback: exact before/after snippets recorded in `docs/refactor-log.md`
- Completion criteria: all 5 items applied, each logged — met

### Phase 2 — `calloc` for `GameState`
- Files: `src/main.c`
- Risk: none beyond the fix itself — single line, no control-flow change; makes uninitialized-
  field reads deterministic instead of heap-garbage-dependent, and is the enabling precondition
  for Phase 3's NULL-guards
- Validation: confirmed no code path relies on any `GameState` field being non-zero before its
  first explicit write (all texture/font/sound/entity fields are written by `loadGame`/
  `load_menu*` before `doRender*`/`process*` read them)
- Rollback: revert to `malloc(sizeof(GameState))`
- Completion criteria: met

### Phase 3 — Eliminate per-frame asset reloading
- Files: `src/loadGame.c`, `src/load_menu.c`, `src/menu_events.c`, `src/pause_events.c`,
  `src/processEvents.c`, `src/process.c`, `src/status.c`, `src/kills_score.c`, `src/X_score.c`,
  `src/x_list_leader.c`
- Risk: the highest-risk phase in this pass, because it touches sound-effect lifetime
  (free/reload cycles) as well as texture lifetime. Mitigated by: (a) never moving *when*
  `loadGame`/`loadGame2`/`load_menu*` are called — only guarding *what happens inside* them, so
  `main.c`'s control flow is untouched; (b) pairing every new NULL-guard with a `= NULL` fix at
  every corresponding free site, so no guard can observe a dangling-but-non-NULL pointer;
  (c) HUD texture fix is a pure destroy-before-recreate with no visible-output change.
- Validation: for every guarded field, grepped all of `src/*.c` for every other read/write of
  that field to confirm no other code path assumes "always freshly reloaded" or reads it between
  a free and the next guarded reload
- Rollback: exact before/after snippets in `docs/refactor-log.md`, one entry per file
- Completion criteria: no texture/font/music/SFX load call remains unconditional inside a
  per-frame or per-shot path; no HUD texture is recreated without destroying its predecessor —
  met

## Phases executed in Pass 2 (validation and hardening)

### Phase 13 — Git baseline
- Files: whole repo (`git init` + initial commit)
- Risk: none (additive; no destructive git command run)
- Validation: `git log`, `git status --short` reviewed before and after commit
- Rollback: `git reset --hard cfb7ca7` returns to this exact point (not run; noted for reference)
- Completion criteria: met — commit `cfb7ca7` exists

### Phase 14 — Real compiler validation
- Files: `Makefile` (additive `mingw*` targets), `inc/header.h` (3 `#ifdef`-gated portability
  shims), `vendor/SDL2-mingw/` (gitignored, downloaded MinGW SDL2 devel packages + compat-include
  shims)
- Risk: none to the macOS build (untouched); Windows-only shims are gated to never affect
  non-Windows compilation
- Validation: `make mingw` — 0 errors, 169 pre-existing warnings (0 introduced by either pass,
  confirmed by line-number cross-reference — see `docs/refactor-log.md`)
- Rollback: delete the appended Makefile section, the 3 `#ifdef` blocks in `header.h`, and
  `vendor/`
- Completion criteria: met

### Phase 15 — Sanitizer validation (attempted, environment-limited)
- Files: `Makefile` (`mingw-asan` target, correctly specified but non-functional here)
- Risk: none (no code depends on this succeeding)
- Validation: two independent MinGW-w64 toolchains attempted, both fail identically
  (`cannot find -lasan`/`-lubsan`) — confirmed, documented gap, not concealed
- Rollback: n/a
- Completion criteria: **not met in this environment** — met once a sanitizer-capable toolchain
  (Linux/macOS gcc/clang, or clang-cl+MSVC) is available

### Phase 16 — Asset-lifecycle sentinel hardening
- Files: `inc/header.h` (2 new `bool` fields), `src/loadGame.c` (guard conditions + flag-set
  statements)
- Risk: low — narrowly scoped to 4 lines total; does not change *when* `loadGame`/`loadGame2` are
  called, only what gates their internal asset-load block
- Validation: rebuilt clean; runtime-verified via the Phase 17 smoke test (repeated calls confirm
  pointers unchanged)
- Rollback: revert guards to `bossTexture == NULL`/`star == NULL`, remove the flag-set lines and
  the 2 new struct fields
- Completion criteria: met, with the residual limitation documented (Phase 7's unchecked
  `IMG_Load`/`SDL_CreateTextureFromSurface` sites mean the flag-based design narrows but does not
  fully close the theoretical partial-failure window — see `docs/refactor-audit.md` Section 5)

### Phase 8 — Full shutdown-path reconstruction (now done, previously deferred)
- Files: new `inc/app.h`, new `src/app.c`, `src/main.c` (init/shutdown sections rewritten)
- Risk: this is the one Pass-2 change that touches the same category of thing (init/shutdown
  control flow) the Pass-1 plan flagged as needing a "running build to rule out regressions" —
  now satisfied, since a running build exists this pass
- Validation: compiled clean; **runtime-verified** — the Phase 17 smoke test calls `app_shutdown`
  twice in a row (idempotency) and completes without crashing, 3 consecutive runs
- Rollback: delete `inc/app.h`/`src/app.c`; restore `main.c`'s previous inline init/shutdown code
  (preserved in `docs/refactor-log.md`'s Pass-1 section and in git commit `cfb7ca7`)
- Completion criteria: met — every resource category from the task's required list (bullets,
  generated/static textures, fonts, chunks, music, renderer, window, mixer, SDL_mixer/ttf/image,
  SDL, `GameState`) is destroyed in dependency order, idempotently, with every pointer nulled
  after release; `IMG_Init` (previously never called) added

### Phase 17 — Non-interactive runtime smoke test
- Files: new `docs/verification/smoke_init_shutdown.c` (test-only; not part of `all`/`mingw`,
  built only by the new `mingw-smoketest` target against `src/*.c` minus `main.c`)
- Risk: none (test-only file, cannot affect the shipped game)
- Validation: 3 consecutive runs, all printing `SMOKE TEST: PASS`; logs saved under
  `docs/verification/`
- Rollback: delete the file and the `mingw-smoketest` Makefile target
- Completion criteria: met for what's automatable in this environment (no desktop input
  injection available) — explicitly does NOT cover keyboard-driven gameplay, collisions,
  pause/resume, or the in-game `SDLK_ESCAPE`/`SDLK_q` sound-cleanup paths, which need a live event
  loop this environment cannot drive

### Phase 18 — New defect found during the mandated nested-loop sweep
- Files: `src/process.c:515`
- Risk: none beyond the fix itself (one-token loop-bound change)
- Validation: compiled clean, same warning count; struct-layout cross-check confirmed the
  corruption target (`smartEnemies[0]`); confirmed via grep this was the only `+= 2` loop in the
  file with an odd bound (6 siblings all use safe even literals)
- Rollback: revert `NUM_ENEMIES - 1` to `NUM_ENEMIES` at process.c:515
- Completion criteria: met

## Phases executed in Pass 3 (scene-state refactor and main-loop simplification)

### Phase 4 — Scene-state enums (was deferred in Pass 2, now done)
- Files: `inc/header.h` (new `AppScene` enum + `scene` field, `enum menu_buttons` deleted,
  `menu_status`/`menu0_status` deprecated in place), new `src/scene.c`
  (`app_change_scene`/`arcade_menu_enter`/`runner_menu_enter`)
- Risk: introducing an unused field/type only — no logic runs yet, since nothing calls
  `app_change_scene()` in this commit
- Validation: `make mingw` — 0 errors, 168 warnings (unchanged); `make mingw-smoketest` — PASS
- Rollback: `git revert a83c59d`
- Completion criteria: met

### Phase 19 — Main-loop simplification and full routing cutover
- Files: `src/main.c` (flattened to one loop), `src/menu_events.c`, `src/pause_events.c`,
  `src/processEvents.c` (all nine live `menu_status`/`menu0_status` write sites converted to
  `app_change_scene()` calls)
- Risk: the highest-risk commit of this pass — this is the actual routing cutover, and every
  write site had to convert together (an intermediate state with only some sites converted would
  break routing). Mitigated by: confirming line-by-line against the pre-refactor file that every
  scene's per-frame function-call order is unchanged; confirming via grep that zero live
  `menu_status`/`menu0_status` writes remain anywhere outside the now-inert `load_menu.c` selfresets
  and the already-dead `leader_events.c`; and the explicit invariant that `game->scene` is
  assigned nowhere except inside `app_change_scene()` itself (stated in `docs/scene-state-map.md`).
- Validation: `make mingw` — 0 errors, 168 warnings, confirmed identical warning set to baseline
  (none on any touched line); `make mingw-smoketest` — PASS, unmodified.
- Rollback: `git revert 5ccca22`
- Completion criteria: met — every reachable screen remains reachable, every transition preserved
  (full table in `docs/scene-state-map.md`), pause resumes the correct originating mode, gameplay
  reset now occurs only at intended entry points (once per arrival, not every frame), no per-frame
  resource loading reintroduced.

### Phase 20 — Non-interactive scene transition test
- Files: new `docs/verification/scene_transition_test.c`, `Makefile` (`mingw-scenetest` target)
- Risk: none (test-only file, cannot affect the shipped game)
- Validation: 16 checks, all passing, 3 consecutive runs; `make mingw` — 0 errors, 168 warnings
  (0 from the new file)
- Rollback: delete the test file and Makefile target
- Completion criteria: met for what's automatable here (no input-injection tool available) —
  covers initial scene, both menu-entry transitions, gameplay→menu (confirming the load-once
  reset fires correctly on return, not just on first entry), quit, invalid-enum handling, and
  pause-resume-to-correct-mode for both Arcade and Runner

### Phase 21 — Interactive manual test (partial)
- Files: none (verification only); new `docs/verification/manual-test-main-menu.png`
- What was actually done: launched the real `endgame-mingw.exe`, confirmed via `Get-Process` it
  started and stayed alive with a real window; captured its actual rendered output via the Win32
  `PrintWindow` API (focus-independent) and visually confirmed the Main menu renders correctly
  end-to-end in the refactored build.
- What was not achieved, stated plainly: `SetForegroundWindow` was rejected by Windows'
  foreground-lock restriction in this automation context (returned `false`, foreground unchanged),
  and a `PostMessage`-based keypress had no effect (SDL2 on Windows appears to need real OS
  keyboard focus, not a posted message). No further transition, and none of the requested manual
  test matrix (Arcade/Runner single/multi/pause/resume/game-over/leaderboard, 5x repeated
  menu↔mode cycles), was interactively verified this pass.
- Rollback: n/a, no code changed.

## Phases executed in Pass 4 (asset loading / session reset separation)

Full audit, classification, and ownership table in `docs/game-session-lifecycle.md`, written
before any Pass 4 code edit. Baseline: commit `82927d6` (tag `refactor-pass-3-scenes`), `make
mingw` 0 errors/168 warnings, both existing test suites passing — confirmed before any edit.

### Phase 22-26 — Split `loadGame()`/`loadGame2()`, checked loaders, three confirmed-bug fixes
- Files: new `src/asset_loader.c`; rewritten `src/loadGame.c` (`loadGame()`/`loadGame2()` deleted,
  replaced by `shared_assets_load/unload`, `arcade_assets_load/unload`, `arcade_session_reset`,
  `runner_assets_load/unload`, `runner_session_reset`); `src/scene.c` (enter-hooks call only
  asset-load, with a Main-menu fallback on failure); `src/menu_events.c` (new-game selection calls
  session-reset explicitly before transitioning); `src/app.c` (promoted 3 static helpers,
  `app_shutdown()` delegates to the 3 new unload functions for unambiguous fields); `inc/header.h`
  (`GameMode` enum, `sharedAssetsLoaded` flag, new prototypes).
- Risk: the largest, most interdependent commit of this pass — main.c/scene.c/menu_events.c all
  needed the full new function set simultaneously to keep compiling, so this combined what the
  original plan scoped as two separate commits (Arcade split, Runner split) into one, explicitly
  justified in the commit message.
- Validation: `make mingw` — 0 errors, 167 warnings (down from 168 — one pre-existing warning on a
  directly-moved line fixed in passing, confirmed via before/after line-number diff). Existing
  lifecycle smoke test and scene transition test both still pass (both updated to call the new
  function names in place of the deleted `loadGame()`/`loadGame2()`).
- Rollback: `git revert 586c031` (also revert `a349e07` for the dependent `multiPlayer` retype).
- Completion criteria: met — asset loading and session reset are fully separate operations; all
  mode asset loads return success/failure and clean up via their own `_unload()` on any failure;
  `*AssetsLoaded` flags set true only after complete success; the `multiPlayer` clobber
  (`loadGame.c:407`'s hardcoded `=1`) and the bullet-array leak (direct `NULL` instead of
  `removeBullet`/`removeSecondBullet`) are both fixed; the shared-texture leak (mult/leaders/
  pause/brick/death/font loaded independently by both modes) is fixed via the new shared-assets
  bucket with its own one-time flag.

### Phase 27 — `multiPlayer` → `GameMode`
- Files: `inc/header.h` only (field retype, name unchanged).
- Risk: none — confirmed via the previous commit's clean build that all ~29 read sites are bare
  boolean truth tests, unaffected by the type change.
- Validation: `make mingw` — 0 errors, 167 warnings (unchanged). Both test suites still pass.
- Rollback: `git revert a349e07`.
- Completion criteria: met — `multiPlayer` now has exactly one writer
  (`arcade_session_reset`/`runner_session_reset`'s first statement), eliminating the two-writers-
  one-value risk flagged during design validation.

### Phase 28 — Non-interactive asset/session lifecycle test
- Files: new `docs/verification/lifecycle_test.c`, `Makefile` (`mingw-lifecycletest` target).
- Risk: test-only file, cannot affect the shipped game.
- Validation: 55 checks; **caught a real gap** in the just-written `arcade_assets_load()`/
  `runner_assets_load()` (the `*AssetsLoaded` early-return skipped re-verifying the shared bucket,
  so a mode already marked loaded would not re-cascade a shared-bucket reload if that bucket had
  been unloaded independently — not reachable from today's actual call graph, but a real hole in
  the function's own contract) — fixed by re-ordering the check, then all 55 checks pass. Both
  other test suites still pass.
- Rollback: delete the test file and Makefile target; the `arcade_assets_load`/
  `runner_assets_load` ordering fix should stay regardless (it's a correctness fix independent of
  the test that found it).
- Completion criteria: met — covers asset lifecycle (both modes + the shared bucket), session
  reset (both modes, including a real `addBullet()`-allocated bullet to verify no leak), pause/
  resume (session state survives unchanged), and the new-game transition sequence.

### Phase 21 (continued) — Interactive manual test, Pass 4
- Files: none; new `docs/verification/manual-test-phase4-main-menu.png`.
- What was done: same `PrintWindow`-capture technique as Pass 3 — launched the real Pass-4 build,
  confirmed it starts and stays alive, captured its actual rendered Main menu (byte-identical file
  size to Pass 3's screenshot, confirming no visual regression).
- What was not attempted again: Pass 3 already established that `SetForegroundWindow` is rejected
  by Windows' foreground-lock restriction in this automation context and `PostMessage`-based
  keypresses don't reach SDL2's input handling — repeating those failed techniques would waste
  time for a known-negative result, so this pass did not re-attempt keyboard injection. No
  transition beyond the Main menu render was interactively verified.

## Phases executed in Pass 5 (frame-pipeline separation)

Full audit, per-scene ordering, and every finding (fixed or deliberately left unfixed) in
`docs/frame-pipeline-map.md`, written before any Pass 5 code edit. Baseline: commit `86dac33`
(tag `refactor-pass-4-lifecycle`, created at the start of this pass since it didn't exist yet),
`make mingw` 0 errors/167 warnings, all three existing test suites passing — confirmed before any
edit.

### Phase 29 — Frame-pipeline map
- Files: new `docs/frame-pipeline-map.md`.
- What was done: exact per-scene ordering (event polling, input mutation, movement, AI, spawning,
  collision, health/score, animation, render, present, transitions) for all 10 scenes, gathered via
  three parallel read-only audits, cross-checked against direct re-reads of every source file
  immediately before writing. No code changed in this step.
- Rollback: delete the file.

### Phase 30 — Explicit named per-mode frame wrappers
- Files: new `src/frame.c` (`arcade_frame`, `runner_frame`), `inc/header.h` (prototypes),
  `src/main.c` (`APP_SCENE_ARCADE_GAME`/`APP_SCENE_RUNNER_GAME` cases call the wrappers instead of
  4 inline calls each).
- Risk: none — each wrapper calls the same functions in the same order as before; introduced as
  two separate commits (`arcade_frame` first, `runner_frame` second) specifically so each landed
  as a pure, independently-verifiable structural move before Phase 31's actual behavior change.
- Validation: `make mingw` — 0 errors, 167 warnings (unchanged) after each commit. All three
  existing test suites still pass after each commit.
- Rollback: `git revert` the two introduction commits; restore the 4 inline calls in `main.c`.
- Completion criteria: met — both gameplay scenes now have one named entry point per mode, with no
  behavior change.

### Phase 31 — Remove gameplay mutation from `doRender2()`
- Files: `src/doRender.c` (mutation + dead duplicate deleted), `src/frame.c`
  (`runner_resolve_death()` added, called from `runner_frame()`).
- What was done: confirmed via grep that `man.isDead` is never set to `1` anywhere in the live
  codebase (only in a commented-out line and in test harnesses), so this branch was dead code —
  relocating it changes zero observable behavior today. A Plan-mode validation pass caught that the
  naive placement (calling the new function *before* `doRender2()`) would either delete the
  death-sprite draw or silently prevent it from ever firing (since the mutation would clear
  `isDead` before the draw's own `if (isDead)` guard ran); corrected to call
  `runner_resolve_death()` *after* `doRender2()` returns instead, which draws correctly and is
  behaviorally identical since nothing between the two points reads the affected fields.
- Validation: `make mingw` — 0 errors, 167 warnings. All three existing test suites still pass;
  the new `mingw-frametest` (Phase 34) directly verifies both the render-purity and the relocated
  mutation's correct firing.
- Rollback: `git revert` the commit; restore the deleted lines in `doRender2()`.
- Completion criteria: met — `doRender2()` no longer mutates any `game->` field.

### Phase 32 — Fix confirmed double scene-transition risk
- Files: `src/processEvents.c` (both `processEvents()` and `processEvents2()`).
- What was done: found via audit that `SDLK_q` (unlike `SDLK_ESCAPE`) does not return early after
  setting the scene, so a same-frame game-over check later in the same function could fire a
  second, overwriting transition. Fixed by guarding each game-over transition call with a check
  that the scene is still the gameplay scene it assumes, rather than a blanket early return (which
  a Plan-mode validation pass showed would also skip Runner's unconditional score-persist step on
  the same rare coincidence).
- Validation: `make mingw` — 0 errors, 167 warnings. All test suites pass; `mingw-frametest`
  exercises both the normal-fire and already-transitioned cases directly for both modes.
- Rollback: `git revert` the commit; remove the four added scene-checks.
- Completion criteria: met — a transition already made earlier in the same call can no longer be
  silently overwritten; the score-persist step is untouched either way.

### Phase 33 — Fix confirmed `manFrames[0]` mis-documentation
- Files: `src/loadGame.c` (`shared_assets_load`/`shared_assets_unload` gain `manFrames[0]`;
  `arcade_assets_load`/`runner_assets_load` lose their destroy-before-overwrite special case),
  `src/app.c` (redundant centralized `manFrames[]` destroy loop removed).
- What was done: confirmed both modes load the byte-for-byte identical file into `manFrames[0]` —
  the "contested field, different source images" comment written in Pass 4 was factually wrong.
  Consolidated into the existing shared-assets bucket, same as `mult`/`leaders`/`pause`/`brick`/
  `death`/`font`.
- Validation: `make mingw` — 0 errors, 167 warnings. All test suites pass.
- Rollback: `git revert` the commit.
- Completion criteria: met — no field is loaded redundantly by both modes' asset groups anymore.

### Phase 34 — Non-interactive frame-pipeline test
- Files: new `docs/verification/frame_pipeline_test.c`, `Makefile` (`mingw-frametest` target).
- What was done: 38 checks covering render purity for `doRender()`/`doRender2()` (including with
  `man.isDead` forced true), the relocated death mutation firing correctly through `runner_frame()`,
  the double-transition guard (both the normal-fire and already-transitioned cases, both modes),
  animation-counter wrap-at-11 and idle/off-ledge no-advance behavior for both `process()` and
  `process2()`, and pause-scene purity across repeated calls.
- This test surfaced a real interaction worth recording: forcing `man.isDead=1` for the relocation
  test also triggers `process2()`'s own, separate, already-broken `deathCountdown` mechanism
  (`runner_session_reset()` sets `deathCountdown=-1`, and `isDead=1` then flips it positive), which
  decrements `gameLives` a second time in the same frame. Not a Pass 5 regression — a pre-existing,
  separately documented defect (see `docs/frame-pipeline-map.md`) — the test isolates it by
  resetting `deathCountdown` to 0 before exercising the mutation being verified, rather than
  conflating the two.
- Validation: **runtime-verified**, not static — `frametest.exe` runs and every check is a live
  assertion. Initial run: 1 failure (the interaction above), diagnosed, test corrected, rebuilt:
  38/38 pass. `make mingw` — 0 errors, 167 warnings (unchanged, confirmed 0 from the new file).
- Rollback: delete the test file and Makefile target.
- Completion criteria: met.

### Phase 21 (continued) — Interactive manual test, Pass 5
- Files: none; new `docs/verification/manual-test-phase5-main-menu.png`.
- What was done: launched the real Pass-5 build, confirmed via a live process handle it started
  and stayed alive, captured its actual rendered Main menu via the same `PrintWindow` technique —
  renders correctly, no visual regression.
- What was not attempted again: Pass 3 already established, with concrete evidence, that
  `SetForegroundWindow` is rejected by Windows' foreground-lock restriction in this automation
  context and that `PostMessage`-based keypresses don't reach SDL2's input handling. Repeating
  those known-negative techniques this pass would not have produced new information, so this pass
  did not re-attempt keyboard injection. No transition beyond the Main menu render was
  interactively verified this pass either.
- Rollback: n/a, no code changed.

## Phases executed in Pass 6 (CI + Runner death-lifecycle repair)

Full trace, defect classification, and repaired sequence in `docs/runner-death-lifecycle.md`,
written before any Pass 6 code edit. Baseline: commit `3419fc0` (tag
`refactor-pass-5-frame-pipeline`, created at the start of this pass since it didn't exist yet),
`make mingw` 0 errors/167 warnings, all four existing test suites passing — confirmed before any
edit.

### Phase 35 — Runner death-state lifecycle map
- Files: new `docs/runner-death-lifecycle.md`.
- What was done: exhaustive read/write trace of `isDead`/`deathCountdown`/`gameLives`/`man.lives`/
  `secondPlayer.lives` across `process.c`, `collisionDetect.c`, `processEvents.c`, `frame.c`,
  `doRender.c`, `loadGame.c`; resolved the life-ownership question (`gameLives` is Runner's sole
  authoritative counter); classified the defect (unreachable today, severely broken once
  reachable); documented the repaired sequence. No code changed in this step.
- Rollback: delete the file.

### Phase 36-38 — Single-shot Runner death lifecycle
- Files: new `src/runner_death.c` (`runner_trigger_death`, `runner_update_death`); `inc/header.h`
  (prototypes); `src/frame.c` (`runner_frame()` calls `runner_update_death()` in place of the
  deleted `runner_resolve_death()`); `src/process.c` (`process2()`: deleted the broken
  `deathCountdown` block; man/secondPlayer movement gated on `!man.isDead`, secondPlayer newly so);
  `src/collisionDetect.c` (both star-collision sites call `runner_trigger_death()`);
  `src/processEvents.c` (both fall-death sites call `runner_trigger_death()`); `src/loadGame.c`
  (clarifying comment on `runner_session_reset()`).
- Confirmed defect (Phase 36): `process2()`'s `deathCountdown` decrement was commented out, so
  once `isDead` ever became true, `gameLives--` fired every frame forever, independent of
  `runner_resolve_death()`'s own decrement — a guaranteed double decrement then unbounded drain,
  overshooting the `gameLives==0` game-over check entirely. Unreachable today (nothing sets
  `isDead=1` live), but a confirmed, severe defect the instant it's wired up.
- Confirmed product decision (Phase 37): the user confirmed wiring star collision and falling into
  the repaired trigger, making "3 lives, death animation, respawn" the real, playable Runner
  behavior — not merely an internally-correct-but-dormant mechanism. Evidence: a commented-out
  `isDead=1`/`isDead=0` pair directly bracketing the live `gameLives=0` star-collision shortcut,
  a working `gameLives=3` reset, and a fully-built (but previously unreachable) death-sprite/
  countdown mechanism, all consistent with an abandoned respawn design.
- Confirmed defect found during design validation (Phase 38, before any code was written): an
  earlier draft of the respawn logic only reset `y`/`dy`, never `x`, and never touched
  `secondPlayer` at all. Since falling off the left edge (`x<0`) is itself a trigger, and player
  movement freezes during the countdown, failing to clamp `x` back to a safe value would make
  every left-edge death re-trigger every ~120 frames with zero player agency to stop it — draining
  all 3 lives in ~6 seconds on the single most common way to die in this mode. Fixed before
  implementation began: respawn clamps `x` to `0` only if negative (mirrored for `secondPlayer`,
  which also gained the movement freeze it previously lacked entirely).
- Risk: this is deliberately **one atomic commit**, not split further — a Plan-mode validation
  pass confirmed that if the old `process2()` block survived even one commit alongside the new
  trigger being wired live, it would immediately reintroduce (and worsen) the exact double-decrement
  bug being fixed.
- Verification performed: `make mingw` — 0 errors, 167 warnings (unchanged). All four existing
  test suites still pass.
- Rollback: `git revert` the commit.
- Completion criteria: met — exactly one life decrement per death trigger, respawn or game over
  exactly once, no re-trigger loop for the fall or multiplayer cases found during validation.

### Phase 39 — Non-interactive Runner death-lifecycle test
- Files: new `docs/verification/runner_death_test.c`, `Makefile` (`mingw-deathtest` target).
- Action: 39 checks covering single death (exactly one decrement, one respawn, returns to idle),
  repeated trigger while active (no extra decrement, no countdown restart), game over at the last
  life (correct transition, no respawn, no repeated transition — verified via the real
  `runner_frame()` pipeline, not just the death function in isolation), pause (death progression
  frozen while paused via the actual `doRender_pause`/`pause_events` functions, resumes correctly
  from where it left off), session reset (clears all death state), scene departure (leaving Runner
  gameplay via the actual `menu0_events`/`doRender_menu0` functions causes no further death-state
  mutation), render purity while dying (including the new `deathCountdown` field), and the
  left-edge-fall/multiplayer respawn fixes specifically.
- Verification performed: **runtime-verified**, not static — `deathtest.exe` runs and every check
  is a live assertion. All 39 checks passed on the first run. `make mingw` — 0 errors, 167 warnings
  (unchanged, confirmed 0 from the new file). All other test suites still pass.
- Rollback: delete the test file and Makefile target.
- Completion criteria: met.

### Phase 40 — Reproducible GitHub Actions CI
- Files: new `.github/workflows/mingw-validation.yml`, new `scripts/setup-mingw-sdl2.sh`,
  `Makefile` (`SDL2_VERSION`/`SDL2_IMAGE_VERSION`/`SDL2_TTF_VERSION`/`SDL2_MIXER_VERSION` variables
  + `print-mingw-versions` target), new `.gitattributes`.
- What was done: Phase 5's PR merged with no CI run at all. Added a workflow (windows-latest,
  `msys2/setup-msys2@v2` with `msystem: MINGW64` — confirmed during design validation that MINGW64,
  not UCRT64, is required because the official SDL2 "mingw" devel packages target the legacy
  MSVCRT runtime matching MINGW64's, not UCRT64's UCRT, even though both environments report the
  same `x86_64-w64-mingw32` triple) that fetches the four pinned SDL2 packages via a new setup
  script, builds, and runs all five non-interactive test suites, uploading logs on failure.
  Extracted the SDL2/_image/_ttf/_mixer version numbers into Makefile variables plus a
  `print-mingw-versions` target so the setup script has one source of truth instead of a third
  hardcoded copy that could drift from the Makefile's own `-I`/`-L` paths. Added `.gitattributes`
  forcing LF line endings on `*.sh` — without it, a CRLF-mangled shebang would break the setup
  script under bash on any checkout with `core.autocrlf=true`, including GitHub's Windows runners.
- Warning policy: unchanged flags (`MINGW_WARN_FLAGS`), no `-Werror`, no `-w` added; the workflow
  logs the warning count (informational) rather than failing the build on it.
- Verification performed: all four pinned SDL2 MinGW package release URLs confirmed reachable
  (HTTP 200) during design validation and again before this commit. Local build/test suite
  unaffected by the Makefile variable extraction — same 167 warnings, all five suites pass.
  **CI itself validated by the PR's own workflow run** — see the PR for the actual result; do not
  merge until it shows green (this phase's explicit requirement).
- Rollback: `git revert` the commit; the version-variable extraction is safe to keep regardless.

## Phases executed in Pass 7 (unused code/asset cleanup)

Full evidence-based audit in `docs/unused-code-assets-audit.md`, written before any Pass 7 code
edit. Baseline: commit `94d117b` (tag `refactor-pre-unused-cleanup`, created at the start of this
pass), `make mingw` 0 errors/167 warnings, all six existing test targets passing — confirmed
before any edit. Three independent research passes (source/symbol reachability, asset
reference-tracing + SHA-256 duplicate hashing, generated/tracked-artifact review), personal
re-verification of every high-impact finding, and a dedicated validation pass that caught one real
gap (the dead `shooting` field's two write sites needed deleting alongside it, or `loadGame.c`
would fail to compile).

### Phase 41 — Unused-code/asset audit
- Files: new `docs/unused-code-assets-audit.md`.
- What was done: complete classification of every source file, function, declaration, macro,
  commented block, and asset — Proven unused / Highly likely unused / Unclear / Used, per the
  phase's own confidence tiers. Only "Proven unused" items were scheduled for deletion.
- Rollback: delete the file; no code affected.

### Phase 42 — Remove unreachable source files and dead helper functions
- Files: deleted `src/mx_atoi.c` through `src/mx_strsplit.c` (16 files, a self-contained dead
  string/array-helper cluster where every function is either uncalled or called only by another
  uncalled function in the same cluster), `src/leader_events.c`, `src/doRender_multiplayer.c`;
  removed `shutdown_status_x`/`shutdown_status_x_list`/`shutdown_status_kills`/`load_chunk` (each
  zero-caller, siblings in the same files left untouched); removed all 18 corresponding
  `header.h` prototypes.
- Risk: none identified — confirmed via a dedicated validation pass that read every deletion site
  directly; no memory-layout, `#include`, or Makefile-listing dependency exists on any of them
  (`SRCS := src/*.c` is a pure glob).
- Validation: `make mingw` — 0 errors, warnings dropped 167→161 (6 pre-existing warnings that
  lived *inside* the deleted `mx_*` files themselves, confirmed via an exact before/after diff of
  the warning list — zero new warnings anywhere else). All six test targets still pass.
- Rollback: `git revert` the two commits.
- Completion criteria: met — every deleted item had zero live callers, confirmed independently
  twice.

### Phase 43 — Remove dead struct fields, macros, orphaned prototype, and obsolete comments
- Files: `inc/header.h` (13 dead `GameState`/`Man` fields, 2 dead macros
  `STATUS_STATE_GAMEOVER`/`LEADERBOARD_TXT`, 1 never-defined prototype `draw_status_x_list`);
  `src/app.c` (7 now-dangling `destroy_texture()` calls removed alongside the fields they
  referenced); `src/doRender.c` (a commented-out "attack render" block tied to the removed
  fields); `src/loadGame.c` (the write-only `shooting` field's 2 write sites — a gap a
  Plan-mode validation pass caught before any code was touched, since leaving them would have
  broken compilation); `src/processEvents.c` (two self-contained obsolete commented-out
  leaderboard-file-persistence blocks, both superseded by the live in-memory `x_list[25]`
  approach, plus several small dangling comment fragments referencing the removed fields).
- Risk: the `shooting`-field/`loadGame.c` coupling was the one real gap found during design
  validation — folded into the same atomic commit rather than left for a follow-up, matching this
  project's established practice (see Pass 6's `runner_death.c` commit) of landing tightly-coupled
  correctness-critical edits together.
- Validation: `make mingw` — 0 errors, 167 warnings (unchanged; none of the removed items
  contributed a warning). All six test targets pass after each of the two commits in this phase.
- Rollback: `git revert` the relevant commits.
- Completion criteria: met — confirmed no memory-layout dependency exists anywhere (single
  `calloc(1, sizeof(GameState))` in `main.c`, no `offsetof`/serialization use of either struct).

### Phase 44 — Remove proven-unused game assets
- Files: 22 asset files deleted (~7.68MB) — an unused 8-color background palette plus 2 alternate
  train sprites, 2 superseded prototype bullet sprites, 5 dead attack/idle/death sprites tied to
  the fields removed in Phase 43, 1 unwired terrain tile, 1 unused sound effect, and 2 dead
  leaderboard data files referenced only from inside the comment blocks removed in Phase 43.
- Risk: none — every asset load in this codebase is a single string literal (confirmed: no
  dynamic path construction exists anywhere), so exhaustive literal grep is fully dispositive.
- Validation: all five asset-loading test suites (which exercise `arcade_assets_load`/
  `runner_assets_load` end to end) pass unmodified after deletion, confirming none of the 22 files
  was ever actually loaded.
- Rollback: `git revert` the commit.
- Completion criteria: met.

### Phase 45 — Repository usage integrity check
- Files: new `scripts/audit_repository_usage.py` (stdlib-only, deterministic), `Makefile` (new
  `audit-repo` target), `.github/workflows/mingw-validation.yml` (new CI step), plus a tiny
  Makefile stale-comment cleanup (`build: remove stale build references`, 2 dead lines under
  `SRCS`, unrelated to any deletion).
- What was done: extracts every `resource/`-shaped string literal from `src/*.c` and confirms
  exact-case existence (a manual case-sensitive directory listing, not `os.path.exists`, so it
  catches casing bugs even on a case-insensitive host); confirms every `header.h` prototype has a
  matching definition. Supports an explicit allowlist — currently used for exactly the 3
  confirmed, pre-existing case-mismatch bugs found during the audit (`Sunset_front.png`,
  `brick_block.png`, `copper_block.png` — real cross-platform defects, deliberately left unfixed
  this phase per its "remove unused content, not fix bugs" charter, documented prominently rather
  than silently allowlisted away).
- Validation: script runs clean (0 findings) against the post-cleanup tree; wired into CI via the
  runner's own pre-installed Python (no MSYS2 dependency, since the script only reads files).
- Rollback: delete the script, Makefile target, and CI step.
- Completion criteria: met.

## Phases executed in Pass 8 (case-sensitive asset path fix + cross-platform validation)

Full mismatch map in `docs/asset-path-portability.md`, written before any Pass 8 code edit.
Baseline: commit `8634928` (tag `refactor-pass-7-unused-cleanup`, created at the start of this
pass), `make mingw` 0 errors/161 warnings, all eight existing local targets passing (including
`audit-repo`, whose allowlist was masking the three known defects) — confirmed before any edit.

### Phase 46 — Case-sensitive asset path mismatch map
- Files: new `docs/asset-path-portability.md`.
- What was done: re-verified all three of Phase 7's confirmed case-mismatch findings directly
  against the current tree (not relied on from the prior summary); resolved the canonical-naming
  question per directory via sibling-file convention (git history is uninformative — the whole
  asset tree was bulk-committed once at repo init); confirmed via a repo-wide search across
  `src/*.c`, headers, test harnesses, the Makefile, the setup script, the CI workflow, and docs
  that no additional mismatch of any kind exists.
- Rollback: delete the file; no code affected.

### Phase 47 — Fix confirmed case-mismatch defects
- Files: `src/loadGame.c`.
- What was done: corrected the three literals to match their already-existing, unmodified
  tracked files — `Sunset_front.png`→`sunset_front.png`, `brick_block.png`→`Brick_block.png`,
  `copper_block.png`→`Copper_block.png`. Zero file renames; the two terrain files' PascalCase
  convention (matching the already-correct `Clay_block.png`) and the background directory's
  majority lowercase convention (10 of 12 files) each independently pointed at "fix the literal,"
  not "rename the file."
- Behavior impact: none — the corrected literals point at content-identical, unmodified files;
  confirmed via the full local test suite, which exercises the real production asset-loading
  path end to end.
- Validation: `make mingw` — 0 errors, 161 warnings (unchanged). All eight local targets pass.
- Rollback: `git revert`.

### Phase 48 — Strengthen the repository asset-path audit
- Files: `scripts/audit_repository_usage.py`, `docs/verification/lifecycle_test.c`.
- What was done: restructured findings into three explicit tiers (errors / known exceptions /
  informational) instead of a flat count. New checks: the resolved path must be a regular file;
  no ambiguous case-colliding sibling may exist in the same directory; a raw literal must not
  contain a Windows-only backslash separator; two or more literals resolving to the same
  canonical file are now reported informationally (never as an error) — this surfaced 4
  legitimate lazy-load-guard duplicates (`jump.wav`, `select.wav`, `shoot.wav`, `menuMus.mp3`)
  that were previously invisible. Removed the three `ALLOWLIST_ASSET_PATHS` entries fixed in
  Phase 47 — the script now reports a clean `Result: PASS` with 0 known exceptions rather than
  silently masking them. Added three named assertions to `lifecycle_test.c`
  (`sheetTextureBack2`/`brick_block`/`copper_block` all non-NULL after `arcade_assets_load()`) so
  a future regression on any one of these three specific fields fails with an unambiguous
  assertion, not just the existing blanket "load succeeded" check.
- Validation: **runtime-verified**, not static — the script actually runs and reports real
  findings (0 errors, 4 legitimate informational notices), not a code-reading exercise. `make
  mingw` — 0 errors, 161 warnings. All eight local targets pass.
- Rollback: `git revert`.

### Phase 49-50 — Case-sensitive Linux CI job + additive Linux build attempt
- Files: `.github/workflows/mingw-validation.yml` (new `linux-asset-validation` job), `Makefile`
  (new `linux`/`linux-smoketest`/`linux-clean` targets, entirely separate variable family from
  `all`/`mingw*`, zero changes to either).
- What was done: Windows CI, however thorough, cannot prove case-sensitive path correctness — its
  filesystem silently accepts a wrong-case request. The new job's mandatory step runs the real
  `audit_repository_usage.py` against Linux's genuinely case-sensitive filesystem — the actual
  proof the Phase 47 corrections are right, not just statically plausible. A second, best-effort
  part (every step `continue-on-error: true`) attempts a full additive Linux build + headless
  smoke test: installs SDL2 dev packages via `apt-get`, generates the same kind of
  `SDL2_image`/`SDL2_ttf`/`SDL2_mixer` compat-include header shim already proven necessary for the
  Windows/MinGW build (most Linux distributions are expected to ship the same flat `SDL2/` header
  layout that required it there), and runs the smoke test with SDL's dummy audio/video drivers
  (GitHub-hosted Linux runners have neither an audio device nor a display server).
- Reasoning: this best-effort part was authored **without access to a Linux toolchain to verify
  against** in this environment — its success is not assumed. `continue-on-error` ensures an
  unverified assumption about apt package names, header layout, or headless driver behavior
  cannot block the job or the PR; only the mandatory case-sensitivity check can. The actual CI
  run on the PR is the deciding, honest signal for this part, reported plainly rather than
  claimed in advance.
- Validation: local build/test suite unaffected by the Makefile additions (confirmed `make -n
  linux` produces syntactically valid, correctly-flagged commands; same 0 errors/161 warnings,
  all eight targets pass). **The workflow's actual run is reported in the pull request.**
- Rollback: `git revert`; the Makefile Linux targets are safe to keep regardless of whether the
  CI job's best-effort steps ever succeed.

## Phases executed in Pass 9 (evidence-based SOLID/GoF architecture refactor)

Full evidence and decision tables in `docs/solid-gof-audit.md`, written before any Pass 9 code
edit. Baseline: commit `e967001` (tag `refactor-pre-solid-gof`), `make mingw` 0 errors/161
warnings, all eight existing local targets passing. Scope discipline throughout: at most two
substantial GoF patterns, one or two small SOLID extractions, at most three new abstractions —
Arcade and Runner remain fully separate, `GameState` untouched, no Singleton/Service
Locator/DI container/ECS.

### Phase 51 — SOLID/GoF audit
- Files: new `docs/solid-gof-audit.md`, `docs/dependency-map.md`.
- What was done: module map, header dependency map, mutable-state ownership map, top-level call
  graph, per-SOLID-principle findings, and a full GoF candidate-pattern decision table
  (required/already-present/rejected/deferred), all gathered directly from the tree via three
  parallel codebase explorations plus targeted direct reads — not carried over from any prior
  summary. Two precision corrections were made to the audit *during* implementation as real
  constraints surfaced (documented transparently in the doc itself, not silently): the exact
  spawn-site count (`loadGame.c`'s `enemyValues` reset loop has a genuinely different field
  contract than every other site and was left unconverted), and the header-split asymmetry
  (`app_change_scene` has 5 real callers vs. `arcade_frame`/`runner_frame`'s 1, changing what was
  safely trimmable from `header.h` without touching unrelated files).
- Rollback: delete both files; no code affected.

### Phase 52 — Factory Method: entity-spawn invariants
- Files: new `src/entity_spawn.c`, `inc/entity_spawn.h`; edited `src/process.c` (8 wave-trigger
  blocks), `src/loadGame.c` (2 of 3 reset loops).
- What was done: `enemy_spawn`/`smart_enemy_spawn`/`boss_spawn` centralize the 5-6-field
  `Enemies` struct initialization previously hand-duplicated at 10 call sites, each now
  bounds-checking its index (a correctness guarantee none of the original inline code had) and
  returning `false` without mutating anything on an out-of-range index. `loadGame.c`'s
  `enemyValues` reset loop is deliberately left inline (its contract omits `visible`/`countShots`,
  unlike every other site — converting it would have silently changed reset behavior).
- Behavior impact: none — every converted call site passes the same literal values it used
  before.
- Validation: `make mingw` — 0 errors, 161 warnings. All 9 local targets pass (including the new
  `docs/verification/entity_spawn_test.c` / `mingw-entityspawntest`, asserting exact field values
  against the original literals plus the new out-of-range rejection).
- Rollback: `git revert`.

### Phase 53 — ISP: focused scene/frame headers
- Files: new `inc/scene.h`, `inc/frame.h`; edited `inc/header.h`, `src/scene.c`, `src/frame.c`,
  `src/main.c`, plus two test harnesses (`frame_pipeline_test.c`, `runner_death_test.c`) that call
  `arcade_frame`/`runner_frame` directly and needed the same new include.
- What was done: mirrors the existing `inc/app.h` pattern. `arcade_frame`/`runner_frame` (exactly
  one definer, one production caller) moved fully out of `header.h` into `frame.h`.
  `app_change_scene` (5 real callers) is declared in *both* `header.h` (unchanged, so 3 unrelated
  callers need no edit) and the new `scene.h` — a legal duplicate prototype, per the task's own
  "leave unresolved declarations in the compatibility header, migrate gradually" guidance.
  `AppScene` itself stays defined in `header.h` (moving it would create a circular include with
  `GameState.scene`'s own dependency on it).
- Validation: `make mingw` — 0 errors, 161 warnings. All 9 local targets pass; `audit-repo` still
  reports `Result: PASS` (prototype-matching check unaffected by the duplicate declaration).
- Rollback: `git revert`.

### Phase 54 — Command: discrete-action input translation
- Files: new `src/input_command.c`, `inc/input_command.h`; edited `src/processEvents.c`
  (`processEvents`/`processEvents2`), `src/menu_events.c` (`menu0_events` only).
- What was done: `GameCommand` enum + `translate_arcade_command`/`translate_runner_command`/
  `translate_menu_command` — pure, synchronous `SDL_Keycode -> GameCommand` functions replacing
  the raw-keycode `switch` dispatch in the two largest, most parallel files in the codebase
  (`processEvents`/`processEvents2`, which independently hand-implement the same 5 keys meaning
  the same 5 actions) and formalizing `menu0_events()`'s existing dual-keycode fall-through
  pattern. `menu_events()` (mode-select submenu) and `pause_events()` are deliberately **not**
  converted — neither duplicates its own keymap the way `processEvents`/`processEvents2` do with
  each other, so conversion wouldn't meaningfully reduce complexity there; recorded as smaller
  deferred debt, not silently dropped. `processEvents2()`'s `SDLK_0` cheat code is intentionally
  left outside `GameCommand` (not duplicated elsewhere) and still checked as a raw keycode
  alongside the new dispatch. No queue, no heap allocation, no change to continuous held-key
  movement polling.
- Behavior impact: none — every converted handler body is byte-for-byte unchanged; only the
  dispatch key (keycode vs. `GameCommand`) changed.
- Validation: `make mingw` — 0 errors, 161 warnings. All 10 local targets pass (including the new
  `docs/verification/command_translation_test.c` / `mingw-commandtest`, asserting every documented
  key maps to its documented command, both dual-keycode pairs, and the `SDLK_0` exclusion).
- Rollback: `git revert`.

### Phase 55 — DIP consistency fix: `load_menu.c`
- Files: `src/load_menu.c`.
- What was done: replaced 3 raw `IMG_Load()`/`SDL_CreateTextureFromSurface()` call sites (the one
  place in the codebase bypassing the checked, NULL-safe `load_texture()` wrapper every other
  texture load already uses) with `load_texture()` calls. Each site's original fail-hard behavior
  (print message, `SDL_Quit()`, `exit(1)`) is unchanged — only the loading call itself is now
  checked and consistent with every other texture-load site.
- Validation: `make mingw` — 0 errors, 161 warnings. All 10 local targets pass.
- Rollback: `git revert`.

## Phases executed in Pass 10 (incremental GameState decomposition and header isolation)

Full evidence in `docs/gamestate-decomposition.md`, written before any Pass 10 code edit.
Baseline: commit `81dda1e` (tag `refactor-pre-gamestate-decomposition`, `main` after Phase 9's
PR #5 merge), `make mingw` 0 errors/161 warnings, all 8 existing local targets passing.

### Phase 56 — GameState ownership audit
- Files: new `docs/gamestate-decomposition.md`.
- What was done: field-by-field `GameState` inventory (owner, lifecycle, init/reset/destroy
  points, candidate group); memory-layout safety verification (exactly one `sizeof(GameState)`
  site, zero memcpy/memset/casts/pointer-arithmetic/offsetof/file-I-O/whole-struct-memcmp) —
  nested structs confirmed safe to introduce; exact access-site counts for every candidate group;
  rationale for selecting `AppContext`/`AssetLifecycleFlags` over audio fields (lifecycle not
  centralized) and Arcade/Runner session data (large/high-risk, explicitly discouraged as a first
  choice). Also found an accidental, pre-existing verbatim duplicate of `doRender`'s prototype
  inside `header.h` itself.
- Rollback: delete the file.

### Phase 57 — Terminology correction
- Files: `docs/design-patterns.md`, `docs/solid-gof-audit.md`.
- What was done: `enemy_spawn`/`smart_enemy_spawn`/`boss_spawn` were labeled "Factory Method" in
  Phase 9's docs. Corrected to "Simple Factory-style creation function" — no creator hierarchy, no
  subclassing, no polymorphic product selection exists or was added; the simpler C approach was
  deliberately chosen. `src/entity_spawn.c`/`inc/entity_spawn.h` unchanged (documentation-only).
- Rollback: `git revert`.

### Phase 58 — Complete scene interface migration
- Files: `inc/header.h`, `src/menu_events.c`, `src/pause_events.c`, `src/processEvents.c`, plus 4
  test harnesses that call `app_change_scene`/`arcade_frame`/`runner_frame` directly.
- What was done: the 3 remaining callers of `app_change_scene()` now include `scene.h` directly;
  `header.h`'s duplicate prototype (deliberately left since Phase 9) is removed — `scene.h` is now
  the single authoritative declaration. Also removed an accidental, unrelated verbatim duplicate
  of `doRender`'s prototype found inside `header.h` itself during the audit.
- Validation: `make mingw` — 0 errors, 161 warnings. All 8 local targets pass.
- Rollback: `git revert`.

### Phase 59-60 — Header self-containment tests + static duplicate-declaration check
- Files: new `docs/verification/header_only_{app,scene,frame,entity_spawn,input_command}.c`,
  `Makefile` (new `mingw-headertest` target), `scripts/audit_repository_usage.py`.
- What was done: one tiny translation unit per focused header, each compiled with
  `-fsyntax-only` (parse + typecheck, no codegen/link) to confirm the header compiles standalone
  with no reliance on inclusion order. Extended the audit script with a new check scanning every
  `inc/*.h` file for duplicate function-prototype declarations — a regression guard against
  reintroducing an accidental duplicate like the `doRender` one found this phase.
- Validation: `make mingw-headertest` passes (0 warnings/errors on all 5 headers); `audit-repo`
  reports `Duplicate declaration errors: 0`.
- Rollback: `git revert`.

### Phase 61 — GameState nested structs: AppContext + AssetLifecycleFlags
- Files: `inc/header.h` (both struct definitions), and every file with a migrated access site:
  `app.c`, `scene.c`, `loadGame.c`, `load_menu.c`, `kills_score.c`, `X_score.c`,
  `x_list_leader.c`, `status.c`, `draw_lifes.c`, `main.c`, `menu_events.c`, `pause_events.c`,
  `processEvents.c`, and 5 `docs/verification/*.c` test files.
- What was done: `AppContext { renderer; scene; }` (field `app`) and `AssetLifecycleFlags
  { arcadeAssetsLoaded; runnerAssetsLoaded; sharedAssetsLoaded; }` (field `assetFlags`) — the two
  cleanest, most centrally-owned groups found in the audit. `window` deliberately excluded (never
  a `GameState` field). Both nested-struct migrations landed in one commit rather than the two
  originally planned: `loadGame.c`, `scene.c`, `header.h`, and 3 test files are touched by both
  migrations, and a clean hunk-level split risked an intermediate non-compiling state — documented
  as a deliberate, transparent deviation from the original commit sequence. Every access site is a
  pure mechanical rename (`game->renderer` → `game->app.renderer`, etc.), zero logic change.
- Behavior impact: none — confirmed by the full regression suite plus the new grouping test.
- Validation: `make mingw` — 0 errors, 161 warnings. All 9 local targets pass.
- Rollback: `git revert` (reverts both groups together).

### Phase 62 — GameState grouping lifecycle test
- Files: new `docs/verification/gamestate_grouping_test.c`, `Makefile` (new
  `mingw-groupingtest` target).
- What was done: verifies both new nested structs are correctly zero-initialized via the existing
  `calloc`, explicitly writable through the real production functions
  (`app_change_scene`/`arcade_assets_load`/`arcade_assets_unload`), reset/cleaned-up correctly, and
  that unrelated `GameState` fields remain untouched by either group's operations.
- Validation: `make mingw` — 0 errors, 161 warnings. All 10 local targets pass.
- Rollback: `git revert`.

## Phases executed in Pass 11 (fixed-timestep player physics)

Full evidence in `docs/physics-timestep-map.md`, written before any Pass 11 code edit. Baseline:
commit `a58268f` (tag `refactor-pre-fixed-timestep`, `main` after Phase 10's PR #6 merge),
`make mingw` 0 errors/161 warnings, all 10 existing local targets passing. Triggered by an external
technical assessment identifying frame-dependent physics (no fixed timestep) as the core defect in
the custom kinematic system; this pass implements the assessment's own "Phase 1 — Stabilize time"
scope only (fixed 60Hz accumulator + player-only per-second unit conversion), narrower than the
assessment's full 6-phase roadmap — Phases 2-6 (input/simulation split, collision correctness,
projectile correctness, game feel, optimization) are explicitly out of scope and deferred.

### Phase 63 — Fixed-timestep physics conversion map
- Files: new `docs/physics-timestep-map.md`.
- What was done: confirmed zero existing timing infrastructure anywhere in the tree; full
  inventory of every frame-dependent player constant (gravity, jump impulse, horizontal
  accel/clamp, jump-hold thrust, friction, snap-to-zero) with exact file:line citations;
  mathematically derived and verified the frame-to-per-second conversion factors (velocity ×60,
  acceleration ×3600); documented the `processEvents`/`processEvents2` → fixed-tick relocation
  finding (see Phase 65); listed everything found but deliberately deferred (enemy/boss/bullet/
  decor/trap timing, the Runner ceiling `onLedge` bug, the bullet triple-movement bug,
  jump-hold-thrust redesign, collision-order issues).
- Rollback: delete the file.

### Phase 64 — Fixed-timestep constants and simulate functions
- Files: `inc/header.h`, `inc/frame.h`, `src/frame.c`.
- What was done: added `PHYSICS_HZ`/`PHYSICS_DT`/`MAX_FRAME_TIME`/`MAX_PHYSICS_STEPS_PER_FRAME`
  and the 8 player per-second constants (`GRAVITY_PER_SEC2`, `JUMP_SPEED_PER_SEC`,
  `RUN_ACCEL_PER_SEC2`, `RUN_MAX_SPEED_PER_SEC`, `ARCADE_JUMP_HOLD_ACCEL_PER_SEC2`,
  `RUNNER_JUMP_HOLD_ACCEL_PER_SEC2`, `RUN_FRICTION_DECAY_PER_TICK`,
  `RUN_SNAP_ZERO_SPEED_PER_SEC`); removed the old frame-tuned `GRAVITY` macro (only 4 call sites,
  all converted in Phase 65); introduced `arcade_simulate`/`runner_simulate` (`src/frame.c`) as the
  fixed-tick entry points, and gave `arcade_frame`/`runner_frame`/`process`/`process2` an explicit
  `float dt` parameter.
- Rollback: `git revert`.

### Phase 65 — Player physics converted to fixed-timestep per-second units
- Files: `src/process.c`, `src/processEvents.c`.
- What was done: converted the 4 gravity sites and 4 man/secondPlayer position-integration sites in
  `process()`/`process2()` to use `dt` and the new per-second constants; converted the 4 jump-impulse
  sites in `processEvents.c` (`-10` → `-JUMP_SPEED_PER_SEC`, still a one-shot discrete assignment,
  no `dt` needed).
- Architectural refinement made during implementation (not in the original plan text): the held-key
  continuous-force blocks (horizontal accel/clamp, jump-hold thrust, friction/snap) were relocated
  out of `processEvents()`/`processEvents2()` into two new standalone functions,
  `apply_arcade_player_forces`/`apply_runner_player_forces` (`src/process.c`) — not directly into
  `process()`/`process2()` as originally planned. Reason: several existing non-interactive tests
  call `process()`/`process2()` directly with manually-set `dx`/`dy`/`slowingDown` state; had the
  held-key code (which reads `SDL_GetKeyboardState`) lived inside those functions, the headless
  "no keys pressed" reading would have silently overwritten the tests' hand-set preconditions. See
  `docs/physics-timestep-map.md` section 4 for the full rationale. Caught via a failing test run,
  fixed by extraction, not by weakening the tests.
- Also caught and corrected during implementation: an initial draft of
  `apply_runner_player_forces` incorrectly copied Arcade's `game->time % 6 == 0` animation-frame
  gating onto Runner's relocated blocks; re-verified against the exact original
  `processEvents2()` content confirmed Runner has no such gating (it advances `animFrame` via a
  different modulus inside `process2()`'s own untouched integration block) — removed before
  finalizing.
- Warning count: 161 → 145 (a decrease). Explained, not just observed: 8 relocated held-key accel
  lines used bare `double` literals against `float` fields (`-Wdouble-promotion`/
  `-Wfloat-conversion`, 2 warnings each = 16); the new code uses explicit `float`-suffixed constants
  multiplied by `dt` (also `float`), eliminating the mismatch. `process.c`'s own warning count held
  exactly flat (70 → 70) despite the added code.
- Rollback: `git revert`.

### Phase 66 — Fixed-timestep accumulator in the main loop
- Files: `src/main.c`.
- What was done: added a real-time accumulator (`SDL_GetPerformanceCounter`/
  `SDL_GetPerformanceFrequency`, real frame time capped at `MAX_FRAME_TIME` to avoid a catch-up
  "spiral of death" after a debugger pause or similar stall). The two gameplay scene cases
  (`APP_SCENE_ARCADE_GAME`, `APP_SCENE_RUNNER_GAME`) each run a bounded
  `while (accumulator >= PHYSICS_DT && steps < MAX_PHYSICS_STEPS_PER_FRAME)` loop calling
  `arcade_simulate`/`runner_simulate`, then render/process-events once, exactly as before. The
  other 7 scene cases (menus, leaderboards, pause) are unchanged and never accrue the accumulator,
  so no time banks up while paused or in a menu. `runner_update_death()` deliberately stays outside
  `runner_simulate()`, called once per real frame after `doRender2()` exactly as before — Phase 5/6
  deliberately renders the death-in-progress state before resolving it, and folding it into the
  fixed-tick step would have changed that order.
- Rollback: `git revert`.

### Phase 67 — Existing test harnesses updated for explicit `dt`
- Files: `docs/verification/frame_pipeline_test.c`, `docs/verification/runner_death_test.c`.
- What was done: mechanical update — every direct call to `process`/`process2`/`arcade_frame`/
  `runner_frame` now passes an explicit `PHYSICS_DT` argument. No assertion values changed: `process`/
  `process2` no longer read keyboard state (Phase 65's extraction), so existing `animFrame`/
  `time%3` assertions remained valid unchanged.
- Rollback: `git revert`.

### Phase 68 — Fixed-timestep physics validation test
- Files: new `docs/verification/physics_timestep_test.c`, `Makefile` (new `mingw-physicstest`
  target), `.github/workflows/mingw-validation.yml` (step + log-upload path, both added in this
  same commit).
- What was done: (1) regression-equivalence — one fixed tick from rest reproduces the old
  per-frame code's exact numeric behavior bit-for-bit (`dy == GRAVITY_PER_SEC2*PHYSICS_DT`, `y`
  moves by exactly `0.5f`); (2) refresh-rate independence — the same total simulated time (~1
  second), chunked into a fast-display pattern (240 small real-time steps) vs. a slow-display
  pattern (30 large real-time steps) driving the same accumulator, produces the same tick count and
  identical final `man.x`/`y`/`dx`/`dy`. One assertion was loosened during implementation: an exact
  `ticksA == 60` check was replaced with `ticksA == ticksB` plus a `59..60` range check, since
  double-precision summation of `1.0/240.0` (×240) or `1.0/30.0` (×30) doesn't guarantee landing on
  exactly `1.0` — the real invariant is that both chunking patterns agree with each other, not that
  either hits a bit-exact tick count.
- Validation: `make mingw` — 0 errors, 145 warnings. All 10 local targets pass.
- Rollback: `git revert`.

### Phase 69 — Final documentation pass
- Files: `docs/physics-timestep-map.md` (section 4 correction to describe the actual
  `apply_*_player_forces` architecture rather than the originally-planned direct relocation),
  `docs/refactor-log.md`, `docs/refactor-plan.md`, `README.md`.
- What was done: standard per-commit doc entries; corrected the one section of
  `docs/physics-timestep-map.md` that had gone stale after Phase 65's extraction refinement; marked
  this document's own pre-existing "Phase 11 — Delta-time conversion" forward-reference as done.
- Rollback: `git revert`.

**Pass 11 deferred items** (full reasoning in `docs/physics-timestep-map.md` section 6): enemy
gravity (`+= 0.4`), boss gravity (`+= 0.1`), smart-enemy discrete impulses, all bullet-movement
code (including a separately confirmed pre-existing bug where bullets can move up to 3× per frame),
background/decor scroll, and the moving-trap `sin`/`cos` block remain frame-tied — not converted,
consistent with the assessment's own later phases (3, 4, 6) covering entities/projectiles/collision
separately. The Runner ceiling `onLedge` bug (`collisionDetect.c:656,714` sets `onLedge=1` on
ceiling hit; Arcade's equivalent correctly sets `0`), jump-hold-thrust redesign (coyote time, jump
buffering, variable jump height), and collision-order/corner-case issues were all found during
exploration and documented, none fixed this pass. Ready to start once a future phase is scoped
specifically for one of these.

## Phases executed in Pass 12 (input/simulation separation)

Full evidence in `docs/input-simulation-separation-map.md`, written before any Pass 12 code edit.
Baseline: commit `8410b18` (tag `refactor-pre-input-simulation-separation`, `main` after Phase
11's PR #7 merge), `make mingw` 0 errors/145 warnings, all 10 existing local targets passing.
Implements the physics assessment's own "Phase 2 — separate input and simulation (`InputState`
struct, edge-triggered de-duplication)", the recommended next step after Pass 11's fixed timestep.

### Phase 70 — Input/simulation separation map
- Files: new `docs/input-simulation-separation-map.md`.
- What was done: confirmed Pass 11 fixed the continuous-force half (held-key accel/jump-hold/
  friction, running at the fixed tick via `apply_*_player_forces`) but left discrete jump-trigger
  handling inside `processEvents()`/`processEvents2()`, called once per real frame rather than at
  the fixed tick rate — the exact gap the assessment's "Phase 2" names. Also found and documented a
  regression: Runner's two jump-impulse sites were never converted off the bare frame-tuned `-10`
  during Pass 11's timestep conversion (Arcade's equivalent sites were correctly converted),
  producing a jump 60× weaker than intended since Pass 11 merged. Designed the `InputState` struct
  and edge-triggered consumption approach, and the deferred-items list.
- Rollback: delete the file.

### Phase 71 — InputState struct and jump-request consumption
- Files: `inc/header.h`, `src/process.c`, `src/frame.c`.
- What was done: added `InputState` (`jumpRequestedPlayer1`/`jumpRequestedPlayer2`, `GameState`
  field `input`) following Phase 61's `AppContext`/`AssetLifecycleFlags` nested-struct pattern; new
  `consume_arcade_jump_requests`/`consume_runner_jump_requests` (`src/process.c`), wired into
  `arcade_simulate`/`runner_simulate` (`src/frame.c`) before `apply_*_player_forces`, matching the
  original relative ordering (a keydown-triggered jump impulse could stack with jump-hold thrust in
  the same real frame before, so consumption runs first, preserving that). No producer sets the
  flags yet — no observable behavior change in this commit.
- Validation: `make mingw` — 0 errors, 145 warnings (unchanged). All 10 local targets pass.
- Rollback: `git revert`.

### Phase 72 — Route jump input through InputState
- Files: `src/processEvents.c`, `src/loadGame.c`.
- What was done: `GAME_COMMAND_JUMP_PLAYER1`/`_PLAYER2` case bodies in both
  `processEvents()`/`processEvents2()` shrink to a single flag-set
  (`game->input.jumpRequestedPlayer1 = true;`) — the `onLedge` check, `dy` assignment, and SFX
  playback move to the consumption functions added in Phase 71, which now run at the fixed physics
  tick rate instead of once per real frame. This is also where Phase 70's discovered regression is
  fixed: both Arcade and Runner now route through the same `consume_*_jump_requests` functions,
  which use `JUMP_SPEED_PER_SEC` uniformly — Runner's jump impulse is corrected as a direct
  consequence of the relocation, not a separate patch. `arcade_session_reset`/`runner_session_reset`
  (`src/loadGame.c`) also clear the new request flags, so a request left pending from a previous
  session (e.g. quit mid-air just after a keydown) can't fire an unwanted instant jump at the start
  of the next one.
- Behavior impact: Arcade jump feel unchanged (its impulse was already correct); Runner jump height
  corrected (previously 60× too weak due to the Phase 11 regression). Everything else (pause, quit,
  scene transitions, SFX loads, cheat code, decor scroll, death triggers) untouched.
- Validation: `make mingw` — 0 errors, 145 warnings (unchanged). All 10 local targets pass.
- Rollback: `git revert`.

### Phase 73 — Input/simulation separation validation test
- Files: new `docs/verification/input_state_test.c`, `Makefile` (new `mingw-inputtest` target),
  `.github/workflows/mingw-validation.yml` (step + log-upload path, both added in this same commit).
- What was done: (1) edge-triggered single consumption — a grounded request fires the jump exactly
  once and clears itself, a second consume call in the same real frame does not re-fire even if
  re-grounded in between; (2) an airborne request is dropped, not buffered, matching the original's
  silent-drop semantics; (3) Runner's jump impulse uses `JUMP_SPEED_PER_SEC`, not the never-
  converted bare `-10` (direct regression-fix proof); (4) the player-2 path for both modes; (5) one
  real `arcade_simulate`/`runner_simulate` call confirms the consumption wires correctly into the
  existing fixed-tick order (`dy` after one tick equals the jump impulse plus one tick's gravity).
- Validation: `make mingw` — 0 errors, 145 warnings. All 11 local targets pass.
- Rollback: `git revert`.

**Pass 12 deferred items** (full reasoning in `docs/input-simulation-separation-map.md` section
4): pause/quit/scene-transition commands (true one-shot, no fixed-tick-owned gating state);
collision-derived scene transitions and the Runner fall-death trigger inside
`processEvents`/`processEvents2` (same *shape* of "reads tick-owned state once per real frame"
issue as jump, but for collision/death consequences — the assessment's own Phase 3, not Phase 2);
shooting (`SDL_SCANCODE_SPACE`/`KP_0` held-check + `shotCount` re-arm timer — needs bullet-pool/
projectile-ownership decisions, the assessment's own Phase 4); the Runner `SDLK_0` cheat code and
menu/pause raw event handling (not gameplay-physics input). Ready to start once a future phase is
scoped specifically for one of these.

## Phases executed in Pass 13 (collision correctness, player-only)

Full evidence in `docs/collision-correctness-map.md`, written before any Pass 13 code edit.
Baseline: commit `c7fb663` (tag `refactor-pre-collision-correctness`, `main` after Phase 12's
PR #8 merge), `make mingw` 0 errors/145 warnings, all 11 existing local targets passing.
Implements the physics assessment's own "Phase 3 — correct collisions: previous-position
tracking, reset grounded each step, standardize hitboxes, fix Runner's ceiling `onLedge` bug" —
already the designated home for the Runner bug per three prior-phase docs.

### Phase 74 — Collision correctness map
- Files: new `docs/collision-correctness-map.md`.
- What was done: confirmed a real bug broader than the already-known Runner ceiling issue —
  `onLedge` is never reset at the start of a collision pass in either mode, for either player, so
  walking off a ledge's edge (no ceiling hit, no jump) never clears it. Found the landmine a naive
  "reset each tick" would hit: the landing check's strict inequality can't re-fire while a player
  rests exactly on a surface, so resetting `onLedge` without also fixing the check would
  immediately un-ground anyone standing still. Designed the fix: a crossing-based landing test
  using a new `Man.prevY` field, captured once per tick before any of that tick's mutations.
  Scoped hitbox standardization narrowly: named the `48x48` literal the five touched blocks
  already shared, without reconciling it against the other scattered hit-test/render sizes found
  (`30x30`, `32x32`, `55x55`, `54x54`) — a gameplay-feel decision, not a correctness fix, listed as
  deferred.
- Rollback: delete the file.

### Phase 75 — Previous-Y tracking and ledge hitbox constants
- Files: `inc/header.h`, `src/collisionDetect.c`.
- What was done: added `Man.prevY` and a new `capture_player_prev_y` function (not yet called from
  anywhere); added `PLAYER_LEDGE_HITBOX_W`/`_H` (`48.0f`, not yet used at any check site). No
  behavior change in this commit — the consuming logic lands in the next phase.
- Validation: `make mingw` — 0 errors, 145 warnings (unchanged). All 11 local targets pass.
- Rollback: `git revert`.

### Phase 76 — Correct player ledge collision
- Files: `inc/header.h`, `src/collisionDetect.c`, `src/frame.c`.
- What was done: wired `capture_player_prev_y` into `arcade_simulate`/`runner_simulate` as the
  first line of each. At all 5 player ledge-collision blocks (Arcade man, Arcade secondPlayer ×2,
  Runner man, Runner secondPlayer): `onLedge` now resets to `0` once per collision pass (before
  the 100-ledge loop, not inside it — resetting inside the loop would undo a landing just detected
  against an earlier ledge); the landing check became crossing-based
  (`prevY + mh <= by && my + mh >= by && my < by + bh && dy >= 0`); the bare `48, 48` literals
  became `PLAYER_LEDGE_HITBOX_W`/`_H`. Runner's two ceiling branches (man, secondPlayer) had their
  `onLedge = 1` corrected to `onLedge = 0`, matching Arcade — the long-documented bug.
  Enemy/boss/smart-enemy ledge collision and the enemy-to-enemy self-collision loop are untouched
  — same player-only narrowing as Phases 11/12.
- Validation: `make mingw` — 0 errors, 145 warnings (unchanged). All 11 local targets pass.
- Rollback: `git revert`.

### Phase 77 — Collision correctness validation test
- Files: new `docs/verification/collision_correctness_test.c`, `Makefile` (new
  `mingw-collisiontest` target), `.github/workflows/mingw-validation.yml` (step + log-upload
  path, both added in this same commit); `src/collisionDetect.c` fix; `docs/collision-correctness-map.md`
  update.
- What was done: (1) walking off a ledge clears `onLedge` — the core bug fix; (2) resting
  continuously across multiple consecutive `collisionDetect`/`collisionDetect2` calls re-affirms
  `onLedge=1`; (3) normal falling-and-landing still works; (4) Runner's ceiling bump clears
  `onLedge` for both players (regression-fix proof); (5) `capture_player_prev_y` sets `prevY`
  correctly. Caught a second landmine while writing test 4: the crossing-based landing check (as
  landed in Phase 76) had no upper bound, so it fired a second time in the same loop iteration
  immediately after a ceiling correction (which sets `my=by+bh, dy=0` — both satisfying the
  landing condition as first written), undoing the correction that had just run. The original
  strict `my < by` had prevented this implicitly; fixed by adding `my < by + bh` back to all 5
  landing checks. Not assumed correct beforehand — caught by the test, fixed, documented.
- Validation: `make mingw` — 0 errors, 145 warnings. All 12 local targets pass.
- Rollback: `git revert`.

**Pass 13 deferred items** (full reasoning in `docs/collision-correctness-map.md` section 7):
enemy/boss/smart-enemy ledge collision and the enemy-to-enemy self-collision loop (same
player-only narrowing as Phases 11/12); deduplicating the two secondPlayer-vs-ledge blocks into
one (a real, pre-existing duplicate, its own already-deferred "Phase 10 — large paired-function
deduplication" item); star-collision (`30×30`) and enemy/boss `collide2d()` hit sizes
(`48×48`/`32×32` mixed); reconciling hit-test size with render size; making ceiling/side checks
crossing-based (no persistent state to reaffirm there, unlike landing). Ready to start once a
future phase is scoped specifically for one of these.

## Phases executed in Pass 14 (projectile correctness, Arcade-only)

Full evidence in `docs/projectile-correctness-map.md`, written before any Pass 14 code edit.
Baseline: commit `c9ff171` (tag `refactor-pre-projectile-correctness`, `main` after Phase 13's
PR #9 merge), `make mingw` 0 errors/145 warnings, all 12 existing local targets passing.
Implements the physics assessment's own "Phase 4 — correct projectiles: move bullets once per
step, swept collision, projectile pool." Bullets are entirely Arcade-only; confirmed zero
involvement in `process2()`/`processEvents2()`/`runner_session_reset()`/`doRender2()`.

### Phase 78 — Projectile correctness map
- Files: new `docs/projectile-correctness-map.md`.
- What was done: corrected the magnitude of the already-documented bullet triple-movement bug —
  three collision loops (vs. `enemyValues[101]`, `smartEnemies[10]`, `boss[2]`) each re-move the
  bullet inside their own loop body, so the real per-tick multiplier is `101+10+2=113`, not "3"
  (prior docs named the loop kinds correctly but never counted total executions). Derived the
  steady-state legacy speed (`0.1 × 113 = 11.3 px/tick`) this phase would need to preserve. Traced
  `unvisible`'s full lifecycle and confirmed it's redundant with an explicit `active` flag (a hit
  was always followed by an immediate, same-iteration free, never a delayed-despawn signal).
  Designed the pool conversion, the speed-preserving `BULLET_SPEED_PER_TICK` constant, and the
  `prevX`-based swept collision test (mirroring Phase 13's own `prevY` design for player landing).
- Rollback: delete the file.

### Phase 79 — Convert bullets to a fixed pool
- Files: `inc/header.h`, `src/process.c`, `src/doRender.c`, `docs/verification/lifecycle_test.c`.
- What was done: `Bullet` gains `prevX`/`active`, loses `unvisible`; `GameState.bullets`/
  `secondBullets` change from `Bullet*[MAX_BULLETS]` (individually `malloc`'d per shot) to
  `Bullet[MAX_BULLETS]` (value arrays). `addBullet`/`addSecondBullet` claim the first inactive
  slot in place; `removeBullet`/`removeSecondBullet` set `active = false` — no `malloc`/`free`
  anywhere in the hot path. The existing `arcade_session_reset`/`app_shutdown` loops that call
  `removeBullet` in a `for` loop need no changes — the new body is a drop-in replacement for the
  old one. Storage representation only: bullets still move up to 113× per tick in this commit,
  documented plainly as an intentional intermediate step. The one existing bullet-touching test
  (`lifecycle_test.c`) gets its two assertions updated for the value-type comparison.
- Validation: `make mingw` — 0 errors, 145 warnings (unchanged). All 12 local targets pass.
- Rollback: `git revert`.

### Phase 80 — Move bullets once per step with swept collision
- Files: `inc/header.h`, `src/process.c`, `src/frame.c`.
- What was done: new `move_arcade_bullets`, called from `arcade_simulate` before `process` — the
  only place bullets now move or get clamped. The 6 collision loops (3 single-player + 3
  multiplayer mirror) drop their movement/clamp/off-screen-despawn code and their point-based hit
  test becomes a segment-vs-rect swept test using the bullet's `prevX`. Hit-response logic
  (score/kill thresholds, despawn-on-hit) unchanged.
- Bug caught before the test was written, not by it: the initial `move_arcade_bullets` reused the
  legacy clamp's *shape* (a max-clamp), which only shrinks `dx` when already larger than the
  bound — correct for the old bound (`0.1`, smaller than spawn `dx=±3`) but silently inert for the
  new bound (`11.3`, larger than spawn `dx`), which would have left bullets moving at the spawn
  speed instead of the intended speed-preserving one. Fixed in its own commit by normalizing `dx`
  to `±BULLET_SPEED_PER_TICK` by sign instead of clamping a magnitude.
- Warning count: 145 → 121 (a decrease, not a suppression). Removing the 12 bare-double-literal
  clamp blocks (`0.1`/`-0.1` against `float` fields, 2 warnings each = 24) in favor of the
  float-suffixed `BULLET_SPEED_PER_TICK` eliminates the `-Wdouble-promotion`/`-Wfloat-conversion`
  pairs those triggered.
- Validation: `make mingw` — 0 errors, 121 warnings. All 12 local targets pass.
- Rollback: `git revert` (two commits: the movement/swept-collision change, then the
  clamp-direction fix).

### Phase 81 — Projectile correctness validation test
- Files: new `docs/verification/projectile_correctness_test.c`, `Makefile` (new
  `mingw-projectiletest` target), `.github/workflows/mingw-validation.yml` (step + log-upload
  path, both added in this same commit).
- What was done: (1) pool spawn/despawn — `addBullet` claims the first inactive slot with `prevX`
  initialized to the spawn position, `removeBullet` deactivates without disturbing other slots;
  (2) move-once-per-step — one `move_arcade_bullets` call moves a bullet by exactly
  `BULLET_SPEED_PER_TICK` in the spawned direction, a second call moves it the same fixed distance
  again; (3) swept collision — a bullet whose `prevX → x` span crosses a target's rect registers
  a hit even though its end-of-tick position alone (already past the target) would miss under the
  old point-only test; (4) an end-to-end `process()` regression check confirms a bullet crossing
  an enemy via the real move+collision pipeline still kills it and increments `kills_score`.
- Validation: `make mingw` — 0 errors, 121 warnings. All 13 local targets pass.
- Rollback: `git revert`.

**Pass 14 deferred items** (full reasoning in `docs/projectile-correctness-map.md` section 7):
enemy/boss AI, movement, and collision-response thresholds (unchanged); the shooting-input
mechanism (`shotCount`/`SDL_SCANCODE_SPACE`/`KP_0`, still real-frame-rate — this phase fixes the
projectiles, not what triggers them); converting bullet speed to real px/s units; extracting
bullet/projectile code into its own module (`docs/solid-gof-audit.md` already names and defers
this SRP item); the render-time direction-flip quirk (a bullet's sprite flips using the player's
*current* facing, not its facing at spawn); target-side swept collision (scoped to the bullet's
own fast movement only). Ready to start once a future phase is scoped specifically for one of
these.

## Phases executed in Pass 15 (game feel)

Full evidence in `docs/game-feel-map.md`, written before any Pass 15 code edit. Baseline: commit
`308b83b` (tag `refactor-pre-game-feel`, `main` after Phase 14's PR #10 merge), `make mingw` 0
errors/121 warnings, all 13 existing local targets passing. Implements the physics assessment's
own "Phase 5 — improve game feel: coyote time, jump buffering, variable jump height."

### Phase 82 — Game feel map
- Files: new `docs/game-feel-map.md`.
- What was done: confirmed no coyote time or jump buffering exist today (jump gated strictly on
  `onLedge`, an airborne request dropped silently — directly asserted by the existing
  `input_state_test.c`, an invariant this pass deliberately supersedes). Confirmed the existing
  "variable jump height" mechanism is a continuous, ungated, uncapped hold-thrust — not the
  standard impulse-plus-release-cut technique — with a real quirk (holding the key while grounded
  adds thrust regardless of `onLedge`). Found no held-to-released transition detection exists
  anywhere to build a release-cut on top of. Documents the user-approved decision (a genuine
  game-feel fork, not resolvable from code alone) to replace the hold-thrust entirely rather than
  layer a release-cut on top of it.
- Rollback: delete the file.

### Phase 83 — Add coyote time and jump buffering
- Files: `inc/header.h`, `src/process.c`, `src/processEvents.c`, `src/loadGame.c`,
  `docs/verification/input_state_test.c`.
- What was done: `InputState`'s `bool jumpRequestedPlayer1/2` retype to
  `int jumpBufferTicksPlayer1/2` (nonzero = pending, within its buffer window); `Man` gains
  `coyoteTicksRemaining`/`jumpKeyHeldLastTick`, both explicitly reset in
  `arcade_session_reset`/`runner_session_reset`. `consume_arcade_jump_requests`/
  `consume_runner_jump_requests` refresh/decay each player's coyote counter every tick, gate
  jump-firing on `onLedge || coyoteTicksRemaining > 0`, and decrement (rather than clear) a
  buffered request that can't fire yet. Deliberately supersedes `input_state_test.c`'s prior
  "airborne request dropped, not buffered" assertions — rewritten to assert the new, intended
  buffering/coyote behavior instead, documented plainly, not silently changed.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 13 local targets pass.
- Rollback: `git revert`.

### Phase 84 — Replace jump-hold thrust with variable jump height
- Files: `inc/header.h`, `src/process.c`.
- What was done: removed `ARCADE_JUMP_HOLD_ACCEL_PER_SEC2`/`RUNNER_JUMP_HOLD_ACCEL_PER_SEC2`
  (confirmed no other reference sites) and the continuous per-tick thrust they drove, from all 4
  `apply_arcade_player_forces`/`apply_runner_player_forces` W/UP-held blocks. Replaced with the
  standard impulse-plus-release-cut technique: releasing the jump key while still rising fast
  (`dy < -JUMP_CUT_SPEED_PER_SEC`) caps the ascent via the new `jumpKeyHeldLastTick` field; holding
  through to apex lets gravity decay `dy` naturally with no cut ever applying. Directly removes the
  grounded-hold-thrust quirk as a side effect — no unconditional per-tick `dy` modification
  survives, only a one-time cut check on the release edge. Existing cosmetic side effects
  (animation/`slowingDown` while held) are unchanged.
- Warning count: 121 → 121 (verified unchanged, not assumed — diffed old vs. new `process.c`
  warnings line-by-line; every warning is the same type at a shifted line number).
- Validation: `make mingw` — 0 errors, 121 warnings. All 13 local targets pass.
- Rollback: `git revert`.

### Phase 85 — Game feel validation test
- Files: new `docs/verification/game_feel_test.c`, `Makefile` (new `mingw-gamefeeltest` target),
  `.github/workflows/mingw-validation.yml` (step + log-upload path, both added in this same
  commit); `src/process.c` fix; `docs/game-feel-map.md` update.
- What was done: (1) coyote time decaying naturally over `COYOTE_TICKS` real consume calls; (2)
  jump buffering on the Runner side; (3) variable jump height (release-cut, no-cut-past-threshold,
  no-cut-without-a-release-edge); (4) confirms the grounded-hold-thrust quirk is gone; (5) the
  player-2 path, both modes. Caught a real off-by-one while writing this test, not by it:
  `consume_arcade_jump_requests`/`consume_runner_jump_requests` ran the coyote refresh/decay
  *before* the jump-fire check, so a request arriving on the coyote window's last valid tick
  (`coyoteTicksRemaining == 1`) was incorrectly denied — the decrement consumed that last tick
  before the check ever saw it. Fixed by moving the jump-fire check before the refresh/decay in
  both functions, in the same commit as the test that caught it.
- Validation: `make mingw` — 0 errors, 121 warnings. All 14 local targets pass.
- Rollback: `git revert`.

**Pass 15 deferred items** (full reasoning in `docs/game-feel-map.md` section 6): enemy/boss
jump-like behavior (unrelated to player jump feel); Runner's fall-death trigger, ledge-collision
logic, ceiling-bug fix (Phase 13's completed territory); tuning
`COYOTE_TICKS`/`JUMP_BUFFER_TICKS`/`JUMP_CUT_SPEED_PER_SEC` through actual playtesting (not
possible without a display in this environment — documented as reasonable starting defaults);
double-jump, air-dash, wall-jump, or any feel mechanic beyond the assessment's named three. Ready
to start once a future phase is scoped specifically for one of these.

## Phases executed in Pass 16 (optimization)

Full evidence in `docs/optimization-map.md`, written before any Pass 16 code edit. Baseline:
commit `7ed5259` (tag `refactor-pre-optimization`, `main` after Phase 15's PR #11 merge), `make
mingw` 0 errors/121 warnings, all 14 existing local targets passing. Implements the physics
assessment's own last phase, "optimization" — unlike Phases 1-5, this one has no demonstrated bug;
whether any loop is actually slow depends on real gameplay load this environment has no display to
measure, so this pass adds a measurement tool and a static complexity audit rather than
speculative loop changes.

### Phase 86 — Optimization map
- Files: new `docs/optimization-map.md`.
- What was done: confirmed no profiling/timing infrastructure exists anywhere beyond the raw
  `SDL_GetPerformanceCounter` calls the Phase 11 accumulator already uses (and discards). Computed
  the real, static iteration counts for every hot loop from the actual array-size constants: the
  bullet-vs-target collision loops in `process()` are the largest by an order of magnitude
  (113,000 checks/tick single-player, 226,000 multiplayer), followed by the ledge/self-collision
  loops in `collisionDetect()` (~11,600/tick), with the render loops walking the full 1000-slot
  bullet array and 100-slot ledge array every real frame. Documents explicitly why no loop is
  restructured based on this alone — no real evidence any of them is an actual bottleneck, and
  guessing would repeat the premature-optimization mistake the assessment's own framing warns
  against.
- Rollback: delete the file.

### Phase 87 — Add opt-in performance logging
- Files: `inc/header.h`, `src/main.c`.
- What was done: `main()` gains timing instrumentation around the fixed-tick physics loop and
  `doRender()`/`doRender2()` in both gameplay scenes, active only when the `ENDGAME_PERF_LOG`
  environment variable is set. Once per real second, prints physics ticks, average physics-tick
  time, and average render time (milliseconds). Every measurement is purely additive — no existing
  line changes; with logging disabled (the default), the only added cost is one `getenv` call at
  startup and unentered branches.
- Behavior impact: none when disabled (confirmed: 0 new warnings attributed to `main.c`, full
  existing suite passes unchanged). When enabled, prints real timing data to the console.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 14 local targets pass. No new
  `docs/verification/*.c` test — `main()` is structurally excluded from every existing test binary
  (`MINGW_SRCS_NO_MAIN`), and the change has no observable gameplay behavior to assert against when
  disabled; verified instead by the unchanged existing suite plus manual code inspection (a real
  run with `ENDGAME_PERF_LOG=1` set would require a display not available here — stated plainly).
- Rollback: `git revert`.

**Pass 16 deferred items** (full reasoning in `docs/optimization-map.md` section 4): restructuring
any of the hot loops found (active-bullet index lists, spatial partitioning, render culling) — no
real evidence any of them cost a noticeable fraction of a frame budget; gathering real profiling
data with `ENDGAME_PERF_LOG=1` on a machine with a display is the natural next step before any of
that is considered. Ready to start once real numbers exist.

## Phases executed in Pass 17 (input snapshot isolation)

Full evidence in `docs/input-snapshot-architecture-map.md`, written before any Pass 17 code edit.
Baseline: commit `b94e363` (tag `refactor-pre-input-snapshot`, `main` after Phase 16's PR #12
merge), `make mingw` 0 errors/121 warnings, all 14 existing local targets passing. Implements
Phase 17 of the user's proposed target frame architecture (`poll events → build input snapshot →
fixed-step loop → render`) — the first of four phases split out from that proposal (17 input
snapshot isolation, 18 AI/forces separation, 19 collision ordering, 20 render interpolation).

### Phase 88 — Input snapshot architecture map
- Files: new `docs/input-snapshot-architecture-map.md`.
- What was done: confirmed `apply_arcade_player_forces`/`apply_runner_player_forces`
  (`src/process.c`) each call `SDL_GetKeyboardState()` directly, inside the fixed-step path — a
  real frame producing multiple physics ticks re-samples live keyboard state a non-deterministic
  number of times. Confirmed jump was already correctly isolated via Phase 12/15's edge-triggered
  `InputState` buffer fields, needing no change. Documents five simplifications adopted over the
  originally proposed capture API (no new function parameters, no `input_begin_frame()`, no event
  wrapper functions, no release/request edge fields, left-wins precedence preserved rather than
  redefined), each backed by the audit evidence for why the simpler shape already satisfies every
  stated invariant.
- Rollback: delete the file.

### Phase 89 — Input snapshot capture helpers
- Files: `inc/header.h`, new `inc/input_snapshot.h`, new `src/input_snapshot.c`, new
  `docs/verification/header_only_input_snapshot.c`, `Makefile` (`mingw-headertest` target gains
  the new header's self-containment check).
- What was done: expanded `InputState` with 6 continuous held-key fields (movement/jump-held/
  shoot-held for both players), alongside the existing edge-triggered jump-buffer ints. New
  `input_capture_arcade()`/`input_capture_runner()` are pure functions reading a caller-supplied
  `SDL_GetKeyboardState()` array into `InputState` — not yet wired into gameplay in this commit.
- Behavior impact: none (nothing new is called yet; all 14 existing targets pass unchanged).
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged, no warnings from the new file).
  All 14 local targets pass. `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert`.

### Phase 90 — Route player input through the snapshot
- Files: `src/main.c`, `src/process.c`, `src/processEvents.c`, `src/loadGame.c`,
  `docs/verification/game_feel_test.c`.
- What was done: `main()` calls `SDL_GetKeyboardState()` exactly once per real frame, before the
  fixed-step accumulator loop, and calls `input_capture_arcade()`/`input_capture_runner()` to
  populate `gameState->input`. `apply_arcade_player_forces`/`apply_runner_player_forces` and
  `processEvents()`'s shoot check read `game->input.*` instead of calling
  `SDL_GetKeyboardState()` themselves — every physics tick and event-poll a given real frame
  produces now sees identical held-key state. `arcade_session_reset()`/`runner_session_reset()`
  explicitly zero the 6 new fields alongside the existing jump-buffer resets. Updated
  `game_feel_test.c`'s header comment (no assertion/call-site change needed): its
  `apply_*_player_forces()` calls now see a deterministic "no key held" state via session-reset's
  explicit zeroing, not a headless-SDL side effect.
- Behavior impact: preserving — same branch structure, same left-wins precedence, same
  variable-jump-height release-cut logic, same shoot cooldown, just reading a frame-frozen
  snapshot instead of live SDL state.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 14 local targets pass,
  including the updated `game_feel_test.c`. `py -3 scripts/audit_repository_usage.py` —
  `Result: PASS`.
- Rollback: `git revert`.

### Phase 91 — Input snapshot validation test
- Files: new `docs/verification/input_snapshot_test.c`, `Makefile` (new
  `mingw-inputsnapshottest` target), `.github/workflows/mingw-validation.yml` (step + log-upload
  path added in the same commit).
- What was done: verifies (1) `input_capture_arcade`/`input_capture_runner` set exactly the right
  fields from a fabricated key array, including that `input_capture_runner` leaves the shoot
  fields untouched; (2) a simulated multi-tick real frame applies held movement on every tick but
  the release-cut only once, since capture happens once per frame, not once per tick; (3) the
  pre-existing left-wins precedence on simultaneous left+right holds, now explicit and tested;
  (4) player 1/2 fields don't cross-mutate; (5) session reset clears every new field, both modes.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 15 local targets pass (20/20
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert`.

**Pass 17 deferred items** (full reasoning in `docs/input-snapshot-architecture-map.md` section
5): AI/enemy input or physics (Phase 18); collision-ordering changes (Phase 19); render
interpolation (Phase 20); shoot cooldown/rate redesign, projectile mechanics, key rebinding, or
controller support; pause/quit/scene-selection handling (confirmed structurally separate already,
stays outside the gameplay input snapshot). Ready to start Phase 18 once this PR merges.

## Phases executed in Pass 18 (AI/forces separation)

Full evidence in `docs/ai-forces-separation-map.md`, written before any Pass 18 code edit.
Baseline: commit `8258ba0` (tag `refactor-pre-ai-forces-separation`, `main` after Phase 17's
PR #13 merge), `make mingw` 0 errors/121 warnings, all 15 existing local targets passing.
Implements the second of four phases split out from the user's proposed target frame
architecture: 17 input snapshot isolation (done), **18 (this one) AI/forces separation**, 19
collision ordering, 20 render interpolation — framed by the user as a structural extraction only,
no change to enemy speeds, jump behavior, targeting rules, collision ordering, hitboxes, spawn
logic, pathfinding, or render interpolation.

### Phase 92 — AI/forces separation map
- Files: new `docs/ai-forces-separation-map.md`.
- What was done: confirmed boss/regular-enemy/smart-enemy movement in `process()` integrates
  position using the velocity decided on a previous tick, then adjusts velocity for the next tick
  — decide and integrate are interleaved by design, not accidentally fused. This ruled out a
  literal 4-function decide/apply/integrate split (would change effective speed/timing by one
  tick, or require a new "pending" state field — both real behavior changes). Confirmed
  `process2()` (Runner) has zero AI-driven entities. Documents the safe alternative: extract each
  entity type's movement into its own named function (mirroring Phase 11's
  `apply_arcade_player_forces` extraction), plus isolate the one piece that *is* safely
  extractable as a true "decide intent" step — the multiplayer smart-enemy nearest-target
  comparison.
- Rollback: delete the file.

### Phase 93 — Extract boss/enemy/smart-enemy movement into src/ai_forces.c
- Files: new `inc/ai_forces.h`, new `src/ai_forces.c`, `src/process.c`, new
  `docs/verification/header_only_ai_forces.c`, `Makefile` (`mingw-headertest` target).
- What was done: moved boss/regular-enemy/smart-enemy movement (previously
  `process.c:636-923`) verbatim into `move_boss_entities()`, `move_regular_enemies()`,
  `move_smart_enemies()` — same statement order and arithmetic, no behavior change. `process()`
  now calls the three functions at the exact same point. Added `smart_enemy_select_target()`,
  replicating the exact `pow()`-based nearest-target comparison the multiplayer smart-enemy chase
  used to do inline — now reusable and independently testable. The confirmed pre-existing
  multiplayer chase asymmetry (its inner `while` loop omits an `x += dx` line the single-player
  version has) is preserved exactly, not fixed, per this phase's narrow scope.
- Behavior impact: none — a structural extraction only.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged; confirmed the ~26 warnings that
  moved from `process.c` to `ai_forces.c` are the same lines relocated, not new ones). All 15
  local targets pass, including `projectile_correctness_test.c` (calls `process()` directly with
  enemy data, asserting bullet-collision behavior that runs *before* the extracted movement
  blocks) unchanged. `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert`.

### Phase 94 — AI/forces separation validation test
- Files: new `docs/verification/ai_forces_test.c`, `Makefile` (new `mingw-aiforcestest` target),
  `.github/workflows/mingw-validation.yml` (step + log-upload path added in the same commit).
- What was done: the first-ever test assertions in this repository on the actual AI movement math
  (previously untested) — boss/regular-enemy band-steering (both directions), gravity clamps, and
  horizontal wraparound; smart-enemy single-player hop-impulse-and-steer chase;
  `smart_enemy_select_target()` picking the nearer of two players in multiplayer and always `man`
  in single-player; and that `process()` still delegates to the extracted functions correctly.
  Assertions use the exact same float literals/expressions the production code uses, so
  comparisons are bit-exact, not tolerance-based.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 16 local targets pass (15/15
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert`.

**Pass 18 deferred items** (full reasoning in `docs/ai-forces-separation-map.md` section 4):
merging the multiplayer smart-enemy chase's two duplicated (and asymmetric) bodies into one — a
real, evidence-backed simplification opportunity, deliberately not done since it would change
which exact lines run for the multiplayer path; any change to `collisionDetect()`/
`collisionDetect2()` (Phase 19's collision-ordering scope); converting boss/enemy/smart-enemy
movement to fixed-timestep per-second units (already a deliberately deferred, separate item since
Phase 11). Ready to start Phase 19 once this PR merges.

## Phases executed in Pass 19 (collision ordering)

Full evidence in `docs/collision-ordering-map.md`, written before any Pass 19 code edit. Baseline:
commit `1ea9a06` (tag `refactor-pre-collision-ordering`, `main` after Phase 18's PR #14 merge),
`make mingw` 0 errors/121 warnings, all 16 existing local targets passing. Implements the third of
four phases split out from the user's proposed target frame architecture: 17 input snapshot
isolation (done), 18 AI/forces separation (done), **19 (this one) collision ordering**, 20 render
interpolation — target sequence "world-solid resolution → body-contact hazards → projectile hits
→ deaths and score consequences → scene transitions."

### Phase 95 — Collision ordering map
- Files: new `docs/collision-ordering-map.md`.
- What was done: full audit of every collision/hazard/consequence/transition site in both Arcade
  and Runner modes, comparing actual execution order against the target's 5-stage sequence. Found
  three "reversed" orderings: (1) Arcade's projectile hits (`process()`) run before world-solid
  resolution (`collisionDetect()`) within a tick — confirmed this must stay this way, since
  world-solid resolution has to run *after* movement or the player visibly tunnels through
  geometry for a tick; (2) Runner's `collisionDetect2()` checks hazard contact before ledge
  resolution — no demonstrated defect either way, flagged and left as-is (mirrors Phase 18's
  handling of the smart-enemy chase asymmetry); (3) both modes' hazard/transition logic runs at a
  different cadence (once per real frame) than world-solid/projectile-hits (once per fixed tick) —
  confirmed Runner's `runner_update_death()` render-then-resolve ordering is intentional,
  documented, and tested, and unifying the cadence split would be a real timing change, not a pure
  extraction. Documents the safe alternative instead: extract each concern into a named function
  matching the target's own vocabulary, without reordering any call site.
- Rollback: delete the file.

### Phase 96 — Extract collision/hazard/transition resolution into src/collision_pipeline.c
- Files: new `inc/collision_pipeline.h`, new `src/collision_pipeline.c`, `src/process.c`,
  `src/processEvents.c`, `src/collisionDetect.c`, new
  `docs/verification/header_only_collision_pipeline.c`, `Makefile` (`mingw-headertest` target).
- What was done: moved six collision/hazard/transition concerns verbatim into a new focused
  module — `resolve_projectile_hits()` (was `process.c:352-524`), `resolve_arcade_hazards()` (was
  `processEvents.c:294-392`), `resolve_arcade_game_over_transition()` (was
  `processEvents.c:394-416`), `resolve_runner_hazard_contact()` (was `collisionDetect.c:656-666`,
  still called before ledge resolution — order preserved, not fixed), `check_runner_fall_hazard()`
  (was `processEvents.c:569-580`), `resolve_runner_game_over_transition()` (was
  `processEvents.c:583-599`). No call site moved, no arithmetic changed.
- Behavior impact: none — a structural extraction only.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 16 local targets pass,
  including `frame_pipeline_test.c` (double-transition guards, render purity) and
  `runner_death_test.c` (single-shot death lifecycle end-to-end) unchanged.
  `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert`.

### Phase 97 — Collision ordering validation test
- Files: new `docs/verification/collision_pipeline_test.c`, `Makefile` (new
  `mingw-collisionorderingtest` target), `.github/workflows/mingw-validation.yml` (step +
  log-upload path added in the same commit).
- What was done: the first-ever direct-call test assertions on the Arcade hazard/game-over-
  transition logic and the Runner fall-hazard/game-over-transition logic — previously only
  reachable via a full `processEvents()`/`processEvents2()` call with a real event loop. Covers
  body-contact hazards (enemy/boss/smartEnemy, both players), reached-bottom kill/score/game-over
  interaction, fall-off-screen, the Arcade/Runner double-transition guards, Runner star-contact
  triggering the death lifecycle, and a direct-call sanity check on `resolve_projectile_hits()`.
- Validation: `make mingw` — 0 errors, 121 warnings (unchanged). All 17 local targets pass (20/20
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert`.

**Pass 19 deferred items** (full reasoning in `docs/collision-ordering-map.md` section 6):
reordering `resolve_runner_hazard_contact()` relative to ledge resolution inside
`collisionDetect2()` — a real behavior change with no demonstrated defect; unifying the per-real-
frame hazard cadence with the per-tick world-solid/projectile-hit cadence — a genuine
architectural option, deferred pending an actual observed defect; render interpolation (Phase 20).
Ready to start Phase 20 once this PR merges.

## Deferred phases (reasoning, and what "ready to start" looks like)

**Pass 9 deferred items** (full reasoning in `docs/solid-gof-audit.md` section 9 and
`docs/design-patterns.md`): `GameState` decomposition into nested structs (real candidate
clusters exist, but combining with the Command/Factory work risked exceeding Pass 9's
abstraction ceiling); unifying the 9 scattered lazy-SFX-reload sites into the
`_assets_load`/`_assets_unload` ownership model; converting `menu_events()`/`pause_events()` to
`GameCommand` (neither duplicates its own keymap the way `processEvents`/`processEvents2` do);
further header decomposition beyond `scene.h`/`frame.h`/`entity_spawn.h`/`input_command.h`. Ready
to start once a future phase is scoped specifically for one of these, each independently, not
combined.

**Pass 10 deferred items** (full reasoning in `docs/gamestate-decomposition.md` section 3):
Arcade/Runner session-data grouping (entities, score, timers — real candidate clusters exist but
span many files with both read and write access from simulation/collision/event-handling code,
much larger and higher-risk than either Pass 10 group); audio-resource grouping (`menuMus`/
`jumpSound`/etc. — lifecycle still not centralized, same reason Pass 9 deferred it); leaderboard/
HUD/menu-state grouping (smaller field count but more scattered ownership, needs its own cleanup
first); the deprecated `menu_status`/`menu0_status` fields (flagged safe-to-remove since Phase 7,
still not removed — out of every phase's charter so far). Ready to start once a future phase picks
one of these specifically.

**Phase 5 — `doRender2` state mutation. Done in Pass 5** (see below) — the original plan assumed
merging the mutation into `process2()`'s existing (separately broken) `deathCountdown` mechanism;
Pass 5's actual fix relocates it to its own small function instead, deliberately not merging with
that unrelated, still-unfixed mechanism (see `docs/frame-pipeline-map.md`).

**Phase 6 — `collisionDetect.c` self-collision guard.** Re-investigated in Pass 5 with a precise
trace (see `docs/frame-pipeline-map.md`): when the enemy-to-enemy loop's two indices are equal,
every directional AABB check uses a strict inequality that evaluates false for identical position/
size values, so no mutation ever occurs for the self-case — confirmed safe-by-construction, not a
demonstrated defect. Left unchanged per the "only fix demonstrated defects" rule; still flagged as
fragile if the struct or comparisons ever change.

**Phase 7 — `IMG_Load` ordering.** Confirmed functionally inert (SDL's
`SDL_CreateTextureFromSurface(renderer, NULL)`/`SDL_FreeSurface(NULL)` are documented-safe
no-ops). Wide (~10 sites across two files), zero live behavioral payoff — batch it in a
dedicated, mechanical future pass now that a compiler is available (Pass 2) to confirm no
accidental behavior change. Note added in Pass 2: this is also the reason Phase 16's asset-group
lifecycle flags can't fully guarantee "a partial load failure never marks the group as loaded" —
fixing this phase would close that residual gap too.

**Phase 8 — Shutdown-path reconstruction.** Done in Pass 2 (see above) — a compiler became
available this pass, which is exactly the precondition this entry originally said it was
waiting for.

**Phase 9 — `mx_*` dead-code removal.** Confirmed zero live callers (beyond the one-line
arithmetic fix already applied to `mx_strsplit.c` in Phase 1, which remains unreachable).
Priority-7 territory per the brief ("only after behavior and ownership are stable") — deletion
of confirmed-dead code is safe whenever it happens, so there's no urgency; deferred purely to
keep Pass 1's diff scoped to behavior/leak fixes.

**Phase 10 — Large paired-function deduplication.** Confirmed real shared sub-patterns (ledge
AABB resolution, animation-frame advance) but materially different mechanics between Arcade
(enemies/bosses/waves) and Runner (infinite-scroll/stars/cheat code) modes. Needs a stable,
build-verified baseline before any extraction, to avoid subtly merging behavior that looks
identical but isn't.

**Phase 11 — Delta-time conversion. Done in Pass 11** (see above) — a fixed 60Hz accumulator now
drives `main()`'s two gameplay scenes, and player gravity/velocity/acceleration/friction/jump
constants are converted to explicit per-second units (`docs/physics-timestep-map.md`). Enemy/boss/
bullet/decor/trap timing remains frame-count-based, deliberately deferred to a future phase scoped
for entity/projectile work specifically.

**Phase 12 — Build/README modernization.** Any real Makefile fix (porting off macOS
`.framework` flags and `clang`-specific paths, fixing the `clean:`/`$(OBJS)` no-op, deciding
whether to keep `-Werror`) cannot be verified at all without a compiler — shipping an "improved"
Makefile in an environment that can't build it would be an unverified guess. README's documented
controls were checked against live code this pass and found accurate; revisit only if a future
phase introduces a real behavior/documentation mismatch.

## Environment/build limitation (updated after Pass 2)

Pass 1 had no git repository and no C compiler available. **Pass 2 resolved both**: git is now
initialized with a baseline commit (`cfb7ca7`), and a real Windows/MinGW build path exists
(`make mingw`, `make mingw-smoketest`) — see `docs/refactor-audit.md` Section 6 for the exact
toolchain, dependency source, and build command. The one remaining, confirmed-unavailable piece
is sanitizer instrumentation (ASan/UBSan) — attempted with two MinGW-w64 distributions, both
lack the required runtime libraries; this is a genuine toolchain gap, not a skipped step. The
project's macOS Makefile target and `README.md`'s documented controls remain untouched and
unverified in this environment (no macOS/clang toolchain available here either).

## Rollback approach

Git is now available (Pass 2): `git log` shows every commit; use `git diff`/`git show` to inspect
any change and standard `git revert`/`git checkout` to undo it (destructive `git reset --hard`
should only be used with explicit user confirmation, per this project's own safety conventions).
`docs/refactor-log.md` remains the authoritative narrative record (why each change was made, what
was checked) even though git now also provides mechanical diffs — the two are complementary, not
redundant: the log explains intent and verification level; git provides exact byte-for-byte diffs
and rollback commands.
