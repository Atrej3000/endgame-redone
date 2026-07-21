# Optimization Map — Ucode_Endgame

Written **before** any Phase 16 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `7ed5259` (tag `refactor-pre-optimization`,
`main` after Phase 15's PR #11 merge) via a complete read of every per-tick and per-frame loop in
`src/process.c`, `src/collisionDetect.c`, `src/doRender.c`, and `src/main.c`, immediately before
this document was written.

## 1. Why this phase does not change any hot-loop code

The assessment's own roadmap names this last phase simply "optimization" (`docs/refactor-plan.md`
already characterizes the full six-phase roadmap this way, e.g. `:860`: "Phases 2-6 (input/
simulation split, collision correctness, projectile correctness, game feel, **optimization**) are
explicitly out of scope and deferred"). Every prior phase in this session (1 through 5) had a
concretely demonstrated bug or gap to fix, found by direct code inspection. Optimization is
different in kind: whether a piece of code is "worth" optimizing depends on whether it's actually
slow in practice, under real gameplay load, on real hardware — a question static reading of the
source cannot answer, and this environment has no display to run the game and measure it directly.

Confirmed by direct inspection: **no profiling or timing infrastructure exists anywhere in this
codebase today** beyond the raw `SDL_GetPerformanceCounter`/`SDL_GetPerformanceFrequency` calls the
Phase 11 fixed-timestep accumulator already uses (`src/main.c:36-42`) — and those are used only to
decide how many physics ticks to run this frame; the measured `frameTime` is never logged,
summarized, or exposed anywhere. Grepping the whole `docs/verification/` suite for any
timing-related symbol returns zero matches — no existing test measures wall-clock performance
either.

**This phase therefore does two things, neither of which requires guessing at what's slow**: adds
an opt-in tool that lets whoever *can* run the game with a display gather real numbers (§3), and
documents the actual static complexity of every hot loop found (§2) so that when real profiling
data eventually does point at something, this map already explains which code owns that cost.
**No loop is restructured, reordered, or "sped up" in this phase** — doing so without evidence
would repeat exactly the mistake the assessment's own phrasing warns against.

## 2. Static algorithmic-complexity inventory

All figures below are per fixed 60Hz physics tick unless noted "per render frame" (render is paced
by the display, not the physics tick, since Phase 11). Confirmed constants (`inc/header.h`):
`MAX_BULLETS` = 1000 (`:48`), `NUM_ENEMIES` = 101 (`:51`), `NUM_SMART_ENEMIES` = 10 (`:52`);
`Enemies boss[2]` (`:330`) and `Ledge ledges[100]` (`:352`) are literal-bounded, no named constant.

**Bullet-vs-target collision loops, `process()` (`src/process.c`)** — confirmed unchanged in shape
since Phase 14/15 (the file's own comment at `:75` still cites the pre-fix "113 loop-executions"
figure this table derives independently):

| Loop | Lines | Nesting | Checks/tick |
|---|---|---|---|
| `bullets` vs `enemyValues` | `:355-379` | `101 × 1000` | 101,000 |
| `bullets` vs `smartEnemies` | `:381-410` | `10 × 1000` | 10,000 |
| `bullets` vs `boss` | `:412-437` | `2 × 1000` | 2,000 |
| `secondBullets` vs `enemyValues` (multiplayer only, `:441`) | `:443-464` | `101 × 1000` | 101,000 |
| `secondBullets` vs `smartEnemies` (multiplayer only) | `:466-491` | `10 × 1000` | 10,000 |
| `secondBullets` vs `boss` (multiplayer only) | `:492-517` | `2 × 1000` | 2,000 |

Single-player total: **113,000 checks/tick** (6.78M/sec at 60Hz). Multiplayer total: **226,000
checks/tick** (13.56M/sec). `move_arcade_bullets()` (`src/process.c:89-121`, added Phase 14) is a
separate, much smaller O(`MAX_BULLETS`) = 1,000 pass (2,000 with `secondBullets`).

**Ledge/self collision loops, `collisionDetect()` (`src/collisionDetect.c`)**:

| Loop | Lines | Nesting | Checks/tick |
|---|---|---|---|
| `enemyValues` vs `ledges` | `:89-149` | `101 × 100` | 10,100 |
| `smartEnemies` vs `ledges` | `:151-211` | `10 × 100` | 1,000 |
| `man` vs `ledges` | `:280-340` | `100` | 100 |
| `boss` vs `ledges` | `:343-402` | `2 × 100` | 200 |
| `secondPlayer` vs `ledges` (unconditional) | `:409-465` | `100` | 100 |
| `secondPlayer` vs `ledges` (multiplayer-duplicate, `:468`) | `:529-587` | `100` | 100 (multiplayer only) |
| enemy-to-enemy self-collision | `:590-651` | `10 × 10` | 100 |

