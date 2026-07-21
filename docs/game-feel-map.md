# Game Feel Map — Ucode_Endgame

Written **before** any Phase 15 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `308b83b` (tag `refactor-pre-game-feel`, `main`
after Phase 14's PR #10 merge) via a complete read of `src/process.c`'s jump-related functions,
`inc/header.h`'s `Man`/`InputState` structs and jump constants, and `docs/verification/input_state_test.c`,
immediately before this document was written.

## 1. No coyote time, no jump buffering exist today

`consume_arcade_jump_requests`/`consume_runner_jump_requests` (`src/process.c:134-161`,
`:1002-1029`) gate the jump impulse strictly on `game->man.onLedge`/`game->secondPlayer.onLedge`:
```c
if (game->input.jumpRequestedPlayer1)
{
    if (game->man.onLedge)
    {
        game->man.dy = -JUMP_SPEED_PER_SEC;
        game->man.onLedge = 0;
        Mix_VolumeChunk(game->jumpSound, 32);
        Mix_PlayChannel(-1, game->jumpSound, 0);
    }
    game->input.jumpRequestedPlayer1 = false;
}
```
If not grounded, the jump doesn't fire — and the request flag is cleared unconditionally
regardless. There is no per-player "ticks since grounded" counter or request-buffer of any kind
anywhere in `Man`, `InputState`, or `GameState` (confirmed by full struct reads, §3/§4 below).

**This exact behavior is directly asserted by the existing test suite** —
`docs/verification/input_state_test.c:67-81`, under the comment *"Airborne request is dropped (not
buffered) — matches the original's silent-drop semantics: a keydown while airborne was never
queued for later, only ever checked once"* — asserting both that the jump doesn't fire and that
`game->input.jumpRequestedPlayer1 == false` immediately after. This phase's coyote time and jump
buffering **deliberately supersede this exact assertion** — not silently, per §5.

## 2. The existing "variable jump height" mechanism is a continuous hold-thrust, not a release-cut

`apply_arcade_player_forces` (`src/process.c:173-293`) and `apply_runner_player_forces`
(`:1038-1114`) each contain a `SDL_SCANCODE_W`/`SDL_SCANCODE_UP` held-check that adds continuous
upward acceleration every tick the key is held:
```c
if (state[SDL_SCANCODE_W])
{
    game->man.dy -= ARCADE_JUMP_HOLD_ACCEL_PER_SEC2 * dt;   // Runner: RUNNER_JUMP_HOLD_ACCEL_PER_SEC2
    ...
}
```
Confirmed by direct read: **no `onLedge` gate exists on this block, and no clamp on the resulting
`dy` exists anywhere in `process()`/`process2()`** (unlike the horizontal `RUN_MAX_SPEED_PER_SEC`
clamps applied to `dx`). Two concrete consequences:
- Holding the key **while already grounded** (before jumping, or after landing while still
  holding) adds upward thrust regardless — this reduces or reverses the effect of gravity on a
  standing player, entirely independent of whether `onLedge`/a jump request are involved at all.
- Holding the key **for an extended time mid-air** adds unbounded upward velocity, limited only by
  how long the key is physically held, not by any code-level cap.

This is refresh-rate-independent in *rate* since Phase 11 (converted to `ACCEL_PER_SEC2 * dt`),
but was never redesigned in *character* — `docs/physics-timestep-map.md` section 6 named this
exact mechanic as deferred to "assessment's own Phase 5."

## 3. No held→released transition detection exists anywhere

`apply_arcade_player_forces`/`apply_runner_player_forces` call `SDL_GetKeyboardState(NULL)` fresh
every tick and only ever test the current-tick level (`if (state[SDL_SCANCODE_W])`) — a pure
polling check with no memory of the previous tick. Confirmed by full-file grep: no `SDL_KEYUP`
case is handled anywhere in `processEvents()`/`processEvents2()`'s event switches, and no
"previous key state" field exists on `Man`, `InputState`, or `GameState`. A release-cut mechanism
needs genuinely new state — there is nothing to repurpose.

**Full current `Man` struct** (`inc/header.h:120-139`) confirms no existing airborne-duration
counter or held-state field: `x, y, w, h, dx, dy, prevY` (Phase 13), `lives, name, onLedge,
isDead`, animation/sprite fields, texture pointers. **Full current `InputState` struct**
(`inc/header.h:276-280`) confirms exactly two `bool` fields: `jumpRequestedPlayer1`,
`jumpRequestedPlayer2` — no buffer countdown, no coyote countdown.

## 4. Design

**User-approved decision** (a genuine game-feel fork, not resolvable from code alone — asked and
answered before this document was finalized): **replace** the continuous hold-thrust entirely with
the standard "full impulse at jump start, cut short on early release" technique, rather than
layering a release-cut on top of the existing thrust. Layering both would leave two overlapping
height mechanisms and the grounded-thrust quirk (§2) unaddressed; replacing gives one clean
mechanism and fixes the quirk as a direct consequence (no unconditional per-tick `dy` modification
survives at all — only a one-time cut check on the release edge).

