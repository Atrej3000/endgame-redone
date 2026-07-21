# Projectile Correctness Map — Ucode_Endgame

Written **before** any Phase 14 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `c9ff171` (tag
`refactor-pre-projectile-correctness`, `main` after Phase 13's PR #9 merge) via a complete read of
`src/process.c`'s bullet-handling code, `inc/header.h`'s `Bullet` struct, `src/doRender.c`'s
bullet rendering, and `src/processEvents.c`'s shooting input, immediately before this document was
written.

## 1. Confirmed: bullets are entirely Arcade-only

Grep for `Bullet|bullets\[|secondBullets\[|addBullet|removeBullet` across `src/*.c` returns matches
only inside Arcade-labeled code: `process()` (not `process2()`), `processEvents()` (not
`processEvents2()`), `doRender()` (not `doRender2()`), `arcade_session_reset`/`arcade_assets_load`/
`arcade_assets_unload` (not the Runner equivalents), and `app_shutdown()`'s mode-agnostic teardown
loop. Direct reads of `process2()`, `processEvents2()`, `runner_session_reset()`, and `doRender2()`
in full confirm **zero** bullet references in any of them — Runner has no shooting mechanic at
all. This phase touches Arcade-only files.

## 2. The bullet triple-movement bug is worse than previously documented

`docs/physics-timestep-map.md` (Phase 11) found: *"each bullet is advanced up to 3 times per
frame, once per enemy-type collision loop."* Confirmed by direct read this is the right
**architecture** finding but understates the **magnitude**. Three separate collision loops exist
in `process()`, each iterating its own full target array and re-running the bullet's movement
inside its own loop body:

| Loop | Target array | Line range | Outer-loop iterations/tick |
|---|---|---|---|
| vs. `enemyValues[]` | `NUM_ENEMIES` = 101 | `process.c:260-289` | 101 |
| vs. `smartEnemies[]` | `NUM_SMART_ENEMIES` = 10 | `process.c:291-329` | 10 |
| vs. `boss[]` | 2 | `process.c:331-365` | 2 |

Each outer loop iterates its **full, fixed-size** target array every tick regardless of how many
entries are actually alive — so `bullets[i]->x += bullets[i]->dx` (and its accompanying clamp,
`if (dx>0.1) dx=0.1; if (dx<-0.1) dx=-0.1;`) executes **101 + 10 + 2 = 113 times per tick per live
bullet**, not "3 times." A byte-identical mirror block for `secondBullets[]`
(`process.c:371-472`, gated on `game->multiPlayer`) applies the same 113×-per-tick multiplication
to player 2's bullets in multiplayer.

**Steady-state speed derivation**: a bullet spawns with `dx = ±3` (`processEvents.c`'s
`addBullet(game, ..., 3)`/`addBullet(game, ..., -3)`). The very first movement application inside
the very first tick's Loop 1 uses this `dx=3` (moving 3px), and the clamp immediately after that
same statement forces `dx` down to `±0.1` for every subsequent application (this tick and all
future ticks) — clamping is idempotent once at the bound, so it stays at exactly `0.1` forever
after. From tick 2 onward, every one of the 113 per-tick applications uses the already-clamped
`dx=0.1`, so the **net per-tick displacement is `0.1 × 113 = 11.3 px`** (tick 1 is a one-time,
imperceptible transient — `3 + 0.1×112 = 14.2` — not worth replicating). At a fixed 60Hz tick,
`11.3 px/tick = 678 px/s`; crossing the 1280px screen width takes roughly 1.9 seconds — plausibly
the speed every current player has actually experienced, however it originated.

## 3. No swept collision

Every collision loop's hit-test is a **bullet-point vs. target-rectangle** check, evaluated only
against the bullet's position *after* this tick's movement:
```c
game->bullets[i]->x > game->enemyValues[j].x && game->bullets[i]->x < game->enemyValues[j].x + 40 &&
game->bullets[i]->y > game->enemyValues[j].y && game->bullets[i]->y < game->enemyValues[j].y + 50
```
(identical shape against `smartEnemies[]`/`boss[]`, same `+40`/`+50` box). No path/segment check
exists — a bullet whose movement this tick carries it from one side of a target to the other
within a single tick has no chance to register a hit, since only the *end* position is tested.
This is exactly the gap "swept collision" (the assessment's own phrase) closes.

## 4. No pool — per-shot heap allocation

`Bullet *bullets[MAX_BULLETS]`/`secondBullets[MAX_BULLETS]` (`inc/header.h:306-307`,
`MAX_BULLETS` = 1000, `:48`) are fixed-size arrays of **pointers**. `addBullet`/`addSecondBullet`
(`process.c:4-30`,`:41-65`) scan for the first `NULL` slot and `malloc(sizeof(Bullet))` into it;
`removeBullet`/`removeSecondBullet` (`:32-39`,`:67-74`) `free()` and NULL the slot. Every shot is
an individual heap allocation/deallocation in a hot path that can run up to 1000 times
concurrently — exactly the per-projectile churn a "projectile pool" (pre-allocated, flag-toggled
slots, zero per-shot `malloc`/`free`) eliminates.

## 5. `unvisible`'s actual lifecycle — confirmed safe to remove, not preserve

Traced fully: each loop resets `bullets[i]->unvisible = 0` unconditionally at loop-body entry,
may set it to `1` on a hit, then immediately checks it (alongside off-screen bounds) in the same
loop body and calls `removeBullet` right there if set — the bullet is `free()`'d and NULLed within
the **same** loop iteration it was marked hit, before any later loop (smart-enemy, boss) can even
observe it (they see `NULL` and skip via their own guard). There is no delayed-despawn behavior
anywhere — `unvisible` is a same-tick-only, immediately-acted-on flag, not a "linger one extra
frame" mechanism its name might suggest. Once bullets gain an explicit `active` flag (needed for
the pool anyway, §6), `unvisible` becomes a pure duplicate of "just deactivated" with no distinct
behavior of its own — removed, not kept alongside `active`, to avoid two same-purpose flags.

## 6. Design: pool, move-once, swept collision

**`Bullet` struct becomes**: `float x, y, dx; float prevX; bool active;` — `prevX` mirrors
`Man.prevY`'s exact role from Phase 13 (captured immediately before this tick's move, consumed by
a crossing/swept test); `active` replaces the NULL-pointer-means-empty convention.

