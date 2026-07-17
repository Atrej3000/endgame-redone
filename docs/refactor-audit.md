# Refactor Audit — Ucode_Endgame

Status: updated after Pass 2 (validation and hardening). This document describes the system
**as it exists**, not how it should look. Existing code is the source of truth; this is a map,
not a redesign proposal.

**Verification-level key**, used throughout this document from Pass 2 onward:
- **Compiled** — built with zero errors using a real C11 compiler (MinGW-w64 GCC, this pass).
- **Runtime-verified** — exercised by an actual running process (the `mingw-smoketest` harness),
  not just compiled.
- **Sanitizer-verified** — would mean an ASan/UBSan-instrumented run; **not available this pass**
  (see Section 6 — no compatible toolchain in this environment; documented, not concealed).
- **Static-only** — reasoned about by reading the code and cross-referencing declarations/sizes,
  without compiling or running it.
Do not read "verified" alone as sanitizer- or runtime-level unless one of the specific labels
above is stated.

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
- **Shutdown path** (pre-Pass-2: `main.c:167-223`): ~24 texture-destroy calls were commented out
  (using stale dot-notation that would not even compile against the pointer-based `GameState`);
  only `SDL_DestroyWindow`, `free(gameState)`, `TTF_Quit()`, `SDL_Quit()` actually ran.
  `Mix_CloseAudio()` and `IMG_Quit()` were never called anywhere. `IMG_Init()` was never called
  anywhere despite ~65 `IMG_Load` call sites. **Fixed in Pass 2**: replaced with
  `app_shutdown()` (new `src/app.c`) — a single, idempotent, null-tolerant cleanup path
  destroying every texture (23 individual fields + 2 arrays + `man`/`secondPlayer`/`enemy`'s
  sprite textures + `train`/`cloud1..8`), closing the font, freeing all chunks/music, destroying
  the renderer/window, and calling `Mix_CloseAudio`/`IMG_Quit`/`TTF_Quit`/`SDL_Quit`, nulling
  every pointer as it goes. **Runtime-verified**: the `mingw-smoketest` harness calls
  `app_shutdown()` twice in a row (idempotency check) and completes without crashing, in 3
  separate runs.
