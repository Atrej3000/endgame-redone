# Input/Simulation Separation Map — Ucode_Endgame

Written **before** any Phase 12 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `8410b18` (tag
`refactor-pre-input-simulation-separation`, `main` after Phase 11's PR #7 merge) via direct reads
of every relevant function, immediately before this document was written.

## 1. Current state — the continuous half was fixed in Phase 11, the discrete half was not

Phase 11 (`docs/physics-timestep-map.md`) relocated *continuous* held-key forces (horizontal
accel/clamp, jump-hold thrust, friction/snap) into `apply_arcade_player_forces`/
`apply_runner_player_forces` (`src/process.c`), called by `arcade_simulate`/`runner_simulate`
(`src/frame.c`) at the fixed 60Hz tick rate. That work is correct and unaffected by this phase.

What Phase 11 did **not** touch: the *discrete* jump-trigger case bodies inside
`processEvents()`/`processEvents2()` (`src/processEvents.c:63-82` Arcade, `:493-526` Runner).
These functions are called **exactly once per real frame** (`src/main.c:70-71,100-104`), *after*
the fixed-tick `while` loop has already run 0–`MAX_PHYSICS_STEPS_PER_FRAME` (5) times that frame
(`src/main.c:60-69,90-99`). The jump case reads `game->man.onLedge`/`game->secondPlayer.onLedge`
(written only by `collisionDetect`/`collisionDetect2`, which run only inside
`arcade_simulate`/`runner_simulate`) and assigns `dy` directly, inline, synchronously — gated only
by SDL's own per-frame event-queue delivery, with:
- **No de-duplication concept**: nothing prevents (or needs to prevent, today) multiple triggers,
  but nothing *guarantees* exactly-once consumption relative to the physics tick either — the
  coupling is implicit, not designed.
- **No buffering**: if a real frame produces zero physics ticks (`accumulator < PHYSICS_DT`), the
  jump is still evaluated inline in `processEvents`, once, against whatever `onLedge` was left by
  the *previous* frame's last tick — not a bug today (this is deterministic, not stale in a
  harmful way, since game state simply hasn't progressed), but architecturally coupled to the
  render/event frame rate rather than the physics tick rate, exactly the shape of problem the
  physics assessment's own "Phase 2 — separate input and simulation" names.

Exact current code (Arcade, `src/processEvents.c:63-82`):
```c
case GAME_COMMAND_JUMP_PLAYER1:
    if (game->man.onLedge)
    {
        game->man.dy = -JUMP_SPEED_PER_SEC;
        game->man.onLedge = 0;
        Mix_VolumeChunk(game->jumpSound, 32);
        Mix_PlayChannel(-1, game->jumpSound, 0);
    }
    break;
case GAME_COMMAND_JUMP_PLAYER2:
    if (game->secondPlayer.onLedge)
    {
        game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
        game->secondPlayer.onLedge = 0;
        Mix_VolumeChunk(game->jumpSound, 32);
        Mix_PlayChannel(-1, game->jumpSound, 0);
    }
    break;
```
Note: `GAME_COMMAND_JUMP_PLAYER2` fires regardless of `game->multiPlayer` — no gating check exists
in the original code. Preserved exactly, not "fixed," by this phase (out of scope; not a
timestep/input-separation concern).

## 2. Regression found during this audit: Runner's jump impulse was never converted

Runner's two jump-impulse sites (`src/processEvents.c:496,520`) are still:
```c
game->man.dy = -10;            // line 496
game->secondPlayer.dy = -10;   // line 520
```
— bare frame-tuned literals, **not** `-JUMP_SPEED_PER_SEC` (`600.0f`, `inc/header.h:71`, whose own
comment reads `// was dy = -10/frame (one-shot impulse)`, confirming this conversion was intended
universally, not Arcade-only). Arcade's two equivalent sites (`processEvents.c:66,76`) **were**
correctly converted to `-JUMP_SPEED_PER_SEC`. Confirmed by direct grep and read — not assumed.

**Impact**: `process2()` (Runner) now integrates `dy` as pixels-per-**second** at a fixed
`dt = 1/60` (Phase 11). A `dy` of `-10` px/s is **60× weaker** than the intended `-600` px/s —
Runner's jump has produced a barely-perceptible hop instead of a real jump ever since Phase 11
merged into `main` (commit `8410b18`). This is a real, user-visible behavior regression, not a
cosmetic inconsistency, and is not something this phase merely happens to touch in passing — the
jump-request relocation below routes both modes through the same corrected constant, closing the
gap as a direct consequence of the redesign. Called out explicitly in the PR rather than silently
folded in.

## 3. `InputState` design

New nested struct on `GameState` (`inc/header.h`, same pattern as Phase 10's `AppContext`/
`AssetLifecycleFlags`):
```c
typedef struct InputState
{
    bool jumpRequestedPlayer1; // edge-triggered: set on SDL_KEYDOWN in processEvents/2,
    bool jumpRequestedPlayer2; // consumed (and cleared) by the next physics tick
} InputState;
```
`processEvents()`/`processEvents2()`'s jump case bodies shrink to a single flag-set
(`game->input.jumpRequestedPlayer1 = true; break;`) — no more inline `onLedge` check, `dy`
assignment, or SFX playback there. Two new functions in `src/process.c`,
`consume_arcade_jump_requests`/`consume_runner_jump_requests`, run at the fixed tick rate
(called from `arcade_simulate`/`runner_simulate`, `src/frame.c`, immediately **before**
`apply_arcade_player_forces`/`apply_runner_player_forces` — preserving the original relative
ordering, since the old `processEvents` applied the keydown jump impulse before its own later
held-key jump-hold-thrust check in the same real frame, allowing both to stack in one frame; this
order is preserved so a keydown can still add jump-hold thrust in the same tick). Each consumption
function: if the request flag is set, checks `onLedge`; if grounded, assigns
`dy = -JUMP_SPEED_PER_SEC` (fixing the Runner regression for both modes uniformly), clears
`onLedge`, plays the jump SFX — then **unconditionally clears the request flag**, whether or not
the jump actually fired. This matches the original one-shot-per-keydown semantics exactly: a
keydown while airborne was always silently dropped, never buffered, in the old code too.

**Why this closes the assessment's named gap**: a single `SDL_KEYDOWN` sets the flag exactly once.
Only the *first* physics tick that runs afterward — whether later in the same real frame (if
`steps < MAX_PHYSICS_STEPS_PER_FRAME` ticks execute), or the first tick of a later frame (if this
frame produced zero ticks) — consumes and clears it. One input edge produces at most one jump,
never dropped across a zero-tick frame, never double-applied across a multi-tick frame, regardless
of the real display's refresh rate. This is the literal meaning of "edge-triggered
de-duplication" from the assessment's own wording, achieved without a queue, heap allocation, or
new abstraction beyond two boolean fields — consistent with `inc/input_command.h`'s own stated
design philosophy ("deliberately not a queue or heap-allocated command object").

**Known, deliberate side effect**: in the zero-tick-this-frame case, jump latency can grow by up to
one real frame relative to today's immediate synchronous assignment. Today's model has no
buffering at all — a keydown during a real stall is simply evaluated against whatever `onLedge`
currently holds, synchronously, with no delay but also no correctness guarantee tying it to a
specific tick. The new model trades a theoretical, bounded (≤1 frame) latency increase for an
explicit, provable one-edge-one-jump guarantee. Documented, not hidden.

**Session reset**: `arcade_session_reset`/`runner_session_reset` (`src/loadGame.c`) already zero
`man.dy`/`onLedge`/`secondPlayer.dy`/`onLedge` explicitly on every new session
(`loadGame.c:172,174,212,214` Arcade; `:425,427,439,441` Runner). This phase adds
`game->input.jumpRequestedPlayer1 = false; game->input.jumpRequestedPlayer2 = false;` to both,
preventing a request flag left set from a previous session (e.g. quit to menu mid-air, the instant
after a keydown but before the next tick consumed it) from firing an unwanted instant jump at the
start of the next session.

## 4. Deliberately out of scope (found during this audit, documented, not touched)

- **Pause / quit / scene-transition commands** (`GAME_COMMAND_PAUSE`, `_QUIT_TO_MODE_MENU`,
  `_QUIT_TO_MAIN_MENU`): true one-shot commands with no fixed-tick-owned gating state read in a
  timing-sensitive way — no staleness problem of the jump's shape exists here.
- **Collision-derived scene transitions** (`processEvents.c:403-423` Arcade, `:596-613` Runner —
  `man.lives == 0` → `app_change_scene`) and the **Runner fall-death trigger**
  (`processEvents.c:583-594` — reads `man.y`/`man.x`, written by `process2` inside the fixed-tick
  loop): the *same shape* of "processEvents reads tick-owned state once per real frame" issue as
  jump, but for collision/death *consequences*, not player *input*. This is the assessment's own
  Phase 3 ("correct collisions") territory, not Phase 2 — a future phase's job.
- **Shooting** (`SDL_SCANCODE_SPACE`/`KP_0` held-check + `shotCount` re-arm timer,
  `processEvents.c:242-263,264-290`): already partially tick-rate-correct today, since
  `game->time` (the re-arm timer's gate) now advances at the fixed tick rate as a structural
  consequence of Phase 11. A full move into the `InputState`/tick-consumption model would need
  bullet-pool/projectile-ownership decisions — the assessment's own Phase 4 ("correct
  projectiles"), not Phase 2.
- **Runner's `SDLK_0` cheat code** (`processEvents.c:463-467`), **menu/pause raw event handling**
  (`src/menu_events.c`, `src/pause_events.c`) — not gameplay-physics input, untouched.

## 5. Test strategy

`docs/verification/input_state_test.c` (new, `mingw-inputtest` target), driving
`consume_arcade_jump_requests`/`consume_runner_jump_requests` directly with hand-set `GameState`
fields (no real event loop, no real keyboard — matching every prior phase's headless-test
convention):
1. **Edge-triggered single consumption**: request set + grounded → jump fires once, flag clears; a
   second consume call (simulating a second physics tick in the same real frame) with the flag
   already cleared does not re-fire even if re-grounded in between.
2. **Airborne request dropped, not buffered**: request set + airborne → no jump, flag still clears
   (matches the original's silent-drop semantics, not an infinite retry).
3. **Runner regression-fix proof**: Runner's consumption function produces
   `dy == -JUMP_SPEED_PER_SEC`, not `-10` — a direct regression test for the bug found in §2.
4. **Player-2 path**, both modes, mirroring 1–2.
5. **Full-pipeline integration**: one real `arcade_simulate`/`runner_simulate` call with the
   request flag set confirms the new function wires correctly into the existing simulate order —
   `dy` after one tick equals `-JUMP_SPEED_PER_SEC + GRAVITY_PER_SEC2*PHYSICS_DT` (jump impulse
   plus one tick's worth of gravity, integrated in `process`/`process2` immediately after).
