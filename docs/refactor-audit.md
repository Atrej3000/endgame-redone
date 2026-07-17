# Refactor Audit — Ucode_Endgame

Status: baseline snapshot taken before Pass 1 changes. This document describes the system
**as it exists**, not how it should look. Existing code is the source of truth; this is a map,
not a redesign proposal.

## 1. Purpose and scope

This audit covers `src/`, `inc/`, `Makefile`, and `README.md`. Bundled vendor headers under
`resource/frameworks/` and binary assets under `resource/` are out of scope as source code
(they are referenced only as file paths that must not be broken).

Total handwritten source: ~5,456 lines across 36 `.c` files and 1 header (`inc/header.h`,
330 lines).

## 2. Architecture overview

The project is built as a single translation unit: the Makefile compiles `src/*.c` in one
`clang` invocation (no per-file object files, despite a stubbed, unused `OBJS` variable).

`inc/header.h` holds every struct definition, macro, and function prototype for the whole
program — there is no per-module header split.

### Central state: `GameState` (`inc/header.h:139-245`)

One struct owns everything:
- World/scroll: `scrollX`, `multiPlayer`
- Score/lives counters: `gameLives`, `sumScore`, `tempScore`, `shotCount`, `shotCountMultiplayer`
- Entities: `Man secondPlayer`, `Man man`, `Enemies enemy`, `Enemies boss[2]`,
  `Enemies enemyValues[NUM_ENEMIES]` (`NUM_ENEMIES` = 101), `Enemies smartEnemies[NUM_SMART_ENEMIES]`
  (10), `Train train`, `Cloud1..Cloud8 cloud1..cloud8` (eight near-identical one-off structs)