- `free(gameState)` running before `SDL_DestroyRenderer(renderer)` — resolved as part of the
  same Pass-2 `app_shutdown()` rewrite (renderer/window are now destroyed before the state is
  freed, matching the task's prescribed order).
- **Sentinel design gap (found during Pass 2's mandated 7-point audit)**: Pass 1's per-mode
  "load once" guards (`loadGame()`/`loadGame2()`) were originally keyed on a single texture
  pointer (`bossTexture`/`star`) that happened to be the first or an early resource loaded in a
  ~30-resource group. Static-only re-analysis found this could not fully guarantee "a partial
  group failure cannot leave the group falsely marked as loaded" — a few `IMG_Load` sites in the
  same functions have no NULL check at all, and no call site anywhere checks whether
  `SDL_CreateTextureFromSurface` itself returned NULL. **Fixed in Pass 2**: replaced with
  explicit `bool arcadeAssetsLoaded`/`runnerAssetsLoaded` lifecycle flags set only as the very
  last statement of each guarded block. This narrows but does not fully close the gap (the
  underlying unchecked-load sites are unchanged, deferred — see Section 7, item 13).
  **Runtime-verified**: the smoke-test harness calls `load_menu1()+loadGame()` and
  `load_menu2()+loadGame2()` three times each and confirms `bossTexture`/`font`/`star` pointers
  are unchanged across repeats.
- **New defect found during Pass 2's mandated project-wide nested-loop sweep**:
  `process.c:515` — `for (int i = 0; i < NUM_ENEMIES; i += 2)` with `NUM_ENEMIES` = 101 (odd)
  writes `enemyValues[i+1]`, which on the last iteration (`i=100`) writes `enemyValues[101]` —
  one past the 101-element array, aliasing into `smartEnemies[0]`. Triggers once per Arcade
  playthrough when `tempScore` reaches exactly 366. **Fixed in Pass 2**: loop bound changed to
  `NUM_ENEMIES - 1`. **Compiled** clean; **static-only** confirmation of the corruption target
  (struct field order in `header.h`) and that no other `+= 2` loop in the file shares this odd
  bound (six sibling loops all use small even literals).
- **Leaderboard display/capacity mismatch (found during Pass 2's explicit leaderboard-bounds
  check, not fixed)**: `x_list[25]` is the declared capacity, and Pass 1's bound check
  (`processEvents.c`) correctly stops writes at 25; but `init_status_x_list()`
  (`x_list_leader.c:4`) only ever displays the first **20** entries (`for (int i = 0; i < 20;
  i++)`), not 25. Any 21st-25th recorded score is stored but never shown on the leaderboard
  screen. This predates Pass 1 and Pass 2 (neither pass changed the display loop's bound); flagged
  here as a **static-only, low-confidence-on-intent** finding rather than fixed, since there is no
  evidence (no sorting logic — `mx_sort_arr_int` is confirmed dead/unused — no comment, no level
  design reference) indicating whether 20 or 25 is the "correct" number, and guessing would risk
  inventing behavior the brief explicitly warns against.

## 6. Build system and environment limitations

- Makefile still targets macOS by default (`all`, `clang`, bundled `.framework`s under
  `resource/frameworks` via `-F`/`-framework`/`-rpath`) — **completely untouched**, still the
  default `make`/`make all` behavior.
- `CFLAGS` (macOS target only) includes `-Werror`: any compiler warning is a hard build failure.
  Not evaluated this pass (no macOS/clang toolchain available here) — the new `mingw` target uses
  its own separate, non-`-Werror` warning flag set (see below) so a warning there cannot silently
  break the macOS build definition.
- `clean:` references `$(OBJS)`, which is never set (the `OBJS` line is commented out,
  `Makefile:24`), making `make clean` a likely no-op. Not fixed this pass (Makefile/README
  changes beyond the additive MinGW targets remain deferred, per the original scope decision).
- Only `all`/`clean`/`mingw`/`mingw-dlls`/`mingw-asan`/`mingw-run`/`mingw-smoketest`/`mingw-clean`
  are marked `.PHONY`; `uninstall`/`reinstall`/`run` (macOS-side, pre-existing) are not.

### Pass 2: a real Windows/MinGW build path now exists (additive, macOS build untouched)

- **Toolchain**: MinGW-w64 GCC 15.2.0 (scoop `gcc` package, nuwen distribution) + GNU Make 4.4.1
  (scoop `make`), installed into this environment this pass.
- **SDL2 dependency source**: official MinGW "devel" packages downloaded directly from
  github.com/libsdl-org releases — `SDL2-devel-2.32.10-mingw.tar.gz`,
  `SDL2_image-devel-2.8.12-mingw.tar.gz`, `SDL2_ttf-devel-2.24.0-mingw.tar.gz`,
  `SDL2_mixer-devel-2.8.2-mingw.tar.gz` — extracted into `vendor/SDL2-mingw/` (gitignored, not
  committed; regeneratable from these exact URLs).
- **Portability fixes required to compile** (all in `inc/header.h`, all `#ifdef`-gated to be
  inert on macOS/Linux — see `refactor-log.md` Pass 2 entries for full before/after):
  `<malloc/malloc.h>` gated to `__APPLE__` only; `SDL_MAIN_HANDLED` defined under `_WIN32` (SDL2
  on Windows rewrites `main()`'s expected signature otherwise); `random()`/`srandom()` mapped to
  `rand()`/`srand()` under `_WIN32` (POSIX/BSD functions absent from the Windows CRT), routed
  through a uniquely-named wrapper to avoid colliding with a local variable named `rand` at
  `processEvents.c:439`.
- **A small include-path shim** (`vendor/SDL2-mingw/compat-include/`, three one-line forwarding
  headers) bridges the project's macOS-framework-style includes
  (`#include <SDL2_image/SDL_image.h>` etc.) to the MinGW devel packages' actual layout
  (`include/SDL2/SDL_image.h`).
- **Exact build command**: `make mingw` (see `Makefile` for the full flag set: `-std=c11 -Wall
  -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wnull-dereference
  -Wdouble-promotion`, deliberately without `-Werror` so warnings are visible/classifiable rather
  than build-breaking).
- **Result**: `make mingw` — **Compiled**, 0 errors, 169 pre-existing warnings (0 introduced by
  either pass — see Section 7's warning classification). `make mingw-smoketest` — **Runtime-
  verified**, real `app_init`/asset-guard/`app_shutdown` cycle, 3 consecutive passing runs.
  `make mingw-asan` — attempted with two different MinGW-w64 distributions (scoop `gcc`/nuwen and
  scoop `mingw-winlibs`/WinLibs 16.1.0); both fail to link (`cannot find -lasan`/`-lubsan`) because
  neither ships sanitizer runtime libraries for the `x86_64-w64-mingw32` target — a genuine,
  confirmed toolchain gap (ASan/UBSan on native Windows is primarily an MSVC/clang-cl feature, not
  a mingw-w64 GCC one), not something disabled or hidden. **No sanitizer run occurred this pass.**
- Full build/run logs saved under `docs/verification/`.

## 7. Confirmed defects catalogue

| # | File:line | Severity | Verification | Status | Description |
|---|---|---|---|---|---|
| 1 | `processEvents.c:445,452` | Critical | Compiled + static-only | **Fixed Pass 1** | `game->boss[i]` should be `game->boss[j]` — out-of-bounds read of a 2-element array for 99/101 outer-loop iterations |
| 2 | `menu_events.c:11,64`, `pause_events.c:11` | High | Compiled + static-only | **Fixed Pass 1** | `event.key.keysym.sym` read without checking `event.type == SDL_KEYDOWN` first |
| 3 | `loadGame.c:556-566` | Medium | Compiled; static-only evidence upgraded in Pass 2 (contiguity proof, see Section 5) | **Fixed Pass 1** | `ledges[96..98]` never initialized; read as garbage (worse pre-Pass-1 since `GameState` wasn't zeroed either) |
| 4 | `processEvents.c:765-768` | Medium | Compiled + static-only | **Fixed Pass 1** | `x_list[x_i]` written with no bound check against `x_list[25]` |
| 5 | `mx_strsplit.c:13` | Low (dead code) | Compiled + static-only | **Fixed Pass 1** | `malloc(sizeof(char*) * amm_words + 1)` operator-precedence bug; function has zero live callers |
| 6 | `main.c:5` (moved to `app.c` in Pass 2) | Medium | Compiled + runtime-verified (smoke test exercises `calloc`'d state every run) | **Fixed Pass 1** | `GameState` allocated with `malloc`, not zero-initialized |
| 7 | `loadGame.c` / `loadGame.c` (loadGame2) | Critical (leak) | Compiled + runtime-verified (smoke test: 3x repeat load, pointer unchanged) | **Fixed Pass 1, hardened Pass 2** | Asset loading (textures/font/music) re-run every frame while on the in-mode menu; Pass 2 replaced the pointer-sentinel guard with explicit `arcadeAssetsLoaded`/`runnerAssetsLoaded` flags (see Section 5) |
| 8 | `load_menu.c` | High (leak) | Compiled + runtime-verified | **Fixed Pass 1** | Mode background texture + `menuMus` reloaded on every repeat mode entry |
| 9 | `menu_events.c`, `pause_events.c`, `processEvents.c`, `process.c` | High (leak) | Compiled + static-only | **Fixed Pass 1** | SFX reloaded via `Mix_LoadWAV` every frame/every shot; paired free sites didn't null the pointer |
| 10 | `status.c`, `kills_score.c`, `X_score.c`, `x_list_leader.c` | Medium (leak) | Compiled + static-only | **Fixed Pass 1** | HUD text textures recreated every frame without destroying the prior instance |
| 11 | `doRender.c:254-256` (`doRender2`) | Medium | Static-only | Deferred | Render function mutates gameplay state (`gameLives--`, `isDead=0`, `y=0`) |
| 12 | `collisionDetect.c:561-622` | Low | Static-only | Deferred | Enemy-vs-enemy loop has no `i==j` guard and only sets `.w/.h` for inner index; harmless today by luck of strict inequalities |
| 13 | `loadGame.c`, `load_menu.c` (~10 sites) | Low | Static-only | Deferred | `IMG_Load`→create-texture→free-surface ordering happens before the NULL check; functionally inert (SDL handles NULL safely) but misleading. Pass 2 note: this is also the residual reason the Pass-2 asset-group lifecycle flags (item 7) can't fully guarantee "partial failure never marks the group loaded" — see Section 5. |
| 14 | Whole codebase | Informational | Static-only | Deferred | No delta-time anywhere; all movement/gravity/animation/spawn timing is frame-count-based (`game->time++` + modulus) combined with vsync |
| 15 | `mx_*.c` (16 files) | Low | Static-only (grep-confirmed zero callers) | Deferred | Confirmed zero live callers outside their own definitions/each other; deletion deferred to a dedicated dead-code pass |
| 16 | Shutdown path (was `main.c:167-223`) | High | Compiled + runtime-verified (idempotent double-`app_shutdown` call, 3 runs) | **Fixed Pass 2** | ~24 texture destroys were commented out; `Mix_CloseAudio`/`IMG_Quit` were never called; `IMG_Init` was never called. Replaced with `app_shutdown()`/`app_init()` in new `src/app.c`. |
| 17 | `process`/`process2`, `collisionDetect`/`collisionDetect2`, `processEvents`/`processEvents2`, `doRender`/`doRender2` | Informational | Static-only | Deferred | Large paired functions (180–1160 lines) with real shared sub-patterns but materially different mechanics; deduplication needs a build-verified baseline |
| 18 | `process.c:515` | Critical | Compiled + static-only (struct-layout cross-check) | **Found and fixed Pass 2** | `for (i < NUM_ENEMIES; i += 2)` with `NUM_ENEMIES`=101 (odd) wrote `enemyValues[i+1]` = `enemyValues[101]` on the last iteration — out-of-bounds write into `smartEnemies[0]`'s memory, once per Arcade run when `tempScore` hits 366. Found via Pass 2's mandated project-wide nested-loop sweep. |
| 19 | `x_list_leader.c:4` vs `x_list[25]` capacity | Low | Static-only | **Found, not fixed** (no evidence of intended value) | Leaderboard display loop only shows the first 20 of up to 25 recorded scores; pre-existing, predates both passes. Not fixed because there's no evidence (no sort logic, no comment) indicating whether 20 or 25 is "correct" — flagged rather than guessed. |

## 8. Behavior explicitly preserved (both passes)

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
- Pass 2's `app_init`/`app_shutdown` preserve the exact same window title/size/position/flags,
  renderer flags, font path/size, and audio parameters as before — only the success-path call
  sequence was restructured into two functions; failure paths are new behavior (see Section 5)
  where none existed before (previously a failed init would dereference NULL moments later with
  no diagnostic).
- Pass 2's Windows/MinGW portability shims (`SDL_MAIN_HANDLED`, `random()`/`srandom()` mapped to
  `rand()`/`srand()`, `<malloc/malloc.h>` gating) are all `#ifdef`-gated to only take effect on
  `_WIN32`/non-`__APPLE__` builds — the macOS build path compiles to the identical preprocessor
  output as before these additions.
