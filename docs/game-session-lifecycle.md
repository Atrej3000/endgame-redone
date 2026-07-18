# Game Session Lifecycle — Ucode_Endgame

Written **before** any Phase 4 code edit, per that phase's own requirement. Describes
`loadGame()`/`loadGame2()` exactly as they existed at commit `82927d6` (tag
`refactor-pass-3-scenes`) — the pre-refactor baseline — then is updated in place once the split
lands, with each new function's actual boundary recorded against this classification.

## 1. Guard boundaries

| Function | Asset guard opens | Asset guard closes | Flag set | Reset tail |
|---|---|---|---|---|
| `loadGame()` (Arcade) | `loadGame.c:5` (`if (!game->arcadeAssetsLoaded)`) | `loadGame.c:346` | `arcadeAssetsLoaded = true` at `:345` | `loadGame.c:348-573`, runs unconditionally every call |
| `loadGame2()` (Runner) | `loadGame.c:578` (`if (!game->runnerAssetsLoaded)`) | `loadGame.c:911` | `runnerAssetsLoaded = true` at `:910` | `loadGame.c:913-1048` (with `985-1048` entirely dead/commented), runs unconditionally every call |

Both functions are called only from `src/scene.c`: `arcade_menu_enter`/`runner_menu_enter`
(scene.c:9-26), themselves invoked from `app_change_scene`'s switch (scene.c:40-47) on *every*
arrival at `APP_SCENE_ARCADE_MENU`/`APP_SCENE_RUNNER_MENU` — including return trips from
gameplay (ESCAPE, game-over), not just the original Main-menu-to-mode transition. `load_menu1()`/
`load_menu2()` (the texture+music+leaderboard-reset load, `load_menu.c`) are conditioned on
`previous_scene == APP_SCENE_MAIN_MENU` and so run only once per true mode entry — a materially
different (and already-correct) cadence from `loadGame`/`loadGame2`'s reset tail, which is the
core problem this phase fixes.

## 2. Statement classification — `loadGame()` (Arcade, `loadGame.c:3-574`)

### Inside the asset guard (one-time, lines 5-346)