- Bullets: `Bullet *bullets[MAX_BULLETS]`, `Bullet *secondBullets[MAX_BULLETS]` (1000 each,
  individually `malloc`'d per shot)
- World geometry: `Star stars[100]`, `Ledge ledges[100]`
- Sprite-sheet frame counters: `CurrentSheetBullet`, `CurrentSheetBullet2`, `CurrentSheetBoss`,
  `CurrentSpriteBack`, `CurrentSpriteBack2`, `CurrentSpriteBack3`
- Textures: 23 individual `SDL_Texture*` fields plus two 12-element arrays (`manFrames`,
  `secondPlayerFrames`)
- Font: `TTF_Font *font` (a single shared font instance)
- Audio: `Mix_Music *menuMus, *battleMus, *runnerMus`;
  `Mix_Chunk *jumpSound, *kickSound, *select, *shootSound, *damageSound`
- Renderer: `SDL_Renderer *renderer` (stored on the state itself, set once in `main.c`)
- Scene routing: `int menu_status`, `int menu0_status` — raw integers, see Section 3
- Leaderboard: `int x_score`, `int x_list[25]`, `int x_i`, `int kills_score`, `int kills_list[25]`,
  `int kills_score_multi`
- Timers: `int time, deathCountdown`
- Misc: `int statusState`, `int iter`

`enum menu_buttons { MENU, SINGLEPLAYER, MULTIPLAYER, LEADERBOARD, EXIT, RUNNER }`
(`header.h:252-259`) is defined but **never referenced anywhere in `src/`** — every routing
site uses raw integer literals instead of these names.

## 3. Game-flow / state-machine map

Outer loop (`src/main.c:39-165`), `while(!done0)`, switches on `menu0_status`:
- `0` — main menu (`menu0_events` + `doRender_menu0`)
- `1` — Arcade/Battle mode: calls `load_menu1()` once, then enters an inner loop
- `2` — Runner mode: calls `load_menu2()` once, then enters an inner loop (mirrors case 1,
  calling the `*2`-suffixed functions throughout)
- `3` — exit (`done0 = 1`)

Inner loop (shared shape, separate `menu_status` variable, once per mode), `while(!done)`:
- `0` — in-mode menu: `loadGame()`/`loadGame2()` + `menu_events()` + `doRender_menu1/2()`
- `1` — single-player play: `process`/`collisionDetect`/`doRender`/`processEvents` (`game->multiPlayer=0`)
- `2` — multiplayer play: same functions (`game->multiPlayer=1` differentiates internally —
  there is no separate multiplayer-specific process/render function despite `doRender_multiplayer`
  existing as a file)
- `3` — leaderboard: `processEvents` + `doRender_leaderboard`
- `4` — exit inner loop (`done = 1`) → `main.c:160-164` resets `menu0_status = 0`, always
  routing back to the main menu regardless of why `done` became 1
- `5` — pause: `doRender_pause` + `pause_events`

The **same integer literals (0-5) are reused with different meaning** across `menu0_status`
and `menu_status` — a real readability trap for anyone reading isolated case labels without
knowing which variable is being switched on. Documented here; not renamed this pass.

## 4. State/ownership map

- `GameState` is allocated once (`main.c:5`), lives for the whole process, freed once
  (`main.c:216`).
- Prior to Pass 1: allocated with `malloc`, not zero-initialized, return value unchecked.
- Menu routing ints are written by `menu_events.c`/`pause_events.c`/`processEvents.c` and read
  by `main.c`'s switch statements.
- Gameplay floats/entity state are written by `process.c`/`loadGame.c` and read by
  `collisionDetect.c`/`doRender.c`.
- Textures/font/music are written by `loadGame.c`/`load_menu.c`, read by `doRender*.c`, and
  (prior to Pass 1) never destroyed by name outside a fully commented-out shutdown block.
- Leaderboard (`x_list[25]`/`x_i`, `kills_list[25]`) is in-memory only. `LEADERBOARD_TXT`
  (`header.h:32`, `"./resource/text/Fonts/leaderboard"`) and a second, inconsistent hardcoded
  path (`"../resource/files/leaderboard.txt"`, `processEvents.c:525` region) are referenced only
  inside fully commented-out `open`/`read`/`write` blocks — no leaderboard file I/O is live.

## 5. Resource-lifetime map (pre-Pass-1 baseline)

- **Textures/font/music**: `loadGame()`/`loadGame2()` (`src/loadGame.c`) were called every
  single frame while sitting on the in-mode menu (`main.c:55`, `main.c:103`, inside the
  `menu_status == 0` case of the inner loop) — turning a one-time ~30-texture + font + music
  load into an unbounded per-frame leak for as long as the player sat on that menu screen.
  **Fixed in Pass 1** via NULL-guarded one-time loading (see `refactor-plan.md`).
- **`load_menu0/1/2`** (`src/load_menu.c`) reload their mode's background texture and the shared
  `menuMus` every time the outer mode is (re-)entered from the main menu — leaked on every
  repeat visit, not just once. **Fixed in Pass 1.**
- **Sound effects** (`select`, `jumpSound`, `shootSound`, `damageSound`, `kickSound`) were
  reloaded via `Mix_LoadWAV` every frame in `menu_events()`/`menu0_events()`/`pause_events()`/
  `processEvents()`/`processEvents2()`, and every shot in `addBullet`/`addSecondBullet`, and
  every frame at the top of `process()`. Freed only conditionally on ESCAPE/quit key handling,
  and — critically — freed **without nulling the pointer**, meaning a naive "only load if not
  yet loaded" guard would have produced a new dangling-pointer bug. **Fixed in Pass 1** by
  pairing NULL-guards at load sites with `= NULL` after every free site.
- **HUD text textures** (`game->label`, `game->labelMultiplayer`) were recreated every frame in
  `init_status_lives`/`init_status_kills`/`init_status_x`/`init_status_x_list` without
  destroying the previous instance first. `init_status_x_list` was the worst offender: it loops
  20 times per call, creating 20 textures into the same shared field, leaking 19 of them per
  call. **Fixed in Pass 1** via destroy-before-recreate.
- **Shutdown path** (`main.c:167-223`): ~24 texture-destroy calls were commented out (using
  stale dot-notation that would not even compile against the pointer-based `GameState`); only
  `SDL_DestroyWindow`, `free(gameState)`, `TTF_Quit()`, `SDL_Quit()` actually ran.
  `Mix_CloseAudio()` and `IMG_Quit()` were never called anywhere. `IMG_Init()` is never called
  anywhere despite ~65 `IMG_Load` call sites. **Not touched in Pass 1** — full shutdown-path
  reconstruction is deferred (see `refactor-plan.md` deferred phases); this audit flags it as a
  known gap, not a defect fixed this pass.
- `free(gameState)` (`main.c:216`) runs before `SDL_DestroyRenderer(renderer)` (`main.c:217`) —
  an ordering smell, not fatal, since `renderer` is a separate local variable, not read through
  the freed struct at that point. Not touched this pass.

## 6. Build system and environment limitations

- Makefile targets macOS: `clang`, bundled `.framework`s under `resource/frameworks` via
  `-F`/`-framework`/`-rpath`.
- `inc/header.h:18` includes `<malloc/malloc.h>`, a macOS/BSD-only header — not portable to
  Linux or Windows.
- `CFLAGS` includes `-Werror`: any compiler warning is a hard build failure.
- `clean:` references `$(OBJS)`, which is never set (the `OBJS` line is commented out,
  `Makefile:24`), making `make clean` a likely no-op.
- Only `all`/`clean` are marked `.PHONY`; `uninstall`/`reinstall`/`run` are not.
- **This environment** (where Pass 1 was performed): Windows, not a git repository, no
  gcc/clang in PATH. **Compilation was not attempted and cannot be attempted here.** No build
  or runtime claim is made anywhere in this audit or the accompanying log — verification for
  every Pass 1 change is manual (line-by-line review, struct/array-size cross-checks, call-graph
  grep), documented per-change in `refactor-log.md`.
- Makefile/README changes are explicitly deferred this pass (see `refactor-plan.md`) — any real
  build-system fix cannot be verified without a compiler.

## 7. Confirmed defects catalogue

| # | File:line | Severity | Confidence | Status | Description |
|---|---|---|---|---|---|
| 1 | `processEvents.c:445,452` | Critical | Confirmed | **Fixed Pass 1** | `game->boss[i]` should be `game->boss[j]` — out-of-bounds read of a 2-element array for 99/101 outer-loop iterations |
| 2 | `menu_events.c:11,64`, `pause_events.c:11` | High | Confirmed | **Fixed Pass 1** | `event.key.keysym.sym` read without checking `event.type == SDL_KEYDOWN` first |
| 3 | `loadGame.c:556-566` | Medium | Confirmed | **Fixed Pass 1** | `ledges[96..98]` never initialized; read as garbage (worse pre-Pass-1 since `GameState` wasn't zeroed either) |
| 4 | `processEvents.c:765-768` | Medium | Confirmed | **Fixed Pass 1** | `x_list[x_i]` written with no bound check against `x_list[25]` |
| 5 | `mx_strsplit.c:13` | Low (dead code) | Confirmed | **Fixed Pass 1** | `malloc(sizeof(char*) * amm_words + 1)` operator-precedence bug; function has zero live callers |
| 6 | `main.c:5` | Medium | Confirmed | **Fixed Pass 1** | `GameState` allocated with `malloc`, not zero-initialized |
| 7 | `loadGame.c` / `loadGame.c` (loadGame2) | Critical (leak) | Confirmed | **Fixed Pass 1** | Asset loading (textures/font/music) re-run every frame while on the in-mode menu |
| 8 | `load_menu.c` | High (leak) | Confirmed | **Fixed Pass 1** | Mode background texture + `menuMus` reloaded on every repeat mode entry |
| 9 | `menu_events.c`, `pause_events.c`, `processEvents.c`, `process.c` | High (leak) | Confirmed | **Fixed Pass 1** | SFX reloaded via `Mix_LoadWAV` every frame/every shot; paired free sites didn't null the pointer |
| 10 | `status.c`, `kills_score.c`, `X_score.c`, `x_list_leader.c` | Medium (leak) | Confirmed | **Fixed Pass 1** | HUD text textures recreated every frame without destroying the prior instance |
| 11 | `doRender.c:254-256` (`doRender2`) | Medium | Confirmed | Deferred | Render function mutates gameplay state (`gameLives--`, `isDead=0`, `y=0`) |
| 12 | `collisionDetect.c:561-622` | Low | Confirmed | Deferred | Enemy-vs-enemy loop has no `i==j` guard and only sets `.w/.h` for inner index; harmless today by luck of strict inequalities |
| 13 | `loadGame.c`, `load_menu.c` (~10 sites) | Low | Confirmed | Deferred | `IMG_Load`→create-texture→free-surface ordering happens before the NULL check; functionally inert (SDL handles NULL safely) but misleading |
| 14 | Whole codebase | Informational | Confirmed | Deferred | No delta-time anywhere; all movement/gravity/animation/spawn timing is frame-count-based (`game->time++` + modulus) combined with vsync |
| 15 | `mx_*.c` (16 files) | Low | Confirmed | Deferred | Confirmed zero live callers outside their own definitions/each other; deletion deferred to a dedicated dead-code pass |
| 16 | Shutdown path, `main.c:167-223` | High | Confirmed | Deferred | ~24 texture destroys commented out; `Mix_CloseAudio`/`IMG_Quit` never called; `IMG_Init` never called |
| 17 | `process`/`process2`, `collisionDetect`/`collisionDetect2`, `processEvents`/`processEvents2`, `doRender`/`doRender2` | Informational | Confirmed | Deferred | Large paired functions (180–1160 lines) with real shared sub-patterns but materially different mechanics; deduplication needs a build-verified baseline |

## 8. Behavior explicitly preserved this pass

- Frame-count-based timing (`game->time++`, modulus checks) combined with vsync — not converted
  to delta-time.
- Exact keybindings and menu routing values (README's documented controls: `1`/`2`/`3` mode
  select, `w a d` + `SPACE` for player 1, `up left right` + `0` for player 2, `p` pause,
  `ESC`/`q` semantics per mode) — verified against live code, found accurate, unchanged.
- All existing gameplay constants (collision box sizes, speeds, gravity, spawn thresholds)
  untouched.
- Large paired mode functions not deduplicated.
- The existing (odd but intentional-looking) behavior that leaderboard history
  (`x_score`/`x_list`) resets every time a mode is re-entered from the main menu — preserved,
  not "fixed," since it's out of scope to judge whether it's a defect.
- Visual output of HUD text, menu backgrounds, and all sprites is unchanged — Pass 1's
  resource-lifetime fixes only change *when memory is freed/reloaded*, never what is drawn.
