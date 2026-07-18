# Runner Death-State Lifecycle — Ucode_Endgame

Written **before** any Phase 6 code edit, per that phase's own requirement. Describes the exact
current (broken) lifecycle at commit `3419fc0` (tag `refactor-pass-5-frame-pipeline`), every
read/write site of the relevant fields with file:line references (re-verified by direct reading
immediately before this document was written), the confirmed defect classification, and the
repaired sequence this phase implements.

## Fields involved

| Field | Type | Declared | Scope |
|---|---|---|---|
| `game->man.isDead` | `int` | `inc/header.h:80` (`Man` struct) | Shared — one flag for the whole death sequence, used by both `man` and (indirectly) `secondPlayer` in Runner |
| `game->secondPlayer.isDead` | `int` | same `Man` struct (secondPlayer is a second `Man` instance) | **Arcade-exclusive** — never referenced anywhere in Runner code (confirmed by grep) |
| `game->deathCountdown` | `int` | `inc/header.h:316` | `GameState`-level, shared across modes and players |
| `game->gameLives` | `int` | `inc/header.h:207` | `GameState`-level, shared — Runner's sole authoritative life counter (see below) |
| `game->man.lives` / `game->secondPlayer.lives` | `short` | `inc/header.h:78` (`Man` struct) | **Arcade-exclusive** — never referenced anywhere in Runner code (confirmed by grep) |

## Life ownership — resolved

`status.c`'s `sprintf(str, "Lifes %d", (int)game->gameLives);` is the one HUD text generator,
called identically from both `doRender()` (Arcade) and `doRender2()` (Runner) via
`draw_status_lives()`/`init_status_lives()`. This confirms `gameLives` is the field actually
displayed to the player in both modes. Runner's own game-over check
(`processEvents.c`, `if (game->gameLives == 0) { ... }`) also tests `gameLives` directly.
`man.lives`/`secondPlayer.lives` are set and read exclusively by Arcade code
(`loadGame.c:177/218` — `arcade_session_reset()`; `processEvents.c:400-540` — Arcade's
`processEvents()`) — confirmed via grep that Runner's `process2()`/`collisionDetect2()`/
`processEvents2()`/`runner_session_reset()` never reference either field.

**Conclusion: `gameLives` is Runner's single authoritative life counter. `man.lives`/
`secondPlayer.lives` are simply unused by Runner — not redundant fields needing synchronization,
but two independent, non-overlapping life systems by design** (Arcade: per-player `.lives` flags
gated by a separate `gameLives` kill-counter; Runner: one shared `gameLives` pool, no per-player
tracking). No new synchronization code is introduced by this phase.

## Current (pre-Phase-6) lifecycle, traced exactly

### Every read/write site

- **`collisionDetect.c:625-639`** (`collisionDetect2()`, collision phase, runs before render):
  ```c
  for (int i = 0; i < 100; i++)
  {
      if (collide2d(man vs stars[i])) {
         // game->man.isDead = 1;      <- commented out
          game->gameLives = 0;         <- live: instant, unconditional game over
  //      game->man.isDead = 0;        <- commented out
      }
      if (collide2d(secondPlayer vs stars[i])) {
          game->gameLives = 0;         <- live: same shortcut, no isDead reference at all
      }
  }
  ```
  The commented-out `isDead=1`/`isDead=0` pair directly bracketing the live `gameLives=0` line is
  strong evidence of an abandoned design: trigger a death *sequence* (animate, then lose one
  life), later replaced by a cruder instant-zero shortcut. `secondPlayer`'s block never had an
  `isDead` reference at all — multiplayer death was always routed through the shared `gameLives`
  pool only.
- **`process.c:1003`** (`process2()`, update phase): `if (!game->man.isDead || game->gameLives >
  0)` gates `man`'s movement/gravity — note the **OR**: movement currently still applies even
  while `isDead==1`, as long as lives remain.
- **`process.c:1028-1048`** (`process2()`, update phase): `secondPlayer`'s movement/gravity block
  has **no `isDead` gate of any kind** — unconditional every frame, regardless of anyone's death
  state.
- **`process.c:1140-1156`** (`process2()`, update phase) — **the confirmed severe defect**:
  ```c
  if (game->man.isDead && game->deathCountdown < 0) { game->deathCountdown = 120; }
  if (game->deathCountdown > 0) {
      game->gameLives--;
      // game->deathCountdown--;  <- commented out: the decrement never happens
  }
  ```
  `runner_session_reset()` (`loadGame.c:455`) sets `deathCountdown = -1`. The very first frame
  `man.isDead` is ever true, this flips `deathCountdown` to `120` (since `120 > -1`... actually
  since `deathCountdown < 0` is true at `-1`), then immediately `deathCountdown > 0` is true, so
  `gameLives--` fires. Because `deathCountdown--` is commented out, `deathCountdown` stays at
  `120` **forever** — meaning `gameLives--` fires **every subsequent frame, unboundedly**, with no
  relationship to whether `isDead` is still true.
