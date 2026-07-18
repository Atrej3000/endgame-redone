# Frame Pipeline Map — Ucode_Endgame

Written **before** any Phase 5 code edit, per that phase's own requirement. Describes the exact
per-scene frame pipeline as it exists at commit `86dac33` (tag `refactor-pass-4-lifecycle`), with
every source location verified by direct reading (not from memory) immediately before this
document was written. Phase 5's own changes are noted inline as "Phase 5 change" callouts;
everything else describes pre-existing, unmodified behavior.

## Legend

Each scene's row lists, in the exact order the code executes them: event polling, input-state
mutation, movement, enemy/boss AI, spawning, collision detection, health/life changes, score
changes, animation-state changes, rendering, `SDL_RenderPresent`, frame pacing, and scene
transitions — using "—" where a phase does not apply to that scene.

## Scene-by-scene pipeline (`src/main.c:29-84`)

### `APP_SCENE_MAIN_MENU` (`main.c:31-34`)
`menu0_events(gameState)` → `doRender_menu0(renderer, gameState)`. No update/collision phase.
`doRender_menu0` (`doRender_menu.c:3-11`) blits one static texture and presents — confirmed no
`game->` writes.

### `APP_SCENE_ARCADE_MENU` / `APP_SCENE_RUNNER_MENU` (`main.c:36-39`, `:58-61`)
`menu_events(gameState)` → `doRender_menu1`/`doRender_menu2` (`doRender_menu.c:13-31`, same
pure-blit-and-present shape). No update/collision phase. Asset loading (`arcade_assets_load`/
`runner_assets_load`) happens inside `menu_events.c`'s scene-enter path (Phase 4), not per-frame.

### `APP_SCENE_ARCADE_GAME` (`main.c:41-46`)
Exact order: `process(gameState)` → `collisionDetect(gameState)` → `doRender(renderer,
gameState)` → `processEvents(window, gameState)`.

- **Update** (`process.c:75-989`): `game->time++` (`:78`); bullet movement + inline projectile
  collision against enemies/smart-enemies/boss, nested per-target so a bullet's position is
  updated once per target it's tested against in the same frame — up to 113 times for a single
  frame with all enemy types active (`:87-300`, load-bearing for current speed feel, not a bug to
  fix); score-threshold-triggered enemy/boss/smart-enemy wave spawning, all `tempScore`-gated
  except the very first wave at `game->time==120` (`:304-570`); boss movement (`:582-616`); enemy
  AI movement (`:619-870`); player movement `man->x += man->dx; man->y += man->dy;` (`:887-888`),
  animation counter `man->animFrame++` gated on `time%3==0 && onLedge && dx!=0 && !slowingDown`,
  wraps at 11 (`:890-903`); gravity (`:909`); death-countdown handling, decrement present and
  working (`:911-924`); second-player mirror, but with its own `animFrameSecond++` **commented
  out** (`:949-962` — Arcade multiplayer's second player animation is frozen; pre-existing
  asymmetry, not touched this phase, no demonstrated defect established).
- **Collision** (`collisionDetect.c:3-623`): enemies-vs-ledges (`:77-137`), smart-enemies-vs-ledges
  (`:139-199`), man-vs-ledges (`:262-318`, sets `onLedge=0` on head bump at `:279`), boss-vs-ledges
  (`:321-380`), secondPlayer-vs-ledges unconditionally (`:382-438`) then **again** inside
  `if (multiPlayer)` (`:502-558` — exact duplicate applied twice per frame in multiplayer; no
  demonstrated observable defect, **left unchanged**), enemy-to-enemy (`:561-622`, both loop
  indices range over the same `smartEnemies[10]` array with no `i==j` guard — **traced precisely:
  when i==j, position/size are identical and every directional AABB check uses a strict
  inequality that evaluates false for identical values, so no mutation ever occurs for the
  self-case; confirmed safe-by-construction today, not a demonstrated defect, left unchanged**).
  No `man.isDead`/score/lives writes anywhere in `collisionDetect()`.