**Pool**: `bullets`/`secondBullets` become `Bullet[MAX_BULLETS]` (value arrays, not pointer
arrays). `addBullet`/`addSecondBullet` scan for the first `!active` slot and set fields directly —
no `malloc`. `removeBullet`/`removeSecondBullet` set `active = false` — no `free`. `GameState` is
still `calloc`'d (`src/app.c`), so every slot starts `active = false` for free — no explicit init
loop needed, same reasoning already established for `Man.prevY`/`InputState` in Phases 12-13. The
existing `arcade_session_reset`/`app_shutdown` loops that call `removeBullet(game, i)` across all
1000 slots need **no changes** — the new body (`active = false`) is a drop-in replacement for the
old one (`free` + `NULL`), same call sites, same effect from the caller's point of view.

**Move-once-per-step**: a new `move_arcade_bullets(GameState *game)` runs once per tick (called
from `arcade_simulate()`, before `process()`), owning the *only* movement/clamp/off-screen-despawn
for both bullet arrays. The 3 (×2 for multiplayer) collision loops keep their per-target-type
logic (AABB test, hit-response, kill thresholds, on-hit despawn) unchanged — they now just *read*
the bullet's already-updated position instead of moving it themselves.

**Speed-preservation, decided deliberately**: fixing "move once" alone would drop the bullet's
effective speed from `11.3 px/tick` to `0.1 px/tick` (the old per-application clamp) — a ~113×
slowdown nobody asked for and no player would recognize as "the same game." That `11.3` was never
a tuned design value, but it *is* the speed already experienced. Matching Phase 11's own precedent
(preserving observable behavior across an architectural fix via a derived, documented conversion
factor), this phase introduces `BULLET_SPEED_PER_TICK = 11.3f` (`= 0.1 × 113`, both legacy values,
documented derivation above). Bullets remain expressed in px/tick, not converted to px/s — a
well-defined unit given the fixed 60Hz tick already in place, and not something the assessment's
Phase 4 wording asks for; a trivial future option (multiply by `PHYSICS_HZ`), not done here.

