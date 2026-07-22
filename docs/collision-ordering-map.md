# Collision Ordering Map — Ucode_Endgame

> **Current-state note (Phase 24).** Sections below are the Phase 19 audit and extraction record,
> not a description of the current implementation. Phase 24 moved from fused
> detection/consequence helpers to bounded, typed `GameEvent` collection and centralized
> application. Fixed-step simulation captures previous transforms, advances movement and
> body/world collision, runs pure Arcade/Runner hazard and projectile detectors, then applies
> queued consequences once. Rendering consumes the resulting state through Phase 20 interpolation
> and never performs simulation work.
>
> `mingw-collisionorderingtest` retains its compatible target name and now also verifies event
> consequences through `docs/verification/collision_pipeline_test.c`.

Written **before** any Phase 19 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `1ea9a06` (tag
`refactor-pre-collision-ordering`, `main` after Phase 18's PR #14 merge) via a complete read of
every collision/hazard/consequence/transition site in `src/`.

## 1. The target pipeline and why the codebase doesn't map onto it as a single sequence

Target: **world-solid resolution → body-contact hazards → projectile hits → deaths and score
consequences → scene transitions**, for one simulation tick.

Full audit of both modes found the actual structure is two decoupled sub-pipelines, not one:

**Arcade** — `arcade_simulate()` (`src/frame.c:4-12`) calls `process()` then `collisionDetect()`
every fixed physics tick (0-5x per real frame, `main.c`'s accumulator):
- `process()` (`src/process.c:347-...`) contains **projectile hits** (6 bullet-collision loops,
  `:359-524`, kill/score/removal consequences fused inline with the hit detection) *before*
  Phase 18's AI-movement calls and player integration.
- `collisionDetect()` (`src/collisionDetect.c:15-652`) is **world-solid** resolution (ledge
  collision for man/secondPlayer/enemy/smartEnemy/boss), called *after* `process()` returns.
- Separately, **body-contact hazards** (player-vs-enemy/boss/smartEnemy contact,
  `processEvents.c:294-381`, `lives=0` consequences fused inline) and the resulting **scene
  transition** (`processEvents.c:394-416`) live in `processEvents()`, called once per **real
  frame** (not per fixed tick), after `doRender()` — a different execution cadence entirely.

**Runner** — `runner_simulate()` (`src/frame.c:14-21`) calls `process2()` then
`collisionDetect2()` every fixed physics tick:
- `collisionDetect2()` (`src/collisionDetect.c:654-793`) checks **body-contact hazards** (star
  contact, `:656-666`, calling `runner_trigger_death()`) *before* **world-solid** ledge resolution
  (`:668-792`) — within the same function.
- A second hazard check (falling off-screen, `processEvents.c:569-580`) lives in
  `processEvents2()`, once per real frame — the same cadence split as Arcade.
- **Deaths/score consequences**: the single-shot `runner_update_death()` (`src/runner_death.c`),
  called after `doRender2()`, before `processEvents2()` (see section 3).
- **Scene transition** (`processEvents.c:595-598`) follows score-persist in the same function.

`process2()` has no projectile hits at all (Runner has no shooting mechanic, confirmed by the
Phase 17 audit and reconfirmed here).

## 2. Ordering mismatches found, and why two of the three are not touched

**Mismatch A — Arcade: projectile hits run before world-solid within a tick.** `process()`
(projectile hits, `:359-524`) executes before `collisionDetect()` (world-solid) in
`arcade_simulate()`. **Not changed**: `collisionDetect()`/`collisionDetect2()` resolve ledge
collision against whatever position exists *when called*. If moved to run *before*
`process()`/`process2()` (to literally put world-solid first), it would resolve *last* tick's
already-corrected position — a near no-op — and that tick's actual movement would then run with no
further correction until *next* tick, letting the player visibly tunnel through geometry for one
tick before `doRender()`/`doRender2()`. World-solid resolution must run **after** movement, not
before — the target list is best read as "collision-response priority once movement has happened
this tick," not literal call order including movement itself. Under that reading, both modes'
existing movement-then-resolve structure already satisfies the intent; swapping the call order
would be a regression, not a formalization.

**Mismatch B — Runner: hazard contact checked before ledge resolution, inside
`collisionDetect2()`.** Star contact (`:656-666`) runs before the ledge loops (`:668-792`) in the
same function. **Not changed**: no existing test depends on this order (confirmed — see section
5), but there's also no confirmed defect motivating a swap. Changing it would alter the exact tick
a hazard-contact death fires in an edge case (contact checked against pre- vs.
post-ledge-correction position) — a real behavior change with no evidence requiring it. Flagged as
a found-but-not-acted-on opportunity, per this session's preserve-unless-proven-broken discipline
(mirrors Phase 18's handling of the multiplayer smart-enemy chase asymmetry).

**Mismatch C — both modes: body-contact hazards run at a different cadence (once per real frame)
than world-solid/projectile-hits (once per fixed tick, 0-5x per real frame).** Not changed: moving
hazard detection into the fixed-tick loop would be a genuine timing/behavior change — a
hazard-caused death could fire up to 4 ticks earlier within a stalled frame than it does today.
Documented as a real architectural option for a future phase, not attempted here without a
demonstrated defect motivating it.

**Runner's `runner_update_death()` render-then-resolve ordering** (called after `doRender2()`,
before `processEvents2()`) is confirmed intentional and already correctly documented
(`docs/runner-death-lifecycle.md`, code comments at `main.c:176-178` and `runner_death.c:17-18`):
running it before render would cut the death animation one frame short, and it must run once per
real frame (not per tick) for the "single-shot" guarantee `runner_death_test.c` locks in to hold at
all. Not touched.

## 3. What this phase does: extract, name, don't reorder

The safe, high-value version of "formalize" — extract each collision/hazard/transition concern
into its own named function, matching the target's own vocabulary, without moving any call site
or changing any behavior. The same kind of extraction Phase 11 (`apply_*_player_forces`) and
Phase 18 (`move_*_entities`) already did successfully.

New `inc/collision_pipeline.h` / `src/collision_pipeline.c`:

| Function | Was | Called from (unchanged position) |
|---|---|---|
| `resolve_projectile_hits(GameState*)` | `process.c:352-524` (lazy sound-load + 6 bullet loops) | `process()`, right after `time++` |
| `resolve_arcade_hazards(GameState*)` | `processEvents.c:294-392` (body contact + reached-bottom + fall-off-screen — already colocated, one cadence) | `processEvents()`, same point |
| `resolve_arcade_game_over_transition(GameState*)` | `processEvents.c:394-416` | `processEvents()`, right after hazards |
| `resolve_runner_hazard_contact(GameState*)` | `collisionDetect.c:656-666` | `collisionDetect2()`, same point (still before ledge resolution — order preserved, Mismatch B) |
| `check_runner_fall_hazard(GameState*)` | `processEvents.c:569-580` | `processEvents2()`, same point |
| `resolve_runner_game_over_transition(GameState*)` | `processEvents.c:583-599` | `processEvents2()`, right after the fall check |

Each function's body is moved verbatim — same statement order, same arithmetic, same consequence
fusion (detection and consequence application stay fused exactly as found; un-fusing them would be
a much larger redesign than "formalize ordering," not requested).

## 4. Test coverage gap this phase closes

Confirmed: no existing test asserts on the Arcade hazard/transition logic or the Runner
fall-hazard/transition logic in isolation — only reachable before now via a full
`processEvents()`/`processEvents2()` call with a real event loop. `docs/verification/
frame_pipeline_test.c` and `runner_death_test.c` exercise these paths only indirectly (double-
transition guards, render purity, single-shot death lifecycle end-to-end), never the hazard-
detection logic itself in isolation. This phase's new test calls each of the six extracted
functions directly with hand-set `GameState` fields, mirroring the established direct-call test
style (Phase 15/18's pattern).

## 5. Existing tests whose guarantees this phase must not disturb

- `docs/verification/frame_pipeline_test.c` — locks in the Arcade/Runner double-transition guards
  (`processEvents.c:401,411,595`) and `doRender()`/`doRender2()` purity. Must keep passing
  unchanged.
- `docs/verification/runner_death_test.c` — locks in the single-shot death lifecycle end-to-end
  via `runner_frame()`, including game-over transition and score-persist ordering. Must keep
  passing unchanged.
- `docs/verification/collision_correctness_test.c` — tests `collisionDetect()`/`collisionDetect2()`
  in isolation (ledge-only); no assertion on the star-hazard-vs-ledge relative order inside
  `collisionDetect2()` — confirms Mismatch B is safe to leave as-is (nothing currently depends on
  it either way).
- `docs/verification/projectile_correctness_test.c` — calls `process()` directly with enemy data;
  does not call `collisionDetect()` — no assertion about projectile-hit-vs-world-solid ordering.

## 6. Deliberately not done

- Reordering `process()`/`collisionDetect()` (or `process2()`/`collisionDetect2()`) relative to
  each other — see Mismatch A.
- Reordering `resolve_runner_hazard_contact()` relative to ledge resolution — see Mismatch B.
- Any change to `runner_update_death()`'s position or the fixed-timestep accumulator.
- Separating consequence application from hit/contact detection.
- Unifying the per-real-frame hazard cadence with the per-tick world-solid/projectile-hit cadence
  — see Mismatch C.
- Render interpolation, broad-phase optimization, hitbox redesign, enemy movement retuning — per
  the user's own explicit exclusion list.