| Lines | Category | Field(s) |
|---|---|---|
| 22 | audio asset loading | `battleMus` (`Mix_LoadMUS`) |
| 26-34 | static asset loading | `bossTexture` |
| 36-44 | static asset loading (**shared with Runner**) | `mult` |
| 47-55 | static asset loading | `bulletTexture` |
| 58-66 | static asset loading | `secondBulletTexture` |
| 69-77 | static asset loading | `secondPlayer.sheetTextureRun2` |
| 80-88 | static asset loading | `secondPlayer.sheetTextureJump2` |
| 91-99 | static asset loading (**shared with Runner**) | `leaders` |
| 102-110 | static asset loading (**shared with Runner**) | `pause` |
| 113-119 | unclear/mixed — **dead-end resource leak** | orphaned `IMG_Load` of `spike_head.png`, never turned into a texture nor freed (pre-existing, not fixed this phase — no live field depends on it) |
| 121-129 | static asset loading (**contested with Runner** — different source image, see §5) | `manFrames[0]` |
| 132-140 | static asset loading | `man.sheetTextureRun` |
| 143-151 | static asset loading | `man.sheetTextureJump` |
| 165-167 | static asset loading (**shared with Runner**) | `death` |
| 170-178 | static asset loading | `enemy.sheetTextureRun` |
| 180-188 | static asset loading | `enemy.sheetTextureRun2` |
| 191-291 | static asset loading | `sheetTextureBack`, `cloud1..8.sheetTextureCloudN`, `sheetTextureSun` |
| 294-302 | static asset loading | `sheetTextureBack2` |
| 305-313 | static asset loading | `train.textureTrain` |
| 316-326 | static asset loading (**shared with Runner**) | `brick` (316-318); Arcade-exclusive: `brick_block`, `copper_block` |
| 328-334 | audio asset loading (font grouped here per the task's own taxonomy) | `font` (**shared with Runner**) |
| 345 | guard bookkeeping | `arcadeAssetsLoaded = true` |

### Outside the guard — session-reset tail (every call, lines 348-573)

| Lines | Category | Notes |
|---|---|---|
| 348-351 | **removed this phase** (see §6) | `game->label` destroy-and-null — redundant with `doRender()`'s own per-frame `init_status_lives()` call |
| 354-356 | score/life/session reset | `kills_score`, `kills_score_multi` |
| 358-370 | player initialization | `man.x/y/dx/dy/onLedge/animFrame/facingLeft/slowingDown/lives/visible0/isDead/shooting` |
| 369 (interleaved) | score/life/session reset | `statusState = STATUS_STATE_LIVES` (sits mid-block; harmless duplication, not deduplicated this phase to minimize diff) |
| 372-399 | **unclear — not in the task's given taxonomy, classified as level/scene-decoration reset** | `train.x/y`, `cloud1..8.x/y` — background parallax-object positions, not player/enemy/bullet state, not "level geometry" in the ledges sense either; grouped with session reset since they reset every call and have no asset-loading character |
| 401-407 | score/life/session reset | `gameLives=10`, `tempScore=0`, `shotCount=0`, `shotCountMultiplayer=0`; **line 407 `multiPlayer = 1` is the confirmed bug — deleted this phase, replaced by `*_session_reset(game, mode)`'s parameterized assignment** |
| 409-424 | player initialization (player 2), gated `if (multiPlayer)` | `secondPlayer.x/y/dy/dx/onLedge/animFrameSecond/facingLeft/slowingDown/lives/visible0/isDead/shooting` |
| 422 (interleaved) | score/life/session reset | second `statusState = STATUS_STATE_LIVES` (same duplication as 369) |
| 427-454 | enemy initialization | `enemyValues[]`, `smartEnemies[]`, `boss[]` |
| 456-467 | bullet initialization | **confirmed leak — direct `= NULL` instead of `removeBullet`/`removeSecondBullet`; fixed this phase** |
| 469 | **removed this phase** (see §6) | `init_status_lives(game)` call — redundant |
| 471-474 | score/life/session reset | `time=0`, `deathCountdown=-1` (`scrollX` reset is commented/dead here — asymmetric vs. Runner, which does reset it; preserved as-is, not "fixed," since it's a pre-existing mode difference not in this phase's scope) |
| 486-573 | level geometry initialization | `ledges[0..99]` (contiguous ranges, confirmed gap-free in an earlier pass) |

## 3. Statement classification — `loadGame2()` (Runner, `loadGame.c:576-1049`)

### Inside the asset guard (one-time, lines 578-911)

| Lines | Category | Field(s) |
|---|---|---|
| 595 | audio asset loading | `runnerMus` |
| 598-607 | static asset loading (**shared with Arcade**) | `mult` |
| 609-618 | static asset loading (**shared with Arcade**) | `leaders` |
| 620-629 | static asset loading | `fon` (Runner-exclusive) |
| 630-639 | static asset loading (**shared with Arcade**) | `pause` |
| 641-650 | static asset loading | `star` (nominally shared field name; Arcade's own load of it is dead/commented, so effectively Runner-exclusive in practice) |
| 652-771 | static asset loading | `manFrames[0]` (**contested**, see §5) + `manFrames[1..11]` (Runner-exclusive) |
| 773-893 | static asset loading | `secondPlayerFrames[0..11]` (Runner-exclusive) |
| 895-897 | static asset loading (**shared with Arcade**, case-differing filename `Clay_block.png` vs `clay_block.png`) | `brick` |
| 899-901 | static asset loading (**shared with Arcade**) | `death` |
| 903-909 | audio/font asset loading (**shared with Arcade**) | `font` |
| 910 | guard bookkeeping | `runnerAssetsLoaded = true` |

### Outside the guard — session-reset tail (every call, lines 913-983 live; 984-1048 dead)

| Lines | Category | Notes |
|---|---|---|
| 913-916 | **removed this phase** (see §6) | `game->label` destroy-and-null — same redundancy as Arcade |
| 918 | score/life/session reset | `x_score = 0` |
| 924-934 | player initialization + interleaved session reset | `man.x/y/dx/dy/onLedge/animFrame/facingLeft/slowingDown`, `gameLives=3`, `man.isDead=0`, `statusState=STATUS_STATE_LIVES` |
| 936-949 | player initialization (player 2), gated `if (multiPlayer)` — **reads `multiPlayer` but never writes it in this function** | `secondPlayer.x/y/dy/dx/onLedge/animFrameSecond/facingLeft/slowingDown/isDead` (note: unlike Arcade, never sets `secondPlayer.lives`/`visible0`/`shooting` — pre-existing mode asymmetry, preserved) |
| 951 | **removed this phase** (see §6) | `init_status_lives(game)` call |
| 953-955 | score/life/session reset | `time=0`, `scrollX=0`, `deathCountdown=-1` |
| 957-983 | **unclear/mixed — level geometry + decorative randomization + RNG side effect** | `ledges[0..99]` (procedurally generated, not fixed coordinates like Arcade) interleaved with `stars[0..99]` position/mode/phase randomization via `random()`/`random_sign()`; consumes the process RNG stream every call today. Classified as level geometry initialization for the split (see §7 for the timing analysis) |
| 985-1048 | dead code (fully commented) | leftover alternate Arcade-style ledge layout and star-init attempts — not live, not touched |

No bullet or enemy/boss arrays exist in `loadGame2()` — Runner mode has neither, confirmed by
grep; this is a genuine mode difference, not an omission.

## 4. Call sites (both functions)

Only one production call site each, both in `src/scene.c`:
- `loadGame(game)` — `scene.c:15`, inside `arcade_menu_enter`, unconditional (every arrival).
- `loadGame2(game)` — `scene.c:25`, inside `runner_menu_enter`, unconditional (every arrival).

(`docs/verification/smoke_init_shutdown.c` also calls both directly, three times each, as a
test harness — not a production call site.)

## 5. Shared/contested resource fields — single-owner decision

| Field | Arcade loads it? | Runner loads it? | Content identical? | Decision |
|---|---|---|---|---|
| `mult` | yes (`:36-44`) | yes (`:598-607`) | yes (same file) | **Shared bucket** — `shared_assets_load()`/`shared_assets_unload()`, own `sharedAssetsLoaded` flag, "first mode visited loads it, second skips" |
| `leaders` | yes (`:91-99`) | yes (`:609-618`) | yes (same file) | Shared bucket |
| `pause` | yes (`:102-110`) | yes (`:630-639`) | yes (same file) | Shared bucket |
| `brick` | yes (`:316-318`, `clay_block.png`) | yes (`:895-897`, `Clay_block.png`) | yes (same file, case-differing literal — same file on this OS) | Shared bucket |
| `death` | yes (`:165-167`) | yes (`:899-901`) | yes (same file) | Shared bucket |
| `font` | yes (`:328-334`) | yes (`:903-909`) | yes (same file/size) | Shared bucket |
| `manFrames[0]` | yes (`:121-129`, from a run-cycle frame) | yes (`:652-661`, from `V_g(rn)0.png`) | **no — different source images** | **Not** folded into the shared bucket (would silently change which image wins from "whichever mode ran most recently" to "whichever mode ran first," an actual behavior difference). Instead: each mode's own load gets a destroy-before-overwrite guard (`if (game->manFrames[0]) { SDL_DestroyTexture(...); }` before creating the new one) — preserves "most-recently-visited mode's image wins," just without leaking the previous one. Destruction of the whole `manFrames[12]` array stays centralized in `app_shutdown()` (see §6), since indices 1-11 are Runner-exclusive but index 0's ownership is genuinely contested. |

No other field is written by both functions.

## 6. HUD label (`game->label`) — resolved, not moved

`init_status_lives(game)` (called from both reset tails) internally calls
`SDL_CreateTextureFromSurface`, which the task's rule forbids inside session-reset. The obvious
fix ("move the call somewhere else in the load/reset split") turns out to be unnecessary:
`doRender()`/`doRender2()` **already call `init_status_lives(game)` unconditionally on every
single rendered gameplay frame** (`doRender.c:187-194` and `:266-272`), which is what actually
keeps the label correct today, independent of anything `loadGame`/`loadGame2` do. The destroy-
and-null block and the `init_status_lives()` call in both reset tails are therefore **dead
weight relative to that per-frame safety net** and are removed entirely from
`arcade_session_reset()`/`runner_session_reset()` in this phase — not moved, not relocated. HUD
label lifecycle remains fully owned by `doRender`/`doRender2` (per-frame regenerate) plus
`app_shutdown()` (final destroy), unaffected by this phase otherwise.

## 7. Session-reset trigger timing — moved from "every menu arrival" to "new-game-start"

Per the task's own worked example (select mode → verify assets → reset session → transition to
gameplay), `arcade_session_reset()`/`runner_session_reset()` are called explicitly from
`menu_events.c`'s `SDLK_1`/`SDLK_2` handlers, immediately before `app_change_scene(...,
APP_SCENE_ARCADE_GAME/RUNNER_GAME)` — not from `arcade_menu_enter`/`runner_menu_enter` (which now
call only the asset-load functions). This is verified safe: nothing renders or reads gameplay
state while sitting on `APP_SCENE_ARCADE_MENU`/`APP_SCENE_RUNNER_MENU` (`doRender_menu0/1/2` only
blit a static background), so whether the reset happens "the moment you land on the menu" or
"the moment you press 1/2 to start" is unobservable — by the time gameplay actually renders, the
reset has already run either way. One consequence, confirmed harmless: if a player returns to the
menu (ESCAPE or game-over) and then quits to the Main menu *without* pressing 1/2 again, the
stale post-game session state persists in `GameState` slightly longer than before (until the next
real new-game-start) rather than being cleared the instant the menu is reached — still invisible,
since nothing reads it outside an active gameplay scene.

## 8. `multiPlayer` — single source of truth

Confirmed via exhaustive grep: ~29 read sites across `process.c`/`processEvents.c`/`doRender.c`/
`collisionDetect.c`/`kills_score.c`, every one a bare boolean truth test, zero `==` comparisons,
zero arithmetic, never assigned anything but literal `0`/`1` anywhere. Retyped `int → GameMode`
(field name unchanged) with zero read-site changes required. The only 3 write sites
(`menu_events.c:23,30`, `loadGame.c:407`) collapse into exactly one: `*_session_reset(game,
mode)`'s first statement, `game->multiPlayer = mode;` — eliminating both the `loadGame.c:407`
clobber bug and the "two independent writers of one value" risk a prior review pass flagged.

## 9. Ownership table

| Resource/state | Owner | Loaded/initialized when | Reset when | Destroyed when |
|---|---|---|---|---|
| Arcade-exclusive textures (bossTexture, bulletTexture, secondBulletTexture, secondPlayer run/jump, man run/jump, enemy run/run2, 9 background/cloud/train, brick_block, copper_block) | `arcade_assets_load`/`arcade_assets_unload` | First arrival at `APP_SCENE_ARCADE_MENU` | Never (long-lived) | `arcade_assets_unload()` (called from `app_shutdown()`) |
| Runner-exclusive textures (fon, star, manFrames[1..11], secondPlayerFrames[0..11]) | `runner_assets_load`/`runner_assets_unload` | First arrival at `APP_SCENE_RUNNER_MENU` | Never | `runner_assets_unload()` (from `app_shutdown()`) |
| Shared textures + font (mult, leaders, pause, brick, death, font) | `shared_assets_load`/`shared_assets_unload` | First arrival at either mode's menu, whichever comes first | Never | `shared_assets_unload()` (from `app_shutdown()`) |
| `manFrames[0]` (contested) | Whichever mode's `_assets_load` ran most recently | Every arrival at that mode's menu (destroy-before-overwrite) | N/A (not session state) | `app_shutdown()`, centralized (see §5) |
| `battleMus`/`runnerMus` | `arcade_assets_load`/`runner_assets_load` respectively | Same as their mode's textures | Never | Respective `_assets_unload()` |
| Menu textures (menu0/menu1/menu2), `menuMus` | `load_menu.c` (untouched this phase) | Once per true Main-menu-to-mode transition | Never (texture); music restarts every entry | `app_shutdown()` (inline, unchanged) |
| SFX chunks (jumpSound/kickSound/select/shootSound/damageSound) | `menu_events.c`/`pause_events.c`/`processEvents.c`/`process.c` (untouched this phase) | Lazily, NULL-guarded, first use | Freed+re-loaded across ESCAPE/quit key handlers (pre-existing pattern, unaffected) | `app_shutdown()` (inline, unchanged) |
| Level geometry (`ledges[100]`) | `arcade_session_reset`/`runner_session_reset` | Every new-game-start | Every new-game-start | N/A (plain array, no destruction) |
| Player state (`man`, `secondPlayer` gameplay fields) | `arcade_session_reset`/`runner_session_reset` | Every new-game-start | Every new-game-start | N/A |
| Enemies/bosses (`enemyValues`, `smartEnemies`, `boss`) | `arcade_session_reset` (Arcade-only) | Every new-game-start | Every new-game-start | N/A |
| Bullets (`bullets[]`, `secondBullets[]`) | `arcade_session_reset` (Arcade-only) | Every new-game-start | Every new-game-start, via `removeBullet`/`removeSecondBullet` (fixed leak) | `app_shutdown()` (`removeBullet`/`removeSecondBullet` loop, unchanged) |
| Score/lives (`gameLives`, `tempScore`, `shotCount*`, `kills_score*`, `x_score`, `x_list`) | `arcade_session_reset`/`runner_session_reset` (gameplay reset); `load_menu1`/`load_menu2` (leaderboard-staging reset, untouched) | Every new-game-start (gameplay); every true mode entry (leaderboard staging) | Same | N/A |
| HUD-generated labels (`label`, `labelMultiplayer`) | `doRender`/`doRender2` via `init_status_lives`/`init_status_kills` (untouched) | Every rendered gameplay frame | Every rendered gameplay frame (destroy-before-recreate) | `app_shutdown()` (final, inline) |

## 10. Confirmed bugs fixed by this phase (not new behavior — closing pre-existing defects)

1. `loadGame.c:407`'s unconditional `multiPlayer = 1` — deleted, replaced by `*_session_reset`'s
   parameterized assignment.
2. Bullet-array leak (`loadGame.c:456-467`'s direct `NULL` assignment instead of `removeBullet`/
   `removeSecondBullet`) — fixed in `arcade_session_reset`.
3. Shared-texture leak-on-second-mode-visit (mult/leaders/pause/brick/death/font) — fixed via the
   shared-assets bucket with its own one-time guard.

None of these change any visible gameplay behavior in the common case (a single mode visited per
process run, or a fresh game started normally) — they only prevent resource leaks and correct an
internal flag that was already being immediately overwritten by the time it mattered in every
observed path.
