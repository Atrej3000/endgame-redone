# Fixed-Timestep Physics Conversion Map — Ucode_Endgame

Written **before** any Phase 11 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `a58268f` (tag `refactor-pre-fixed-timestep`,
`main` after Phase 10's PR #6 merge) via three parallel codebase explorations plus targeted direct
reads immediately before this document was written.

## 1. Current state — confirmed frame-dependent, zero timing infrastructure

- No `SDL_GetTicks`, `SDL_GetPerformanceCounter`, `SDL_GetPerformanceFrequency`, `SDL_Delay`
  (active), or any delta-time variable exists anywhere in `src/*.c`/`inc/*.h`. The only match is a
  dead comment (`src/processEvents.c:753`, `//SDL_Delay(2000);`).
- `SDL_RENDERER_PRESENTVSYNC` (`src/app.c:167`) is the sole existing frame-pacing mechanism — it
  paces the *renderer*, not physics.
- `process()` (Arcade) and `process2()` (Runner) each run exactly once per `main()` loop iteration,
  via `arcade_frame`/`runner_frame` (`src/frame.c`), which itself runs once per real rendered
  frame. `game->time` (`inc/header.h`) is a plain `int` frame counter, incremented once per call
  (`src/process.c:79` and `:827`) — not a real elapsed-time value.
- `main()`'s loop (`src/main.c:31-80`) is a single flat `while (gameState->app.scene !=
  APP_SCENE_QUIT)` with a 9-case `switch`. Only 2 of the 9 cases (`APP_SCENE_ARCADE_GAME`,
  `APP_SCENE_RUNNER_GAME`) call anything physics-related (`arcade_frame`/`runner_frame` →
  `process`/`process2` + `collisionDetect`/`collisionDetect2`). The other 7 cases (menus,
  leaderboards, pause) call only event/render pairs — no physics function among them.

## 2. Full inventory of frame-dependent constants (player fields only — see §5 for scope)

All confirmed by direct read of `src/process.c`/`src/processEvents.c` at the current commit.

| Constant | Value | Site(s) | Notes |
|---|---|---|---|
| `GRAVITY` | `0.5f` (`inc/header.h:48`) | `process.c:743,802` (Arcade man/secondPlayer), `:858,886` (Runner man/secondPlayer) | Only referenced at these 4 sites — confirmed by grep, nowhere else |
| Jump impulse | `dy = -10` | `processEvents.c:66,76` (Arcade), `:601,625` (Runner) | One-shot discrete assignment on `GAME_COMMAND_JUMP_PLAYER1`/`_PLAYER2`, gated on `onLedge` |
| Horizontal accel | `dx += 0.5` / `-= 0.5` | `processEvents.c:252,268` (Arcade man), `:333,349` (Arcade 2P), `:654,664` (Runner man), `:693,703` (Runner 2P) | Held-key `SDL_SCANCODE_A`/`D`/`LEFT`/`RIGHT` |
| Horizontal clamp | `±6` | Same 8 sites, immediately after each accel line | |
| Jump-hold vertical thrust | Arcade `dy -= 0.2f` (`processEvents.c:239,320`); Runner `dy -= 0.15f` (`processEvents.c:646,685`) | Held-key `SDL_SCANCODE_W`/`UP` | **Confirmed divergence**: Arcade and Runner already use different rates — preserved, not unified, this phase |
| Friction | `dx *= 0.8f` | `processEvents.c:284,365` (Arcade), `:674,713` (Runner) | Identical across all 4 sites |
| Snap-to-zero | `fabsf(dx) < 0.1f → dx = 0` | Same 4 sites, immediately after friction | |

**Not touched this phase** (confirmed present, deliberately deferred — see §5): boss gravity
`+= 0.1` capped `1.5` (`process.c:421-424`); enemy gravity `+= 0.4` capped `2.5`
(`process.c:485-488`); smart-enemy discrete impulses (`dy = -6`, `dy -= 10`, `dy += 1`,
`process.c:523,546,560,575,609,615,629,640,665,671,685,696`); bullet horizontal motion
(`process.c:94,126,166,206,238,273` — also has a separate, pre-existing correctness bug: each
bullet is advanced up to 3 times per frame, once per enemy-type collision loop; not fixed here);
background/decor scroll (`processEvents.c:138-211`, train/clouds); moving-trap `sin`/`cos` of
`game->time*0.06f` (`process.c:900,904`).

## 3. Conversion-factor derivation (verified, not assumed)

Old code stores velocity in the *same* field (`dx`/`dy`) that position integration consumes
directly: `y += dy` once per frame, frame duration implicitly `1/60 s`. This means `dy` is
numerically "pixels moved per frame," and any constant added to `dy` once per frame (e.g.
`GRAVITY`) is numerically "pixels-per-frame added per frame" — an acceleration in
frame²-denominated units.

Converting a per-frame quantity to per-second requires multiplying by the appropriate power of 60
(the assumed frame rate the constants were tuned against):
- **Velocity** (`px/frame` → `px/s`): multiply by **60** (one power, since velocity is
  distance/time and "per frame" already carries one implicit `1/60 s` factor).
- **Acceleration** (`px/frame²` → `px/s²`): multiply by **3600** (`60²`, two implicit `1/60 s`
  factors).

| Quantity | Old (px/frame or px/frame²) | ×factor | New (px/s or px/s²) |
|---|---|---|---|
| Gravity (`GRAVITY_PER_SEC2`) | `0.5` | ×3600 | `1800.0f` |
| Jump impulse (`JUMP_SPEED_PER_SEC`) | `10` | ×60 | `600.0f` |
| Horizontal accel (`RUN_ACCEL_PER_SEC2`) | `0.5` | ×3600 | `1800.0f` |
| Horizontal clamp (`RUN_MAX_SPEED_PER_SEC`) | `6` | ×60 | `360.0f` |
| Arcade jump-hold (`ARCADE_JUMP_HOLD_ACCEL_PER_SEC2`) | `0.2` | ×3600 | `720.0f` |
| Runner jump-hold (`RUNNER_JUMP_HOLD_ACCEL_PER_SEC2`) | `0.15` | ×3600 | `540.0f` |
| Snap-to-zero threshold (`RUN_SNAP_ZERO_SPEED_PER_SEC`) | `0.1` | ×60 | `6.0f` |
| Friction (`RUN_FRICTION_DECAY_PER_TICK`) | `0.8` (dimensionless multiplicative decay) | unchanged | `0.8f`, applied as `powf(0.8f, dt*PHYSICS_HZ)` |

**Regression proof this preserves today's exact behavior at 60Hz**: with `dt` fixed at
`1/60`, one tick from rest: `dy += GRAVITY_PER_SEC2 * dt = 1800 * (1/60) = 30.0f` (px/s);
position update `y += dy * dt = 30.0f * (1/60) = 0.5f` — bit-identical to today's
`y += dy; dy += 0.5f` sequence's first-tick delta. Friction: `powf(0.8f, dt*60.0f) =
powf(0.8f, (1/60)*60) = powf(0.8f, 1.0f) = 0.8f` exactly — identical to today's bare `*= 0.8f`.
This identity holds at every tick, not just the first, since `dt` never varies (fixed-timestep,
no interpolation). **`docs/verification/physics_timestep_test.c` asserts this numerically.**

## 4. Architectural relocation: `processEvents`/`processEvents2` → `apply_*_player_forces`

The held-key continuous-force blocks (horizontal accel/clamp, jump-hold thrust, friction/snap)
currently live in `processEvents()`/`processEvents2()` — a **different function** from
`process()`/`process2()` (where gravity/position-integration lives). `arcade_frame`/`runner_frame`
call `process`→`collisionDetect`→`doRender`→`processEvents` in that order (`src/frame.c`), all
four once per real frame today.

If only `process`/`process2` move to a fixed 60Hz tick while `processEvents`/`processEvents2`
keep running once per real frame, **gravity/integration becomes frame-rate-independent but
acceleration/friction/jump-hold does not** — the described bug would only be half-fixed. This
phase therefore **relocates only the continuous, held-key force-application code** (not the
`SDL_PollEvent` discrete-event loop, not jump-impulse-on-keydown, not scene transitions, not lazy
SFX loads, not background/decor scroll, not animation-frame gates) out of `processEvents()`/
`processEvents2()`, so it runs at the fixed tick rate. This is not the assessment's own "Phase 2 —
separate input and simulation" (no `InputState` struct, no `input_poll()` function, no
edge-triggered de-duplication across ticks is introduced) — it is the minimum relocation required
for this phase's own stated goal (frame-rate-independent player physics) to actually hold for
every constant in §2, not just gravity.

**Refinement made during implementation, not in the original plan text**: the relocated code does
*not* land directly inside `process()`/`process2()`. It lives in two new standalone functions,
`apply_arcade_player_forces(GameState*, float dt)` and `apply_runner_player_forces(GameState*,
float dt)` (`src/process.c`), called from `arcade_simulate`/`runner_simulate` (`src/frame.c`)
immediately *before* `process`/`process2` each fixed tick. Reason: `process()`/`process2()` are
called directly by several existing non-interactive tests (`frame_pipeline_test.c`,
`runner_death_test.c`) with manually-set `dx`/`dy`/`slowingDown` state and no real window — if the
held-key code (which calls `SDL_GetKeyboardState(NULL)`) lived inside `process`/`process2`
themselves, those tests' hand-set preconditions would be silently overwritten by "no keys pressed"
on every call, corrupting assertions unrelated to input. Keeping `process`/`process2` free of any
keyboard read preserves their direct testability exactly as before; `apply_*_player_forces` are the
only new call sites that touch `SDL_GetKeyboardState`, and both run at the fixed tick rate via
`arcade_simulate`/`runner_simulate`, so the phase's frame-rate-independence goal still holds for
every constant in §2.

`SDL_GetKeyboardState(NULL)` needs no window/event-loop context, so reading it from
`apply_arcade_player_forces`/`apply_runner_player_forces` (which only take `GameState*`, no
`SDL_Window*`) is straightforward.

## 5. Deliberate scope boundary and known side effect

**Enemies, bosses, bullets, background/decor scroll, and moving traps are explicitly not
converted this phase** — narrower than the assessment's own Phase 1 wording ("player gravity,
velocity, acceleration, friction"), and consistent with the assessment's own later phases (3, 4,
6) covering entities/projectiles/collision separately. Converting these now would multiply the
diff size and risk without being what was asked.

**Known, documented side effect**: `game->time` (incremented once per `process()`/`process2()`
call, `process.c:79,827`) will now advance at a fixed 60/s rate instead of the render rate, since
`process`/`process2` become fixed-tick. `processEvents()`/`processEvents2()`'s background-scroll
and sprite-animation gates (`game->time % N == 0`, untouched, still called once per real frame)
will sample this now-independently-advancing counter — meaning on a non-60Hz display, background
scroll/animation-frame timing (never frame-rate-correct before this phase either) may shift
slightly relative to player movement. This is not a regression this phase introduces from
nothing — it is an existing, pre-existing frame-dependence in background/decor timing, now simply
decoupled from player-physics timing rather than coupled to it. Documented, not fixed.

## 6. Everything else found during exploration, confirmed real, explicitly deferred

- **Runner ceiling `onLedge` bug** (`src/collisionDetect.c:656,714`): `collisionDetect2()` sets
  `onLedge = 1` on a ceiling hit (`dy < 0` branch), identical to its landing-hit handling — unlike
  Arcade's `collisionDetect()`, which correctly sets `onLedge = 0` on ceiling contact
  (`collisionDetect.c:279`) and `onLedge = 1` only on landing (`:293`). Confirmed by direct read
  of both functions' full ceiling/landing branches. Real, pre-existing, not fixed this phase (a
  collision-logic fix, not a timestep fix).
- **Bullet triple-movement bug** (`process.c:94,126,166` and `:206,238,273`): each live bullet's
  `x` position is advanced by `dx` up to 3 separate times per frame — once inside each of the
  enemy/smart-enemy/boss collision-check loops. Confirmed by direct read of all three loop bodies.
  Real, pre-existing, not fixed this phase (a projectile-architecture fix, assessment's own
  Phase 4).
- **Jump-hold continuous thrust** (§2): both modes let holding the jump key add continuous upward
  force after the initial jump impulse, effectively giving mid-air thrust and making jump height
  refresh-rate-dependent in *character* (not just magnitude, since it's now converted to
  per-second). This phase converts the *rate* faithfully but does not remove or redesign the
  mechanic itself (coyote time, jump buffering, variable jump height via early-release cut) —
  assessment's own Phase 5 ("Improve game feel").
- **Collision resolution order / corner cases, camera/scroll coupling to physics, deterministic
  seeding**: not investigated this phase (out of scope; assessment's own Phase 3/6).

## 7. Test strategy

`docs/verification/physics_timestep_test.c` (new, `mingw-physicstest` target):
1. **Refresh-rate independence**: the same total simulated time (a fixed number of physics ticks,
   each with `dt = PHYSICS_DT`), driven through `arcade_simulate`/`runner_simulate` directly, must
   produce identical final `man.x`/`man.y`/`man.dx`/`man.dy` regardless of how many *calls* it
   took to accumulate that many ticks (simulating different real frame rates chunking the same
   total time differently) — this is the direct, load-bearing proof of the fix.
2. **Regression-equivalence**: one tick from a known rest state reproduces the exact numeric
   deltas derived in §3 (`dy == 30.0f`, `y` moves by exactly `0.5f`), proving the conversion math
   is correct, not just internally consistent.
