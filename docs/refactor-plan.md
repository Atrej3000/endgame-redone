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

**Phase 11 — Delta-time conversion.** Explicitly required by the brief to wait until a stable,
comparable baseline exists (documented current timing assumptions, stable main loop, isolated
time constants) before any conversion — not started this pass beyond documenting the current
frame-count-based timing in the audit.

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