- **Render** (`doRender.c:3-202`): backgrounds/sun/clouds/buildings/train/ledges/enemies/boss/
  bullets/player/second-player sprites, then HUD (`draw_status_lives`, `init_status_lives` +
  `draw_status_lives2`, `init_status_kills` + `draw_status_kills`), then `SDL_RenderPresent`
  (`:201`). **Confirmed zero writes to any `game->` field anywhere in this function** — already
  fully render-pure.
- **Events** (`processEvents.c:3-554`): `SDL_PollEvent` loop (`:15-116`) handles `ESCAPE` (scene
  transition + `return done;`, early-exits the rest of the function for that frame, `:34-50`),
  `p` (pause transition, `:52-59`), `w`/`UP` (jump impulses, `:61-80`), `q` (scene transition, **no
  early return**, `:92-107`). After the poll loop, an unconditional continuous section
  (`:117-553`) does held-key movement/animation-sprite advances, parallax/train/cloud scroll,
  sprite-frame counters, inline `collide2d()` hit-testing against enemies/smart-enemies/boss
  (duplicating `collisionDetect`'s job in a different, projectile/body-contact sense),
  fall-death check (`:508-516`), and the game-over scene transition (`:518-552`).
  **Confirmed double-transition risk (Phase 5 fix, see below)**: since `q` does not return early,
  if `game->man.lives` also reaches 0 in the same frame `q` was pressed, the game-over block at
  `:518-552` fires its own `app_change_scene(game, APP_SCENE_ARCADE_MENU)` after `q`'s handler
  already set `APP_SCENE_MAIN_MENU`, silently overwriting it.
  **Phase 5 change**: `:522` and `:529`'s transition calls are now guarded by
  `if (game->scene == APP_SCENE_ARCADE_GAME)` so a transition already fired earlier in the same
  call is not overwritten.

### `APP_SCENE_ARCADE_LEADERBOARD` (`main.c:48-51`)
`processEvents(window, gameState)` (same function as gameplay — still polls all its key handlers
and runs its full continuous section, a pre-existing quirk documented in
`docs/scene-state-map.md`) → `doRender_leaderboard(renderer, gameState)`
(`doRender_leaderboard.c:3-10`, confirmed pure: one static blit + present).

### `APP_SCENE_ARCADE_PAUSE` / `APP_SCENE_RUNNER_PAUSE` (`main.c:53-56`, `:75-78`)
`doRender_pause(renderer, gameState)` (`doRender_pause.c:3-10`, confirmed pure) →
`pause_events(gameState)`. No update/collision phase; gameplay fully frozen.

### `APP_SCENE_RUNNER_GAME` (`main.c:63-68`)
Exact order: `process2(gameState)` → `collisionDetect2(gameState)` → `doRender2(renderer,
gameState)` → `processEvents2(window, gameState)`.

- **Update** (`process.c:991-1170`): `game->time++` (`:993`); player movement + animation counter
  (same 11-wrap/`time%3==0` gating as Arcade, `:1003-1024`), second-player movement + animation
  (**live here**, unlike Arcade's frozen equivalent, `:1028-1047`); "moving traps" star
  oscillation via `sinf`/`cosf` (`:1050-1066`); death-countdown block (`:1140-1157`) — **confirmed
  incomplete**: `game->gameLives--` at `:1148` runs unconditionally every frame once
  `deathCountdown > 0`, but the decrement of `deathCountdown` itself is commented out (`:1150`),
  so once triggered this would drain `gameLives` forever, never stopping. This is a real,
  separate defect from render purity/scene transitions — **not fixed this phase** (would touch
  life/score rules, out of scope; also currently unreachable since nothing sets `man.isDead=1`,
  see below); scroll handling (`:1158-1169`).
- **Collision** (`collisionDetect.c:625-756`): man/secondPlayer vs `stars[]` (`:627-639`, instant
  `game->gameLives = 0`; note the adjacent commented-out `game->man.isDead = 1` at `:631` — the
  only place in the entire codebase that would ever set `isDead`, and it's dead/commented),
  man-vs-ledges (`:641-697`, sets `onLedge=1` even on a head bump at `:658` — **differs from
  Arcade's `onLedge=0` on head bump at `collisionDetect.c:279`**; a real behavioral asymmetry
  between the two modes, not proven wrong, **documented, not changed**), secondPlayer-vs-ledges
  (`:699-755`, same asymmetry).
- **Render** (`doRender.c:204-281`): background/second-player/ledges/hero-animation/death-sprite/
  stars, then HUD (`draw_status_lives`, `init_status_lives` + `draw_status_lives2`,
  `init_status_x` + `draw_status_x`), then `SDL_RenderPresent` (`:279`).
  **Confirmed one live gameplay mutation**, nested in `if (game->man.isDead)` (`:248-257`): draws
  the death sprite (`:250-251`), then unconditionally does `game->gameLives--; game->man.isDead =
  0; game->man.y = 0;` (`:253-256`). **Confirmed via grep: nothing anywhere in the live codebase
  ever sets `man.isDead = 1`** (the only occurrence is the commented-out line at
  `collisionDetect.c:631`, and a test harness) — this branch is dead code today; relocating it
  changes zero observable behavior now but removes a correctness trap for whenever it's
  reactivated. Lines `:227-236` are a fully-commented dead duplicate of the same block, safe to
  delete outright.
  **Phase 5 change**: the mutation (`:253-256`) moves to a new function `runner_resolve_death()`
  (`src/frame.c`), called immediately *after* `doRender2()` returns (not before — see rationale
  below). The death-sprite draw (`:248-251`) stays inside `doRender2()`, still gated on
  `if (man.isDead)`, unchanged.
- **Events** (`processEvents.c:556-818`): mirrors Arcade's structure — `ESCAPE` (`:590-600`,
  early return), `p` (`:602-609`), `w`/`UP` (`:611-644`), `q` (`:621-634`, **no early return**,
  same double-transition risk as Arcade), plus a Runner-only cheat key `0` (`:647-650`).
  Continuous section (`:660-818`): held-key movement, infinite-field ledge regeneration
  (`:750-774`), scroll-based `x_score` tracking (`:775-776`), fall/off-screen death check
  (`:778-789`), and the game-over block (`:792-815`) which **also persists the death's score**
  into `x_list` (`:794-798`, `game->x_i < 25` bounds-checked) before transitioning
  (`:814`).
  **Phase 5 change**: `:814`'s transition call is now guarded by
  `if (game->scene == APP_SCENE_RUNNER_GAME)`, matching the Arcade fix. The score-save at
  `:794-798` remains unconditional — it must still run even if an earlier handler (`q`) already
  changed the scene this frame.

### `APP_SCENE_RUNNER_LEADERBOARD` (`main.c:70-73`)
`processEvents2(window, gameState)` → `doRender_leaderboard2(renderer, gameState)`
(`doRender_leaderboard.c:12-25`). **Confirmed NOT render-pure**: calls `init_status_x_list(game)`
(`:20`) unconditionally every call, which itself (`x_list_leader.c:3-32`) loops 20 times, each
iteration doing a fresh `TTF_RenderText_Blended` + destroy-then-recreate of `game->label` — 20
texture create/destroy cycles per rendered frame on this one screen. No dirty-flag/previous-value
skip exists anywhere in the HUD code (`init_status_lives`/`init_status_kills`/`init_status_x`
confirmed the same: unconditional regenerate-every-call, just without the 20x multiplier). Per
the task's own explicit allowance to defer caching "when it would expand the phase" —
**documented only, not fixed this phase**; a future optimization candidate.

### `APP_SCENE_QUIT` (`main.c:80-82` `default` case, or the `while` condition itself)
Ends the main loop; `app_shutdown()` runs. No per-frame pipeline.

## Phase 5 changes summary

| # | Change | File(s) | Frame-position impact |
|---|---|---|---|
| 1 | `arcade_frame()`/`runner_frame()` named wrappers, replacing 4 inline calls each | `src/frame.c` (new), `src/main.c` | None — identical call order preserved exactly |
| 2 | `runner_resolve_death()` extracted from `doRender2()`, called after it returns | `src/doRender.c`, `src/frame.c` | Moves from "mid-draw, before `SDL_RenderPresent`" to "after `doRender2()` returns, before `processEvents2()`" — nothing in between reads the affected fields, so this is behaviorally identical; branch is dead code today regardless |
| 3 | Scene-checked guard on both game-over transition sites | `src/processEvents.c:522,529,814` (approx., pre-change line numbers) | Prevents a same-frame double transition; zero player-visible effect since nothing renders gameplay state on the destination menu scenes |
| 4 | `manFrames[0]` moved into `shared_assets_load()`/`shared_assets_unload()` | `src/loadGame.c`, `src/app.c` | No frame-order impact — this is a one-time load-time change, not a per-frame one |

## Findings explicitly NOT acted on this phase (documented, not fixed)

1. Enemy-to-enemy self-collision (`collisionDetect.c:561-622`) — proven safe-by-construction
   today (see above), fragile if the struct or comparisons ever change. Left unchanged.
2. `onLedge`-on-head-bump asymmetry between Arcade (`:279`, sets 0) and Runner (`:658`, sets 1) —
   real, not proven wrong, could be intentional Runner "feel." Left unchanged.
3. Duplicate secondPlayer-vs-ledges check in `collisionDetect()` (`:382-438` and `:502-558`) — no
   demonstrated observable defect from the redundancy. Left unchanged.
4. `process2()`'s incomplete `deathCountdown` decrement (`:1140-1157`, missing decrement vs.
   Arcade's working `process()` sibling at `:911-924`) — a real, separate defect, but out of this
   phase's render/scene-transition scope, and currently unreachable (nothing sets `isDead=1`
   live). Left unchanged.
5. HUD texture churn (`init_status_lives`/`kills_score`/`X_score`/`init_status_x_list`) — no
   dirty-flag caching added; explicitly deferred per the task's own allowance.
6. Further splitting of `processEvents()`/`processEvents2()` into separate event-translation vs.
   continuous-simulation functions — considered and rejected: these functions run *last* in the
   frame (after render), so splitting their continuous logic into a separately-timed call risks
   exactly the kind of reordering the task forbids ("after rendering to before movement... unless
   fixing a separately confirmed defect"). The one concrete defect in this boundary (double
   transition) is fixed narrowly (item 3 above) without a full split.
7. Arcade's frozen second-player run animation (`process.c:949-962`, increment commented out) —
   pre-existing asymmetry with Runner's live equivalent, no demonstrated defect. Left unchanged.

## `manFrames[0]` correction (see `docs/game-session-lifecycle.md` for the fuller history)

Both `arcade_assets_load()` (`loadGame.c:100`, pre-Phase-5) and `runner_assets_load()`
(`loadGame.c:367`, pre-Phase-5) loaded the byte-for-byte identical path
`"./resource/images/main_hero/run/V_g(rn)0.png"` — the "contested field, different source
images" comment written during Phase 4 was factually incorrect. Only reader:
`draw_status_lives()` (`src/status.c:31`), called identically from both `doRender()` and
`doRender2()`. **Phase 5 fix**: consolidated into `shared_assets_load()`/`shared_assets_unload()`
alongside `mult`/`leaders`/`pause`/`brick`/`death`/`font`; `app_shutdown()`'s centralized
`manFrames[]` destroy loop (`app.c:62-65`) now starts at index 1.
