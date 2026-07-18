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
| 5 | `doRender2` gameplay-state mutation moved into `process2()` | Deferred |
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
| 21 | `doRender2` gameplay-state mutation (Phase 5, listed above as item 5) | Still deferred — unaffected by Pass 3, which only changed routing, not `doRender2`'s internals |

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

## Deferred phases (reasoning, and what "ready to start" looks like)

**Phase 5 — `doRender2` state mutation.** Moving `game->gameLives--`/`isDead`/`y` reset from
`doRender.c:254-256` into `process2()` (to mirror how `process()` already does death-handling
correctly via `deathCountdown`) changes exactly when in the frame lifecycle a life is deducted.
Needs a running build to rule out a double-decrement or missed-decrement regression. Ready once
build/run verification is available.

**Phase 6 — `collisionDetect.c` self-collision guard.** Zero live payoff today (harmless by
luck of strict-inequality AABB checks); any fix still touches enemy-vs-enemy collision math.
Ready once a compiler/runtime is available to screen for subtle AABB-resolution-order changes.

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