**New constants** (`inc/header.h`): `COYOTE_TICKS` (`6`, ~100ms at the fixed 60Hz tick),
`JUMP_BUFFER_TICKS` (`6`, ~100ms), `JUMP_CUT_SPEED_PER_SEC` (`200.0f`, roughly a third of
`JUMP_SPEED_PER_SEC`'s `600.0f`). Unlike Phase 14's `BULLET_SPEED_PER_TICK`, these have **no
legacy value to preserve** — this is genuinely new behavior, not a fixed-timestep conversion of
existing behavior. Documented plainly as reasonable, tunable starting defaults, not a
derived/proven constant; retuning later is a one-line change since they're named.

**`Man` gains** `int coyoteTicksRemaining;` (refreshed to `COYOTE_TICKS` every grounded tick,
counts down while airborne) and `bool jumpKeyHeldLastTick;` (last tick's held-state, for
release-edge detection). Both get explicit resets in `arcade_session_reset`/`runner_session_reset`
— same reasoning Phase 12 established for the original request flags: a stale nonzero coyote
counter or stale held-flag carried over from a previous session could grant an undeserved
coyote-jump or misfire a cut at the start of a new one.

**`InputState` retypes** `bool jumpRequestedPlayer1/2` → `int jumpBufferTicksPlayer1/2` — the
minimal representation change that serves both roles at once: nonzero means "a jump is pending,
within its buffer window." `processEvents()`/`processEvents2()`'s `GAME_COMMAND_JUMP_PLAYER1/2`
cases set it to `JUMP_BUFFER_TICKS` (previously `= true`).

**Coyote time + jump buffering**, both inside `consume_arcade_jump_requests`/
`consume_runner_jump_requests`, run every tick:
```c
// Coyote time: refreshed while grounded, counts down while airborne.
if (game->man.onLedge) { game->man.coyoteTicksRemaining = COYOTE_TICKS; }
else if (game->man.coyoteTicksRemaining > 0) { game->man.coyoteTicksRemaining--; }

// Jump buffering, gated by coyote-or-grounded.
if (game->input.jumpBufferTicksPlayer1 > 0)
{
    if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
    {
        game->man.dy = -JUMP_SPEED_PER_SEC;
        game->man.onLedge = 0;
        game->man.coyoteTicksRemaining = 0;   // prevents a double-jump within the same window
        game->input.jumpBufferTicksPlayer1 = 0;
        Mix_VolumeChunk(...); Mix_PlayChannel(...);
    }
    else
    {
        game->input.jumpBufferTicksPlayer1--; // still pending; expires after JUMP_BUFFER_TICKS
    }
}
```
A jump pressed slightly before landing persists for `JUMP_BUFFER_TICKS` ticks and fires the
instant the player becomes grounded or is still within coyote range; if neither happens before the
window expires, it decays to `0` with no jump. Zeroing `coyoteTicksRemaining` on fire is essential:
without it, a second buffered request arriving in the same still-open coyote window would also
succeed — an unintended free double-jump.

**Variable jump height (release-cut)**, replacing the hold-thrust in `apply_arcade_player_forces`/
`apply_runner_player_forces`'s four `SDL_SCANCODE_W`/`UP` blocks:
```c
bool wHeld = state[SDL_SCANCODE_W];
if (wHeld) { /* existing animation/slowingDown side effects, unchanged, purely cosmetic */ }
else if (game->man.jumpKeyHeldLastTick && game->man.dy < -JUMP_CUT_SPEED_PER_SEC)
{
    game->man.dy = -JUMP_CUT_SPEED_PER_SEC;
}
game->man.jumpKeyHeldLastTick = wHeld;
```
Releasing early while still rising fast (`dy` more negative than the cut bound) caps the ascent;
holding through to apex lets gravity decay `dy` naturally, and once it crosses above the cut
threshold on its own, a later release has no effect — already a "full" jump. `ARCADE_JUMP_HOLD_ACCEL_PER_SEC2`/
`RUNNER_JUMP_HOLD_ACCEL_PER_SEC2` are removed entirely — confirmed via grep that the 4 sites this
phase touches are their only references anywhere in `src/`/`inc/`.

## 5. Exactly what changes in `docs/verification/input_state_test.c`, and why

Section 2 of that file (comment: *"Airborne request is dropped (not buffered)"*) is rewritten to
assert the opposite of its current claim: an airborne request without an available coyote window
now **persists** (the buffer counter decrements, doesn't zero) rather than clearing immediately —
and a new assertion proves it later fires if the player lands within the buffer window. Every
other `== true`/`== false` comparison against `jumpRequestedPlayer1/2` throughout the file becomes
`> 0`/`== 0` against the renamed, retyped `jumpBufferTicksPlayer1/2`. This is a deliberate,
documented supersession of a previously-locked-in invariant — not a silent behavior change slipped
past an old test.

## 6. Deliberately out of scope (found, documented, not touched)

- Enemy/boss jump-like behavior (bosses/smart-enemies use their own discrete impulses, unrelated
  to player jump feel).
- Runner's fall-death trigger, ledge-collision logic, ceiling-bug fix — Phase 13's completed
  territory, not re-touched.
- Tuning `COYOTE_TICKS`/`JUMP_BUFFER_TICKS`/`JUMP_CUT_SPEED_PER_SEC` through actual playtesting —
  not possible without a display in this environment; documented as reasonable starting defaults.
- Double-jump, air-dash, wall-jump, or any feel mechanic beyond the assessment's named three.

## 7. Test strategy

`docs/verification/game_feel_test.c` (new, `mingw-gamefeeltest` target), calling
`consume_arcade_jump_requests`/`consume_runner_jump_requests`/`apply_arcade_player_forces`/
`apply_runner_player_forces` directly with hand-set `GameState` fields: (a) coyote time — a jump
requested within `COYOTE_TICKS` of leaving a ledge still succeeds; one tick past the window, it
doesn't; (b) jump buffering — a jump requested while airborne succeeds the instant the player
lands within `JUMP_BUFFER_TICKS`; past the window with no landing, the buffered request expires
with no jump; (c) variable jump height — releasing the jump key early while still rising fast cuts
`dy` to `-JUMP_CUT_SPEED_PER_SEC`; holding through (or releasing after `dy` has already decayed
past the cut threshold) leaves `dy` unaffected; (d) the removed grounded-hold-thrust quirk is
gone — holding W while grounded no longer modifies `dy` at all.
