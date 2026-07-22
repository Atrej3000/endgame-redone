# AI/Forces Separation Map — Ucode_Endgame

> **Current-state note (Phase 21).** This is the Phase 18 extraction record. Its statements about
> frame-count-based AI describe the pre-Phase-21 baseline only: boss, regular-enemy, and
> smart-enemy movement now uses explicit per-second constants and fixed-step `dt`, together with
> bullets, scrolling, traps, and the Runner multiplayer camera.

Written **before** any Phase 18 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `8258ba0` (tag
`refactor-pre-ai-forces-separation`, `main` after Phase 17's PR #13 merge) via a complete read of
every place `enemyValues[]`/`smartEnemies[]`/`boss[]` position or velocity is mutated.

## 1. The gap this phase closes

The user's proposed target frame architecture names a four-step per-entity pipeline: select
target/decide intent → desired movement/acceleration → apply forces and limits → integrate
position. All of it currently lives fused together inside the single, large `process()` function
in `src/process.c`, mixed in with bullet-collision logic, spawn triggers, and player integration.
Direct audit of every movement site:

- **Boss movement** (`src/process.c:636-670`): a fixed, self-referential drift pattern that never
  reads player position — gravity accumulation (`dy += 0.1`, clamp 1.5), a position-band steering
  check (`300<y<350`, drift toward screen-center `x=640`), and horizontal wraparound. Integration
  (`x += dx`, `:638-639`) happens first, using last tick's velocity; the steering/gravity
  adjustment that follows (`:641-661`) computes *next* tick's velocity — decide and integrate are
  interleaved by construction, not accidentally fused.
- **Regular enemies** ("STUPID BOTS", `src/process.c:673-710`): the same shape, different band
  constants (`x` vs 590/610, `y` vs 150/170) and gravity constant (`dy += 0.4`, clamp 2.5). Also
  purely self-referential.
- **Smart enemies, single-player** (`src/process.c:712-796`): the one entity type with real
  player-tracking — reads `game->man.x`/`.y` directly. A `while` loop (`:723-744`) does a
  vertical-approach phase (steer `dx` toward `man.x` by ±1 per iteration, clamp ±4, decrement `dy`
  by 10 each iteration, until `y <= man.y` or `dx` becomes nonzero); an `if`/`else if` pair
  (`:763-790`) does horizontal steering, each branch also performing `x += dx` and forcing
  `dy = -6` (a hop impulse) when `dx == 0` at the moment the branch fires. Decide and apply are
  fully fused, including inside the stateful `while` loop.
- **Smart enemies, multiplayer** (`src/process.c:799-922`): the identical chase logic duplicated
  twice — once against `game->man` (`:808-861`), once against `game->secondPlayer`
  (`:862-917`) — gated by a nearest-target squared-distance comparison (`:805-806`, using `pow()`,
  double precision, not narrowed to `powf`). **Confirmed pre-existing asymmetry**: the
  multiplayer chase's inner `while` loop (`:813-828`) omits the `x += dx` line the single-player
  version has (`:728`, `:737`) — a latent quirk, not touched by this phase.
- **`process2()`** (Runner mode, `src/process.c:1207-1379` in this tree): confirmed **zero**
  references to `enemyValues`/`smartEnemies`/`boss[` anywhere in the function. The only moving
  non-player entity is `stars[]` (`:1275-1288`), a pure deterministic function of `time`/`phase`/
  `mode`, no player-position read, no AI decision of any kind. **No extraction needed on the
  Runner side** — there is nothing there to separate.
- **`entity_spawn.c`** (`enemy_spawn`/`smart_enemy_spawn`/`boss_spawn`): every current call site
  (`src/process.c`, `src/loadGame.c`) passes `dx=0, dy=0` — confirmed via grep, no exceptions. No
  spawn-time "intent" is persisted anywhere; movement is decided fresh every tick by the blocks
  above. Nothing to change here.
- **`Enemies` struct** (`inc/header.h:109-117`): a single `dx`/`dy` field pair does double duty as
  both "this tick's decided steering" and "the value used to integrate position" for all three of
  `boss[2]`/`enemyValues[NUM_ENEMIES]`/`smartEnemies[NUM_SMART_ENEMIES]` — no separate
  desired/applied fields exist.
- **`collisionDetect()`** (`src/collisionDetect.c`) also mutates these same three arrays'
  position/velocity as a ledge-bounce side effect (`:89-149` enemies, `:151-211` smart enemies,
  `:343-402` boss — the boss block also writes `boss[j].onLedge`, `:589-651` smart-enemy-vs-
  smart-enemy bounce). This is a *second* force/limit-application site, already structurally
  separate from `process()` (its own function/file, called after `process()` inside
  `arcade_simulate()`, `src/frame.c:4-11`). **Left completely untouched** — consolidating these
  two limit-application sites is exactly the collision-ordering question the user reserved for
  Phase 19.
- **No existing test** asserts on the actual AI movement math (boss drift, regular-enemy drift, or
  smart-enemy chase) anywhere in `docs/verification/`. `projectile_correctness_test.c` calls
  `process()` directly with enemy data, but only asserts the bullet-collision side effect (which
  runs *before* these movement blocks in `process()`'s body) — a real, confirmed coverage gap this
  phase's test closes for the first time.

## 2. Design decision: why not a literal 4-function-per-entity decomposition

The user's diagram names four sequential boxes, but the evidence above shows today's boss/
regular-enemy/smart-enemy logic *integrates position using the velocity decided on a previous
tick, then adjusts velocity for the next tick* — decide and integrate are interleaved by design.
Splitting this into "decide → apply limits → integrate" as four literally sequential steps would
require either integrating with a freshly-decided velocity the same tick (changing effective
speed/timing by one tick — a real behavior change) or introducing a new "pending" field to bridge
the two ticks (a small but real state-shape change) — neither is safe under the user's explicit
"do not change enemy speeds" constraint.

This phase instead does the safe, high-value version of "structural extraction": pulling each
entity type's per-tick movement out of the single giant `process()` into its own named,
single-purpose function — the same kind of extraction Phase 11 already did for player movement
(`apply_arcade_player_forces`/`apply_runner_player_forces`, pulled out of `process()`/`process2()`
without changing any arithmetic) — preserving statement order and math exactly.

**One piece is safely extractable as a true, standalone "decide intent" step**: the multiplayer
smart-enemy nearest-target comparison (`:805-806`) is a pure, side-effect-free decision that can
be isolated into its own function without touching the two (already-duplicated, already-
asymmetric) chase-logic bodies that follow it. This becomes `smart_enemy_select_target()` — the
one place this phase delivers a literal "select target / decide intent" box from the user's
diagram as reusable, independently testable code.

## 3. What this phase adds

New `inc/ai_forces.h` / `src/ai_forces.c` (a focused module, matching the established pattern of
`entity_spawn.c`/`input_command.c`/`input_snapshot.c`):
```c
void move_boss_entities(GameState *game);
void move_regular_enemies(GameState *game);
void move_smart_enemies(GameState *game);
Man *smart_enemy_select_target(GameState *game, const Enemies *smartEnemy);
```
- `move_boss_entities`/`move_regular_enemies`: `src/process.c:636-670`/`:673-710` moved verbatim.
- `smart_enemy_select_target`: replicates the exact `pow()`-based comparison at `:805-806`
  (double precision preserved), returning `&game->man` unconditionally when `!game->multiPlayer`
  (matching that the single-player chase always targets `man`), else the nearer of
  `&game->man`/`&game->secondPlayer`.
- `move_smart_enemies`: `src/process.c:712-923` moved verbatim, with exactly one change — the
  inline `pow()` comparison at `:805-806` is replaced by
  `if (smart_enemy_select_target(game, &game->smartEnemies[i]) == &game->man)`, an identical
  decision now reusable/testable, still followed by the exact same two (still-duplicated,
  still-asymmetric) chase-logic bodies verbatim.

None of lines 636-923 reference `dt`/`PHYSICS_DT` — this AI movement remains frame-count-based,
matching `docs/physics-timestep-map.md`'s already-documented deferral of enemy/boss/bullet timing
conversion to a future phase, not reopened here. No new parameter is needed on any of the three
movement functions.

`src/process.c` gains `#include "ai_forces.h"`; lines 636-923 are replaced with three calls
(`move_boss_entities(game); move_regular_enemies(game); move_smart_enemies(game);`) at the exact
same point in `process()`, so the bullet-collision loops before it and the player-integration code
after it (`:926` onward) are completely untouched.

## 4. Deliberately not done (found, documented, not touched)

- Merging the multiplayer smart-enemy chase's two duplicated (and asymmetric) bodies into one —
  a real, evidence-backed simplification opportunity, but doing so would change which exact lines
  run for the multiplayer path, risking a subtle behavior change. Flagged here, left alone.
- Any change to `collisionDetect()`/`collisionDetect2()` — Phase 19's collision-ordering scope.
- `process2()` — confirmed to have zero AI-driven entities; nothing to extract.
- Enemy speeds, jump behavior, targeting rules, hitboxes, spawn logic, pathfinding, render
  interpolation (Phase 20) — none reopened here, per the user's own explicit exclusion list.
- Converting boss/enemy/smart-enemy movement to fixed-timestep per-second units — already a
  deliberately deferred, separate item since Phase 11.
