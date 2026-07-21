# Input Snapshot Architecture Map — Ucode_Endgame

Written **before** any Phase 17 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `b94e363` (tag `refactor-pre-input-snapshot`,
`main` after Phase 16's PR #12 merge) via a complete read of every SDL input read and every
gameplay-relevant `SDL_Event` handler in `src/`.

## 1. The gap this phase closes

The user proposed a target frame architecture: poll events once, build an input snapshot once,
then run the fixed-step physics loop against that stable snapshot, then render. Phases 1-16 built
the fixed-step loop itself (Phase 11) and edge-triggered jump requests (Phase 12/15), but never
addressed *continuous* held-key input specifically. Direct audit confirms:

- **`apply_arcade_player_forces`** (`src/process.c:206-341`) and **`apply_runner_player_forces`**
  (`src/process.c:1116-1200`) each call `SDL_GetKeyboardState(NULL)` as their first statement
  (`:208`, `:1118`) — **live, inside the fixed-step path**. `arcade_simulate()`/`runner_simulate()`
  (`src/frame.c:4-21`) call them from inside the accumulator's `while` loop
  (`src/main.c`), which runs 0–`MAX_PHYSICS_STEPS_PER_FRAME` (5) times per real frame. A real frame
  producing 3 physics ticks re-samples live keyboard state 3 separate times, non-deterministically.
- **`processEvents()`** (`src/processEvents.c:110`) makes its own, separate `SDL_GetKeyboardState(NULL)`
  call for the shoot key (`SDL_SCANCODE_SPACE`/`SDL_SCANCODE_KP_0`, gated by a `shotCount`/
  `game->time`-modulo cooldown, `:231-278`) — already once per real frame (this call happens after
  the physics accumulator loop and after `doRender`, per `main.c`'s per-scene case block), just not
  centralized with the other two reads above.
- **Jump is already correctly isolated** and needs no change: `SDLK_w`/`SDLK_UP` keydowns set
  `game->input.jumpBufferTicksPlayer1`/`Player2` (`src/processEvents.c:67,70`, via
  `translate_arcade_command()`/`translate_runner_command()`, `src/input_command.c:3-20`,`:22-39`),
  consumed at the fixed tick rate by `consume_arcade_jump_requests`/`consume_runner_jump_requests`
  (`src/process.c:140-194`, `:1052-1106`, unchanged since Phase 15's coyote-time/ordering fix).
- `InputState` (`inc/header.h:303-307`) currently holds only the two jump-buffer `int` fields.

Confirmed effect of the live-SDL-read seam: `docs/verification/game_feel_test.c` cannot construct an
explicit "key held" scenario at all — its header comment (`:8-14`) states it relies on the headless
CI environment always reporting "no keys pressed" from the real `SDL_GetKeyboardState()` call inside
`apply_arcade_player_forces`/`apply_runner_player_forces`, simulating a release edge only by
presetting `Man.jumpKeyHeldLastTick = true` beforehand. This is a real testability gap this phase
also closes.

## 2. Full current-state inventory

**Every `SDL_GetKeyboardState()` call site** (3 live, 1 dead/commented):

| Site | Function | Path |
|---|---|---|
| `src/process.c:208` | `apply_arcade_player_forces` | fixed-step (0-5x/frame) |
| `src/process.c:1118` | `apply_runner_player_forces` | fixed-step (0-5x/frame) |
| `src/processEvents.c:110` | `processEvents` | per-real-frame |
| `src/pause_events.c:33` | (commented out) | dead |

**Continuous-key branches** (all in `apply_arcade_player_forces`/`apply_runner_player_forces`,
mirrored for arcade/runner and for player1/player2):
- Jump-held + variable-jump-height release-cut: `state[SDL_SCANCODE_W]`/`[SDL_SCANCODE_UP]`
  compared each tick against `Man.jumpKeyHeldLastTick` (`inc/header.h:150`).
- Movement: `if (state[SDL_SCANCODE_A]) {...} else if (state[SDL_SCANCODE_D]) {...} else {friction}`
  (`src/process.c:232-274`, mirrored for player2 at `:297-339` and for Runner at `:1129-1200`) — left
  already wins on simultaneous press, via explicit `if`/`else if` order, not by accident.

**Shoot** (`src/processEvents.c:231-278`, Arcade only — confirmed no shoot mechanic anywhere in
Runner/`processEvents2`): a held-key check (`SDL_SCANCODE_SPACE`/`SDL_SCANCODE_KP_0`) gated by a
manual cooldown counter (`shotCount`/`shotCountMultiplayer`, reset when `game->time % 23 == 0`) —
not edge-triggered, but already evaluated exactly once per real frame.

**Discrete/edge-triggered gameplay input** (`src/input_command.c`, Command pattern, Phase 9):
`translate_arcade_command()`/`translate_runner_command()` map `SDLK_w`/`SDLK_UP` to
`GAME_COMMAND_JUMP_PLAYER1`/`PLAYER2`, consumed at `src/processEvents.c:67,70`/`:489,492` — already
correctly separated from the fixed-step loop, no change needed.

**Pause/quit/scene-selection** — confirmed structurally separate from gameplay input already:
`src/pause_events.c` and the mode-select `src/menu_events.c:menu_events()` use raw
`switch(event.key.keysym.sym)` (not routed through `input_command.c`, unlike `processEvents`/
`processEvents2`/`menu0_events`), but in all cases route exclusively through `app_change_scene()`
(`src/scene.c`) rather than any field this phase touches. Out of scope for Phase 17, per the user's
own scoping of the gameplay input snapshot.

**Multiplayer key mapping** (confirmed, not centralized in one place today — unchanged by this
phase): Player 1 = `W` (jump) + `A`/`D` (move) + `Space` (shoot, arcade only). Player 2 = `UP`
(jump) + `LEFT`/`RIGHT` (move) + numpad `0` (shoot, arcade only, `game->multiPlayer`-gated).

**Existing tests that touch `InputState`/the continuous-input path**:
- `docs/verification/input_state_test.c` — hand-sets `game->input.jumpBufferTicksPlayer1/2` as
  plain `int` fields; never touches continuous input or calls `apply_*_player_forces`. Unaffected by
  this phase as long as the two jump-buffer fields keep their exact name/type (they do).
- `docs/verification/game_feel_test.c` — calls `apply_arcade_player_forces`/`apply_runner_player_forces`
  directly (lines `120,135,149,164,173,184,192`) and relies on the headless-SDL "no keys held" side
  effect described above. **This phase's implementation commit must update these call sites** to set
  `game->input.jumpHeldPlayer1`/`Player2` (and movement fields, where relevant to a given assertion)
  explicitly instead, preserving every existing assertion's outcome.

## 3. Design decision: simplifications vs. the originally proposed API

Found during audit; adopted here rather than the literal originally proposed shape, each because
the current code already satisfies the underlying goal more directly:

1. **No new parameter on `apply_arcade_player_forces`/`apply_runner_player_forces`.** Both already
   receive `GameState *game`, which already embeds `InputState input` — their bodies change (read
   `game->input.*` instead of calling SDL), signatures don't. Zero call-site argument changes
   anywhere, including both existing test files.
2. **No `input_begin_frame()`.** Continuous fields are fully overwritten by straight assignment
   every capture call; nothing transient needs clearing. Buffered `jumpBufferTicks*` fields are
   untouched by this phase (Phase 15's decay/consume logic already owns their lifecycle correctly).
3. **No `input_handle_arcade_event`/`input_handle_runner_event` wrapper functions.** The existing
   `SDL_PollEvent` loops already set `jumpBufferTicks*` correctly via the Command-pattern
   translation functions; that path is already isolated from the fixed-step loop and needs no
   restructuring.
4. **No `jumpReleasedPlayer1/2` edge fields.** Release-cut already works via the pre-existing
   `Man.jumpKeyHeldLastTick` compared each tick against one frame-frozen `jumpHeldPlayer1` value —
   once capture happens exactly once per real frame (before the tick loop starts), a multi-tick
   frame calls `apply_*_player_forces` several times with an *identical* `jumpHeldPlayer1`, so the
   held-vs-last-tick mismatch (and the resulting cut) can only actually fire on the one tick where
   it's new. "One release edge per frame, not per tick" falls out for free — no new field needed.
5. **No `shootRequestedPlayer1/2` edge fields.** Shoot is a held-key check with a manual
   tick-based cooldown, already evaluated once per real frame in `processEvents()` — it needs only a
   continuous `shootHeldPlayer1/2` field, not buffering across ticks. The cooldown logic itself
   (`:249-278`) is untouched.
6. **Left/right precedence: preserve the existing rule, don't redefine it.** The current
   `if (moveLeft) {...} else if (moveRight) {...}` already resolves simultaneous presses
   deterministically (left wins) — this phase keeps that exact behavior and makes it an explicit,
   tested rule, rather than changing it to a "cancel to zero" rule that would be an observable
   gameplay-feel change with no demonstrated defect motivating it (out of step with this session's
   preserve-unless-proven-broken discipline, e.g. Phase 13/14/15's scoping decisions).

## 4. What Phase 17 actually adds

- `InputState` (`inc/header.h`) gains 6 `bool` fields: `moveLeftPlayer1`, `moveRightPlayer1`,
  `jumpHeldPlayer1`, `shootHeldPlayer1`, and the player-2 mirrors. The 2 existing jump-buffer `int`
  fields are untouched.
- New `inc/input_snapshot.h`/`src/input_snapshot.c`: `input_capture_arcade(InputState*, const
  Uint8*)` (sets all 8 continuous fields) and `input_capture_runner(InputState*, const Uint8*)`
  (sets the 6 movement/jump-held fields only — Runner has no shoot mechanic, so its capture function
  doesn't touch the shoot fields at all). Both pure functions, no `GameState` dependency.
- `src/main.c` calls `SDL_GetKeyboardState(NULL)` once per real frame and calls the matching
  `input_capture_*` function before the fixed-step accumulator's tick loop, so every tick and the
  later `processEvents`/`processEvents2` call that same frame see one consistent snapshot.
- `apply_arcade_player_forces`/`apply_runner_player_forces` and `processEvents()`'s shoot block read
  `game->input.*` instead of calling `SDL_GetKeyboardState` themselves.
- `arcade_session_reset()`/`runner_session_reset()` (`src/loadGame.c`) explicitly zero the 6 new
  fields alongside the existing jump-buffer resets (defensive, matching the Phase 2 calloc
  precedent — not strictly required for correctness since capture always runs before any consumer
  reads these fields each frame, but cheap and gives the new test something concrete to assert).

## 5. Deliberately not done (found, documented, not touched)

- AI/enemy input or physics — Phase 18.
- Any collision-ordering change — Phase 19.
- Render interpolation — Phase 20.
- Shoot cooldown/rate redesign, projectile mechanics, key rebinding, controller support.
- Pause/quit/scene-selection handling — confirmed structurally separate already; stays outside the
  gameplay input snapshot per the user's own scoping.
- Left/right simultaneous-press behavior change (see section 3, item 6).