Single-player total: ~11,600/tick; multiplayer: ~11,700/tick — roughly a tenth the cost of the
bullet loops. The `NUM_SMART_ENEMIES²` self-collision loop is unchanged and still the one
`docs/refactor-plan.md` already documents as "confirmed safe-by-construction, not a demonstrated
defect" (a self-index never mutates, by construction of the strict-inequality AABB checks) — left
alone here for the same reason it was left alone before: no demonstrated defect, and now also no
demonstrated performance cost.

**Render loops, `doRender()`/`doRender2()` (`src/doRender.c`)** — both the Arcade and Runner
renderers live in this one file (`doRender2` is the Runner equivalent, `:197-256`). These run once
per **real** frame, not once per fixed tick:
- Ledges: `doRender()` walks all 100 (`:64-79`, three sub-ranges, each an unconditional
  `SDL_RenderCopy` — no visibility/active guard). `doRender2()`'s background-ledge loop
  (`:220-224`) is likewise a full, unconditional 100-iteration walk.
- Bullets: `doRender()` walks the full `MAX_BULLETS`=1000-slot array every frame (`:111-117`),
  branching on `.active` before the actual `SDL_RenderCopyEx` call; `secondBullets` gets its own
  full 1000-slot walk when `multiPlayer` (`:119-130`).
- `smartEnemies`/`enemyValues`/`boss` (`:83-108`): 10/101/2 iterations, each branching on
  `.visible`.
- Runner's stars loop (`:235-239`): full 100, unconditional `SDL_RenderCopy`, runs even outside the
  `STATUS_STATE_GAME` gate.

No spatial partitioning, active-index list, or other compaction exists anywhere in the collision or
render code — every loop above is a full walk of its backing array, gated only by a per-element
branch.

## 3. The profiling tool this phase adds, and how to use it

`src/main.c` gains opt-in timing instrumentation, off unless the `ENDGAME_PERF_LOG` environment
variable is set to any value:
```
ENDGAME_PERF_LOG=1 ./build-mingw/endgame-mingw.exe
```
Once per real second, a line prints to the console:
```
[perf] ticks=60 physics_ms=0.412 render_ms=1.203
```
(`ticks` = physics steps that ran this second; `physics_ms`/`render_ms` = average time per physics
tick / per render call, in milliseconds). This is the exact, real number the static table in §2
cannot provide — running the game with this enabled, in both single-player and multiplayer,
Arcade and Runner, with a realistic number of enemies/bullets on screen, is what turns "113,000
checks/tick, in theory" into "X milliseconds, in practice, on real hardware." **This environment
has no display to do that run** — the instrumentation is added so whoever does have one can.

When `ENDGAME_PERF_LOG` is unset (the default, unchanged for every existing player), the only added
cost is one `getenv` call at startup and a handful of `if (perfLoggingEnabled)`-gated branches
whose bodies never execute — no measurable behavior change, confirmed by the full existing test
suite still passing unchanged (§5).

## 4. Deliberately not done (found, documented, not touched)

- Restructuring any of the loops in §2 (active-bullet index lists to skip inactive `MAX_BULLETS`
  slots, spatial partitioning for ledge/collision checks, render culling) — no real evidence any
  of them cost a noticeable fraction of a frame budget; changing them without measurement would be
  exactly the premature optimization the assessment's own Phase 6 framing warns against.
- A new `docs/verification/*.c` test — `main()` is structurally excluded from every existing test
  binary (`MINGW_SRCS_NO_MAIN` excludes it, since only one `main()` symbol can exist per binary),
  and the instrumentation itself has no observable gameplay behavior to assert against when
  disabled (the default). Verified instead by: the full existing suite passing unchanged (confirms
  the `main.c` change doesn't break the build) and manual inspection of the new code for
  correctness (a real run with `ENDGAME_PERF_LOG=1` set, watching console output, would require a
  display not available here — stated plainly, not claimed).
- Any change to `enemy`/`boss`/`bullet` timing units, hitbox reconciliation, or other items already
  found and deferred by Phases 11-15 — none of that is re-opened here.
