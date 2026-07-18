# Refactor Log

This file records every change actually made, in chronological order. One entry per change,
appended only — never edited retroactively (corrections get a new entry).

## Pass 1 summary (2026-07-17)

12 changes applied across `src/processEvents.c`, `src/menu_events.c`, `src/pause_events.c`,
`src/loadGame.c`, `src/load_menu.c`, `src/process.c`, `src/main.c`, `src/mx_strsplit.c`,
`src/status.c`, `src/kills_score.c`, `src/X_score.c`, `src/x_list_leader.c` (entries below).
Covers Phases 0-3 of `docs/refactor-plan.md`. No git repository was available in this
environment (declined by the user) and no C compiler was available (Windows environment; the
project's own Makefile is macOS-specific regardless). Verification for every change was manual:
re-reading each changed region, cross-checking struct/array sizes against `inc/header.h`, grepping
for every read/write of every guarded field to rule out a missed call site, and a final
brace-count sanity check across all 12 touched files (open `{` count equals close `}` count in
every file). No build was attempted; no build or runtime claim is made anywhere in this log.

## Pass 2 summary — validation and hardening (2026-07-17)

This pass turned Pass 1's uncompiled patch into a compiled, partially runtime-verified,
git-protected baseline. Environment: Windows, no compiler previously available. This pass
installed MinGW-w64 GCC (scoop `gcc`, nuwen 15.2.0) and the official SDL2/SDL2_image/SDL2_ttf/
SDL2_mixer MinGW "devel" packages (from github.com/libsdl-org releases, vendored locally and
gitignored under `./vendor/`), added an additive `mingw`/`mingw-asan`/`mingw-smoketest` Makefile
target family (the macOS `all` target and its flags are untouched), and initialized git
(`git init` + baseline commit `cfb7ca7`) since none existed and the user asked for a real
rollback point this pass. See `docs/refactor-plan.md` for the full build-environment writeup and
`docs/verification/` for saved build/run logs. Every entry below states precisely how it was
verified: compilation, a real runtime smoke test, or static inspection — sanitizer instrumentation
was attempted but is unavailable in this environment (see its own entry below).

## Entry template

### [YYYY-MM-DD] Short title
- Phase: (phase number/name from refactor-plan.md)
- File(s): path:lines
- Before:
  ```c
  (exact original snippet)
  ```
- After:
  ```c
  (exact new snippet)
  ```
- Reasoning: (why, one-two sentences)
- Behavior impact: (preserving | deliberate change: what changes and why it's a defect fix)
- Verification performed: (e.g. manual re-read of surrounding lines; cross-checked struct
  layout in header.h; no compiler available in this environment, compilation not attempted)
- Rollback: (paste Before block back over After block at file:lines)

---

(Entries begin below, added as Pass 1 changes are made.)

### [2026-07-17] Fix out-of-bounds boss[] read in processEvents.c
- Phase: 1 (Priority-1 correctness fixes)
- File(s): src/processEvents.c:445, src/processEvents.c:452
- Before:
  ```c
            if (collide2d(game->man.x, game->man.y, game->boss[i].x, game->boss[i].y, 48, 48, 32, 32))
            {
                game->man.lives = 0;
                game->man.y = 1000;
            }
            if (game->multiPlayer)
            {
                if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->boss[i].x, game->boss[i].y, 30, 30, 30, 30))
  ```
- After:
  ```c
            if (collide2d(game->man.x, game->man.y, game->boss[j].x, game->boss[j].y, 48, 48, 32, 32))
            {
                game->man.lives = 0;
                game->man.y = 1000;
            }
            if (game->multiPlayer)
            {
                if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->boss[j].x, game->boss[j].y, 30, 30, 30, 30))
  ```
- Reasoning: outer loop `i` runs 0..100 (NUM_ENEMIES), but `boss[2]` only has indices 0-1; the correctly-scoped inner loop variable `j` (0..1) was already used correctly two lines below (line 458) in the same block, confirming `j` was the intended index here too.
- Behavior impact: deliberate defect fix. For 99 of 101 outer iterations, boss-vs-player collision now checks the real 2 boss instances instead of reading out of bounds into adjacent struct memory (`enemyValues[NUM_ENEMIES]` immediately follows `boss[2]` in GameState).
- Verification performed: re-read processEvents.c:404-475 after the edit to confirm `j` is used consistently through the whole inner loop block; cross-checked `boss[2]`/`enemyValues[NUM_ENEMIES]` declaration order in inc/header.h:156-157. No compiler available in this environment; compilation not attempted.
- Rollback: paste the Before block back over processEvents.c:445 and :452.

### [2026-07-17] Guard keyboard-union reads with event.type check
- Phase: 1 (Priority-1 correctness fixes)
- File(s): src/menu_events.c:9-55 (menu_events), src/menu_events.c:65-101 (menu0_events), src/pause_events.c:9-27 (pause_events)
- Before (each site, same shape):
  ```c
  while (SDL_PollEvent(&event))
  {
      switch (event.key.keysym.sym) {
          ...
      }
  }
  ```
- After (each site, same shape):
  ```c
  while (SDL_PollEvent(&event))
  {
      if (event.type == SDL_KEYDOWN)
      {
      switch (event.key.keysym.sym) {
          ...
      }
      }
  }
  ```
- Reasoning: `event.key.keysym.sym` is a union member only meaningfully populated for keyboard events; reading it unconditionally means any non-keydown event delivered in the same poll batch (mouse motion, window focus, etc.) is read as if it were a key press, which can spuriously hit `default:` and silently reset `menu_status`/`menu0_status`. The already-correct pattern in `processEvents.c:25-27` (`case SDL_KEYDOWN: { switch(event.key.keysym.sym) ... }`) was mirrored here.
- Behavior impact: preserving for the common case (a real keydown event with no interleaved other events, which is what a human key press produces); only changes the buggy edge case where a non-keyboard event was previously misread as a key.
- Verification performed: re-read all three full files after editing to confirm brace balance (one added `if` open-brace and one added close-brace per site, switch body untouched) and that no case label or handler body changed. No compiler available in this environment; compilation not attempted.
- Rollback: remove the added `if (event.type == SDL_KEYDOWN) { ... }` wrapper (and its matching closing brace) at each of the three sites, restoring the bare `switch` directly under `while (SDL_PollEvent(&event)) {`.

### [2026-07-17] Extend uninitialized ledges[96..98] fill
- Phase: 1 (Priority-1 correctness fixes)
- File(s): src/loadGame.c:556 (loop bound)
- Before:
  ```c
  for (int i = 87; i < 96; i++)
  ```
- After:
  ```c
  for (int i = 87; i < 99; i++)
  ```
- Reasoning: the established range 87-95 filled `w=48,h=48,x=(i-73)*48,y=0`; indices 96-98 were never written by any loop before the explicit `ledges[99]` assignment right after, leaving them as uninitialized (pre-Pass-1: heap-garbage) `Ledge` structs read every frame by `collisionDetect()`/`doRender()`.
- Behavior impact: judgment call, flagged as an assumption (no level-design reference exists to confirm 96-98 were meant to continue the 87-95 pattern rather than being a distinct short segment) — chose to extend the established pattern since it is the smaller, more consistent assumption, and strictly safer than leaving 3 ledges as heap garbage regardless.
- Verification performed: re-read loadGame.c:540-567 after the edit to confirm indices 96, 97, 98 are now covered and `ledges[99]`'s explicit override is unchanged; cross-checked `Ledge ledges[100]` size in inc/header.h:176. No compiler available; compilation not attempted.
- Rollback: change `i < 99` back to `i < 96` at loadGame.c:556.

### [2026-07-17] Bound-check leaderboard x_list write in processEvents2
- Phase: 1 (Priority-1 correctness fixes)
- File(s): src/processEvents.c:765-771
- Before:
  ```c
      if (game->gameLives == 0)
      {
          game->x_list[game->x_i] = game->x_score;
          game->x_i++;

          //
          /*char buff[1] = " ";
  ```
- After:
  ```c
      if (game->gameLives == 0)
      {
          if (game->x_i < 25)
          {
              game->x_list[game->x_i] = game->x_score;
              game->x_i++;
          }

          //
          /*char buff[1] = " ";
  ```
- Reasoning: `x_list` is `int x_list[25]` (inc/header.h:232); `x_i` is only reset at program start and on mode-entry (load_menu1/load_menu2), and increments here once per completed runner run with no prior bound check.
- Behavior impact: preserving for any session with fewer than 25 completed runner-mode runs (the overwhelmingly common case); beyond 25 runs, now silently stops recording instead of writing past the array into adjacent GameState fields (kills_score, kills_list at header.h:234-236).
- Verification performed: re-read processEvents.c:763-791 after editing to confirm brace balance and that the commented-out block and `game->menu_status = 0;` afterward are unaffected. No compiler available; compilation not attempted.
- Rollback: remove the `if (game->x_i < 25) { ... }` wrapper at processEvents.c:767-771, restoring the two unconditional statements.

### [2026-07-17] Fix mx_strsplit.c malloc operator-precedence bug
- Phase: 1 (Priority-1 correctness fixes)
- File(s): src/mx_strsplit.c:13
- Before:
  ```c
      arr = malloc(sizeof(char *) * amm_words + 1);
  ```
- After:
  ```c
      arr = malloc(sizeof(char *) * (amm_words + 1));
  ```
- Reasoning: original expression allocates `amm_words * sizeof(char*) + 1` bytes (one extra byte) rather than `(amm_words + 1) * sizeof(char*)` (one extra pointer slot, needed for the `arr[j] = NULL;` sentinel written after the loop).
- Behavior impact: no live behavior change — confirmed via grep that `mx_strsplit` has zero callers anywhere in src/ outside its own commented-out test `main()` in the same file. Function remains unreachable; only the dead code's correctness improved. Deletion of the function itself is deferred (Phase 9, mx_* dead-code removal).
- Verification performed: grepped `mx_strsplit` across all of src/*.c to confirm no live caller before and after the change.
- Rollback: revert to `malloc(sizeof(char *) * amm_words + 1);` at mx_strsplit.c:13.

### [2026-07-17] GameState allocation: malloc -> calloc
- Phase: 2 (deterministic initialization)
- File(s): src/main.c:5
- Before:
  ```c
      GameState *gameState =  (GameState*)malloc(sizeof(GameState));
  ```
- After:
  ```c
      GameState *gameState =  (GameState*)calloc(1, sizeof(GameState));
  ```
- Reasoning: single-line, single-file change with no control-flow implications; every texture/font/sound/entity field in GameState is written by loadGame()/load_menu*() before doRender*()/process*() ever reads it, so zero-initializing has no effect on any currently-reviewed code path except making previously-uninitialized reads (e.g. the ledges[96..98] gap fixed above) deterministically zero instead of heap-garbage. Also the enabling precondition for the NULL-guarded one-time asset loading applied next (Phase 3).
- Behavior impact: preserving for all code paths traced; strictly safer for any latent uninitialized-field read not yet discovered.
- Verification performed: confirmed no field is read before its first explicit write in loadGame()/load_menu*()/main.c's own explicit x_score/x_list zeroing loop. No compiler available; compilation not attempted.
- Rollback: revert to `malloc(sizeof(GameState));` at main.c:5.

### [2026-07-17] Guard loadGame()/loadGame2() asset loading with one-time NULL-guards
- Phase: 3 (eliminate per-frame asset reloading)
- File(s): src/loadGame.c:3-345 (loadGame), src/loadGame.c:576-906 (loadGame2)
- Before: both functions unconditionally ran ~30 IMG_Load/SDL_CreateTextureFromSurface calls plus a Mix_LoadMUS and TTF_OpenFont every single call; both were called every frame while sitting on the in-mode menu (main.c:55 for loadGame, main.c:103 for loadGame2), via `case 0: loadGame(gameState); ...` / `case 0: loadGame2(gameState); ...`.
- After: wrapped the entire asset-loading portion of each function (everything from the local `surface` declaration through the font-load block) in a one-time guard:
  ```c
  void loadGame(GameState *game)
  {
      if (game->bossTexture == NULL)
      {
      ... (unchanged asset loading code) ...
      }

      game->label = NULL;
      // ... unchanged gameplay-state reset code continues, runs every call ...
  ```
  ```c
  void loadGame2(GameState *game)
  {
      if (game->star == NULL)
      {
      ... (unchanged asset loading code) ...
      }

      game->label = NULL;
      // ... unchanged gameplay-state reset code continues, runs every call ...
  ```
- Reasoning: each function fuses two concerns — one-time asset acquisition and per-entry gameplay-state reset (positions, lives, ledges, bullets). The state-reset portion must keep running every call (that's what gives "start a new game" a clean slate); only the asset-loading portion needed to stop repeating. `bossTexture` was chosen as loadGame()'s sentinel and `star` as loadGame2()'s because they are each set exactly once, inside only their own function, and read only by that same mode's render path (verified: `game->bossTexture` is read only at doRender.c:106, inside `doRender()`/Battle mode; `game->star` is read only at doRender.c:263, inside `doRender2()`/Runner mode) — so using `game->font` (which both functions also set, to the same shared field) as the sentinel was considered and rejected: it would have caused whichever mode's loader ran second in a session to skip its own asset loading entirely, since the other mode's loader would already have set `font` non-NULL. Relies on Phase 2's `calloc` change so both sentinel fields start `NULL`.
- Behavior impact: preserving. First entry into each mode still loads all its textures/font/music exactly as before; every subsequent frame spent on that mode's in-mode menu now skips the reload instead of repeating it, eliminating the leak. Gameplay-state reset (score/lives/position/ledges) is untouched and still runs every single call, exactly as before.
- Verification performed: grepped `game->star`/`game->bossTexture` across all of src/ to confirm each is read only by its own mode's render function (doRender.c:106 vs :263) before committing to them as sentinels; re-read both functions in full after editing to confirm brace balance (one inserted open-brace + one inserted close-brace per function, no other line changed) and that the gameplay-state-reset code following each guarded block is byte-for-byte unchanged. No compiler available in this environment; compilation not attempted.
- Rollback: remove the `if (game->bossTexture == NULL) { ... }` wrapper (and matching close-brace before `game->label = NULL;`) from `loadGame()`, and the `if (game->star == NULL) { ... }` wrapper from `loadGame2()`.

### [2026-07-17] Guard load_menu0/1/2 texture and menuMus reload
- Phase: 3 (eliminate per-frame asset reloading)
- File(s): src/load_menu.c (all three functions, full file rewritten with guards)
- Before (shape, per function): unconditional `Mix_LoadMUS` into `game->menuMus`, unconditional `IMG_Load`+`SDL_CreateTextureFromSurface`+`SDL_FreeSurface` into the mode's texture field, then `Mix_PlayMusic`, then the NULL check — every call (i.e. leaked on every repeat entry into that outer mode from the main menu, not just once).
- After (shape, per function): `menuMus` load wrapped in `if (game->menuMus == NULL)`; texture load + its NULL-check-and-exit wrapped in `if (game->menuX == NULL)` (X = 0/1/2); `menu0_status`/`menu_status` reset, `Mix_PlayMusic(game->menuMus, -1)`, and (for load_menu1/load_menu2) the `x_score`/`x_list` reset loop all remain unguarded, running every call exactly as before.
- Reasoning: these loaders are called once per outer-mode entry (not per-frame), but re-entering the same mode multiple times in one session (main menu -> Battle -> main menu -> Battle again) reloaded the same texture and music every time, orphaning the previous handles. `menu0`/`menu1`/`menu2` are independent fields per function, making them safe, unambiguous per-function sentinels; `menuMus` is shared across all three but the guard is written identically in each, so whichever of the three runs first loads it and the other two correctly skip the reload.
- Behavior impact: preserving. Music still (re)starts via `Mix_PlayMusic` on every mode entry (unconditional, unchanged); leaderboard-reset quirk on `load_menu1`/`load_menu2` re-entry is unchanged; only the texture/music *reload* is eliminated on repeat entries.
- Verification performed: re-read the full file after editing to confirm brace balance in all three functions and that `surface` stays properly scoped inside each guarded block (declared and used only there, matching the pattern already applied to loadGame()/loadGame2()). No compiler available; compilation not attempted.
- Rollback: remove the `if (game->menuMus == NULL) { ... }` and `if (game->menuX == NULL) { ... }` wrappers in each of the three functions, restoring the unconditional loads.

### [2026-07-17] Guard SFX reload sites and null pointers after every Mix_FreeChunk/Mix_FreeMusic
- Phase: 3 (eliminate per-frame asset reloading)
- File(s): src/menu_events.c (both functions, `select`), src/pause_events.c (`select`), src/processEvents.c:8-12 and :561-568 (`jumpSound`/`select`/`shootSound` load guards in `processEvents`/`processEvents2`), src/processEvents.c:41-48, :95-106, :595-598, :624-633 (null-after-free at all four ESCAPE/quit handlers), src/process.c:6-8, :42-44 (`shootSound` guard in `addBullet`/`addSecondBullet`), src/process.c:80-85 (`damageSound`/`kickSound` guard in `process()`)
- Before: every one of these `Mix_LoadWAV` calls ran unconditionally every frame (or every shot, for `addBullet`/`addSecondBullet`); every `Mix_FreeChunk`/`Mix_FreeMusic` call freed the chunk/music without setting the field back to `NULL` afterward.
- After: each load site is now `if (game-><field> == NULL) { game->field = Mix_LoadWAV(...); }`; each free site now has `game->field = NULL;` immediately after the corresponding `Mix_FreeChunk`/`Mix_FreeMusic` call.
- Reasoning: the load guards alone would have been unsafe without the null-after-free companion fix — without it, a freed `Mix_Chunk`/`Mix_Music` pointer stays non-NULL (dangling), so a guard checking `== NULL` would wrongly conclude "already loaded" and skip reloading, leaving a dangling pointer that gets played the next time that sound is triggered (a new use-after-free that did not exist before this pass, since the old code masked the dangling pointer by reloading fresh every frame regardless). Doing both together restores the correct cycle: load once -> play many times -> free on exit -> pointer is NULL again -> reload correctly on next entry.
- Behavior impact: preserving. Every sound still plays with the same timing and volume as before; the only change is that the underlying `Mix_Chunk`/`Mix_Music` resource is acquired once per "session" (bounded by the existing free-on-ESCAPE/quit points) instead of every single frame or every single shot.
- Verification performed: grepped every `Mix_LoadWAV`/`Mix_LoadMUS`/`Mix_FreeChunk`/`Mix_FreeMusic` call across src/*.c after editing to confirm every load site has a matching guard and every free site has a matching null-out, with no site missed. Re-read each changed region to confirm brace/statement structure. No compiler available; compilation not attempted.
- Rollback: remove each `if (game->field == NULL) { ... }` wrapper around a load call, and remove each `game->field = NULL;` line immediately following a `Mix_FreeChunk`/`Mix_FreeMusic` call, at the file:line locations listed above.

### [2026-07-17] Destroy-before-recreate for shared HUD label textures
- Phase: 3 (eliminate per-frame asset reloading)
- File(s): src/status.c (`init_status_lives`), src/kills_score.c (`init_status_kills`, both `label` and `labelMultiplayer`), src/X_score.c (`init_status_x`), src/x_list_leader.c (`init_status_x_list`)
- Before: each function unconditionally did `game->label = SDL_CreateTextureFromSurface(...)` (or `labelMultiplayer`), overwriting the previous frame's texture without destroying it. `init_status_x_list` loops 20 times per call, so 19 of the 20 textures it created were leaked on every single call before this fix.
- After: immediately before each `game->label = SDL_CreateTextureFromSurface(...)` (and the `labelMultiplayer` equivalent), added:
  ```c
  if (game->label) {
      SDL_DestroyTexture(game->label);
      game->label = NULL;
  }
  ```
- Reasoning: `game->label` is a single shared field reused by four different HUD subsystems depending on which mode/screen is active; the fix is safe regardless of which function last wrote it, since it only checks the field's current value, not which subsystem owns it.
- Behavior impact: preserving. Visible HUD output (text, position, color) is unchanged; only the previous texture is now freed before the new one replaces it.
- Verification performed: re-read all four changed files after editing to confirm the destroy-then-recreate ordering and that `shutdown_status_*` (called at unrelated state-transition points) still safely destroys whatever the current `label`/`labelMultiplayer` value is. No compiler available; compilation not attempted.
- Rollback: remove the `if (game->label) { SDL_DestroyTexture(...); game->label = NULL; }` (or `labelMultiplayer`) block immediately preceding each texture-creation line in the four files listed above.

### [2026-07-17] Destroy label texture before nulling in loadGame()/loadGame2()
- Phase: 3 (eliminate per-frame asset reloading)
- File(s): src/loadGame.c:347 (loadGame), src/loadGame.c (loadGame2, corresponding line)
- Before:
  ```c
      game->label = NULL;
  ```
- After:
  ```c
      if (game->label) {
          SDL_DestroyTexture(game->label);
      }
      game->label = NULL;
  ```
- Reasoning: noticed while verifying the HUD-texture-leak fix (previous entry) that `loadGame()`/`loadGame2()` also unconditionally null `game->label` as part of their per-call state-reset section (which runs every frame while sitting on the in-mode menu), without destroying whatever texture it may have been pointing to — the same class of leak as the `init_status_*` functions, just triggered from a different call site. Fixed for consistency since it's the identical pattern already applied elsewhere in this same phase.
- Behavior impact: preserving. `game->label` still ends up `NULL` afterward exactly as before; only the previously-referenced texture (if any) is now freed first instead of orphaned.
- Verification performed: re-read both call sites after editing; brace count for the whole file confirmed balanced (99 open / 99 close) after the change. No compiler available; compilation not attempted.
- Rollback: remove the `if (game->label) { SDL_DestroyTexture(game->label); }` line immediately before each `game->label = NULL;` in loadGame.c.

---

### [2026-07-17] Pass 2 — git baseline
- Phase: rollback safety (pre-Pass-2)
- File(s): repository-wide (`git init` + `git add -A` + commit)
- Action: initialized git in the repo root (none existed) and committed the complete Pass-1 state as commit `cfb7ca7` ("chore: preserve initial correctness and resource-lifetime fixes"). `.gitignore` (already present in the working tree) correctly excludes build artifacts; nothing else was excluded.
- Reasoning: this pass's own instructions require a reliable rollback mechanism before further edits; the user had earlier declined git for Pass 1 but explicitly asked for it in this pass.
- Verification performed: `git log -1`, `git status --short` reviewed before committing (1338 files staged: source, docs, and the project's own pre-existing vendored resources/frameworks — nothing unexpected).
- Rollback: `git reset --hard cfb7ca7` restores exactly this point (not run in this pass; noted for the user's reference only, since it is a destructive command).

### [2026-07-17] Pass 2 — Windows/MinGW portability shims in inc/header.h
- Phase: 3 (build validation path)
- File(s): inc/header.h
- Before:
  ```c
  #pragma once

  //our frameworks
  #include <SDL2/SDL.h>
  ...
  #include <math.h>
  #include <malloc/malloc.h>
  #include <limits.h>
  ```
- After: three additive, `#ifdef`-gated blocks, each inert on macOS/Linux:
  1. `#include <malloc/malloc.h>` wrapped in `#ifdef __APPLE__` (confirmed via grep that no Apple-specific malloc extension, e.g. `malloc_size`, is used anywhere in src/ — the include only ever needed `malloc`/`free`, already available via the already-included `<stdlib.h>`).
  2. `#ifdef _WIN32 #define SDL_MAIN_HANDLED #endif` before the SDL includes, because SDL2 on Windows rewrites `main()` to expect `(int argc, char *argv[])` so `SDL2main` can hook `WinMain`; this project's `main()` takes no arguments.
  3. `#ifdef _WIN32` shim mapping `random()`/`srandom()` (POSIX/BSD, no Windows-CRT equivalent) to `rand()`/`srand()`, routed through a uniquely-named wrapper function (`ucode_endgame_win32_random`) rather than a bare `rand()` expansion, because `src/processEvents.c:439` declares a local variable literally named `rand` (`int rand = random() % 2;`) that would otherwise shadow the C library `rand()` the macro expands to.
- Reasoning: none of these affect macOS/Linux compilation at all (every block is `#ifdef`-gated to a platform that was never buildable here anyway); each was required to get any Windows compilation at all, discovered by iterating on real compiler errors, not guessed in advance.
- Behavior impact: none on macOS (unchanged). On Windows, `random()` now returns `rand()`'s value range instead of glibc/BSD `random()`'s — a different PRNG algorithm/range, scoped only to this validation build; the shipped macOS build's randomness is untouched.
- Verification performed: compiled clean (0 errors) after each addition, iteratively, using the real MinGW GCC toolchain (see build log below).
- Rollback: remove all three `#ifdef`-gated blocks from inc/header.h.

### [2026-07-17] Pass 2 — additive MinGW build path (Makefile, vendor/, compat-include shims)
- Phase: 3 (build validation path)
- File(s): Makefile (new `mingw`/`mingw-dlls`/`mingw-asan`/`mingw-run`/`mingw-smoketest`/`mingw-clean` targets appended; `all`/`clean`/`run` and all existing variables untouched), `.gitignore` (added `/vendor/` and `/build-mingw/`), new `vendor/SDL2-mingw/compat-include/{SDL2_image,SDL2_ttf,SDL2_mixer}/*.h` (one-line forwarding headers so the project's macOS-framework-style `#include <SDL2_image/SDL_image.h>` etc. resolve against the MinGW devel packages' actual layout, `include/SDL2/SDL_image.h`)
- Action: installed MinGW-w64 GCC 15.2.0 and GNU Make 4.4.1 via `scoop`; downloaded the official SDL2/SDL2_image/SDL2_ttf/SDL2_mixer MinGW "devel" packages (SDL2-devel-2.32.10-mingw.tar.gz, SDL2_image-devel-2.8.12-mingw.tar.gz, SDL2_ttf-devel-2.24.0-mingw.tar.gz, SDL2_mixer-devel-2.8.2-mingw.tar.gz, all from github.com/libsdl-org/*/releases) into `vendor/SDL2-mingw/` (gitignored, not committed — regeneratable from the documented URLs in docs/refactor-plan.md).
- Reasoning: user explicitly chose the native-MinGW validation path over a Docker-based Linux container. macOS Makefile (`all`, `CC := clang`, `.framework` flags) is completely untouched; the new targets are additive and self-contained.
- Verification performed: `make mingw` produces `build-mingw/endgame-mingw.exe` plus the four required runtime DLLs; confirmed via `ls`.
- Rollback: delete the appended Makefile section, the two `.gitignore` lines, and `vendor/`.

### [2026-07-17] Pass 2 — strict-warning compilation and classification
- Phase: 3 (build validation path)
- File(s): none changed by this entry (classification only; the one fix it prompted is its own entry below)
- Action: compiled all 36 `src/*.c` files with `-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wnull-dereference -Wdouble-promotion` via the new `mingw` target. Result: 0 errors, 169 warnings.
- Classification (full list cross-referenced against every line touched in Pass 1 and Pass 2): **zero warnings fall on any line either pass modified.** All 169 are pre-existing code that had never been compiled with these flags before. Breakdown: `-Wfloat-conversion`/`-Wdouble-promotion` (100, mostly `process.c`/`collisionDetect.c`/`doRender.c` float-literal-vs-`float`-field arithmetic in physics/collision code — pre-existing, high-risk-in-principle but empirically safe today since all values involved stay well within float's exact-integer range), `-Wconversion` (58, mostly `int`-to-`float` narrowing assigning small screen-coordinate values — pre-existing, lower priority), `-Wshadow` (6, variable name reuse across sibling `if` blocks in `kills_score.c`/`doRender.c` — pre-existing, real but inert maintainability risk), `-Wsign-conversion` (5, `int`-to-`size_t`/`unsigned` at `main.c:11`'s `srandom` call and inside the confirmed-dead `mx_*.c` string-utility layer — pre-existing, dead-code-scoped where applicable). Full log saved at `docs/verification/mingw-build-strict-warnings.log`.
- Behavior impact: none (compilation/classification only).
- Verification performed: exhaustive line-number cross-reference between every warning and every line changed in this and the prior pass; zero overlap found.
- Rollback: n/a (no code change).

### [2026-07-17] Pass 2 — one warning fixed (introduced by this pass's own new code)
- Phase: 3 (build validation path)
- File(s): src/app.c
- Before: `srandom((int)time(NULL));`
- After: `srandom((unsigned int)time(NULL));`
- Reasoning: this exact line was relocated from main.c into the new `app_init()` (see its own entry below); under strict warnings it produced one `-Wsign-conversion` warning (`srandom`/`srand` expects `unsigned int`). Since the line is now part of newly-written code this pass, fixed it directly rather than classifying it as pre-existing.
- Behavior impact: none — `(unsigned int)time(NULL)` and `(int)time(NULL)` produce the same bit pattern passed to `srand`, just without the sign-conversion warning.
- Verification performed: rebuilt; warning count dropped from 169 to 168, confirming this was the only warning in the new code.
- Rollback: revert the cast to `(int)`.

### [2026-07-17] Pass 2 — replaced implicit texture-pointer sentinels with explicit lifecycle flags
- Phase: 3 (asset-lifecycle hardening, response to the mandated 7-point sentinel audit)
- File(s): inc/header.h (new `bool arcadeAssetsLoaded, runnerAssetsLoaded` fields), src/loadGame.c (guard conditions and flag-set statements in both `loadGame()` and `loadGame2()`)
- Before: `loadGame()` guarded its ~30-resource asset-load block on `if (game->bossTexture == NULL)`, and `loadGame2()` on `if (game->star == NULL)` — each an individual texture that happened to be the first (or an early) resource loaded in its group.
- After: guards changed to `if (!game->arcadeAssetsLoaded)` / `if (!game->runnerAssetsLoaded)`, with `game->arcadeAssetsLoaded = true;` / `game->runnerAssetsLoaded = true;` added as the very last statement inside each guarded block (immediately before its closing brace), not tied to any individual texture.
- Reasoning: audited against the 7 required conditions (NULL-initialized: yes, via Phase-2's `calloc`; assigned only after load: yes for the new flags, since they're now set last; never destroyed while siblings remain expected: not yet applicable, no in-play "unload" event exists; reset to NULL on unload: n/a for the same reason, handled at final shutdown instead; **partial-failure-cannot-falsely-mark-loaded: the OLD pointer-sentinel design could NOT fully guarantee this** — a few `IMG_Load` sites in `loadGame()`/`loadGame2()` have no NULL check at all, and no call site anywhere checks whether `SDL_CreateTextureFromSurface` itself returned NULL, so if the specific texture used as the sentinel happened to load successfully while a *later* texture in the same ~30-resource block silently failed, the group would be marked "loaded" while a texture inside it was actually NULL; re-entry: correct — mode re-entry within one process run intentionally does not reload, by design, this is the leak fix from Pass 1; cross-mode reuse: confirmed safe by grep, `bossTexture` is read only in `doRender()`/Arcade and `star` only in `doRender2()`/Runner). Per the task's own prescribed remediation ("replace implicit pointer sentinels with explicit lifecycle state"), introduced the two boolean flags.
- Behavior impact: preserving for the checked/successful-load path (the overwhelming common case, since virtually every `IMG_Load` failure still calls `exit(1)` immediately, as before) — the flag is only reachable at the end of a run that didn't already terminate the process. **Not fully closed**: the handful of unchecked `IMG_Load` sites and the universally-unchecked `SDL_CreateTextureFromSurface` return values remain pre-existing gaps (documented, deferred — see `docs/refactor-plan.md` Phase 7); the flag-relocation fix narrows but does not eliminate the theoretical partial-failure window, and this is stated plainly rather than overclaimed.
- Verification performed: rebuilt (0 errors, same 169→168 warning count, no new warnings); grepped to confirm `game->arcadeAssetsLoaded`/`runnerAssetsLoaded` are each set exactly once, inside the correct function; smoke test (see below) empirically exercises this guard by calling `load_menu1()+loadGame()` and `load_menu2()+loadGame2()` three times each and confirming the resulting texture/font pointers are identical across calls.
- Rollback: revert the two guard conditions to `game->bossTexture == NULL`/`game->star == NULL`, remove the two flag-set statements, and remove the two new `GameState` fields.

### [2026-07-17] Pass 2 — re-evaluated the ledges[96..98] fix with contiguity evidence
- Phase: 1 (re-verification, no further code change)
- File(s): src/loadGame.c (no change this entry — evidence-gathering only)
- Action: enumerated every ledge-initialization `for` loop in `loadGame()` in declaration order: `[0,12) [12,24) [24,32) [32,40) [40,51) [51,59) [59,67) [67,78) [78,87) [87,99)`, then `ledges[99]` set explicitly. This is a perfectly contiguous, non-overlapping sequence covering indices 0–98 with zero gaps, immediately followed by a single explicit override of index 99 — and `loadGame2()` (Runner mode) independently uses the exact same two-part shape (a full `i<100` algorithmic loop, then an explicit override of `ledges[99]` only).
- Reasoning: the task asked whether the original `i<96` bound (leaving 96/97/98 uninitialized) was an intentional gap or an off-by-three loop-bound bug. The perfect contiguity of the corrected sequence (0→98 with no gaps), corroborated by Runner mode's independent parallel design (full range + one reserved final slot), is strong evidence this was a bug, not an intentional short segment — a claim that could not be made with this level of confidence during Pass 1 (which flagged it only as a judgment call).
- Behavior impact: n/a, no code change this entry — the Pass-1 fix (extending the loop to `i<99`) stands, now with materially higher confidence attached in the audit.
- Verification performed: exact loop-bound enumeration via grep against `inc/header.h`'s declared array capacity (`Ledge ledges[100]`).
- Rollback: n/a.

### [2026-07-17] Pass 2 — centralized, idempotent application lifecycle (new inc/app.h, src/app.c)
- Phase: new (application lifecycle — corresponds to the original refactor brief's suggested `app.h`/`app.c` module)
- File(s): new inc/app.h, new src/app.c, src/main.c (rewritten init/shutdown sections)
- Before: `main()` called `malloc`/`SDL_Init`/`SDL_CreateWindow`/`SDL_CreateRenderer`/`TTF_Init`/`Mix_OpenAudio` with no return-value checks at all; shutdown was ~45 lines of dead/commented-out code (stale non-pointer dot-notation that wouldn't even compile if uncommented) plus four live lines (`SDL_DestroyWindow`, `free(gameState)`, `SDL_DestroyRenderer`, `TTF_Quit`, `SDL_Quit`) that destroyed neither textures, fonts, sound chunks, nor music, and never called `Mix_CloseAudio`/`IMG_Quit`. `IMG_Init` was never called anywhere despite ~65 `IMG_Load` sites.
- After: `app_init(GameState**, SDL_Window**, SDL_Renderer**)` checks every step (`calloc`, `SDL_Init`, `IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG)` — newly added, was previously entirely missing — `SDL_CreateWindow`, `SDL_CreateRenderer`, `TTF_Init`, `Mix_OpenAudio`) and on any failure prints which step failed via `SDL_GetError()`/`IMG_GetError()`/`TTF_GetError()`/`Mix_GetError()`, calls `app_shutdown` on whatever partially succeeded, and returns `false` (main() returns exit code 1, no `exit()` call). `app_shutdown(GameState**, SDL_Window**, SDL_Renderer**)` is idempotent and null-tolerant throughout: stops playback (`Mix_HaltChannel(-1)`, `Mix_HaltMusic()`) → frees all `MAX_BULLETS` bullets/secondBullets via the existing `removeBullet`/`removeSecondBullet` → destroys every texture field in `GameState` (23 individual fields + the two 12-element arrays + `man`/`secondPlayer`/`enemy`'s 7+7+2 sprite-sheet textures + `train`/`cloud1..8`'s textures — confirmed by grep that `enemyValues[]`/`smartEnemies[]`/`boss[]` never have their own per-instance textures assigned, only the single shared `man`/`secondPlayer`/`enemy` master instances do, so no double-free risk from destroying a shared pointer twice) → closes the font → frees the 5 chunks + 3 music tracks → destroys the renderer → destroys the window → calls `Mix_CloseAudio()`/`IMG_Quit()`/`TTF_Quit()` (each documented-safe to call even if never successfully initialized) → `SDL_Quit()` → frees `GameState` — nulling every pointer immediately after destroying/freeing it, via three tiny local helpers (`destroy_texture`/`free_chunk`/`free_music`) that no-op on an already-NULL pointer.
- Reasoning: directly implements this pass's mandated items 6 ("complete application shutdown") and 7 ("correct initialization failure handling"). Scoped narrowly to the top-level `main()` init/shutdown boundary only — the ~65 per-asset `exit(1)` calls inside `loadGame.c`/`load_menu.c` are unchanged and remain a documented, deferred issue (Phase 7 of `docs/refactor-plan.md`); rewriting those was explicitly out of this pass's scope (a much larger, separately-risked change).
- Behavior impact: preserving for the success path (identical sequence of calls, same window size/title/flags, same renderer flags, same font/audio parameters). Deliberately new behavior only on failure paths that previously didn't exist at all (previously, e.g., a failed `SDL_CreateWindow` would dereference a NULL window pointer moments later with no diagnostic) — startup failures now print a specific error and exit cleanly with status 1 instead of crashing or continuing with invalid state. `IMG_Init` is a new, additive call with no prior behavior to preserve (its absence was itself Pass-1-documented defect #16).
- Verification performed: compiled clean (0 warnings from the new files); **real runtime smoke test** (see next entry) exercised the actual `app_init`/`app_shutdown` functions end to end multiple times, including a deliberate repeated-shutdown idempotency check.
- Rollback: delete inc/app.h and src/app.c; restore main.c's previous inline init/shutdown code (preserved verbatim in this file's Pass-1 section above and in git commit `cfb7ca7`).

### [2026-07-17] Pass 2 — fixed a newly-discovered out-of-bounds write in process.c (odd-sized array, `+=2` loop)
- Phase: 1 (correctness — found during the mandated project-wide nested-loop index-confusion sweep)
- File(s): src/process.c:515
- Before:
  ```c
          for (int i = 0; i < NUM_ENEMIES; i += 2)
          {
              game->enemyValues[i].x = 620;
              ...
              game->enemyValues[i + 1].x = 580;
              ...
          }
  ```
- After:
  ```c
          for (int i = 0; i < NUM_ENEMIES - 1; i += 2)
          {
  ```
  (loop body otherwise unchanged)
- Reasoning: `NUM_ENEMIES` = 101 (odd, `inc/header.h:53`). The loop steps by 2 and writes both `enemyValues[i]` and `enemyValues[i+1]` per iteration; on the final iteration (`i=100`), `enemyValues[i+1]` = `enemyValues[101]` is one past the 101-element array (valid indices 0–100), aliasing into the immediately-following `smartEnemies[0]` (confirmed via `inc/header.h`'s field order: `boss[2]`, then `enemyValues[NUM_ENEMIES]`, then `smartEnemies[NUM_SMART_ENEMIES]`, with no intervening fields). This code only runs once per Arcade playthrough, when `game->tempScore` reaches exactly 366 (a wave-spawn threshold). Found via an explicit project-wide sweep for the same bug class as the already-fixed `boss[i]`/`boss[j]` defect (nested/nearby loop with an array-bound mismatch), requested by this pass; six other structurally similar `+= 2` loops in the same file (lines 307/325/344/376/408/453) all use small even literal bounds (10/20/30/50/70/100) and were confirmed safe by the same check — this was the only one using the odd `NUM_ENEMIES`.
- Behavior impact: deliberate defect fix. Previously, reaching `tempScore == 366` in Arcade mode corrupted `smartEnemies[0]`'s position/velocity/visibility (overwriting it with `x=580, y=-10000, dx=0, dy=0, visible=1`) as an unintended side effect of this wave-spawn code. After the fix, the loop stops one iteration earlier (last pair spawned is `enemyValues[98]`/`enemyValues[99]`; `enemyValues[100]`, the array's last slot, is simply not touched by this specific spawn call, same as before the bug — the fix only removes the overflowing final iteration, it does not add a new one to compensate).
- Verification performed: grepped all `+= 2` loops in process.c to confirm this was the only one with a mismatched odd bound; rebuilt (0 errors, same warning count, confirming no new warning from this one-token change); cross-checked struct field order in `inc/header.h` to confirm exactly what memory the OOB write had been corrupting.
- Rollback: revert `NUM_ENEMIES - 1` back to `NUM_ENEMIES` at process.c:515.

### [2026-07-17] Pass 2 — sanitizer instrumentation attempted, confirmed unavailable in this environment
- Phase: 3 (sanitizer validation, best-effort)
- File(s): Makefile (`mingw-asan` target added, documented as currently non-functional here)
- Action: attempted `-fsanitize=address,undefined` with two different MinGW-w64 GCC distributions (scoop `gcc`/nuwen 15.2.0, and scoop `mingw-winlibs`/WinLibs 16.1.0). Both fail at the link step: `cannot find -lasan` / `cannot find -lubsan` — neither distribution bundles the sanitizer runtime libraries for the `x86_64-w64-mingw32` target. This is a known, documented ecosystem gap (ASan/UBSan on native Windows is primarily supported through the MSVC toolchain via clang-cl, not mingw-w64 GCC); no Visual Studio/MSVC Build Tools are installed in this environment, and installing them was judged out of scope for this pass's "smallest practical validation path."
- Reasoning: rather than silently dropping sanitizer validation or claiming it ran, the attempt, the exact failure, and the reason are recorded here per this pass's explicit instruction not to conceal or fake verification. The `mingw-asan` Makefile target is left in place, correctly specified, ready to use in an environment with a sanitizer-capable toolchain (a real Linux/macOS build, or clang-cl + MSVC on Windows).
- Behavior impact: n/a (no sanitizer run occurred).
- Verification performed: two independent toolchain attempts, both producing the identical class of linker error; confirmed no `*asan*`/`*ubsan*` files exist anywhere under either toolchain's install directory.
- Rollback: n/a.

### [2026-07-17] Pass 2 — real runtime smoke test (non-interactive init/shutdown/asset-guard cycle)
- Phase: 3 (runtime verification)
- File(s): new docs/verification/smoke_init_shutdown.c (test-only, not part of the shipped game; excluded from the `mingw`/`all` targets, built only by the new `mingw-smoketest` Makefile target against all of `src/*.c` except `main.c`)
- Action: since this environment has no way to inject keyboard/window input into a native window (only browser-page automation is available, not arbitrary desktop windows), a full interactive menu-navigation smoke test could not be automated. Instead, wrote a small standalone harness that calls the real `app_init()`, then `load_menu0()` twice (confirming the pointer is unchanged the second time), then `load_menu1()+loadGame()` three times in a row and `loadGame()` once more (confirming `bossTexture`/`font` are unchanged across repeats and `arcadeAssetsLoaded` reads `1`), then `load_menu2()+loadGame2()` three times and `loadGame2()` once more (confirming `star` unchanged, `runnerAssetsLoaded` reads `1`), then `app_shutdown()`, then `app_shutdown()` again on the now-NULLed pointers (idempotency check). Ran three times.
- Result (all three runs): `app_init` succeeded with a real accelerated renderer (this machine has a real GPU/display, so this exercised the actual SDL/IMG/TTF/Mix subsystems and window/renderer creation, not a headless stub); every "unchanged" check reported `yes`; the second `app_shutdown()` call completed without crashing; final line `SMOKE TEST: PASS` printed every time. Full logs saved at `docs/verification/mingw-smoketest-run1.log` and `mingw-smoketest-run2-expanded.log`.
- Reasoning: this is the closest automatable equivalent to the task's requested "repeated transitions: menu → gameplay → menu, several times, verify no crashes/no duplicated music/no repeated asset loading" — it directly exercises the exact functions Pass 1's leak fixes and this pass's sentinel hardening changed, under the real OS/GPU/audio stack, not a mock.
- Verification performed: **this is genuine runtime verification**, not static inspection — see the distinction maintained throughout this log and in `docs/refactor-audit.md`. Explicitly not covered: actual keyboard-driven gameplay (movement, shooting, collisions, pause/resume, leaderboard entry) and the `SDLK_ESCAPE`/`SDLK_q` in-game sound-cleanup paths in `processEvents.c`, since those require a live event loop this environment cannot drive automatically.
- Rollback: delete docs/verification/smoke_init_shutdown.c and the `mingw-smoketest` Makefile target; no shipped code is affected.

## Pass 3 summary — scene-state refactor and main-loop simplification (2026-07-18)

Baseline: commit `ac00c87` (tagged `refactor-pass-2-validated`), working tree clean, `make mingw`
0 errors/168 warnings, `make mingw-smoketest` PASS — all confirmed before any edit this pass.
Replaced the two raw `int` routing fields (`menu0_status`/`menu_status`, the latter reused with
different meaning depending which mode was active) and `main.c`'s two nested `while`/`switch`
loops with one authoritative `AppScene` enum field, one flat application loop, and an explicit
`app_change_scene()` transition function (new `src/scene.c`). Every routing write site across
`menu_events.c`, `pause_events.c`, and `processEvents.c` (both `processEvents`/`processEvents2`)
was converted to call `app_change_scene()` instead of assigning the legacy fields directly. Full
before/after transition table, per-scene resource/lifecycle map, and every preserved pre-existing
quirk are in `docs/scene-state-map.md`, written before any code edit per this phase's own
requirement. Four commits: `a83c59d` (enums + scene.c), `5ccca22` (main.c flattened + write-site
conversions), `ff630bc` (scene transition test), and this documentation commit.

### [2026-07-18] Tag baseline + write docs/scene-state-map.md
- Phase: safety gate + pre-edit documentation
- File(s): git tag `refactor-pass-2-validated` on commit `ac00c87`; new `docs/scene-state-map.md`
- Action: confirmed working tree clean, current commit hash, and no existing rollback tag before
  tagging one; then wrote the complete state-transition map (every reachable screen's numeric
  representation, entry condition, input/update/render functions, exit transitions, resources
  loaded, and blocks/pauses/resets behavior) purely from reading the pre-refactor code — no code
  changed in this step.
- Verification performed: `git status`/`git log -1`/`git tag -l` reviewed; `make mingw` (0
  errors/168 warnings) and `make mingw-smoketest` (PASS) re-confirmed as the safety gate before
  any edit.
- Rollback: `git tag -d refactor-pass-2-validated`; delete `docs/scene-state-map.md`.

### [2026-07-18] Introduce AppScene enum and GameState.scene field; delete enum menu_buttons
- Phase: Pass 3 (commit `a83c59d`)
- File(s): `inc/header.h`
- Before: routing driven by `int menu_status`/`int menu0_status`; an unused, misleadingly-named
  `enum menu_buttons` (last member `RUNNER=5` didn't mean "runner mode" — 5 actually meant
  "pause") sat in the header with zero references anywhere in `src/`.
- After: new `typedef enum AppScene { APP_SCENE_MAIN_MENU, APP_SCENE_ARCADE_MENU,
  APP_SCENE_ARCADE_GAME, APP_SCENE_ARCADE_LEADERBOARD, APP_SCENE_ARCADE_PAUSE,
  APP_SCENE_RUNNER_MENU, APP_SCENE_RUNNER_GAME, APP_SCENE_RUNNER_LEADERBOARD,
  APP_SCENE_RUNNER_PAUSE, APP_SCENE_QUIT }` placed before the `GameState` struct; new field
  `AppScene scene;` (commented "write ONLY via app_change_scene()"); `enum menu_buttons` deleted;
  `menu_status`/`menu0_status` commented `// DEPRECATED` in place, not deleted.
- Reasoning: one authoritative field, per this phase's core requirement. Leaderboard and pause
  got separate Arcade/Runner enum values (not one shared value each) specifically so `AppScene`
  alone is always sufficient to pick the correct render/input function pair and pause-resume
  target, without needing a second field to disambiguate — validated during planning against a
  second read of every consuming function. `GameMode` enum (considered per the phase's own
  example) was deliberately **not** introduced: it would have zero use sites this phase, since
  `multiPlayer`'s ~15+ existing gameplay-logic read sites are explicitly not being touched — the
  same "dead enum" problem being fixed by deleting `enum menu_buttons`. `enum menu_buttons`
  itself was judged safe to delete outright (not just deprecate) since it had zero historical call
  sites, unlike `menu_status`/`menu0_status` which were live right up until this phase.
- Verification performed: `make mingw` — 0 errors, 168 warnings (identical to baseline; this
  commit adds an unused-so-far field and deletes a dead enum, no logic runs yet). `make
  mingw-smoketest` — PASS (unaffected, doesn't touch scene routing).
- Rollback: `git revert a83c59d` or restore from tag `refactor-pass-2-validated`.

### [2026-07-18] Implement src/scene.c (app_change_scene, arcade_menu_enter, runner_menu_enter)
- Phase: Pass 3 (commit `a83c59d`)
- File(s): new `src/scene.c`
- Action: `app_change_scene(GameState*, AppScene next_scene)` validates the target is in range
  (forces `APP_SCENE_QUIT` and logs to stderr otherwise), records `game->scene` as
  `previous_scene`, assigns the new value, then dispatches to a static enter-hook for
  `APP_SCENE_ARCADE_MENU`/`APP_SCENE_RUNNER_MENU` only (the only two scenes with real one-time
  entry work). `arcade_menu_enter()` calls `load_menu1()` **only** when `previous_scene ==
  APP_SCENE_MAIN_MENU` (matching how often it fired before this refactor — exactly once, on the
  true Main-menu-to-mode transition), then unconditionally calls `loadGame()` (matching the
  *observable* effect of its old every-single-frame call site, now firing once per arrival
  instead — nothing renders or reads gameplay state while sitting on a menu screen, confirmed by
  reading `doRender_menu0/1/2`, so this is not player-visible). `runner_menu_enter()` mirrors this
  with `load_menu2()`/`loadGame2()`. No `arcade_leave`/`runner_leave` functions were added: the
  real "leave" side effects (freeing `Mix_Chunk`/`Mix_Music` on ESCAPE/quit) already happen inline
  in `processEvents.c`'s key handlers, with no shared/duplicated logic there worth extracting.
- Reasoning: this is the single highest-risk piece of the whole phase (validated explicitly
  during planning) — if any write site elsewhere assigned `game->scene` directly instead of
  calling `app_change_scene()`, the once-per-arrival reset would silently stop firing on
  "return from gameplay"/"game over" paths, leaving stale gameplay state (position, lives, score,
  enemies) to carry into the next session. The invariant "`game->scene` is written only inside
  `app_change_scene()`" is stated explicitly in `docs/scene-state-map.md` and enforced by
  construction: every other write site converted in this phase calls the function, never assigns
  the field.
- Verification performed: `make mingw` — 0 errors, 168 warnings, confirmed 0 warnings from
  `scene.c` itself. `make mingw-smoketest` — PASS (this commit doesn't wire `app_change_scene`
  into any caller yet, so no behavior changes; the function compiles and is reachable only from
  within `scene.c` at this point).
- Rollback: delete `src/scene.c` and its prototype in `inc/header.h`.

### [2026-07-18] Flatten main.c to one loop; convert every scene-routing write site
- Phase: Pass 3 (commit `5ccca22`)
- File(s): `src/main.c`, `src/menu_events.c`, `src/pause_events.c`, `src/processEvents.c`
- Before: `main.c` had `while(!done0) switch(menu0_status) { case 1: while(!done)
  switch(menu_status) {...} case 2: while(!done) switch(menu_status) {...} }` — two nested
  infinite loops, `done`/`done0` locals, and a post-inner-loop block that always reset
  `menu0_status=0` regardless of why the inner loop ended. Nine live sites across
  `menu_events.c`/`pause_events.c`/`processEvents.c` assigned `menu_status`/`menu0_status`
  directly (menu selection, pause enter/resume, quit-to-menu, game-over-to-menu).
- After: `main.c` is one `while (gameState->scene != APP_SCENE_QUIT) { switch (gameState->scene)
  {...} }` with ten cases (one per `AppScene` value) plus a `default` that force-quits via
  `app_change_scene`. Confirmed line-by-line against the pre-refactor file that every scene's
  function-call order is unchanged. All nine write sites now call `app_change_scene(game,
  TARGET)`: `menu_events.c`'s shared menu handler checks `game->scene ==
  APP_SCENE_RUNNER_MENU` to pick `APP_SCENE_RUNNER_GAME`/`_LEADERBOARD` vs the Arcade
  equivalents (keys `1`/`2`/`3`), and always targets `APP_SCENE_MAIN_MENU` on `q`;
  `pause_events.c`'s shared pause handler checks `game->scene == APP_SCENE_RUNNER_PAUSE` to
  resume into the correct mode's gameplay scene, and always targets `APP_SCENE_MAIN_MENU` on `q`;
  `processEvents()`/`processEvents2()` (each mode-exclusive, no scene check needed) target fixed
  scenes for ESCAPE/pause/quit/game-over. Per validation feedback, `menu_events.c`'s `SDLK_SPACE`
  and `default` cases (today's "stay at the same menu" no-ops) call **nothing** rather than a
  same-scene `app_change_scene`, avoiding a pointless self-transition. `load_menu.c` and
  `leader_events.c` are untouched — their internal `menu_status`/`menu0_status` writes are now
  inert (nothing reads those fields anymore) but harmless to leave in place; `leader_events.c`
  remains confirmed dead/unreachable code, undeleted.
- Reasoning: this is the actual routing cutover and could not be split into smaller commits
  without an intermediate broken state (main.c depends on every write site being converted, and
  vice versa). Preserved exactly: the asymmetry where quitting a mode (`q`) always lands on the
  Main menu, never ends the program (only the Main menu's own `q`/`ESCAPE` reaches
  `APP_SCENE_QUIT`); the lack of a distinct game-over scene (death routes to the same in-mode-menu
  scene as voluntarily pressing ESCAPE); the pre-existing quirk where pausing from the leaderboard
  resumes into live gameplay, never back to the leaderboard; the pre-existing unwired `SDL_QUIT`
  (window-close) during gameplay, which still does nothing. All of these are documented explicitly
  in `docs/scene-state-map.md` as confirmed-preserved, not silently fixed.
- Verification performed: `make mingw` — 0 errors, 168 warnings, confirmed identical set of
  warnings to the baseline (none on any line this commit touches — the 12 pre-existing
  `processEvents.c` warnings are all in movement code at lines untouched by this refactor). `make
  mingw-smoketest` — PASS, unmodified.
- Rollback: `git revert 5ccca22`.

### [2026-07-18] Add non-interactive scene transition test
- Phase: Pass 3 (commit `ff630bc`)
- File(s): new `docs/verification/scene_transition_test.c`, `Makefile` (`mingw-scenetest` target)
- Action: 16 checks covering initial scene, Main-menu→Arcade-menu and →Runner-menu transitions
  (confirming asset-load guards and the load-once leaderboard/music reset fire only on the true
  Main-menu-origin transition), a gameplay→menu transition confirming that reset does *not*
  re-fire on return from gameplay while the per-arrival gameplay-state reset still does, the quit
  transition, out-of-range enum handling (forces safe quit, no crash), and pause resuming to the
  correct originating mode for both Arcade and Runner (by replicating `pause_events.c`'s exact
  resume expression against a directly-set precondition scene — the one documented, comment-flagged
  exception to "never assign `scene` directly," since there's no way to drive a real `SDL_Event`
  here). All 16 checks pass, 3 consecutive runs.
- Verification performed: **runtime-verified**, not static — the built `scenetest.exe` actually
  runs and every check is a live assertion against the real `app_change_scene()`/`loadGame()`/
  `load_menu1()` etc., not a code-reading exercise. `make mingw` — 0 errors, 168 warnings
  (confirmed 0 from the new test file). `make mingw-smoketest` — still PASS.
- Rollback: delete `docs/verification/scene_transition_test.c` and the `mingw-scenetest` target.

### [2026-07-18] Interactive manual test — real window launch, main-menu render confirmed; further transitions not achievable
- Phase: Pass 3 (manual verification, no commit — evidence only)
- File(s): none changed; new `docs/verification/manual-test-main-menu.png`
- Action: launched the actual `build-mingw/endgame-mingw.exe` (not a test harness) as a real
  process, confirmed via `Get-Process` that it started, created a window titled "Game Window,"
  and stayed alive (no crash) for 5+ seconds with no input. Captured the window's actual rendered
  contents using the Win32 `PrintWindow` API (a focus-independent capture method, since
  `SetForegroundWindow` proved unreliable in this automation context — see below) — the client
  area measured exactly 1280x720, matching `WIDTH`/`HEIGHT` in `inc/header.h`. The captured image
  (saved at `docs/verification/manual-test-main-menu.png`) shows the real Main menu rendering
  correctly: "ARCADE GAME 2 IN 1", "1. MUSHROOM HUSTLE", "2. BRICK RUNNER III", "EXIT" — confirming
  `APP_SCENE_MAIN_MENU`'s `doRender_menu0` and the underlying `load_menu0()` texture load work
  end-to-end in this refactored build.
- **What could not be achieved, stated honestly rather than glossed over**: attempted to send a
  `1` keypress via `PostMessage(WM_KEYDOWN/WM_KEYUP)` to trigger the Main-menu→Arcade-menu
  transition and confirm it visually — the screen was unchanged afterward (SDL2 on Windows
  appears to require real OS-level keyboard focus, not just a posted message, to register key
  events). Attempted to establish real focus via `SetForegroundWindow`/`BringWindowToTop` — this
  returned `false` and the OS foreground window did not change, which is Windows' documented
  foreground-lock restriction preventing a background automation process from stealing focus from
  the active session. Pursuing further workarounds (thread-input attachment, simulated hardware
  input) was judged not worth the time given the risk of an unreliable result being
  misinterpreted as a passing or failing test. **No transition beyond the initial Main-menu render
  was interactively verified.** The full manual test matrix requested by this phase (Arcade
  single/multi/pause/resume/game-over/leaderboard, Runner equivalents, 5x repeated menu↔mode
  cycles) was **not performed** — this is stated plainly rather than claimed.
- An earlier screenshot attempt in this same session (before switching to `PrintWindow`) captured
  what appeared to be an unrelated, already-running Arcade gameplay window via unreliable
  full-screen-capture-plus-focus-stealing — this was almost certainly a stale/leftover window
  from an earlier, unrelated session, not evidence about this build. It is disregarded; only the
  `PrintWindow`-based capture (focus-independent, verified against the exact PID launched this
  session) is treated as real evidence.
- Rollback: n/a (no code changed; the screenshot is documentation evidence only).

## Pass 4 summary — separate asset loading from session reset (2026-07-18)

Baseline: commit `82927d6` (tag `refactor-pass-2-validated` from Pass 2 — Pass 3's own tag,
`refactor-pass-3-scenes`, was created at the start of this pass since it didn't exist yet), working
tree clean, `make mingw` 0 errors/168 warnings, both existing test suites passing — all confirmed
before any edit. Split `loadGame()`/`loadGame2()` (each combining a one-time asset-load guard with
an unconditional per-call gameplay-state reset) into 7 explicit functions
(`shared_assets_load/unload`, `arcade_assets_load/unload`, `arcade_session_reset`,
`runner_assets_load/unload`, `runner_session_reset`), backed by 4 new checked SDL loading helpers
(`src/asset_loader.c`). Full audit, statement-by-statement classification, and resource ownership
table in `docs/game-session-lifecycle.md`, written before any code edit per this phase's own
requirement. Six commits: `fbf9cae` (lifecycle doc), `be2ddb1` (checked loaders), `586c031` (the
combined Arcade+Runner split), `a349e07` (`multiPlayer`→`GameMode`), `ac55082` (lifecycle tests),
and this documentation commit.

### [2026-07-18] Tag baseline + write docs/game-session-lifecycle.md
- Phase: safety gate + pre-edit documentation
- File(s): git tag `refactor-pass-3-scenes` on commit `82927d6`; new `docs/game-session-lifecycle.md`
- Action: confirmed working tree clean, current commit hash, and that the Phase 3 rollback tag
  didn't yet exist before creating it; ran `make mingw`/`mingw-smoketest`/`mingw-scenetest` as the
  safety gate; then wrote the complete statement-by-statement classification of `loadGame()`/
  `loadGame2()` (every line's category: static/audio asset loading, level geometry, player/enemy/
  bullet init, score/life/session reset, animation reset, or unclear/mixed) purely from reading the
  pre-Pass-4 code, plus the resource ownership table required by this phase — no code changed in
  this step.
- Verification performed: `make mingw` (0 errors/168 warnings), `make mingw-smoketest` (PASS),
  `make mingw-scenetest` (PASS) all re-confirmed as the safety gate before any edit.
- Rollback: `git tag -d refactor-pass-3-scenes`; delete `docs/game-session-lifecycle.md`.

### [2026-07-18] Add checked SDL asset loading helpers; promote app.c's destroy helpers
- Phase: Pass 4 (commit `be2ddb1`)
- File(s): new `src/asset_loader.c`, `src/app.c`, `inc/header.h`
- Action: `load_texture`/`load_music`/`load_chunk`/`load_font` (each: rejects NULL arguments, nulls
  the output pointer before attempting, refuses to overwrite an already-loaded destination
  (returns `false` instead), frees the temporary `SDL_Surface` on every path, reports the failing
  path via `stderr`, never calls `exit()`/`SDL_Quit()`). `app.c`'s `destroy_texture`/`free_chunk`/
  `free_music` promoted from `static` to `extern` (header.h prototypes added) so the upcoming
  mode-specific unload helpers could reuse them instead of duplicating null-safe destroy logic.
  `app_shutdown()` itself unchanged in this commit.
- Behavior impact: none — nothing calls the four new loaders yet; this commit only introduces
  them, compiling standalone.
- Verification performed: `make mingw` — 0 errors, 168 warnings (unchanged), confirmed 0 warnings
  from the new file. Both existing test suites still pass.
- Rollback: delete `src/asset_loader.c`, revert the `static` removal in `app.c`, remove the new
  header.h prototypes.

### [2026-07-18] Separate asset loading from session reset (Arcade + Runner, combined)
- Phase: Pass 4 (commit `586c031`)
- File(s): `inc/header.h`, `src/loadGame.c` (rewritten), `src/scene.c`, `src/menu_events.c`,
  `src/app.c`, `docs/verification/smoke_init_shutdown.c`, `docs/verification/scene_transition_test.c`
- Before: `loadGame()`/`loadGame2()` each combined a one-time-guarded asset-load block with an
  unconditional per-call gameplay-state reset tail; both ran on every arrival at
  `APP_SCENE_ARCADE_MENU`/`APP_SCENE_RUNNER_MENU` via `src/scene.c`'s enter-hooks.
- After: `arcade_menu_enter`/`runner_menu_enter` call only `arcade_assets_load()`/
  `runner_assets_load()` (falling back to `APP_SCENE_MAIN_MENU` on failure, printing the reason to
  `stderr`). `arcade_session_reset(game, mode)`/`runner_session_reset(game, mode)` are now called
  explicitly from `menu_events.c`'s `SDLK_1`/`SDLK_2` handlers, immediately before the transition
  to the gameplay scene — matching this phase's required "select mode → verify assets loaded →
  reset session → transition" sequence. `app_shutdown()` now delegates to the three new
  `*_unload()` functions for every field they unambiguously own, instead of duplicating their
  destroy lists inline.
- Three confirmed pre-existing bugs fixed as part of the split (not new behavior — see
  `docs/game-session-lifecycle.md` §10 for the full reasoning on why each is invisible in the
  common case):
  1. `loadGame.c:407`'s unconditional `game->multiPlayer = 1;` (ran on every call, making the
     very next `if (game->multiPlayer)` guards effectively always-true) — deleted; the mode is now
     set exactly once, as `*_session_reset()`'s first statement, from an explicit parameter.
  2. Bullet-array leak: the old reset loops did `game->bullets[i] = NULL;`/`secondBullets[i] =
     NULL;` directly, leaking any bullet still in flight; now calls `removeBullet(game, i)`/
     `removeSecondBullet(game, i)` (the existing, correct free-then-NULL functions from
     `process.c`), closing the leak.
  3. Shared-texture leak: `mult`/`leaders`/`pause`/`brick`/`death`/`font` were each loaded
     independently by both `loadGame()` and `loadGame2()` into the same field — visiting both
     modes in one process lifetime orphaned whichever texture loaded first. Fixed via a new
     `shared_assets_load()`/`shared_assets_unload()` pair with its own `sharedAssetsLoaded` flag:
     whichever mode is visited first loads the 6 shared fields, the other mode's load is skipped.
     `manFrames[0]` (written by both modes with genuinely *different* source images) got a
     narrower fix instead — destroy-before-overwrite at each mode's own load site, preserving
     today's "most-recently-visited mode's image wins" outcome without the leak, and its
     destruction stays centralized in `app_shutdown()` (not delegated to either mode's
     `_unload()`) since no single mode exclusively owns it.
- The redundant `init_status_lives()` call and `game->label` destroy-and-null block (present in
  both old reset tails) were dropped entirely, not relocated — `doRender()`/`doRender2()` already
  call `init_status_lives(game)` unconditionally on every rendered gameplay frame
  (`doRender.c:187-194`/`:266-272`), which is what actually keeps the HUD label correct regardless
  of load/reset timing; the calls inside `loadGame()`/`loadGame2()` were dead weight relative to
  that per-frame safety net.
- Behavior impact: **deliberate, evidence-based timing change, confirmed unobservable**. Session
  reset now fires once per new-game-start instead of once per menu arrival — verified safe because
  nothing renders or reads gameplay state while sitting on a menu screen (`doRender_menu0/1/2` only
  blit a static background), so the end state by the time gameplay actually starts is identical
  either way. The three bug fixes above are deliberate defect fixes, not preserved-as-is behavior;
  each is invisible in the overwhelmingly common case (a single mode visited per session, or a
  normal fresh-game start) and only changes outcomes in the specific buggy edge cases (visiting
  both modes in one process lifetime; a bullet still in flight at the exact moment of returning to
  the menu; the mode-selection flag being read before the next explicit selection).
- Verification performed: `make mingw` — 0 errors, 167 warnings (down from 168 — one pre-existing
  `-Wdouble-promotion`/`-Wfloat-conversion` pair on a line directly moved into
  `runner_session_reset()` was fixed in passing, since the line was genuinely touched by the
  extraction). Both existing test suites (`smoke_init_shutdown.c`, `scene_transition_test.c`,
  updated to call the new function names) still pass; `scene_transition_test.c`'s check that used
  to assert "returning to the menu resets gameplay state" was corrected to assert the new,
  intentional behavior instead (verified the old assertion would now be wrong), with two new
  checks added exercising `arcade_session_reset()` directly.
- Rollback: `git revert 586c031` (this single commit covers what the plan originally scoped as two
  — splitting further would have left an intermediate state that doesn't compile, since
  `main.c`/`scene.c`/`menu_events.c` all needed the full new function set simultaneously).

### [2026-07-18] `multiPlayer` int → GameMode retype
- Phase: Pass 4 (commit `a349e07`)
- File(s): `inc/header.h` only
- Before: `int multiPlayer;`
- After: `GameMode multiPlayer;` (field name unchanged)
- Reasoning: confirmed via the previous commit's build that all ~29 read sites across
  `process.c`/`processEvents.c`/`doRender.c`/`collisionDetect.c`/`kills_score.c` are bare boolean
  truth tests (`if (game->multiPlayer)`/`if (!game->multiPlayer)`) — zero `==` comparisons, zero
  arithmetic, so the retype requires zero changes to any of them (`GAME_MODE_SINGLE_PLAYER=0`/
  `GAME_MODE_MULTIPLAYER=1` match the int's previous values exactly).
- Behavior impact: none — purely a type-safety improvement; `multiPlayer` now has exactly one
  writer (`arcade_session_reset`/`runner_session_reset`'s first statement), already wired up in
  the previous commit.
- Verification performed: `make mingw` — 0 errors, 167 warnings (unchanged), confirming all 29
  read sites compiled identically. Both test suites still pass.
- Rollback: `git revert a349e07`.

### [2026-07-18] Add non-interactive asset/session lifecycle test
- Phase: Pass 4 (commit `ac55082`)
- File(s): new `docs/verification/lifecycle_test.c`, `Makefile` (`mingw-lifecycletest` target)
- Action: 55 checks covering, per mode: asset lifecycle (starts unloaded → loads successfully →
  flag true → a second load is a pointer-identical no-op → unload nulls every owned pointer and
  clears the flag → a second unload doesn't crash → reload succeeds again), the shared bucket's
  own independent lifecycle and its interaction with both modes' loaders, session reset (mutates
  score/lives/position/enemy/bullet state — including a real `addBullet()`-allocated bullet to
  verify no leak on reset — then confirms fresh values and that asset pointers/loaded flags are
  completely untouched), pause/resume (session state survives unchanged across both transitions),
  and the new-game transition sequence (reset + assets retained + correct scene).
- **This test found a real, previously-undiscovered gap** in the code just written this pass:
  `arcade_assets_load()`/`runner_assets_load()` checked their own `*AssetsLoaded` flag *before*
  re-verifying the shared bucket via `shared_assets_load()`. If the shared bucket were ever
  unloaded independently while a mode's own flag stayed true — not reachable from today's actual
  call graph (`shared_assets_unload()` is currently only called from `app_shutdown()`), but a real
  hole in the function's own contract that "returning `true` means every asset this mode needs is
  loaded" — a subsequent call to that mode's loader would skip re-loading the shared bucket
  entirely. Fixed by re-ordering: `shared_assets_load()` is now checked unconditionally, before
  each mode's own early-return, in both `arcade_assets_load()` and `runner_assets_load()`
  (`src/loadGame.c`).
- Behavior impact: the ordering fix is a genuine robustness improvement to code introduced this
  same pass, not a change to any previously-existing behavior (the old `loadGame()`/`loadGame2()`
  had no equivalent shared-bucket concept at all, so there's no "prior behavior" to compare against
  here).
- Verification performed: **runtime-verified**, not static — the built `lifecycletest.exe` runs
  and every check is a live assertion, not a code-reading exercise. Initial run: 2 failures
  (`sharedAssetsLoaded true again`, `mult non-NULL again`), diagnosed to the ordering gap above,
  fixed, rebuilt: 55/55 pass. `make mingw` — 0 errors, 167 warnings (unchanged, confirmed 0 from
  the new test file). Both other test suites still pass.
- Rollback: delete `docs/verification/lifecycle_test.c` and the `mingw-lifecycletest` target; the
  `arcade_assets_load`/`runner_assets_load` ordering fix should be kept regardless of whether the
  test itself is kept, since it's a correctness fix independent of the test that found it.

### [2026-07-18] Interactive manual test — Pass 4, real launch + Main-menu render re-confirmed
- Phase: Pass 4 (manual verification, no commit — evidence only)
- File(s): none changed; new `docs/verification/manual-test-phase4-main-menu.png`
- Action: launched the real Pass-4 `build-mingw/endgame-mingw.exe`, confirmed via `Get-Process` it
  started, created a window, and stayed alive (no crash). Captured the window's actual rendered
  contents via the same focus-independent `PrintWindow` technique used in Pass 3 — the resulting
  PNG is byte-for-byte the same file size as Pass 3's equivalent screenshot, confirming the Main
  menu still renders identically (no visual regression from this pass's changes).
- What was not attempted again, and why: Pass 3 already established, with concrete evidence, that
  `SetForegroundWindow` is rejected by Windows' foreground-lock restriction in this automation
  context (returns `false`, foreground window unchanged) and that a `PostMessage`-based keypress
  has no effect on SDL2's input handling on Windows. Re-attempting either this pass would have
  reproduced the same known-negative result at the cost of time better spent elsewhere, so this
  pass did not repeat them. **No transition beyond the initial Main-menu render was interactively
  verified this pass either** — stated plainly, matching Pass 3's own disclosure.
- Rollback: n/a (no code changed; the screenshot is documentation evidence only).
