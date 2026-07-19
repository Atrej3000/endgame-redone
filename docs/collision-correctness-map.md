# Collision Correctness Map — Ucode_Endgame

Written **before** any Phase 13 code edit, per this session's established audit-first pattern.
Evidence gathered directly from the tree at commit `c7fb663` (tag
`refactor-pre-collision-correctness`, `main` after Phase 12's PR #8 merge) via a complete read of
`src/collisionDetect.c` (755 lines) and its supporting structs, immediately before this document
was written.

## 1. The core bug: `onLedge` is never reset at the start of a collision pass

`src/collisionDetect.c` never unconditionally clears `man.onLedge`/`secondPlayer.onLedge` before
evaluating a tick's collisions, in either `collisionDetect()` (Arcade) or `collisionDetect2()`
(Runner). The field only changes via two narrow paths:
- a ceiling-bump branch inside the man/secondPlayer ledge-collision loop (correct in Arcade,
  buggy in Runner — see §2), or
- `consume_arcade_jump_requests`/`consume_runner_jump_requests` (`src/process.c`, Phase 12)
  clearing it when a jump is actually consumed while grounded.

**Consequence, confirmed by direct trace, not assumed**: a player who walks horizontally off the
edge of a ledge — no ceiling hit, no jump — never has `onLedge` cleared. It stays `1` indefinitely
while they fall. This means, today, in both Arcade and Runner, for both `man` and `secondPlayer`:
the player can jump again mid-air after walking off a ledge (since the jump-consumption gate
`if (onLedge)` is still satisfied), and any code that reads `onLedge` to choose idle-vs-falling
animation is wrong while airborne this way. This is a real, demonstrated defect distinct from —
and broader than — the already-known Runner ceiling bug (§2): it affects **both** modes and
**both** players, and has nothing to do with ceilings.

This is exactly what "reset grounded each step" (the assessment's own phrase) fixes: reset
`onLedge = 0` once per collision pass, then let the landing check re-affirm it to `1` only if the
player is actually still resting on a surface this tick.

## 2. The Runner ceiling bug — confirmed exactly as already documented elsewhere

Already found and documented in three prior-phase docs (`docs/physics-timestep-map.md` §6,
`docs/refactor-plan.md`'s Pass 11 deferred-items list, `docs/input-simulation-separation-map.md`
§4) — cited here, not re-derived. Confirmed by direct read at the current line numbers:

- Runner `man`, ceiling branch, `collisionDetect.c:645-658`: sets `onLedge = 1` at **line 656** on
  a `dy<0` head-bump — should be `0`.
- Runner `secondPlayer`, ceiling branch, `collisionDetect.c:703-716`: sets `onLedge = 1` at
  **line 714** — same bug.
- Arcade's structurally identical branches are correct: `man` ceiling → `onLedge=0` at **line
  279**; `boss` ceiling → `onLedge=0` at **line 340**; `secondPlayer` ceiling → `onLedge=0` at
  **line 399** (unconditional block) and **line 519** (multiplayer-duplicate block, §5).

Bumping your head while airborne in Runner currently re-grounds you instead of leaving you
falling.

## 3. The landmine "reset grounded each step" would hit without previous-position tracking

The landing/floor check uses a **strict** inequality:
```c
if (my + mh > by && my < by && game->man.dy > 0)   // collisionDetect.c:285 (Arcade man)
```
After landing, `y` is corrected to exactly `by - mh` (line 288). On a later tick where the player
is simply resting (not moving vertically), `my == by - mh` exactly, so `my + mh == by` —
**not** `> by`. If `onLedge` were reset to `0` at the top of every tick and this same strict check
were left unchanged, a standing player would lose `onLedge` on the very next tick, since the
landing condition can never re-fire while at rest — only while actively crossing into the ledge
from above.

This is precisely the role "previous-position tracking" plays here: a **crossing-based** test —
was the player's bottom edge above the ledge's top last tick, and at-or-below it this tick? —
correctly re-affirms landing whether the player just touched down *or* has been resting there for
many ticks, since both cases satisfy `prevY + mh <= by && my + mh >= by`. Verified this is the
*only* place in the file needing this treatment: ceiling and side (left/right) checks are one-shot
corrections (they zero a velocity and correct a position once, with nothing that needs to be
reaffirmed every subsequent tick), so they are left as pure current-position threshold tests,
unchanged.

**A second landmine, found while writing the test for this exact scenario**: the ceiling and
landing checks are separate `if` blocks, not `if`/`else if`, evaluated back-to-back for the same
ledge in the same loop iteration. The original code relied on this being safe because its strict
`my < by` landing condition could never be true immediately after a ceiling correction (which sets
`my` to `by + bh`, i.e. *past* the ledge's top). The crossing-based rewrite's initial form
(`prevY + mh <= by && my + mh >= by`) has no upper bound at all, so it re-fired immediately after
a ceiling correction in the same iteration — `my` had just been set to `by + bh` (satisfying
`my + mh >= by` trivially) and `dy` had just been zeroed by that same ceiling branch (satisfying
the new `dy >= 0`), producing a landing correction directly contradicting the ceiling correction
that just ran. Caught by the new test (`docs/verification/collision_correctness_test.c`), not
assumed correct. **Fixed** by adding `my < by + bh` back to the landing condition — position must
not already be at or past the ledge's *bottom* surface — which blocks the erroneous re-fire (since
a just-corrected ceiling position sits exactly at `by + bh`) while still permitting every normal
landing (which lands somewhere between `prevY` and `by + bh`, not past it).

## 4. No previous-position field exists; only `prevY` is added

Confirmed by repo-wide grep (`prevX|prevY|lastX|lastY|oldX|oldY` across `src/*.c`/`inc/*.h`):
zero matches anywhere. `Man` (`inc/header.h:96-109`) has no such field. This phase adds exactly
`float prevY;` to `Man` — not `prevX`, since nothing in this phase's scope (only the landing check,
per §3) consumes a previous-X value; the side checks use only current-position full-overlap tests
and don't need it. Adding `prevX` now with no consumer would be a dead field — deliberately not
added; can be introduced later if a future phase's crossing-based side/ceiling logic needs it.

**Capture point**: a new `capture_player_prev_y(GameState *game)` (`src/collisionDetect.c`) sets
`man.prevY = man.y; secondPlayer.prevY = secondPlayer.y;`, called as the **very first line** of
both `arcade_simulate`/`runner_simulate` (`src/frame.c`) — before `consume_*_jump_requests`, before
`apply_*_player_forces`, before `process`/`process2` moves anything. This captures "position at
the end of the previous tick" every tick, unconditionally, which is exactly the value the current
tick's `collisionDetect`/`collisionDetect2` (which runs *after* `process`/`process2` has already
moved the player this tick) needs to compare against.