- **`frame.c`** (Phase 5's `runner_resolve_death()`, called from `runner_frame()` after
  `doRender2()`, before `processEvents2()`): `if (game->man.isDead) { game->gameLives--;
  game->man.isDead = 0; game->man.y = 0; }` — a second, independent decrement, on the same frame
  `isDead` first becomes true.
- **`doRender.c:235-239`** (`doRender2()`, render phase): `if (game->man.isDead) { ...draw the
  death sprite, blinking via time%20<10... }` — confirmed render-pure (Phase 5), no mutation. Only
  ever draws at `man`'s position; no equivalent `secondPlayer` death sprite exists anywhere.
- **`processEvents.c`** (`processEvents2()`, events phase, runs last): fall-death checks
  (`man.y>=719 || man.x<0`, and the `secondPlayer` equivalent inside `if (multiPlayer)`) also do
  `game->gameLives = 0;` directly — the same instant-zero shortcut as the star-collision sites.
  The actual game-over check, `if (game->gameLives == 0) { ...score-persist (unconditional)...;
  if (game->scene == APP_SCENE_RUNNER_GAME) { app_change_scene(game, APP_SCENE_RUNNER_MENU); } }`,
  is already correct and double-transition-guarded (a Phase 5 fix) — **not touched this phase**.
