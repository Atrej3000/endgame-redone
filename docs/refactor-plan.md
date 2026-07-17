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
| 4 | Raw scene-state ints → descriptive enums (`menu_status`, `menu0_status`) | Deferred |
| 5 | `doRender2` gameplay-state mutation moved into `process2()` | Deferred |
| 6 | `collisionDetect.c` enemy-vs-enemy self-collision guard | Deferred |
| 7 | `IMG_Load`/NULL-check ordering normalization (~10 sites) | Deferred |
| 8 | Full shutdown-path reconstruction (texture/font/chunk/music destruction, `Mix_CloseAudio`, `IMG_Quit`, `IMG_Init`) | Deferred |
| 9 | `mx_*` dead-code removal (16 files) | Deferred |
| 10 | Large paired-function deduplication (`process`/`process2`, etc.) | Deferred (explicitly out of scope until a build-verified baseline exists) |
| 11 | Delta-time conversion | Deferred (explicitly out of scope until a stable, comparable baseline exists) |
| 12 | Build system / README modernization | Deferred |

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

## Deferred phases (reasoning, and what "ready to start" looks like)

**Phase 4 — Scene-state enums.** `enum menu_buttons` already exists (`header.h:252-259`) but is
unused. Replacing raw ints with it touches every routing site in `main.c`, `menu_events.c`,
`pause_events.c`, `processEvents.c` — a wide, mechanical, but non-trivial-to-verify change
without being able to run the game. Ready to start once a compiler is available for smoke
testing.

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
dedicated, mechanical future pass once a compiler can confirm no accidental behavior change.

**Phase 8 — Shutdown-path reconstruction.** Requires deciding a correct destruction order for
~24 textures, the font, 5 sound chunks, 3 music tracks, and the renderer/window, none of which
can be smoke-tested here (does destroying texture X before texture Y ever matter? — untestable
without running the binary). Ready once build/run verification is available; should be scoped
as its own phase, not bundled with anything else.

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

## Environment/build limitation (restated)

No git repository, no C compiler available in this environment. The project's Makefile is
macOS-specific regardless (`clang`, bundled `.framework`s, `-rpath`) and `inc/header.h` includes
a BSD/macOS-only header. No compilation was attempted or claimed to succeed at any point in this
pass. See `docs/refactor-audit.md` Section 6 for full detail.

## Rollback approach (no git)

Every change in `docs/refactor-log.md` records the exact "before" code. To roll back any single
change, locate its log entry and paste the "before" block back over the corresponding file:lines.
Because Pass 1's changes are each independently scoped (no change depends on another compiling
or behaving a certain way), any subset can be reverted without affecting the rest.