**No explicit reset added in `arcade_session_reset`/`runner_session_reset`.** Reasoned through,
not assumed: on the first real tick after a session reset, `capture_player_prev_y` still runs
first (it's unconditional, every tick), setting `prevY` to the just-reset spawn `y` before anything
else touches it that tick. There is no tick where `prevY` could be read before being set correctly
relative to that tick's own `y`. An explicit reset in `loadGame.c` would be inert code.

## 5. Hitbox literals — real, pre-existing inconsistency; standardized only where touched

Every ledge-collision block in `collisionDetect.c` hardcodes a **local** `float mw = 48, mh = 48;`
— never reading `Man.w`/`Man.h` (which exist in the struct, `inc/header.h:98`, but are unused for
this purpose). Confirmed the *same* `man`/`secondPlayer` entities are hit-tested at genuinely
different sizes depending on which check runs: `48×48` against ledges (this file), `30×30` against
stars (`collisionDetect.c:629,633`, Runner), and a mix of `48×48`/`32×32` against enemies/bosses
(`processEvents.c:292,331,355`) or `30×30` against smart-enemies (`processEvents.c:338,364,371`).
Render sizes diverge further still: `man`/`secondPlayer` are drawn at `55×55` (Arcade,
`doRender.c:136,143,152,162`) or `54×54` (Runner, `doRender.c:215,226`).

**Scope decision, made deliberately, not by oversight**: reconciling hit-test size with render
size is a gameplay-feel change (it changes how generous or strict collision feels), not a
correctness fix — out of scope for a phase about collision *logic* correctness. What this phase
*does* do: replace the repeated bare `48, 48` at the five blocks it already touches (player ledge
collision) with two named constants, `PLAYER_LEDGE_HITBOX_W`/`PLAYER_LEDGE_HITBOX_H` (both
`48.0f`, same numeric value — zero behavior change). The many other scattered hitbox literals
(enemy/boss/smart-enemy/star, in this file and `processEvents.c`) are left exactly as they are and
listed in §7 as deferred — "standardize hitboxes" here means "give the ones this phase touches one
documented name," not "make every hitbox in the game consistent with every other."

## 6. The five blocks this phase touches

All confirmed by direct read, exact current line numbers:

| Entity/mode | Block | Ceiling `onLedge` | Landing check | Reset placement |
|---|---|---|---|---|
| Arcade `man` | `:262-318` | `=0` (correct) | rewritten, crossing-based | once, before `:262` |
| Arcade `secondPlayer` (unconditional) | `:382-438` | `=0` (correct) | rewritten | once, before `:382` |
| Arcade `secondPlayer` (multiplayer-duplicate, `:502-558`, inside `if (game->multiPlayer)` at `:441`) | `:502-558` | `=0` (correct) | rewritten (kept consistent with its twin, not deduplicated — §7) | **none** — the first block's reset already covers this tick; a second reset here would be redundant, not incorrect, since nothing between the two blocks changes `secondPlayer`'s position |
| Runner `man` | `:639-695` | `=1` → **fixed to `0`** | rewritten | once, before `:639` |
| Runner `secondPlayer` | `:697-753` | `=1` → **fixed to `0`** | rewritten | once, before `:697` |

The reset must happen **once per collision pass**, not once per ledge inside the 100-ledge loop —
resetting inside the loop body would clear a landing just detected against ledge #5 the moment the
loop reaches ledge #6 with no match, undoing the very thing the loop just did.

## 7. Deliberately out of scope (found, documented, not touched)

- **Enemy/boss/smart-enemy ledge collision, and the enemy-to-enemy self-collision loop**
  (`collisionDetect.c:77-199,561-622`) — same "player-only" narrowing this session has applied
  consistently since Phase 11. Boss's ceiling logic is already correct (`:340`); its own
  walk-off-edge ungrounding gap is the same shape as the player's bug but left for a future phase
  scoped to enemies.
- **Deduplicating the two `secondPlayer`-vs-ledge blocks** (`:382-438` and `:502-558`) into one —
  a real, pre-existing duplicate, already flagged in `docs/frame-pipeline-map.md` and assigned to
  its own future "Phase 10 — large paired-function deduplication" item in `docs/refactor-plan.md`.
  Both copies get the same three fixes applied here; neither is merged away.
- **Star-collision (`30×30`) and enemy/boss `collide2d()` hit sizes (`48×48`/`32×32`/`30×30`
  mixed)** — real, confirmed inconsistencies, not touched (§5).
- **Reconciling hit-test size with render size** — a gameplay-feel decision, not a correctness
  bug (§5).
- **Making ceiling/side checks crossing-based** — no persistent state to reaffirm there, unlike
  landing (§3); left as pure current-position tests.

## 8. Test strategy

`docs/verification/collision_correctness_test.c` (new, `mingw-collisiontest` target), calling
`collisionDetect()`/`collisionDetect2()` directly with hand-set `GameState` fields (no file in
`docs/verification/*.c` has ever called these functions directly before — genuinely new coverage):
1. **Walking off a ledge clears `onLedge`** — the core bug fix: place the player resting on a
   ledge (`onLedge=1`), move `x` off the ledge's horizontal span with `y` unchanged, call
   `collisionDetect`/`collisionDetect2` — assert `onLedge` becomes `0`.
2. **Resting continuously re-affirms `onLedge=1`** across multiple consecutive calls (updating
   `prevY` between them the way `capture_player_prev_y` would each real tick) — proves the
   crossing-based fix handles the exact-rest case, not just the instant of first contact.
3. **Normal falling-and-landing still works** — regression-equivalence: falling from well above a
   ledge still triggers landing and clears `dy`.
4. **Runner's ceiling bump now clears `onLedge`** for both `man` and `secondPlayer` — direct
   regression-fix proof for the bug in §2.
5. **`capture_player_prev_y` sets `prevY` to the current `y`** for both players.
