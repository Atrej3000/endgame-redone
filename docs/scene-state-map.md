# Scene / State Map — Ucode_Endgame

Written **before** any Phase 3 code edit, per that phase's own requirement, and updated in place
for Phase 4 (asset/session lifecycle separation — see `docs/game-session-lifecycle.md` for the
full detail). Describes the system as it existed at commit `ac00c87` (tag
`refactor-pass-2-validated`) — the pre-Phase-3 baseline — then is updated with each phase's actual
landed behavior.

**Phase 4 update**: the "Arcade/Runner in-mode menu" rows below described Phase 3's behavior
(`loadGame()`/`loadGame2()` reset gameplay state on every menu arrival). As of Phase 4, menu
arrival triggers **only** asset loading (`arcade_assets_load`/`runner_assets_load`) — session
reset (`arcade_session_reset`/`runner_session_reset`) now fires explicitly when a new game is
selected (`menu_events.c`'s `SDLK_1`/`SDLK_2`), immediately before the transition to the gameplay
scene, not merely by arriving at the menu. This is a further application of the same
"unobservable while nothing renders that state" reasoning Phase 3 already established for moving
the reset from "every frame" to "every arrival" — Phase 4 moves it one step further, from "every
arrival" to "new-game-start."

**Phase 5 update**: the Arcade/Runner gameplay rows' "Update fn(s)"/"Render fn(s)" columns below
describe the pre-Phase-5 direct calls from `main.c`. As of Phase 5, `main.c` calls one named
function per mode instead (`arcade_frame(game, window, renderer)`/`runner_frame(...)`, new
`src/frame.c`), which internally calls the exact same functions in the exact same order — a pure
naming/structural change, not a routing change. See `docs/frame-pipeline-map.md` for the full
frame-level audit (event/update/collision/render ordering, the fixed double-scene-transition risk
in `processEvents`/`processEvents2`'s game-over checks, and the relocated `doRender2()` gameplay
mutation) — that document is now the authoritative source for per-frame behavior; this file
remains authoritative for scene-to-scene routing.

## Legend

- **Old representation**: the raw `(menu0_status, menu_status)` pair that identified this screen
  before Phase 3.
- **New representation**: the single `AppScene` enum value that identifies it after Phase 3.

## Pre-Phase-3 routing summary (for reference)

Two raw `int` fields drove all routing: `menu0_status` (outer: 0=main menu, 1=Arcade, 2=Runner,
3=quit) and `menu_status` (inner, **reused with different meaning per outer mode**: 0=in-mode
menu, 1=singleplayer gameplay, 2=multiplayer gameplay, 3=leaderboard, 4=exit-mode-to-main-menu,
5=pause). `main.c` had two nested `while` loops: an outer one switching on `menu0_status`,
containing (inside cases 1 and 2) an inner one switching on `menu_status`. Confirmed via
exhaustive grep: no gameplay file (`process.c`, `collisionDetect.c`, `doRender.c`) ever read or
wrote either field — they were pure routing-layer state, which is what made a full cutover to a
single authoritative field safe.

## State table

| Scene | Old repr. | New repr. | Entry condition | Input handler | Update fn(s) | Render fn(s) | Exit transitions | Resources loaded on entry | Blocks/pauses/resets? |
|---|---|---|---|---|---|---|---|---|---|
| Main menu | `menu0_status==0` | `APP_SCENE_MAIN_MENU` | Program start, or any mode's quit-to-menu key | `menu0_events` | — | `doRender_menu0` | `b`/`1`→Arcade menu; `r`/`2`→Runner menu; `q`/`ESC`→Quit | `load_menu0()` — once only, at program start, never re-invoked (menu0/menuMus, guarded by `!=NULL`) | No gameplay to block/pause; nothing reset (there is no gameplay state relevant here) |
| Arcade in-mode menu | `menu0_status==1, menu_status==0` | `APP_SCENE_ARCADE_MENU` | From Main menu (`1`/`b`), or returning from Arcade gameplay/leaderboard/pause (ESC, game over, or pause `q`) | `menu_events` (shared with Runner menu) | *(Phase 4: none — session reset moved to new-game-start, see below)* | `doRender_menu1` | `1`→Arcade game, single-player session reset first; `2`→Arcade game, multiplayer session reset first; `3`→Arcade leaderboard; `q`→Main menu | `load_menu1()` **only when arriving from Main menu** (menu1 texture guarded by `!=NULL`; unconditional `Mix_PlayMusic(menuMus)`; resets `x_score`/`x_list`) + `arcade_assets_load()` (Phase 4; guarded by `arcadeAssetsLoaded`, fires once ever) every arrival | **Phase 4 change**: no longer resets gameplay state on arrival (that only happens now via an explicit `arcade_session_reset()` call in `menu_events.c`'s `1`/`2` handlers, right before the transition to `APP_SCENE_ARCADE_GAME`) |
| Arcade gameplay | `menu0_status==1, menu_status==1 or 2` | `APP_SCENE_ARCADE_GAME` | From Arcade menu (`1` or `2`, after `arcade_session_reset()` runs), or resuming from Arcade pause (`ESC`, no reset) | `processEvents` | `process`, `collisionDetect` | `doRender` | `ESC`→Arcade menu; `p`→Arcade pause; `q`→Main menu; lives reach 0 (game over)→Arcade menu | None (assets already loaded via Arcade menu) | Runs live; `multiPlayer` (`GameMode` as of Phase 4, previously `int`) distinguishes single/multi — both numeric sub-cases called byte-identical functions, confirmed, which is why they collapse into one scene |
| Arcade leaderboard | `menu0_status==1, menu_status==3` | `APP_SCENE_ARCADE_LEADERBOARD` | From Arcade menu (`3`) | `processEvents` (same function as gameplay — still polls all its key handlers) | — | `doRender_leaderboard` | `ESC`→Arcade menu; `q`→Main menu; `p`→Arcade pause (**pre-existing quirk, preserved**: resuming from this lands in live gameplay, never back at the leaderboard, because `processEvents` is reused verbatim and pause-resume always targets the gameplay scene) | None | Gameplay itself is not "running" but its update functions aren't called either — purely a static-texture screen driven by the shared input handler |
| Arcade pause | `menu0_status==1, menu_status==5` | `APP_SCENE_ARCADE_PAUSE` | From Arcade gameplay or leaderboard (`p`) | `pause_events` (shared with Runner pause) | — | `doRender_pause` (shared with Runner pause) | `ESC`→Arcade gameplay (resume); `q`→Main menu | None | Gameplay fully frozen (no update calls); `Mix_PauseMusic()`/`Mix_ResumeMusic()` bracket the pause (called inline in `processEvents`/`pause_events`, unchanged) |
| Runner in-mode menu | `menu0_status==2, menu_status==0` | `APP_SCENE_RUNNER_MENU` | Mirror of Arcade menu | `menu_events` (shared) | *(Phase 4: none, same as Arcade)* | `doRender_menu2` | Mirror of Arcade menu | `load_menu2()` only from Main menu + `runner_assets_load()` (Phase 4) every arrival | Mirror of Arcade menu (Phase 4 change) |
| Runner gameplay | `menu0_status==2, menu_status==1 or 2` | `APP_SCENE_RUNNER_GAME` | Mirror of Arcade gameplay | `processEvents2` | `process2`, `collisionDetect2` | `doRender2` | Mirror of Arcade gameplay (game over trigger is `gameLives==0`, appends score to `x_list` first) | None | Mirror of Arcade gameplay |
| Runner leaderboard | `menu0_status==2, menu_status==3` | `APP_SCENE_RUNNER_LEADERBOARD` | Mirror of Arcade leaderboard | `processEvents2` | — | `doRender_leaderboard2` (also calls `init_status_x_list`, pre-existing, unrelated to this phase) | Mirror of Arcade leaderboard | None | Mirror of Arcade leaderboard |
| Runner pause | `menu0_status==2, menu_status==5` | `APP_SCENE_RUNNER_PAUSE` | Mirror of Arcade pause | `pause_events` (shared) | — | `doRender_pause` (shared) | Mirror of Arcade pause | None | Mirror of Arcade pause |
| Quit | `menu0_status==3` (true quit) **or** `menu_status==4` inside a mode (routes to Main menu, NOT program exit — see note) | `APP_SCENE_QUIT` (true quit only) | Main menu's own `q`/`ESC` | — | — | — | Ends the main loop; `app_shutdown()` runs | n/a | n/a |

**Critical asymmetry, preserved exactly**: the old `menu_status==4` ("exit mode") never actually
quit the program — it only ever routed back to `menu0_status=0` (Main menu). Only the Main menu's
own quit key reached `menu0_status==3` and ended the program. Under the new model, every
mode-quit key (`processEvents.c`'s/`processEvents2.c`'s/`pause_events.c`'s `q`) targets
`APP_SCENE_MAIN_MENU`, never `APP_SCENE_QUIT` — only `menu0_events`'s own `q`/`ESC` targets
`APP_SCENE_QUIT`.

## Invariant introduced by Phase 3

`game->scene` is written **only** inside `app_change_scene()` (`src/scene.c`). Every routing
write site elsewhere calls `app_change_scene(game, TARGET)` — never assigns the field directly —
with zero exceptions, including transitions to scenes that have no enter-hook (pause, leaderboard,
gameplay, quit). This is what makes the once-per-arrival `loadGame()`/`loadGame2()` reset fire
correctly on every "land on the in-mode menu" path (from Main menu, from gameplay via ESC, from
game-over, from leaderboard/pause via `q`), not just the original main-menu-to-mode entry.

The one documented exception is the new non-interactive test harness
(`docs/verification/scene_transition_test.c`), which directly pokes `game->scene` to set up a
precondition scene before exercising `app_change_scene()` itself — a test-setup technique, not a
production code path.

## No distinct "game over" scene

Dying in either mode routes directly to that mode's in-mode-menu scene (`APP_SCENE_ARCADE_MENU`/
`APP_SCENE_RUNNER_MENU`) — the same destination as voluntarily pressing `ESC`. There is no
`STATUS_STATE_GAMEOVER` scene distinction anywhere (the `STATUS_STATE_GAMEOVER` macro itself is
defined but never assigned anywhere in the codebase, confirmed by grep). This is preserved exactly
as-is; introducing a distinct game-over scene was considered out of scope ("do not invent
transitions not supported by the current game").

## Confirmed pre-existing defects/quirks, explicitly NOT fixed by this phase

1. **`SDL_QUIT` (window-close button) during gameplay does nothing.** `processEvents`/
   `processEvents2` set a local `done=1` on `SDL_QUIT`, but `main.c` never captured that return
   value even before this phase (called as a bare statement) — the flattened loop preserves this
   exactly; clicking the window's close button while playing still has no effect.
2. **Quitting from pause frees no audio resources**, while quitting from live gameplay frees
   4-6 `Mix_Chunk`/`Mix_Music` pointers (`processEvents.c:93-107`, `:622-634` vs
   `pause_events.c:20`, which frees nothing). Both are valid "return to Main menu" transitions;
   this asymmetry predates Phase 3 and is unrelated to scene routing.
3. **Pausing while viewing the leaderboard resumes into live gameplay, not back to the
   leaderboard** (since `processEvents`'s `p` handler runs during the leaderboard case too, and
   pause-resume always targets the gameplay scene). Preserved exactly, not "fixed."
4. **`leader_events.c` is dead code** — references `menu_status` but is never called from
   `main.c` (only referenced in two commented-out lines). Left in place, undeleted, this phase.

## Legacy fields remaining after Phase 3

- `int menu_status`, `int menu0_status` — kept declared in `GameState`, commented `// DEPRECATED`.
  After this phase, nothing reads or writes them anywhere except `load_menu.c`'s own internal
  self-resets (`load_menu0/1/2()` still set them to 0 as a harmless leftover — writing to a field
  nobody reads is inert, and leaving these three lines alone was judged lower-risk than touching a
  file with no other reason to change this phase). Safe to delete entirely in a future phase.
- `multiPlayer` — deferred by Phase 3 (kept as `int`), **retyped to `GameMode` in Phase 4** (field
  name unchanged; all ~29 existing boolean-truth-test read sites across `process.c`/
  `processEvents.c`/`doRender.c`/`collisionDetect.c`/`kills_score.c` compile and behave
  identically, confirmed by the Phase 4 build). See `docs/game-session-lifecycle.md` for the full
  audit and the single-writer resolution (`arcade_session_reset()`/`runner_session_reset()`).
- `enum menu_buttons` — **deleted** this phase (not kept as legacy): confirmed zero references
  anywhere, directly superseded by `AppScene`, and its last member (`RUNNER=5`) was actively
  misleading (5 meant "pause", not "runner mode"). Judged safe and in-scope to remove rather than
  deprecate, unlike `menu_status`/`menu0_status` which had real historical call sites right up
  until this phase.