**A bug caught before the test was written, not by it**: the first version of
`move_arcade_bullets` reused the legacy code's *shape* — `if (dx > BULLET_SPEED_PER_TICK) dx =
BULLET_SPEED_PER_TICK;` (and the mirrored `<` branch) — a max-clamp. That only shrinks `dx` when
it's already larger than the bound. The legacy per-application clamp (`0.1`) was *smaller* than
the spawn `dx` (`±3`), so it always fired, shrinking toward `0.1` every time. The new bound
(`11.3`) is *larger* than the spawn `dx`, so a max-clamp of the same shape never fires — the
bullet would have silently kept moving at the spawn speed (`3`/tick) instead of the intended,
speed-preserving `11.3`/tick, defeating the whole point of the constant. Fixed by normalizing
`dx` to `±BULLET_SPEED_PER_TICK` by sign instead of clamping a magnitude:
```c
game->bullets[i].dx = (game->bullets[i].dx >= 0) ? BULLET_SPEED_PER_TICK : -BULLET_SPEED_PER_TICK;
```
This reproduces the old code's actual effect (spawn direction, fixed magnitude every tick)
regardless of which bound happens to be larger — corrected in its own commit before the
validation test was written.

**Swept collision**: the point test becomes a segment-vs-rect test using `prevX`:
```c
float bx0 = fminf(bullets[i].prevX, bullets[i].x), bx1 = fmaxf(bullets[i].prevX, bullets[i].x);
if (bx1 > target.x && bx0 < target.x + 40 && bullets[i].y > target.y && bullets[i].y < target.y + 50)
```
Bullets have no `dy` (never move vertically), so only the X side needs sweeping — the Y check
stays a simple point-test. This directly mirrors Phase 13's own `prevY`-based crossing test for
player landing, applied to the axis that actually matters for a horizontally-moving bullet.

## 7. Deliberately out of scope (found, documented, not touched)

- Enemy/boss AI, movement, or the collision-response thresholds (one-hit-kill for plain enemies,
  6-hit for smart enemies, 31-hit for bosses) — unchanged.
- Shooting-input mechanism (`shotCount`/`SDL_SCANCODE_SPACE`/`KP_0` re-arm timer,
  `processEvents.c:231-279`, still real-frame-rate) — Phase 12 explicitly deferred "bullet-pool/
  projectile-ownership decisions" to this phase, but the assessment's own Phase 4 wording is about
  the *projectiles*, not the *input that spawns them*; a future phase could route shooting through
  an `InputState`-style edge-triggered mechanism the way Phase 12 did for jump.
- Converting bullet speed to real px/s units (multiplying by `dt`) — see §6.
- Extracting bullet/projectile code into its own module — `docs/solid-gof-audit.md` already names
  and defers this exact SRP item.
- The render-time direction-flip quirk (`doRender.c` flips a bullet's sprite using the *player's
  current* facing rather than the bullet's facing at spawn — a bullet fired while facing left,
  then the player turns right before it despawns, flips mid-flight) — a minor, pre-existing
  cosmetic issue, unrelated to movement/collision/pool correctness.
- Target-side swept collision (enemies/boss also move each tick) — scoped to the bullet's own fast
  movement only, matching the assessment's phrasing.

## 8. Test strategy

`docs/verification/projectile_correctness_test.c` (new, `mingw-projectiletest` target), calling
the relevant functions directly with hand-set `GameState` fields (matching every prior phase's
direct-call convention): (a) pool spawn/despawn — `addBullet` activates a slot, `removeBullet`
deactivates it, without disturbing other slots; (b) move-once-per-step — one
`move_arcade_bullets` call moves a bullet by exactly `BULLET_SPEED_PER_TICK`, proving the 113×
bug is gone; (c) swept collision — a bullet whose `prevX → x` span crosses a target's rect
registers a hit even when its end-of-tick position alone would miss (the old point test would
fail here, the new swept test correctly fires); (d) one real `process(game, dt)` call with a
bullet positioned to cross an enemy still kills it and deactivates the bullet, confirming the
refactored collision loop still works end-to-end after having its movement code removed.