- **`loadGame.c:420-478`** (`runner_session_reset()`): sets `gameLives=3`, `man.isDead=0`,
  `man.dy=0`, `deathCountdown=-1`, and (only if multiplayer) `secondPlayer.isDead=0`. Does not
  reset `man.x`/`secondPlayer.x`/`secondPlayer.y`/`secondPlayer.dy` specifically for death
  purposes (they're covered by the broader session-reset assignments already present).

### Traced consequence — why this is a severe, not merely theoretical, defect

Assume `gameLives == 1` at the moment `man.isDead` first becomes `1` (i.e., a death that should be
the final, game-ending one):

1. **Frame N** — `process2()` runs: `isDead=1`, `deathCountdown=-1` → flips to `120` →
   `deathCountdown>0` → `gameLives--` (`1 → 0`). *(First decrement, inside `process2()` itself.)*
   `collisionDetect2()` runs (no further effect). `doRender2()` draws the death sprite (`isDead`
   still `1`). `runner_resolve_death()` runs: `isDead` still `1` → `gameLives--` (`0 → -1`)
   *(second decrement)*, `isDead=0`, `man.y=0`. `processEvents2()` runs: checks
   `if (gameLives==0)` — but `gameLives` is now `-1`, not `0` — **the check fails**, no game-over
   transition fires this frame.
2. **Frame N+1** — `isDead` is now `0` (cleared), but `deathCountdown` is **still `120`** (never
   decremented) → `process2()`'s block fires again: `deathCountdown>0` → `gameLives--`
   (`-1 → -2`). *(Third decrement, with no trigger at all this time.)*
3. **Every subsequent frame** — the same unconditional `gameLives--` repeats forever (`deathCountdown`
   is permanently stuck at `120`), since nothing in the current code ever resets it back to a
   non-positive value once `runner_resolve_death()`'s own reset (`isDead=0`) has already happened
   and nothing else touches `deathCountdown`. `gameLives` races toward increasingly negative
   numbers, the HUD shows an increasingly negative "Lifes" count, and — critically —
   `gameLives==0` is **never true again** (it overshot from `1` straight past `0` to `-1` in a
   single frame and only moves further away from `0` from then on), so **the game-over scene
   transition never fires**. The player is stuck in `APP_SCENE_RUNNER_GAME` indefinitely, with a
   spiraling negative life count, no game over, no recovery.

## Defect classification

- **Currently reachable in live play: no.** Confirmed via grep: nothing anywhere in the live
  codebase sets `man.isDead = 1` (the only occurrence is the commented-out line in
  `collisionDetect.c`, plus test-only harness code). Both star collision and falling currently
  bypass `isDead` entirely via the direct `gameLives = 0` shortcut.
- **Reachable and severely broken the instant it's wired up: yes**, as traced above — not a
  theoretical concern, a guaranteed-reproducible sequence given `gameLives` starts at a small
  positive integer and `deathCountdown` starts at `-1`.
- **Test-only reachability today**: the closest thing to a live trigger is
  `docs/verification/frame_pipeline_test.c` (Phase 5), which forces `man.isDead = 1` directly to
  test `doRender2()`'s render purity and the relocated mutation — that test already had to work
  around this exact interaction (resetting `deathCountdown` to `0` before asserting, to isolate
  the mutation being tested from this pre-existing bug), which is what first surfaced it plainly.
- **Per this phase's own instruction ("do not preserve incorrect behavior merely because it is
  currently unreachable") and the confirmed product decision to wire the trigger into live
  collision/fall detection: this phase repairs the mechanism and connects it, rather than leaving
  it dormant.**

## Repaired lifecycle (this phase)

```
alive
  -> death trigger (star collision OR falling, either player)
       runner_trigger_death(game): idempotent -- no-op if already dying
       sets man.isDead=1, deathCountdown=120 (existing constant, reused verbatim)
  -> death visual
       doRender2() draws the death sprite every frame isDead==1, unchanged, still render-pure
       player movement (man AND secondPlayer) frozen for the duration (see below)
  -> countdown
       runner_update_death(game), called once per frame from runner_frame() (after doRender2(),
       before processEvents2() -- same relative position runner_resolve_death() held)
       decrements deathCountdown each frame while > 0; does nothing else during this window
  -> life decrement (exactly once, at countdown-end)
       gameLives-- fires exactly once, the frame deathCountdown reaches 0
  -> respawn or game over
       respawn: isDead=0, man.y=0, man.dy=0, man.x clamped to 0 if negative (secondPlayer mirrors
                this if multiplayer); deathCountdown reset to -1 (idle sentinel)
       game over: processEvents2()'s existing, already-correct, already-guarded
                  `if (gameLives==0) { score-persist; app_change_scene(..., APP_SCENE_RUNNER_MENU); }`
                  sees the countdown-end decrement in the same frame it happens -- fires once,
                  correctly, no code change needed here
```

### Why the respawn position-reset had to include more than `y`

An earlier design draft (caught during Plan-mode validation, before any code was written) reset
only `y`/`dy` at respawn — matching what the old, never-functional code did. This is insufficient
now that falling is *also* wired into the trigger: if a player dies by falling off the left edge
(`man.x < 0`), and movement is frozen during the countdown (see below), `man.x` never changes for
the full 120 frames — so if `x` isn't also clamped back to a safe value at respawn, the exact same
`man.x < 0` condition is still true the instant `isDead` clears, and `runner_trigger_death()` fires
again immediately. With movement frozen, the player has **no way to escape this** — it would
silently drain all 3 lives in roughly 6 seconds on the single most common way to die in this mode.
The fix: clamp `man.x` (and `secondPlayer.x`, mirrored) to `0` **only if it's currently negative**
— the minimal correction for the one condition that's actually unsafe, not a full position reset
(which would discard the player's forward progress unnecessarily, unlike a full new-game start).

### Why `secondPlayer` needed the same treatment

`secondPlayer`'s movement in `process2()` had **no `isDead` gate at all** before this phase —
gravity (`dy += GRAVITY`) applied unconditionally every frame, including during another player's
(or its own) death countdown. Combined with no respawn-position reset for `secondPlayer`
whatsoever, a multiplayer session could drain lives continuously with no fix in the pre-Phase-6
design. This phase gates `secondPlayer`'s movement on the same shared `man.isDead` flag (death is
a shared, "pause the world" event for both players in Runner, consistent with the single shared
`gameLives` pool) and applies the identical respawn-position clamp to `secondPlayer`.

### Death sprite timing — stated explicitly, not left implicit

The death sprite is visible starting the **same frame** `runner_trigger_death()` fires (since
`doRender2()`'s `if (isDead)` check and the trigger both happen within the same `runner_frame()`
call, trigger before render) and remains visible for the full 120-frame countdown, disappearing
the frame `isDead` is cleared (countdown-end). This is a **new, real visual behavior** compared to
today's live game (where death is currently instant, with no animation ever shown) — stated here
plainly as the intended, confirmed behavior change, not a preserved timing.

### Movement freeze — deliberate, not incidental

`process2()`'s man-movement gate changes from `if (!game->man.isDead || game->gameLives > 0)` to
plain `if (!game->man.isDead)`. `secondPlayer`'s movement gains the same gate (new — it had none
before). This is necessary for two reasons: it keeps the death sprite's position visually stable
for the whole animation, and — combined with the respawn-position clamp above — it prevents
gravity/momentum from creating a *new* unsafe position while frozen. Background/parallax/star
oscillation logic is **not** gated; there is no evidence it needs to freeze, and doing so would be
unjustified scope creep beyond the confirmed defect.

## Multiplayer — deliberate choice, documented

Both players share one `gameLives` pool and one `isDead`/`deathCountdown` state (matching the
pre-existing shared-pool design — Runner never had per-player Runner lives). A `secondPlayer`
star-collision or fall-death now triggers the same shared death sequence, and the death sprite
remains anchored to `man`'s position regardless of which player triggered it — a pre-existing
visual asymmetry (no `secondPlayer` death sprite has ever existed) that this phase does not build
out further, since doing so would be new visual work outside this phase's charter.

## What is NOT changed

- The game-over destination (`APP_SCENE_RUNNER_MENU`) — preserved exactly. No evidence the
  intended destination was ever `APP_SCENE_RUNNER_LEADERBOARD`.
- The score-persist step (`x_list[x_i] = x_score`) — stays unconditional, exactly as Phase 5 left
  it.
- Background/star/scroll logic — not gated on death state.
- Arcade's own `isDead`/`deathCountdown` handling in `process()` (lines 883-924) — untouched;
  Arcade's mechanism is separate, already working (its own `deathCountdown--` is not commented
  out), and out of this phase's scope entirely.
