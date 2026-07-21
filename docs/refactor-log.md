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

## Pass 5 summary — separate gameplay update, collision, and rendering responsibilities (2026-07-18)

Baseline: commit `86dac33` (tag `refactor-pass-4-lifecycle`, created at the start of this pass
since it didn't exist yet), working tree clean, `make mingw` 0 errors/167 warnings, all three
existing test suites passing — all confirmed before any edit. Made the per-mode frame pipeline
explicit and named (`arcade_frame`/`runner_frame`, new `src/frame.c`), removed the one confirmed
gameplay mutation hiding in a render function (`doRender2()`'s death-handling block, confirmed
dead code today), fixed a real double-scene-transition risk in `processEvents.c`, and corrected a
factually-wrong "contested field" comment about `manFrames[0]` by consolidating it into the
existing shared-assets bucket. Full per-scene pipeline audit and every finding (fixed or
deliberately left unfixed, with reasoning) in `docs/frame-pipeline-map.md`, written before any
code edit per this phase's own requirement. Eight commits: `810e511` (pipeline map), `a9f0d37`
(`arcade_frame`), `45f3000` (`runner_frame`), `1b9483e` (mutation relocation), `ad70fa1`
(transition guard fix), `1889b96` (`manFrames[0]` consolidation), `1c79d78` (frame pipeline test),
and this documentation commit.

### [2026-07-18] Tag baseline + write docs/frame-pipeline-map.md
- Phase: safety gate + pre-edit documentation
- File(s): git tag `refactor-pass-4-lifecycle` on commit `86dac33`; new `docs/frame-pipeline-map.md`
- Action: confirmed working tree clean, current commit hash, and that the Phase 4 rollback tag
  didn't yet exist before creating it; ran `make mingw-clean && make mingw` (0 errors/167 warnings)
  and all three existing test suites as the safety gate; launched 3 parallel read-only Explore
  agents to audit `main.c`, `doRender.c`, `process.c`, `collisionDetect.c`, `processEvents.c`, the
  HUD files, and `loadGame.c`; then wrote the complete per-scene frame-pipeline map, re-verifying
  every cited file:line directly (not from the agents' summaries alone) before writing — no code
  changed in this step.
- Verification performed: `make mingw` (0 errors/167 warnings), `mingw-smoketest`/`mingw-scenetest`/
  `mingw-lifecycletest` (all PASS) re-confirmed as the safety gate before any edit.
- Rollback: `git tag -d refactor-pass-4-lifecycle`; delete `docs/frame-pipeline-map.md`.

### [2026-07-18] Introduce explicit arcade_frame() and runner_frame() wrappers
- Phase: Pass 5 (commits `a9f0d37`, `45f3000`)
- File(s): new `src/frame.c`, `inc/header.h`, `src/main.c`
- Before: `main.c`'s `APP_SCENE_ARCADE_GAME`/`APP_SCENE_RUNNER_GAME` cases each inlined 4 calls
  directly (`process`/`collisionDetect`/`doRender`/`processEvents`, and the Runner equivalents).
- After: each case calls one named function (`arcade_frame(game, window, renderer)`/
  `runner_frame(...)`) that wraps the same 4 calls in the same order — a pure structural move with
  zero logic change, landed as two separate commits (Arcade first, then Runner) specifically so
  each could be verified independently before Phase 31's actual behavior change in the same file.
- Reasoning: directly implements this phase's "minimal frame structure, derived from the actual
  implementation" requirement — a Plan-mode design pass confirmed the real call order is
  update→collision→render→events (not the conceptual events-first example given in this phase's
  own brief), so the wrappers preserve that exact order rather than reordering to match the
  example. Menu/pause/leaderboard scene cases were left as direct inline calls (already 1-2 calls,
  no hidden mutation concern, confirmed pure) — wrapping them too was judged over-engineering
  relative to the phase's narrow objective.
- Verification performed: `make mingw` — 0 errors, 167 warnings (unchanged) after each commit. All
  three existing test suites still pass after each commit.
- Rollback: `git revert 45f3000` then `git revert a9f0d37`; restore the 4 inline calls in `main.c`.

### [2026-07-18] Remove gameplay mutation from doRender2()
- Phase: Pass 5 (commit `1b9483e`)
- File(s): `src/doRender.c`, `src/frame.c`
- Before:
  ```c
  if (game->man.isDead)
  {
      SDL_Rect rect = {game->scrollX + game->man.x, game->man.y - 10, 38, 83};
      SDL_RenderCopyEx(renderer, game->death, NULL, &rect, 0, NULL, (game->time % 20 < 10));

      game->gameLives--;
      game->man.isDead = 0;
      game->man.y = 0;
  }
  ```
  (plus a fully-commented dead duplicate of the same block, deleted outright)
- After: the draw call stays in `doRender2()`, unchanged, still gated on `if (man.isDead)`; the
  three mutation lines move into a new `static void runner_resolve_death(GameState *game)` in
  `src/frame.c`, called from `runner_frame()` immediately *after* `doRender2()` returns.
- Reasoning: confirmed via grep that `man.isDead` is never set to `1` anywhere in the live
  codebase (only a commented-out line in `collisionDetect.c` and test harnesses), so this branch
  is dead code today — relocating it changes zero observable behavior now, but removes a
  correctness trap for whenever it's reactivated. A Plan-mode validation pass caught that the
  originally-proposed placement (calling the new function *before* `doRender2()`, between
  collision and render) was wrong: it would clear `isDead` before `doRender2()`'s own
  `if (isDead)` guard ever ran, silently preventing the death sprite from ever drawing. Calling it
  *after* `doRender2()` instead avoids this entirely, and is behaviorally identical to the
  mutation's old position since nothing between the two points reads `man.isDead`/`gameLives`/
  `man.y` (`SDL_RenderPresent`, inside `doRender2()`, touches no game state).
- Behavior impact: none today (dead code); closes a latent bug for whenever `isDead` is ever set.
- Verification performed: `make mingw` — 0 errors, 167 warnings. All three existing test suites
  still pass; the new frame-pipeline test (see below) directly verifies both `doRender2()`'s
  purity (even with `isDead` forced true) and the relocated mutation firing correctly via
  `runner_frame()`.
- Rollback: `git revert 1b9483e`; restore the deleted lines in `doRender2()`.

### [2026-07-18] Prevent double scene transition within one event-processing frame
- Phase: Pass 5 (commit `ad70fa1`)
- File(s): `src/processEvents.c` (both `processEvents()` and `processEvents2()`)
- Before: `SDLK_q`'s handler set the scene (e.g. to `APP_SCENE_MAIN_MENU`) with no early return,
  unlike `SDLK_ESCAPE`'s handler (which returns immediately after its own transition); execution
  then reached the same call's end-of-function game-over check, which could fire its own
  unconditional `app_change_scene()` if the relevant lives count also hit zero that same frame,
  silently overwriting `q`'s chosen destination.
- After: each game-over transition call is now guarded:
  ```c
  if (game->scene == APP_SCENE_ARCADE_GAME)   // or APP_SCENE_RUNNER_GAME
  {
      app_change_scene(game, APP_SCENE_ARCADE_MENU);   // or APP_SCENE_RUNNER_MENU
  }
  ```
  Runner's score-persist into `x_list` (which precedes its transition call) stays unconditional.
- Reasoning: a Plan-mode validation pass considered and rejected a blanket
  `if (scene != GAME) return;` placed right after the event-poll loop — it would also skip
  Runner's score-persist bookkeeping on the same rare coincidence (same-frame `q` press and
  `gameLives` reaching 0), silently dropping a completed run's score. Guarding only the transition
  call itself avoids that: the redundant transition is skipped, but the score is always saved.
- Behavior impact: player-invisible in the overwhelmingly common case (nothing renders gameplay
  state on the destination menu scenes either way); the only observable change is in the specific
  same-frame-quit-and-death coincidence, where the player now correctly lands on whichever menu
  they actually chose instead of having it silently overwritten.
- Verification performed: `make mingw` — 0 errors, 167 warnings. All three existing test suites
  still pass; the new frame-pipeline test directly exercises both the normal-fire and
  already-transitioned cases for both modes.
- Rollback: `git revert ad70fa1`; remove the four added scene-checks, restoring the unconditional
  transition calls.

### [2026-07-18] Consolidate manFrames[0] into the shared asset bucket
- Phase: Pass 5 (commit `1889b96`)
- File(s): `src/loadGame.c`, `src/app.c`
- Before: `arcade_assets_load()` and `runner_assets_load()` each independently loaded
  `manFrames[0]` with a destroy-before-overwrite guard, commented "contested field, different
  source images"; `app_shutdown()` separately destroyed the whole 12-element `manFrames[]` array
  in a centralized loop "since no single mode exclusively owns it."
- After: confirmed via direct file read that both load sites use the byte-for-byte identical path
  (`"./resource/images/main_hero/run/V_g(rn)0.png"`) — the "different source images" comment was
  factually wrong. Moved the load into `shared_assets_load()`/`shared_assets_unload()` alongside
  `mult`/`leaders`/`pause`/`brick`/`death`/`font`; removed both modes' destroy-before-overwrite
  special case and their now-inaccurate comments. `app_shutdown()`'s centralized `manFrames[]`
  destroy loop is removed entirely (not merely narrowed to start at index 1): index 0 is now
  covered by `shared_assets_unload()` and indices 1-11 were already covered by
  `runner_assets_unload()`, leaving nothing for a centralized loop to do.
- Reasoning: Pass 4's own comment claimed the two modes load different images, which turned out to
  be false — the only reader (`draw_status_lives()`, `src/status.c`) is mode-agnostic, and both
  load sites use the identical string literal. This is the "smallest explicit ownership change"
  this phase calls for when a cross-mode field turns out not to be genuinely contested.
- Behavior impact: none observable — this is a one-time load-time change, not a per-frame one; the
  texture ends up loaded exactly once regardless of which mode is visited first, same as before
  (just via a different, non-redundant code path).
- Verification performed: `make mingw` — 0 errors, 167 warnings. All three existing test suites
  still pass (the lifecycle test's asset-load/unload checks cover the shared bucket generally,
  though it doesn't assert on `manFrames[0]` specifically).
- Rollback: `git revert 1889b96`.

### [2026-07-18] Add non-interactive frame-pipeline test
- Phase: Pass 5 (commit `1c79d78`)
- File(s): new `docs/verification/frame_pipeline_test.c`, `Makefile` (`mingw-frametest` target)
- Action: 38 checks covering render purity for `doRender()` (already pure) and `doRender2()`
  (post-fix, including with `man.isDead` forced true to exercise the previously-mutating branch),
  the relocated death mutation firing correctly end-to-end through `runner_frame()`, the double
  scene-transition guard (both the normal-fire case and the already-transitioned case, both
  modes), animation-counter wrap-at-11 and idle/off-ledge no-advance behavior for both `process()`
  and `process2()`, and pause-scene purity across 5 repeated calls.
- **This test found a real interaction, not a Pass 5 regression**: forcing `man.isDead = 1` to
  exercise the relocated mutation also triggers `process2()`'s own, separate, already-documented
  `deathCountdown` mechanism — `runner_session_reset()` sets `deathCountdown = -1`, and with
  `isDead = 1` this flips it to `120` and decrements `gameLives` a second time within `process2()`
  itself, in the same frame. This is the pre-existing bug documented in `docs/frame-pipeline-map.md`
  (missing `deathCountdown` decrement, distinct from this phase's actual changes) becoming visible
  for the first time simply because this is the first test to ever force `isDead = 1`. Fixed the
  test (not the pre-existing bug, which stays deliberately out of scope this phase) by resetting
  `deathCountdown` to `0` before exercising the relocated-mutation assertion, isolating it from the
  unrelated mechanism.
- Verification performed: **runtime-verified**, not static — `frametest.exe` runs and every check
  is a live assertion against the real functions, not a code-reading exercise. Initial run: 1
  failure (the interaction above), diagnosed, test corrected, rebuilt: 38/38 pass, 3 consecutive
  runs. `make mingw` — 0 errors, 167 warnings (unchanged, confirmed 0 from the new file). All
  other test suites still pass.
- Rollback: delete `docs/verification/frame_pipeline_test.c` and the `mingw-frametest` target.

### [2026-07-18] Interactive manual test — Pass 5, real launch + Main-menu render re-confirmed
- Phase: Pass 5 (manual verification, no commit — evidence only)
- File(s): none changed; new `docs/verification/manual-test-phase5-main-menu.png`
- Action: launched the real Pass-5 `build-mingw/endgame-mingw.exe`, confirmed via a live process
  handle that it started and stayed alive (no crash) for 3+ seconds. Captured the window's actual
  rendered contents via the same focus-independent `PrintWindow` technique used in Passes 3-4 —
  the Main menu renders correctly ("ARCADE GAME 2 IN 1", "1. MUSHROOM HUSTLE", "2. BRICK RUNNER
  III", "EXIT"), confirming no visual regression from this pass's changes.
- What was not attempted again, and why: Pass 3 already established, with concrete evidence, that
  `SetForegroundWindow` is rejected by Windows' foreground-lock restriction in this automation
  context and that a `PostMessage`-based keypress has no effect on SDL2's input handling on
  Windows; Pass 4 confirmed the same limitation still holds and did not re-attempt it either.
  Repeating either technique this pass would reproduce the same known-negative result, so it
  wasn't re-attempted. **No transition beyond the initial Main-menu render was interactively
  verified this pass** — stated plainly, matching every prior pass's own disclosure.
- Rollback: n/a (no code changed; the screenshot is documentation evidence only).

## Pass 6 summary — CI + Runner death-lifecycle repair (2026-07-18)

Baseline: commit `3419fc0` (tag `refactor-pass-5-frame-pipeline`, created at the start of this
pass since it didn't exist yet), working tree clean, `make mingw` 0 errors/167 warnings, all four
existing test suites passing -- confirmed before any edit. Two independent objectives: repair a
confirmed severe defect in Runner's death-state mechanism (unreachable today, but a guaranteed
double-decrement-then-unbounded-drain the instant it's wired up) and wire it into live gameplay
per a confirmed product decision, and add a GitHub Actions workflow since Phase 5's PR merged with
no independent CI run at all. Full defect trace and repaired lifecycle in
`docs/runner-death-lifecycle.md`, written before any code edit. Four commits: `75a6637` (lifecycle
doc), `30ce209` (the atomic gameplay fix), `d7fcbd4` (death test), `f38a032` (CI workflow), plus
this documentation commit.

### [2026-07-18] Tag baseline + write docs/runner-death-lifecycle.md
- Phase: safety gate + pre-edit documentation
- File(s): git tag `refactor-pass-5-frame-pipeline` on commit `3419fc0`; new
  `docs/runner-death-lifecycle.md`
- Action: confirmed working tree clean, current commit hash, and that the Phase 5 rollback tag
  didn't yet exist before creating it (Phase 5's PR merged via `3419fc0`, no tag was created at
  merge time); ran `make mingw-clean && make mingw` (0 errors/167 warnings) and all four existing
  test suites as the safety gate; then traced every read/write of `isDead`/`deathCountdown`/
  `gameLives`/`man.lives`/`secondPlayer.lives` across `process.c`/`collisionDetect.c`/
  `processEvents.c`/`frame.c`/`doRender.c`/`loadGame.c`, re-verifying every citation by direct
  reading immediately before writing -- no code changed in this step.
- Verification performed: `make mingw` (0 errors/167 warnings), all four existing test suites
  (PASS) re-confirmed as the safety gate before any edit.
- Rollback: `git tag -d refactor-pass-5-frame-pipeline`; delete `docs/runner-death-lifecycle.md`.

### [2026-07-18] Make Runner death progression single-shot
- Phase: Pass 6 (commit `30ce209`)
- File(s): new `src/runner_death.c`; `inc/header.h`; `src/frame.c`; `src/process.c`;
  `src/collisionDetect.c`; `src/processEvents.c`; `src/loadGame.c`
- Before: `process2()` had a broken `deathCountdown` block (`process.c:1140-1156`) -- the decrement
  of `deathCountdown` itself was commented out, so once `man.isDead` ever became true,
  `game->gameLives--` fired every frame, forever, independent of `runner_resolve_death()`'s own
  decrement (added in Phase 5). `collisionDetect2()`'s star-collision checks and
  `processEvents2()`'s fall-death checks both bypassed this mechanism entirely via a direct
  `game->gameLives = 0;` -- instant, unconditional game over regardless of lives remaining -- with
  a commented-out `isDead=1`/`isDead=0` pair sitting directly next to the star-collision shortcut.
- After: one authoritative death sequence, `src/runner_death.c`:
  ```c
  void runner_trigger_death(GameState *game)   // idempotent -- no-op while already dying
  {
      if (game->man.isDead) { return; }
      game->man.isDead = 1;
      game->deathCountdown = 120;   // existing constant, reused verbatim
  }

  void runner_update_death(GameState *game)   // called once per frame from runner_frame()
  {
      if (!game->man.isDead) { return; }
      if (game->deathCountdown > 0) { game->deathCountdown--; return; }
      game->gameLives--;
      game->man.isDead = 0;
      game->man.y = 0;
      game->man.dy = 0;
      if (game->man.x < 0) { game->man.x = 0; }
      if (game->multiPlayer)
      {
          game->secondPlayer.y = 0;
          game->secondPlayer.dy = 0;
          if (game->secondPlayer.x < 0) { game->secondPlayer.x = 0; }
      }
      game->deathCountdown = -1;
  }
  ```
  `runner_frame()` calls `runner_update_death()` in place of the deleted `runner_resolve_death()`,
  same relative position (after `doRender2()`, before `processEvents2()`). `process2()`'s broken
  block is deleted entirely, in the same commit. Both star-collision sites
  (`collisionDetect.c`) and both fall-death sites (`processEvents.c`) now call
  `runner_trigger_death(game)` instead of the direct `gameLives = 0` shortcut. `process2()`'s
  movement gate changes from `if (!man.isDead || gameLives>0)` to plain `if (!man.isDead)`;
  `secondPlayer`'s movement (which had no `isDead` gate at all before) gains the same gate,
  nested inside the existing `if (multiPlayer)` block.
- Reasoning: confirmed via grep that `man.isDead` is never set to `1` anywhere in the live
  codebase before this change (only the commented-out line and test harnesses) -- the mechanism
  was unreachable, but a Plan-mode validation pass traced precisely what would happen the instant
  it became reachable: a death at `gameLives==1` would decrement twice in one frame (once in the
  old `process2()` block, once in `runner_resolve_death()`), overshoot past `gameLives==0` to `-1`
  before the game-over check ever saw exactly `0`, and then keep decrementing forever since
  `deathCountdown` never returned to a non-positive value -- the game-over transition would never
  fire again. The user confirmed (product decision) wiring star collision and falling into the
  repaired trigger for real, rather than fixing the mechanism's correctness while leaving it
  disconnected -- the commented-out `isDead=1`/`isDead=0` pair, the working `gameLives=3` reset,
  and the fully-built death-sprite/countdown mechanism together were strong evidence of an
  abandoned "3 lives, animate, respawn" design.
- **Critical defect caught during design validation, before any code was written**: an earlier
  draft of the respawn logic only reset `y`/`dy`, not `x`, and never touched `secondPlayer` at all.
  Since falling off the left edge (`man.x < 0`) is itself one of the newly-wired triggers, and
  player movement freezes during the 120-frame countdown, failing to clamp `x` back to a safe
  value at respawn would leave the exact same `x<0` condition true the instant `isDead` cleared --
  re-triggering death every ~120 frames, forever, with zero player agency to escape it (movement
  is frozen), silently draining all 3 lives in ~6 seconds on the single most common way to die in
  this mode. `secondPlayer` needed identical treatment (no prior `isDead` gate on its movement at
  all, and no respawn-position reset), or a continuous multiplayer life-drain would result. Fixed
  before implementation began -- final design clamps `x` to `0` only if negative (preserving
  forward progress otherwise) and mirrors the same treatment onto `secondPlayer`.
- Behavior impact: **deliberate, confirmed gameplay change** -- Runner changes from "any star touch
  or fall is instant, unconditional game over" to "3 lives, each hit triggers a ~2-second death
  animation (blinking every 10 frames via the existing `time%20<10` pattern, unchanged) and costs
  exactly one life, respawning at a safe position until the last life is lost." Game-over
  destination (`APP_SCENE_RUNNER_MENU`) and the score-persist step are unchanged from Phase 5.
  Arcade's own `isDead`/`deathCountdown` handling in `process()` is untouched.
- Verification performed: `make mingw` -- 0 errors, 167 warnings (unchanged). All four existing
  test suites still pass unmodified.
- Rollback: `git revert 30ce209` (this single commit is deliberately atomic -- a Plan-mode
  validation pass confirmed splitting the deletion of `process2()`'s broken block from the new
  trigger's introduction would leave a real double-decrement window in between).

### [2026-07-18] Add non-interactive Runner death-lifecycle test
- Phase: Pass 6 (commit `d7fcbd4`)
- File(s): new `docs/verification/runner_death_test.c`, `Makefile` (`mingw-deathtest` target)
- Action: 39 checks -- single death (trigger, drive to idle, exactly one decrement, one respawn),
  repeated trigger while active (no extra decrement, countdown not restarted), game over at the
  last life (driven through the real `runner_frame()` pipeline until the scene actually changes,
  confirming the transition, no respawn, no repeated transition), pause (death frozen across real
  `doRender_pause`/`pause_events` calls, resumes from where it left off, not restarted), session
  reset (clears all death state), scene departure (leaving via the real `menu0_events`/
  `doRender_menu0` functions causes no further mutation), render purity while dying (extended from
  Phase 5's version to also check the new `deathCountdown` field), and the left-edge-fall/
  multiplayer respawn fixes specifically (`x` clamped instead of left negative;
  `secondPlayer` frozen during the countdown and correctly reset at respawn).
- Verification performed: **runtime-verified**, not static -- `deathtest.exe` runs and every check
  is a live assertion against the real functions. All 39 checks passed on the first run (the
  design's respawn-position fix, already caught and corrected before implementation, meant no
  test failures needed diagnosing this time, unlike Phase 5's frame-pipeline test). `make mingw` --
  0 errors, 167 warnings (unchanged, confirmed 0 from the new file). All other test suites still
  pass.
- Rollback: delete `docs/verification/runner_death_test.c` and the `mingw-deathtest` target.

### [2026-07-18] Add reproducible MinGW validation workflow
- Phase: Pass 6 (commit `f38a032`)
- File(s): new `.github/workflows/mingw-validation.yml`, new `scripts/setup-mingw-sdl2.sh`,
  `Makefile`, new `.gitattributes`
- Action: Phase 5's PR merged with no CI run at all. Added a workflow triggered on `pull_request`
  and `push` to `main`, running on `windows-latest` with `msys2/setup-msys2@v2`
  (`msystem: MINGW64`, installing `mingw-w64-x86_64-gcc` + `make`), that fetches the four pinned
  SDL2 MinGW devel packages via a new setup script, then runs `make mingw-clean`/`mingw`/
  `mingw-smoketest`/`mingw-scenetest`/`mingw-lifecycletest`/`mingw-frametest`/`mingw-deathtest` as
  separate steps (each `shell: msys2 {0}`, so a failure is attributed to the right target),
  uploading captured logs as an artifact on failure.
- **MINGW64, not UCRT64, confirmed required** during design validation: the official SDL2 "mingw"
  devel packages are built against the legacy MSVCRT runtime matching MINGW64's, not UCRT64's
  UCRT -- both environments report the identical `x86_64-w64-mingw32` triple via
  `gcc -dumpmachine`, so the triple alone doesn't decide this; documented explicitly in the
  workflow's own comments so a future maintainer doesn't "simplify" to UCRT64 on that basis.
- `Makefile` gained `SDL2_VERSION`/`SDL2_IMAGE_VERSION`/`SDL2_TTF_VERSION`/`SDL2_MIXER_VERSION`
  variables (replacing the version numbers previously hardcoded directly into `MINGW_INCLUDES`/
  `MINGW_LIBDIRS`/`mingw-dlls`) plus a `print-mingw-versions` target, so the new setup script
  reads the version numbers from one place rather than hardcoding a third copy that could drift
  out of sync -- a version mismatch between script and Makefile would otherwise fail silently in
  CI with a confusing "file not found" rather than an obvious version error.
- New `.gitattributes` forces LF line endings on `*.sh` -- without it, a CRLF-mangled shebang would
  break the setup script under bash on any checkout with `core.autocrlf=true` (confirmed this
  machine has it set), including GitHub's Windows runners, which commonly default the same way.
- All four pinned SDL2 MinGW package release URLs confirmed reachable via `curl` HTTP 200 checks,
  both during design validation and again immediately before this commit.
- Warning policy: unchanged `MINGW_WARN_FLAGS`, no `-Werror` added, no `-w` added; the workflow
  greps the build log for `warning:` and logs the count (informational only, matching this pass's
  explicit instruction not to fail CI on the pre-existing 167).
- Verification performed: local build/test suite unaffected by the Makefile variable extraction --
  same 0 errors/167 warnings, all five suites pass. The setup script itself run locally
  (idempotent-skip path, since this machine's `vendor/` was already populated) and its recreated
  `compat-include/*.h` shims confirmed byte-identical to the originals. **The workflow's actual
  first run is reported in the pull request** -- this phase's own requirement is not to merge
  until it shows green.
- Rollback: `git revert f38a032`; the Makefile version-variable extraction is safe to keep
  regardless of whether the workflow itself is kept.

## Pass 7 summary — unused code/asset cleanup (2026-07-18)

Baseline: commit `94d117b` (tag `refactor-pre-unused-cleanup`, created at the start of this pass
since it didn't exist yet), working tree clean, `make mingw` 0 errors/167 warnings, all six
existing test targets passing — all confirmed before any edit. Three independent research passes
(source/symbol reachability across `src/`+`inc/`+`docs/verification/`+`.github/`; asset
reference-tracing + SHA-256 duplicate hashing across `resource/`; generated/tracked-artifact
review via `git ls-files`+`.gitignore`), each independently re-verified by direct grep/read before
being trusted, plus a dedicated Plan-mode validation pass that caught one real gap before any code
was touched. Full audit in `docs/unused-code-assets-audit.md`, written first. Eight commits:
`3ffdeec` (audit doc), `d258527` (dead fields/macros), `b0b59fa` (dead helper functions), `2dacd2b`
(unreachable source files), `9e7adcf` (obsolete commented code), `6982d2c` (unused assets),
`e57a06e` (stale Makefile comments), `1b1fbba` (integrity check script), and this documentation
commit.

### [2026-07-18] Tag baseline + write docs/unused-code-assets-audit.md
- Phase: safety gate + pre-edit documentation
- File(s): git tag `refactor-pre-unused-cleanup` on commit `94d117b`; new
  `docs/unused-code-assets-audit.md`
- Action: confirmed working tree clean (after fast-forwarding local `main` to include the
  already-merged Phase 6 commits, which had not yet been pulled locally), current commit hash,
  and that the pre-cleanup rollback tag didn't yet exist before creating it; ran
  `make mingw-clean && make mingw` (0 errors/167 warnings) and all six existing test targets
  (`smoketest`/`scenetest`/`lifecycletest`/`frametest`/`deathtest`) as the safety gate; launched 3
  parallel read-only Explore agents to audit source reachability, asset references + duplicate
  hashing, and tracked-artifact hygiene; personally re-verified every high-impact claim via direct
  grep/read before trusting it; then wrote the complete audit — no code changed in this step.
- Verification performed: `make mingw` (0 errors/167 warnings),
  `mingw-smoketest`/`mingw-scenetest`/`mingw-lifecycletest`/`mingw-frametest`/`mingw-deathtest`
  (all PASS) re-confirmed as the safety gate before any edit.
- Rollback: `git tag -d refactor-pre-unused-cleanup`; delete `docs/unused-code-assets-audit.md`.

### [2026-07-18] Remove dead GameState/Man fields and macros
- Phase: Pass 7 (commit `d258527`)
- File(s): `inc/header.h`, `src/app.c`, `src/doRender.c`, `src/loadGame.c`, `src/processEvents.c`
- Action: removed 13 confirmed-zero-reference struct fields (`Man.shooting` — write-only, `.
  visible2`, `.attack`, `.currentSpriteAttack1`, `.currentSpriteSkill`, `.sheetTextureAttack1`,
  `.sheetTextureSkill`; `GameState.kills_list[25]`, `.countWaves`, `.musicChannel`,
  `.secondPlayerImage`, `.enemyFrame`, `.background`, `.enemyTexture2`) and 2 dead macros
  (`STATUS_STATE_GAMEOVER`, `LEADERBOARD_TXT`), plus the one never-defined `draw_status_x_list`
  prototype. Landed atomically with every now-dangling touch point: 7 `destroy_texture()` calls in
  `app.c` (the 4 dead-texture fields plus the man/secondPlayer `sheetTextureAttack1/Skill` pairs),
  the commented "attack render" block in `doRender.c`, `loadGame.c`'s 2 `shooting = 0;` write
  sites, and several small dangling comment fragments in `processEvents.c` that referenced the
  removed `shooting`/`attack` fields.
- Reasoning: a Plan-mode validation pass (reading every deletion site directly, not just the
  audit's summary) caught that `shooting`'s two `loadGame.c` write sites had to be deleted in the
  *same* commit as the field itself, or the build would fail — folded in before any code was
  written, not discovered by a failed build.
- Behavior impact: none — every removed item had zero live reads (or, for the write-only
  `shooting` field, zero reads anywhere at all). Confirmed no memory-layout dependency exists
  (single `calloc(1, sizeof(GameState))` in `main.c`, no `offsetof`/serialization use of either
  struct).
- Verification performed: `make mingw` — 0 errors, 167 warnings (unchanged). All six test targets
  pass.
- Rollback: `git revert d258527`.

### [2026-07-18] Remove dead helper functions
- Phase: Pass 7 (commit `b0b59fa`)
- File(s): `src/X_score.c`, `src/x_list_leader.c`, `src/kills_score.c`, `src/asset_loader.c`,
  `inc/header.h`
- Action: removed `shutdown_status_x`, `shutdown_status_x_list`, `shutdown_status_kills`, and
  `load_chunk` — each confirmed zero callers via repo-wide grep (including all 5
  `docs/verification/*.c` test harnesses). `load_chunk` specifically: every real SFX load site (8
  checked) uses raw `Mix_LoadWAV()` directly and already replicates the NULL-guard this helper
  would provide, so nothing was lost by leaving them as-is. Each file's sibling `init_*`/`draw_*`
  functions are untouched and remain live. Removed the one corresponding `header.h` prototype
  (`shutdown_status_kills`; the other two never had one).
- Behavior impact: none.
- Verification performed: `make mingw` — 0 errors, 167 warnings (unchanged). All six test targets
  pass.
- Rollback: `git revert b0b59fa`.

### [2026-07-18] Remove unreachable source files
- Phase: Pass 7 (commit `2dacd2b`)
- File(s): 16 `mx_*.c` files deleted (`mx_atoi`, `mx_count_words`, `mx_file_to_str`, `mx_itoa`,
  `mx_sort_arr_char`, `mx_sort_arr_int`, `mx_strcat`, `mx_strcmp`, `mx_strcpy`, `mx_strdel`,
  `mx_strdup`, `mx_strjoin`, `mx_strlen`, `mx_strncpy`, `mx_strnew`, `mx_strsplit`),
  `src/leader_events.c`, `src/doRender_multiplayer.c`, and the 18 corresponding `header.h`
  prototypes.
- Reasoning: the `mx_*` cluster is fully self-contained and dead — every function either has zero
  callers, or is called only by another function in the same cluster that itself has zero
  external callers, confirmed by tracing every internal cross-call (`mx_strdup`→`mx_strnew`+
  `mx_strcpy`; `mx_strjoin`→`mx_strdup`+`mx_strnew`+`mx_strcat`+`mx_strlen`; `mx_strsplit`→
  `mx_count_words`+`mx_strnew`+`mx_strncpy`; `mx_sort_arr_char`→`mx_strcmp`), none of which escape
  the cluster. `leader_events.c` re-confirms `docs/scene-state-map.md`'s own prior finding, fresh
  against current code, not assumed from memory. `random_sign.c` (grouped alongside the dead
  `mx_sort_arr_char` under the same "Leo functions" header comment) was independently confirmed
  live (`loadGame.c:467,474`, `processEvents.c:770,777`) and explicitly *not* deleted.
- Behavior impact: none.
- Verification performed: `make mingw` — 0 errors, warnings dropped 167→161. Diffed the exact
  warning list before/after: exactly 6 warnings disappeared, all originating inside the deleted
  `mx_*` files themselves (`mx_atoi.c` ×3, `mx_itoa.c` ×1, `mx_strnew.c` ×1, `mx_strsplit.c` ×1) —
  zero new warnings anywhere else. All six test targets pass.
- Rollback: `git revert 2dacd2b`.

### [2026-07-18] Remove obsolete commented-out implementation
- Phase: Pass 7 (commit `9e7adcf`)
- File(s): `src/processEvents.c`, `src/doRender_leaderboard.c`, `src/x_list_leader.c`
- Action: deleted two self-contained dead blocks in `processEvents.c` — Arcade's multiplayer
  game-over branch (dead pointer-arithmetic pseudocode referencing an undeclared `leader`
  variable, plus a file-write attempt against `resource/files/leaderboard.txt`) and Runner's
  game-over branch (a file-write attempt against `LEADERBOARD_TXT` using the just-removed
  `mx_itoa`/`mx_strlen`) — both fully superseded by the live in-memory `x_list[25]`-based score
  persistence immediately adjacent. Also cleaned up two small stale comment fragments left over
  from a past merge/rename, referencing the already-nonexistent `draw_status_x_list` and
  `mx_sort_arr_int`.
- Behavior impact: none — pure comment removal; the live guards/comments immediately before and
  after each deleted block were confirmed intact.
- Verification performed: `make mingw` — 0 errors, 161 warnings (unchanged). All six test targets
  pass.
- Rollback: `git revert 9e7adcf`.

### [2026-07-18] Remove proven-unused game assets
- Phase: Pass 7 (commit `6982d2c`)
- File(s): 22 asset files deleted (~7.68MB) — see `docs/unused-code-assets-audit.md` section 5.6
  for the full list with sizes.
- Reasoning: every asset load in this codebase is a single string literal passed directly to
  `load_texture`/`load_music`/`load_chunk`/`load_font` or a raw `Mix_LoadWAV`/`Mix_LoadMUS`/
  `TTF_OpenFont` call — confirmed no dynamic path construction exists anywhere (`sprintf`/
  `strcat`/`mx_strcat`/`mx_strjoin` hits are all unrelated to file paths), making exhaustive
  string-literal grep fully dispositive for "is this asset ever loaded."
- Behavior impact: none — confirmed by running all five asset-loading test suites (which exercise
  `arcade_assets_load`/`runner_assets_load` end to end) unmodified after deletion; all still pass,
  proving none of the 22 files was ever actually loaded.
- Verification performed: `make mingw` — 0 errors, 161 warnings (unchanged). All six test targets
  pass.
- Rollback: `git revert 6982d2c`.

### [2026-07-18] Remove stale build references
- Phase: Pass 7 (commit `e57a06e`)
- File(s): `Makefile`
- Action: removed two dead commented-out lines under `SRCS` (`#src/main.c \`, `#src/status.c \`)
  — leftover from some earlier Makefile shape, unrelated to the current glob-based `src/*.c`, no
  effect on any target.
- Behavior impact: none.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All six test targets pass.
- Rollback: `git revert e57a06e`.

### [2026-07-18] Add non-interactive repository usage integrity check
- Phase: Pass 7 (commit `1b1fbba`)
- File(s): new `scripts/audit_repository_usage.py`, `Makefile` (new `audit-repo` target),
  `.github/workflows/mingw-validation.yml` (new CI step)
- Action: a dependency-light (Python stdlib only), deterministic script with two checks: (1)
  extracts every `resource/`-shaped string literal from `src/*.c` and confirms the file exists
  with **exact case** — a manual case-sensitive directory-listing comparison, not
  `os.path.exists`, so it correctly catches casing bugs even on a case-insensitive host filesystem
  like this machine's Windows; (2) confirms every `header.h` function prototype has a matching
  definition somewhere in `src/*.c`.
- **This script found something real on its first run**: the same 3 case-mismatched literals
  documented in the audit (`Sunset_front.png`, `brick_block.png`, `copper_block.png` — source
  requests lowercase, on-disk files are capitalized, works today only because Windows is
  case-insensitive, would break `IMG_Load` on case-sensitive macOS/Linux). Also initially reported
  5 false-positive "dangling prototypes" (`load_menu0/1/2`, `init_status_x_list`, `random_sign`)
  before the definition-matching regex was fixed to accept K&R-style brace-on-same-line function
  definitions, not just brace-on-next-line — after the fix, 0 false positives.
- Reasoning: added the 3 confirmed case-mismatch paths to an explicit allowlist (with a comment
  explaining why, citing the audit doc) rather than fixing the casing bug in this phase — fixing a
  naming/casing bug is a different kind of change than removing unused content, out of this
  phase's charter; the allowlist makes the script a meaningful gate against *new* dangling
  references instead of always failing on a known, already-documented issue.
- Verification performed: **runtime-verified**, not static — the script actually runs and reports
  real findings, not a code-reading exercise. Wired into CI via the runner's own pre-installed
  Python (no MSYS2 dependency needed, since the script only reads files). `make mingw` — 0 errors,
  161 warnings (unchanged). All six test targets pass.
- Rollback: delete the script, the `audit-repo` Makefile target, and the CI step.

## Pass 8 summary — fix case-sensitive asset paths + cross-platform resource validation (2026-07-19)

Corrects the three case-mismatched asset-path literals Pass 7 found and deliberately deferred
(`Sunset_front.png`, `brick_block.png`, `copper_block.png`), removes the temporary allowlist that
was masking them, restructures `scripts/audit_repository_usage.py`'s output into explicit
errors/known-exceptions/informational tiers with several new checks (regular-file, case-colliding
sibling, backslash literal, duplicate-alias), adds three named lifecycle-test assertions for the
three corrected fields, and adds a genuinely case-sensitive Linux CI job — since a Windows runner,
however thorough, cannot prove case-sensitive path correctness by actually failing to load a file.
Baseline confirmed before any edit: commit `8634928` (tag `refactor-pass-7-unused-cleanup`,
created this pass), `make mingw` 0 errors/161 warnings, all eight existing local targets passing.

### [2026-07-19] Map case-sensitive asset path defects
- Phase: Pass 8 (commit `d49601f`)
- File(s): new `docs/asset-path-portability.md`
- Action: re-verified all three of Pass 7's confirmed mismatches directly against the current
  tree; resolved the canonical-naming question per directory via sibling-file convention (git
  history is uninformative — the whole asset tree was bulk-committed once at repo init, commit
  `cfb7ca7`, so no rename history exists to consult); confirmed via a repo-wide search across
  `src/*.c`, headers, test harnesses, the Makefile, the setup script, the CI workflow, and docs
  that no additional mismatch of any kind exists (case, slash direction, repeated separators,
  leading `./`, extension case, Unicode look-alikes, adjacent literal splits, path aliases).
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim in the doc directly from the
  current tree (not carried over from Pass 7's summary text).
- Rollback: delete the file.

### [2026-07-19] Correct exact casing of active asset paths
- Phase: Pass 8 (commit `590d359`)
- File(s): `src/loadGame.c:116,118,119`
- Before:
  ```c
  ok = ok && load_texture(game->renderer, "./resource/images/background/Sunset_front.png", &game->sheetTextureBack2);
  ...
  ok = ok && load_texture(game->renderer, "./resource/images/terrain/brick_block.png", &game->brick_block);
  ok = ok && load_texture(game->renderer, "./resource/images/terrain/copper_block.png", &game->copper_block);
  ```
- After:
  ```c
  ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_front.png", &game->sheetTextureBack2);
  ...
  ok = ok && load_texture(game->renderer, "./resource/images/terrain/Brick_block.png", &game->brick_block);
  ok = ok && load_texture(game->renderer, "./resource/images/terrain/Copper_block.png", &game->copper_block);
  ```
- Reasoning: each corrected literal now matches its already-existing, unmodified tracked file —
  chosen over renaming the tracked files because it's the smaller, safer correction and matches
  each directory's own naming convention (see `docs/asset-path-portability.md`). Zero file renames.
- Behavior impact: none — same content, previously "working" only because Windows' filesystem is
  case-insensitive; now also correct on case-sensitive macOS/Linux.
- Verification performed: `make mingw` — 0 errors, 161 warnings (unchanged). All eight local
  targets pass, including the strengthened lifecycle test's three new named assertions
  confirming `sheetTextureBack2`/`brick_block`/`copper_block` all load non-NULL.
- Rollback: `git revert 590d359`.

### [2026-07-19] Strengthen repository asset-path audit
- Phase: Pass 8 (commit `1389b84`)
- File(s): `scripts/audit_repository_usage.py`, `docs/verification/lifecycle_test.c`
- Action: restructured findings into three explicit tiers (errors / known exceptions /
  informational) instead of a flat count; added a regular-file check, a same-directory
  case-colliding-sibling check, a raw-literal backslash check, and a duplicate-alias tracker
  (2+ literals resolving to the same canonical file, reported informationally, never as an
  error); emptied `ALLOWLIST_ASSET_PATHS` now that the three entries it held are actually fixed;
  added three named assertions to `lifecycle_test.c` (`sheetTextureBack2 (sunset_front.png)`,
  `brick_block (Brick_block.png)`, `copper_block (Copper_block.png)`, all non-NULL) immediately
  after the existing blanket `arcade_assets_load() succeeds` check.
- Reasoning: a flat "0 findings" output can silently hide a masking allowlist entry forever; the
  new tiered output makes it structurally impossible to hide an undiagnosed mismatch behind a
  clean-looking result, and the duplicate-alias check turns a previously-invisible pattern
  (multiple lazy-load guard sites referencing the same sound file) into visible, harmless
  information instead of leaving it unexamined.
- Verification performed: **runtime-verified** — script actually run against the corrected tree:
  `Asset path errors: 0`, `Prototype errors: 0`, `Known exceptions: 0`, `Informational notices: 4`
  (legitimate duplicate lazy-load-guard references: `menuMus.mp3` x3, `select.wav` x5,
  `shoot.wav` x3, `jump.wav` x2), `Result: PASS`. `make mingw` — 0 errors, 161 warnings
  (unchanged). All eight local targets pass.
- Rollback: `git revert 1389b84`.

### [2026-07-19] Validate asset paths on a case-sensitive runner
- Phase: Pass 8 (commit `fce8ef8`)
- File(s): `.github/workflows/mingw-validation.yml` (new `linux-asset-validation` job; warning
  baseline comment updated 167→161), `Makefile` (new `linux`/`linux-smoketest`/`linux-clean`
  targets, entirely separate `LINUX_*` variable family, zero changes to `all`/`mingw*`)
- Action: added a second CI job on `ubuntu-latest` whose mandatory step runs the real
  `audit_repository_usage.py` against a genuinely case-sensitive filesystem — the actual proof
  the prior commit's corrections are right, not just statically plausible. A second, best-effort
  part (every step `continue-on-error: true`) attempts a full additive Linux build + headless
  smoke test via `apt-get`-installed SDL2 dev packages, a compat-include header shim mirroring
  the one already proven necessary for the Windows/MinGW build, and SDL's dummy audio/video
  drivers (GitHub-hosted Linux runners have neither audio hardware nor a display server).
- Reasoning: the best-effort part was authored without access to a Linux toolchain to verify
  against in this environment, so `continue-on-error` ensures an unverified assumption about
  package names, header layout, or headless driver behavior cannot block the job or the PR — only
  the mandatory case-sensitivity check can. The actual CI run on the PR is the deciding signal.
- Verification performed: local build/test suite unaffected (`make -n linux` produces
  syntactically valid, correctly-flagged commands; `make mingw` still 0 errors/161 warnings, all
  eight targets pass). The workflow's actual Linux-runner result is reported in the pull request,
  not assumed here.
- Rollback: `git revert fce8ef8`; the Makefile Linux targets are harmless to keep regardless of
  whether the CI job's best-effort steps ever succeed.

## Pass 9 summary — evidence-based SOLID/GoF architecture refactor (2026-07-19)

Applies SOLID/GoF only where the codebase already showed repeated, demonstrated pain, per that
phase's own explicit "not a checklist exercise" charter: at most two substantial GoF patterns, one
or two small SOLID extractions, at most three new abstractions total. Baseline confirmed before
any edit: commit `e967001` (tag `refactor-pre-solid-gof`), `make mingw` 0 errors/161 warnings, all
eight existing local targets passing. Full evidence, decision tables, and pattern rationale in
`docs/solid-gof-audit.md`, `docs/dependency-map.md`, and `docs/design-patterns.md`.

### [2026-07-19] Audit SOLID violations and GoF candidates
- Phase: Pass 9 (commit `e745aeb`)
- File(s): new `docs/solid-gof-audit.md`, `docs/dependency-map.md`
- Action: module map, header dependency map, mutable-state ownership map, top-level call graph,
  per-principle SOLID findings, and a full GoF candidate-pattern decision table
  (required/already-present/rejected/deferred) gathered directly from the tree via three parallel
  codebase explorations plus targeted direct reads.
- **Two precision corrections made during implementation, not silently**: the exact
  entity-spawn-duplication count was refined once `loadGame.c`'s `enemyValues` reset loop was
  found to have a genuinely different field contract than every other site (it never sets
  `visible`/`countShots`); the header-split scope was refined once `app_change_scene`'s 5 real
  callers (vs. `arcade_frame`/`runner_frame`'s 1) were confirmed by grep, changing what could
  safely move fully out of `header.h`.
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete both files.

### [2026-07-19] Centralize enemy/smart-enemy/boss spawn invariants
- Phase: Pass 9 (commit `3a53e41`)
- File(s): new `src/entity_spawn.c`, `inc/entity_spawn.h`; `src/process.c` (8 wave-trigger
  blocks), `src/loadGame.c` (2 of 3 reset loops), `Makefile` (new `mingw-entityspawntest` target),
  new `docs/verification/entity_spawn_test.c`
- Action: `enemy_spawn`/`smart_enemy_spawn`/`boss_spawn` (Factory Method) replace 10 hand-written
  inline `Enemies` struct initializations with bounds-checked creation calls; each rejects an
  out-of-range index without mutating anything, a correctness guarantee none of the original
  inline code had. `loadGame.c`'s `enemyValues` reset loop is deliberately left inline (its
  contract genuinely differs — see the audit correction above).
- Behavior impact: none — every converted call site passes the same literal values it used
  before; regression-verified by the new test's exact-value assertions against the original
  literals.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 9 local targets pass.
- Rollback: `git revert 3a53e41`.

### [2026-07-19] Extract focused scene and frame headers
- Phase: Pass 9 (commit `42517d3`)
- File(s): new `inc/scene.h`, `inc/frame.h`; `inc/header.h`, `src/scene.c`, `src/frame.c`,
  `src/main.c`, `docs/verification/frame_pipeline_test.c`, `docs/verification/runner_death_test.c`
- Action: mirrors the existing `inc/app.h` pattern. `arcade_frame`/`runner_frame` (1 definer, 1
  production caller) moved fully out of `header.h` into `frame.h`; two test harnesses that also
  call them directly needed the same include, caught immediately by a failed build.
  `app_change_scene` (5 real callers) is declared in both `header.h` (unchanged, so 3 unrelated
  callers need no edit this phase) and the new `scene.h` — a legal duplicate prototype, per the
  task's own "leave unresolved declarations in the compatibility header, migrate gradually"
  guidance. `AppScene` stays defined in `header.h` (moving it would create a circular include with
  `GameState.scene`'s own dependency on it).
- Behavior impact: none.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 9 local targets pass;
  `audit-repo` still `Result: PASS`.
- Rollback: `git revert 42517d3`.

### [2026-07-19] Introduce discrete-action command translation
- Phase: Pass 9 (commit `645e13f`)
- File(s): new `src/input_command.c`, `inc/input_command.h`; `src/processEvents.c`
  (`processEvents`/`processEvents2`), `src/menu_events.c` (`menu0_events` only), `Makefile` (new
  `mingw-commandtest` target), new `docs/verification/command_translation_test.c`
- Action: `GameCommand` enum + `translate_arcade_command`/`translate_runner_command`/
  `translate_menu_command` — pure `SDL_Keycode -> GameCommand` functions replacing the raw-keycode
  switch dispatch for the 5 keys `processEvents()`/`processEvents2()` independently duplicate
  (ESCAPE/p/w/UP/q), plus a formalization of `menu0_events()`'s existing dual-keycode
  fall-through. `menu_events()`/`pause_events()` deliberately not converted this phase (§9 of the
  audit). `processEvents2()`'s `SDLK_0` cheat code intentionally stays outside `GameCommand`,
  still checked as a raw keycode. No queue, no heap allocation, no change to continuous held-key
  polling.
- Behavior impact: none — every converted handler body is byte-for-byte unchanged; only the
  dispatch key changed from keycode to `GameCommand`.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 10 local targets pass
  (including the new test asserting every documented key-to-command mapping, both dual-keycode
  pairs, and the `SDLK_0` exclusion).
- Rollback: `git revert 645e13f`.

### [2026-07-19] Route load_menu.c texture loads through the checked loader
- Phase: Pass 9 (commit `2a4425d`)
- File(s): `src/load_menu.c`
- Action: replaced 3 raw `IMG_Load()`/`SDL_CreateTextureFromSurface()` call sites — the one place
  in the codebase bypassing the checked, NULL-safe `load_texture()` wrapper every other texture
  load already uses — with `load_texture()` calls. Each site's original fail-hard behavior (print
  message, `SDL_Quit()`, `exit(1)`) is unchanged; only the loading call itself is now checked.
- Behavior impact: none on success; adds the same NULL-safety every other loader site already has
  on failure.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 10 local targets pass.
- Rollback: `git revert 2a4425d`.

## Pass 10 summary — incremental GameState decomposition and header isolation (2026-07-19)

Finishes the scene-interface header migration Phase 9 deliberately left duplicated, corrects
the entity-spawn functions' pattern terminology, and introduces exactly two cohesive nested
structs inside `GameState` (`AppContext`, `AssetLifecycleFlags`), chosen from an evidence-based
field-by-field audit rather than the task's example list by default. Baseline confirmed before
any edit: commit `81dda1e` (tag `refactor-pre-gamestate-decomposition`), `make mingw` 0
errors/161 warnings, all 8 existing local targets passing. Full evidence in
`docs/gamestate-decomposition.md`.

### [2026-07-19] Audit GameState ownership and header dependencies
- Phase: Pass 10 (commit `daa324b`)
- File(s): new `docs/gamestate-decomposition.md`
- Action: field-by-field `GameState` inventory; memory-layout safety verification (exactly one
  `sizeof(GameState)` site, zero memcpy/memset/casts/pointer-arithmetic/offsetof/file-I-O/
  whole-struct-memcmp touching `GameState`); exact access-site counts for every candidate group;
  selection rationale for `AppContext`/`AssetLifecycleFlags` over audio fields (lifecycle not
  centralized, per Phase 9's own finding) and Arcade/Runner session data (large/high-risk,
  explicitly discouraged as a first choice). Found an accidental, pre-existing verbatim duplicate
  of `doRender`'s prototype inside `header.h` itself, unrelated to the documented
  `app_change_scene` duplication.
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete the file.

### [2026-07-19] Clarify simple factory pattern terminology
- Phase: Pass 10 (commit `294765e`)
- File(s): `docs/design-patterns.md`, `docs/solid-gof-audit.md`
- Action: `enemy_spawn`/`smart_enemy_spawn`/`boss_spawn` were labeled "Factory Method" in Phase
  9's docs. Corrected to "Simple Factory-style creation function" (informal, not a GoF pattern of
  its own) — no creator hierarchy, no subclassing, no polymorphic product selection exists or was
  added. `src/entity_spawn.c`/`inc/entity_spawn.h` unchanged.
- Behavior impact: none (documentation only).
- Verification performed: `make mingw` — 0 errors, 161 warnings (unaffected, as expected).
- Rollback: `git revert 294765e`.

### [2026-07-19] Complete scene interface migration
- Phase: Pass 10 (commit `c926cea`)
- File(s): `inc/header.h`, `src/menu_events.c`, `src/pause_events.c`, `src/processEvents.c`,
  `docs/verification/scene_transition_test.c`, `docs/verification/lifecycle_test.c`,
  `docs/verification/runner_death_test.c`, `docs/verification/frame_pipeline_test.c`
- Action: the 3 remaining callers of `app_change_scene()` now include `scene.h` directly;
  `header.h`'s duplicate prototype (deliberately left since Phase 9, pending exactly this
  migration) is removed. Also removed an accidental, unrelated verbatim duplicate of `doRender`'s
  prototype found inside `header.h` itself. 4 test harnesses calling `app_change_scene`/
  `arcade_frame`/`runner_frame` directly needed the same `#include "scene.h"` addition — caught
  immediately by failed builds, not silently missed.
- Behavior impact: none.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 8 local targets pass.
- Rollback: `git revert c926cea`.

### [2026-07-19] Validate focused header self-containment
- Phase: Pass 10 (commit `3c8410c`)
- File(s): new `docs/verification/header_only_{app,scene,frame,entity_spawn,input_command}.c`,
  `Makefile` (new `mingw-headertest` target), `scripts/audit_repository_usage.py`
- Action: one tiny translation unit per focused header, each compiled with `-fsyntax-only`
  (parse + typecheck, no codegen/link, since these files have no runtime behavior) to confirm
  each header compiles standalone with no reliance on inclusion order. Extended the audit script
  with a new check scanning every `inc/*.h` file (not just `header.h`) for duplicate
  function-prototype declarations, reported in the same tiered error/known-exception format as
  the existing checks — a regression guard against reintroducing an accidental duplicate like the
  `doRender` one found this phase.
- Verification performed: `make mingw-headertest` — all 5 headers compile with 0
  warnings/errors. `audit-repo` — `Duplicate declaration errors: 0`, `Result: PASS`.
- Rollback: `git revert 3c8410c`.

### [2026-07-19] Group application and asset lifecycle state
- Phase: Pass 10 (commit `9895bad`)
- File(s): `inc/header.h` (both struct definitions), `src/app.c`, `src/scene.c`,
  `src/loadGame.c`, `src/load_menu.c`, `src/kills_score.c`, `src/X_score.c`,
  `src/x_list_leader.c`, `src/status.c`, `src/draw_lifes.c`, `src/main.c`, `src/menu_events.c`,
  `src/pause_events.c`, `src/processEvents.c`, and 5 `docs/verification/*.c` test files
- Action: introduces `AppContext { renderer; scene; }` (field `app`) and `AssetLifecycleFlags
  { arcadeAssetsLoaded; runnerAssetsLoaded; sharedAssetsLoaded; }` (field `assetFlags`) — the two
  cleanest, most centrally-owned groups identified in the audit. `window` deliberately excluded
  (never a `GameState` field — adding it would introduce new state, not migrate existing state).
  Every access site is a pure mechanical rename (`game->renderer` → `game->app.renderer`,
  `game->scene` → `game->app.scene`, `game->arcadeAssetsLoaded` →
  `game->assetFlags.arcadeAssetsLoaded`, etc.) — zero logic, control-flow, constant, or behavior
  change. Both groups land in one commit rather than the two originally planned:
  `loadGame.c`/`scene.c`/`header.h` and 3 test files are touched by both migrations, and a clean
  hunk-level split risked an intermediate non-compiling state — a deliberate, transparent
  deviation from the original commit sequence, documented here and in the PR.
- Behavior impact: none — confirmed by the full regression suite; `GameState` remains a single
  `calloc(1, sizeof(GameState))` allocation (`src/app.c:128`, unchanged), so both new nested
  structs are correctly zero-initialized with no code change needed at the allocation site.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 9 local targets pass.
- Rollback: `git revert 9895bad` (reverts both groups together).

### [2026-07-19] Validate nested GameState lifecycle
- Phase: Pass 10 (commit `e29eb20`)
- File(s): new `docs/verification/gamestate_grouping_test.c`, `Makefile` (new
  `mingw-groupingtest` target)
- Action: verifies `AppContext`/`AssetLifecycleFlags` are correctly zero-initialized via the
  existing `calloc`, explicitly writable through the real production functions
  (`app_change_scene`, `arcade_assets_load`/`arcade_assets_unload`), reset/cleaned-up correctly,
  and that unrelated `GameState` fields (`man.lives`, `app.scene` during an asset-flag change)
  remain untouched by either group's operations.
- Verification performed: `make mingw` — 0 errors, 161 warnings. All 10 local targets pass (13/13
  new assertions pass).
- Rollback: `git revert e29eb20`.

## Pass 11 summary — fixed-timestep player physics (2026-07-19)

Triggered by an external technical assessment of the custom kinematic physics system, which
identified frame-dependent physics (no fixed timestep — a 144Hz display runs physics ~2.4x faster
than 60Hz) as the core correctness defect. This pass implements the assessment's own "Phase 1 —
Stabilize time" scope only: a fixed 60Hz accumulator drives `main()`'s two gameplay scenes, and
player (man/secondPlayer) gravity/velocity/acceleration/friction/jump constants are converted to
explicit per-second units. Enemies, bosses, bullets, background/decor scroll, and moving traps are
explicitly not touched this pass. Baseline confirmed before any edit: commit `a58268f` (tag
`refactor-pre-fixed-timestep`, `main` after Phase 10's PR #6 merge), `make mingw` 0 errors/161
warnings, all 10 existing local targets passing. Full evidence in `docs/physics-timestep-map.md`.

### [2026-07-19] Map fixed-timestep physics conversion
- Phase: Pass 11 (commit `5828ace`)
- File(s): new `docs/physics-timestep-map.md`
- Action: confirmed zero existing timing infrastructure anywhere in the tree (no `SDL_GetTicks`/
  `SDL_GetPerformanceCounter`/delta-time variable); full inventory of every frame-dependent player
  constant with exact file:line citations; mathematically derived and verified the frame-to-
  per-second conversion factors (velocity ×60, acceleration ×3600) against the exact old per-frame
  numeric behavior at a fixed 60Hz tick; documented the planned `processEvents`/`processEvents2` →
  fixed-tick relocation; listed everything found but deliberately deferred (enemy/boss/bullet/
  decor/trap timing, the Runner ceiling `onLedge` bug, the bullet triple-movement bug,
  jump-hold-thrust redesign, collision-order issues).
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete the file.

### [2026-07-19] Add fixed-timestep constants and simulate functions
- Phase: Pass 11 (commit `41940a0`)
- File(s): `inc/header.h`, `inc/frame.h`, `src/frame.c`
- Action: added `PHYSICS_HZ`/`PHYSICS_DT`/`MAX_FRAME_TIME`/`MAX_PHYSICS_STEPS_PER_FRAME` and the 8
  player per-second constants; removed the old frame-tuned `GRAVITY` macro; introduced
  `arcade_simulate`/`runner_simulate` as the new fixed-tick entry points (`process`/`process2` +
  `collisionDetect`/`collisionDetect2`); gave `arcade_frame`/`runner_frame`/`process`/`process2` an
  explicit `float dt` parameter.
- Behavior impact: none yet (constants unused until the next commit).
- Rollback: `git revert 41940a0`.

### [2026-07-19] Convert player physics to fixed-timestep per-second units
- Phase: Pass 11 (commit `41940a0`)
- File(s): `src/process.c`, `src/processEvents.c`
- Action: converted the 4 gravity sites and 4 man/secondPlayer position-integration sites in
  `process()`/`process2()` to use `dt` and the new per-second constants; converted the 4
  jump-impulse sites in `processEvents.c`; relocated the held-key continuous-force blocks
  (horizontal accel/clamp, jump-hold thrust, friction/snap) out of `processEvents()`/
  `processEvents2()` directly into `process()`/`process2()` as originally planned.
- Behavior impact: at the fixed `dt=1/60` used everywhere in this pass, numerically identical to
  the old per-frame behavior (verified in `docs/physics-timestep-map.md` section 3); the
  *character* of the behavior becomes frame-rate-independent, which was the point.
- Verification performed: 4 test-harness assertions failed at this point
  (`FRAME TEST: FAIL - process(): animFrame advances when moving, on-ledge, time%3==0` and 3
  similar) — root cause: `process()`/`process2()` now called `SDL_GetKeyboardState(NULL)`
  internally, which in the headless test environment always reports "no keys pressed," silently
  overwriting the tests' manually-set `dx`/`slowingDown` preconditions. Fixed in the next commit by
  extraction rather than by weakening the tests.
- Rollback: `git revert 41940a0`.

### [2026-07-19] Drive the game loop with a fixed-timestep accumulator
- Phase: Pass 11 (commit `3ea1ae2`)
- File(s): `src/main.c`
- Action: added a real-time accumulator (`SDL_GetPerformanceCounter`/
  `SDL_GetPerformanceFrequency`, real frame time capped at `MAX_FRAME_TIME`). The two gameplay
  scene cases each run a bounded `while (accumulator >= PHYSICS_DT && steps <
  MAX_PHYSICS_STEPS_PER_FRAME)` loop calling `arcade_simulate`/`runner_simulate`, then render/
  process-events once, exactly as before. The other 7 scene cases are unchanged and never accrue
  the accumulator. `runner_update_death()` deliberately kept outside `runner_simulate()`, called
  once per real frame after `doRender2()` exactly as before, preserving Phase 5/6's deliberate
  render-before-resolve ordering for the Runner death sequence.
- Behavior impact: `process`/`process2` (and everything they call, including still-unconverted
  enemy/boss/bullet/decor/trap code) now execute at a fixed real-world rate rather than once per
  rendered frame — the intended fix.
- Rollback: `git revert 3ea1ae2`.

### [2026-07-19] Extract held-key force application from process()/process2()
- Phase: Pass 11 (commit `78c6fde`)
- File(s): `inc/header.h`, `inc/frame.h`, `src/frame.c`, `src/process.c`
- Action: design refinement made during implementation, not in the original plan text. Extracted
  the held-key continuous-force code (previously relocated directly into `process()`/`process2()`
  by the earlier commit) into two new standalone functions, `apply_arcade_player_forces`/
  `apply_runner_player_forces` (`src/process.c`), called by `arcade_simulate`/`runner_simulate`
  immediately before `process`/`process2`. This restores `process()`/`process2()` to being free of
  any real-keyboard dependency, fixing the 4 test failures from the previous commit without
  weakening any assertion. Also caught and corrected during this same commit: an initial draft of
  `apply_runner_player_forces` had incorrectly copied Arcade's `game->time % 6 == 0`
  animation-frame gating onto Runner's blocks — re-verified against the exact original
  `processEvents2()` content confirmed Runner has no such gating, and the erroneous blocks were
  removed before finalizing.
- Behavior impact: none relative to the previous commit's intended behavior — this is a pure
  internal restructuring that fixes testability, not a functional change to the converted physics.
- Verification performed: `make mingw` — 0 errors, 145 warnings (down from 161; explained in
  `docs/physics-timestep-map.md` — 8 relocated held-key lines previously used bare `double`
  literals against `float` fields, triggering `-Wdouble-promotion`/`-Wfloat-conversion` twice each;
  the new code uses explicit `float`-suffixed constants, eliminating the mismatch). All 9
  pre-existing local targets pass.
- Rollback: `git revert 78c6fde`.

### [2026-07-19] Update existing test harnesses for explicit dt
- Phase: Pass 11 (commit `8e46a7d`)
- File(s): `docs/verification/frame_pipeline_test.c`, `docs/verification/runner_death_test.c`
- Action: mechanical update — every direct call to `process`/`process2`/`arcade_frame`/
  `runner_frame` now passes an explicit `PHYSICS_DT` argument. No assertion values changed.
- Behavior impact: none (test-only).
- Verification performed: `make mingw-frametest`/`make mingw-deathtest` — both `ALL PASS`.
- Rollback: `git revert 8e46a7d`.

### [2026-07-19] Validate fixed-timestep physics
- Phase: Pass 11 (commit `15d5111`)
- File(s): new `docs/verification/physics_timestep_test.c`, `Makefile` (new `mingw-physicstest`
  target), `.github/workflows/mingw-validation.yml` (step + log-upload path added in the same
  commit — both halves of the CI wiring done together this time, after Phases 9 and 10 each
  forgot the CI-wiring half of a new local test target and had to fix it in a follow-up commit)
- Action: (1) regression-equivalence — one fixed tick from rest reproduces the old per-frame
  code's exact numeric behavior bit-for-bit; (2) refresh-rate independence — the same total
  simulated time, chunked into a fast-display pattern (240 small real-time steps) vs. a
  slow-display pattern (30 large real-time steps), produces the same tick count and identical
  final `man.x`/`y`/`dx`/`dy`. One assertion was loosened during implementation from an exact
  `ticksA == 60` check to `ticksA == ticksB` plus a `59..60` range check, since double-precision
  summation doesn't guarantee landing on exactly `1.0` — the real invariant is that both chunking
  patterns agree with each other.
- Verification performed: `make mingw` — 0 errors, 145 warnings. All 10 local targets pass
  (10/10 new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert 15d5111`.

## Pass 12 summary — input/simulation separation (2026-07-19)

Implements the physics assessment's own "Phase 2 — separate input and simulation (`InputState`
struct, edge-triggered de-duplication)", the recommended follow-on to Pass 11's fixed timestep.
Pass 11 fixed the continuous-force half of the frame-rate-dependence problem (held-key
accel/jump-hold/friction, now on the fixed tick); this pass closes the remaining discrete half —
jump-trigger handling, which was still evaluated once per real frame rather than at the fixed tick
rate. Also fixes a regression discovered during this pass's audit: Runner's jump impulse was never
converted off a bare frame-tuned `-10` during Pass 11, making Runner's jump 60× weaker than
intended ever since Pass 11 merged. Baseline confirmed before any edit: commit `8410b18` (tag
`refactor-pre-input-simulation-separation`), `make mingw` 0 errors/145 warnings, all 10 existing
local targets passing. Full evidence in `docs/input-simulation-separation-map.md`.

### [2026-07-19] Map input/simulation separation
- Phase: Pass 12 (commit `497b8c1`)
- File(s): new `docs/input-simulation-separation-map.md`
- Action: confirmed the exact gap left by Pass 11 (discrete jump-trigger handling still coupled to
  the real-frame rate via `processEvents`/`processEvents2`, called once per frame after the
  fixed-tick loop has already run 0–5 times); found and documented the Runner jump-impulse
  regression (bare `-10`, never converted, confirmed by direct read of both the constant's own
  doc-comment and the actual call sites); designed the `InputState` struct and edge-triggered
  consumption approach; listed everything deliberately deferred (collision/death-trigger staleness,
  shooting/projectile input, menu/pause/cheat-code handling).
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete the file.

### [2026-07-19] Add InputState and jump-request consumption
- Phase: Pass 12 (commit `c44f4c3`)
- File(s): `inc/header.h`, `src/process.c`, `src/frame.c`
- Action: added `InputState` (`jumpRequestedPlayer1`/`jumpRequestedPlayer2`, `GameState` field
  `input`), following Phase 61's `AppContext`/`AssetLifecycleFlags` nested-struct pattern; new
  `consume_arcade_jump_requests`/`consume_runner_jump_requests` (`src/process.c`), wired into
  `arcade_simulate`/`runner_simulate` (`src/frame.c`) immediately before `apply_*_player_forces`,
  preserving the original relative ordering (a keydown-triggered jump could stack with jump-hold
  thrust in the same real frame before; preserved by running consumption first).
- Behavior impact: none yet — no producer sets the new flags in this commit.
- Verification performed: `make mingw` — 0 errors, 145 warnings (unchanged). All 10 local targets
  pass.
- Rollback: `git revert`.

### [2026-07-19] Route jump input through InputState
- Phase: Pass 12 (commit `8173289`)
- File(s): `src/processEvents.c`, `src/loadGame.c`
- Action: `GAME_COMMAND_JUMP_PLAYER1`/`_PLAYER2` case bodies in both
  `processEvents()`/`processEvents2()` shrink to a single flag-set — the `onLedge` check, `dy`
  assignment, and SFX playback move to the consumption functions added in the previous commit,
  which now run at the fixed physics tick rate. This is where the Runner jump-impulse regression
  found during this pass's audit is fixed: both modes now route through the same
  `consume_*_jump_requests` functions, which use `JUMP_SPEED_PER_SEC` uniformly. Also clears the
  new request flags in `arcade_session_reset`/`runner_session_reset`, preventing a request left
  pending from a previous session from firing an unwanted instant jump at the start of the next.
- Behavior impact: Arcade jump feel unchanged (impulse was already correct); Runner jump height
  corrected (previously 60× too weak). Everything else unchanged.
- Verification performed: `make mingw` — 0 errors, 145 warnings (unchanged). All 10 local targets
  pass.
- Rollback: `git revert 8173289`.

### [2026-07-19] Validate input/simulation separation
- Phase: Pass 12 (commit `c4432f4`)
- File(s): new `docs/verification/input_state_test.c`, `Makefile` (new `mingw-inputtest` target),
  `.github/workflows/mingw-validation.yml` (step + log-upload path added in the same commit)
- Action: (1) edge-triggered single consumption — a grounded request fires the jump exactly once
  and clears itself, a second consume call in the same real frame does not re-fire even if
  re-grounded in between; (2) an airborne request is dropped, not buffered; (3) Runner's jump
  impulse uses `JUMP_SPEED_PER_SEC`, not the never-converted bare `-10` (direct regression-fix
  proof); (4) the player-2 path for both modes; (5) one real `arcade_simulate`/`runner_simulate`
  call confirms the consumption wires correctly into the existing fixed-tick order.
- Verification performed: `make mingw` — 0 errors, 145 warnings. All 11 local targets pass (14/14
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert c4432f4`.

## Pass 13 summary — collision correctness, player-only (2026-07-20)

Implements the physics assessment's own "Phase 3 — correct collisions: previous-position
tracking, reset grounded each step, standardize hitboxes, fix Runner's ceiling `onLedge` bug" —
already the designated home for the Runner bug per `docs/physics-timestep-map.md`,
`docs/refactor-plan.md`, and `docs/input-simulation-separation-map.md`, all three written in
earlier passes. Found a real bug broader than the already-known Runner ceiling issue during this
pass's own audit: `onLedge` was never reset at the start of a collision pass in either mode, for
either player, so walking off a ledge's edge (no ceiling hit, no jump) never cleared it. Baseline
confirmed before any edit: commit `c7fb663` (tag `refactor-pre-collision-correctness`), `make
mingw` 0 errors/145 warnings, all 11 existing local targets passing. Full evidence in
`docs/collision-correctness-map.md`.

### [2026-07-20] Map collision correctness
- Phase: Pass 13 (commit `5b16114`)
- File(s): new `docs/collision-correctness-map.md`
- Action: confirmed `onLedge` is never reset at the start of a collision pass in either
  `collisionDetect()`/`collisionDetect2()`, for either player — a walk-off-ledge bug distinct from
  and broader than the already-documented Runner ceiling bug. Found the landmine a naive "reset
  each tick" would hit: the landing check's strict inequality can't re-fire while a player rests
  exactly on a surface, so resetting `onLedge` without also fixing the check would immediately
  un-ground anyone standing still. Designed the fix (a crossing-based landing test using a new
  `Man.prevY` field). Scoped hitbox standardization narrowly — named the shared `48x48` literal
  without reconciling it against other scattered hit-test/render sizes.
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete the file.

### [2026-07-20] Add previous-Y tracking and ledge hitbox constants
- Phase: Pass 13 (commit `eade7b8`)
- File(s): `inc/header.h`, `src/collisionDetect.c`
- Action: added `Man.prevY` and `capture_player_prev_y()` (not yet called from anywhere);
  added `PLAYER_LEDGE_HITBOX_W`/`_H` (`48.0f`, not yet used at any check site).
- Behavior impact: none yet.
- Verification performed: `make mingw` — 0 errors, 145 warnings (unchanged). All 11 local targets
  pass.
- Rollback: `git revert eade7b8`.

### [2026-07-20] Correct player ledge collision
- Phase: Pass 13 (commit `83ff619`)
- File(s): `inc/header.h`, `src/collisionDetect.c`, `src/frame.c`
- Action: wired `capture_player_prev_y()` into `arcade_simulate()`/`runner_simulate()` as the
  first line of each. At all 5 player ledge-collision blocks (Arcade man, Arcade secondPlayer ×2,
  Runner man, Runner secondPlayer): `onLedge` resets to `0` once per collision pass (before the
  100-ledge loop, not inside it); the landing check became crossing-based; the bare `48, 48`
  literals became named constants. Runner's two ceiling branches (man, secondPlayer) had their
  `onLedge = 1` corrected to `onLedge = 0`, matching Arcade — the long-documented bug.
  Enemy/boss/smart-enemy ledge collision and the enemy-to-enemy self-collision loop are untouched.
- Behavior impact: walking off a ledge now correctly leaves the player airborne in both modes
  (previously they'd stay "grounded" indefinitely); Runner's head-bump now correctly leaves the
  player airborne instead of instantly re-grounding them mid-air.
- Verification performed: `make mingw` — 0 errors, 145 warnings (unchanged). All 11 local targets
  pass.
- Rollback: `git revert 83ff619`.

### [2026-07-20] Validate collision correctness
- Phase: Pass 13 (commit `5da8602`)
- File(s): new `docs/verification/collision_correctness_test.c`, `Makefile` (new
  `mingw-collisiontest` target), `.github/workflows/mingw-validation.yml` (step + log-upload path
  added in the same commit), `src/collisionDetect.c` (see below), `docs/collision-correctness-map.md`
- Action: (1) walking off a ledge clears `onLedge`; (2) resting continuously across multiple
  consecutive `collisionDetect`/`collisionDetect2` calls re-affirms `onLedge=1`; (3) normal
  falling-and-landing still works; (4) Runner's ceiling bump clears `onLedge` for both players
  (regression-fix proof); (5) `capture_player_prev_y` sets `prevY` correctly. Caught a second
  landmine while writing test 4, not assumed correct beforehand: the crossing-based landing check
  (as landed in the previous commit) had no upper bound, so it fired a second time in the same
  loop iteration immediately after a ceiling correction (which sets `my=by+bh, dy=0` — both
  satisfying the landing condition as first written), undoing the correction that had just run.
  The original strict `my < by` had prevented this implicitly; fixed by adding `my < by + bh` back
  to all 5 landing checks in the same commit as the test that caught it.
- Verification performed: `make mingw` — 0 errors, 145 warnings. All 12 local targets pass (12/12
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert 5da8602`.

## Pass 14 summary — projectile correctness, Arcade-only (2026-07-20)

Implements the physics assessment's own "Phase 4 — correct projectiles: move bullets once per
step, swept collision, projectile pool." Corrects the magnitude of the already-documented bullet
triple-movement bug: the real per-tick multiplier is `101+10+2=113` (once per
enemy/smart-enemy/boss collision-loop iteration), not "3" — prior docs named the loop kinds
correctly but never counted total executions. Converts bullets from individually-`malloc`'d
pointer arrays to a fixed value-array pool, fixes the movement bug with a speed-preserving
constant (rather than silently accepting a ~113× slowdown), and adds swept collision using a new
`prevX` field mirroring Phase 13's own `prevY` design. Baseline confirmed before any edit: commit
`c9ff171` (tag `refactor-pre-projectile-correctness`), `make mingw` 0 errors/145 warnings, all 12
existing local targets passing. Full evidence in `docs/projectile-correctness-map.md`.

### [2026-07-20] Map projectile correctness
- Phase: Pass 14 (commit `76e7ed8`)
- File(s): new `docs/projectile-correctness-map.md`
- Action: corrected the bullet triple-movement bug's magnitude (113×/tick, not 3×) with an exact
  derivation; derived the legacy steady-state speed (`0.1 × 113 = 11.3 px/tick`) to preserve;
  traced `unvisible`'s full lifecycle and confirmed it's redundant with an explicit `active` flag;
  designed the pool conversion, the `BULLET_SPEED_PER_TICK` constant, and the `prevX`-based swept
  collision test.
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete the file.

### [2026-07-20] Convert bullets to a fixed pool
- Phase: Pass 14 (commit `49c6dac`)
- File(s): `inc/header.h`, `src/process.c`, `src/doRender.c`, `docs/verification/lifecycle_test.c`
- Action: `Bullet` gains `prevX`/`active`, loses `unvisible`; `bullets`/`secondBullets` change from
  `Bullet*[MAX_BULLETS]` to `Bullet[MAX_BULLETS]`. `addBullet`/`addSecondBullet` claim the first
  inactive slot in place — no `malloc`; `removeBullet`/`removeSecondBullet` set `active = false`
  — no `free`. The existing `arcade_session_reset`/`app_shutdown` cleanup loops need no changes —
  the new body is a drop-in replacement for the old one. Storage representation only: bullets
  still move up to 113× per tick in this commit, documented plainly as an intentional
  intermediate step, not an oversight. Updates `lifecycle_test.c`'s two `bullets[0]` assertions
  for the new value-type comparison — the only existing test touching bullets.
- Behavior impact: none beyond the storage representation.
- Verification performed: `make mingw` — 0 errors, 145 warnings (unchanged). All 12 local targets
  pass.
- Rollback: `git revert 49c6dac`.

### [2026-07-20] Move bullets once per step with swept collision
- Phase: Pass 14 (commits `d413fb2`, `ee07fd0`)
- File(s): `inc/header.h`, `src/process.c`, `src/frame.c`
- Action: new `move_arcade_bullets`, called from `arcade_simulate` before `process` — the only
  place bullets now move or get clamped. The 6 collision loops (3 single-player + 3 multiplayer
  mirror) drop their movement/clamp/off-screen-despawn code; their point-based hit test becomes a
  segment-vs-rect swept test using the bullet's `prevX`. Hit-response logic unchanged.
- Error caught and fixed before the test was written (commit `ee07fd0`, immediately after
  `d413fb2`): the first version reused the legacy clamp's *shape* (a max-clamp), which only
  shrinks `dx` when already larger than the bound — correct for the old bound (`0.1`, smaller
  than spawn `dx=±3`) but silently inert for the new bound (`11.3`, larger than spawn `dx`), which
  would have left bullets moving at the spawn speed instead of the intended speed-preserving one.
  Fixed by normalizing `dx` to `±BULLET_SPEED_PER_TICK` by sign instead of clamping a magnitude.
- Warning count: 145 → 121 (a decrease, not a suppression). Removing the 12 bare-double-literal
  clamp blocks (`0.1`/`-0.1` against `float` fields, 2 warnings each = 24) in favor of the
  float-suffixed `BULLET_SPEED_PER_TICK` eliminates the `-Wdouble-promotion`/`-Wfloat-conversion`
  pairs those triggered; the new code introduces no new warnings.
- Verification performed: `make mingw` — 0 errors, 121 warnings. All 12 local targets pass.
- Rollback: `git revert ee07fd0 d413fb2`.

### [2026-07-20] Validate projectile correctness
- Phase: Pass 14 (commit `e12884e`)
- File(s): new `docs/verification/projectile_correctness_test.c`, `Makefile` (new
  `mingw-projectiletest` target), `.github/workflows/mingw-validation.yml` (step + log-upload path
  added in the same commit)
- Action: (1) pool spawn/despawn — `addBullet` claims the first inactive slot with `prevX`
  initialized to the spawn position, `removeBullet` deactivates without disturbing other slots;
  (2) move-once-per-step — one `move_arcade_bullets` call moves a bullet by exactly
  `BULLET_SPEED_PER_TICK` in the spawned direction, a second call moves it the same fixed distance
  again; (3) swept collision — a bullet whose `prevX → x` span crosses a target's rect registers
  a hit even though its end-of-tick position alone would miss under the old point-only test;
  (4) an end-to-end `process()` regression check confirms a bullet crossing an enemy via the real
  pipeline still kills it and increments `kills_score`.
- Verification performed: `make mingw` — 0 errors, 121 warnings. All 13 local targets pass (15/15
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert e12884e`.

## Pass 15 summary — game feel (2026-07-21)

Implements the physics assessment's own "Phase 5 — improve game feel: coyote time, jump buffering,
variable jump height." Confirms none of the three exist today: jump is gated strictly on
`onLedge`, and the existing "variable jump height" mechanism is a continuous, ungated, uncapped
hold-thrust rather than the standard impulse-plus-release-cut technique, with a real quirk (holding
the key while grounded adds thrust regardless of `onLedge`). Per user approval, replaces the
hold-thrust entirely rather than layering a release-cut on top of it. Baseline confirmed before any
edit: commit `308b83b` (tag `refactor-pre-game-feel`), `make mingw` 0 errors/121 warnings, all 13
existing local targets passing. Full evidence in `docs/game-feel-map.md`.

### [2026-07-21] Map game feel
- Phase: Pass 15 (commit `f0207c5`)
- File(s): new `docs/game-feel-map.md`
- Action: confirmed no coyote time or jump buffering exist today, and the existing jump-hold-thrust
  mechanic's real quirk (unconditional per-tick `dy` modification, no `onLedge` gate, no clamp).
  Confirmed no held-to-released transition detection exists anywhere. Documented the user-approved
  design decision (replace, not layer) and exactly which `input_state_test.c` invariant this pass
  deliberately supersedes.
- Behavior impact: none (documentation only).
- Verification performed: manual re-derivation of every claim directly from the current tree.
- Rollback: delete the file.

### [2026-07-21] Add coyote time and jump buffering
- Phase: Pass 15 (commit `b3831ed`)
- File(s): `inc/header.h`, `src/process.c`, `src/processEvents.c`, `src/loadGame.c`,
  `docs/verification/input_state_test.c`
- Action: `InputState`'s `bool jumpRequestedPlayer1/2` retype to `int jumpBufferTicksPlayer1/2`;
  `Man` gains `coyoteTicksRemaining`/`jumpKeyHeldLastTick`, both explicitly reset in session-reset.
  `consume_arcade_jump_requests`/`consume_runner_jump_requests` refresh/decay each player's coyote
  counter every tick and gate jump-firing on `onLedge || coyoteTicksRemaining > 0`; a buffered
  request that can't fire yet decrements instead of clearing. Deliberately supersedes
  `input_state_test.c`'s "airborne request dropped, not buffered" assertions — rewritten to assert
  the new buffering/coyote behavior, documented plainly.
- Behavior impact: jump now succeeds within a short grace window after leaving a ledge (coyote
  time) and a jump pressed slightly before landing now fires on landing instead of being dropped
  (buffering).
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged). All 13 local targets
  pass.
- Rollback: `git revert b3831ed`.

### [2026-07-21] Replace jump-hold thrust with variable jump height
- Phase: Pass 15 (commit `22398c0`)
- File(s): `inc/header.h`, `src/process.c`
- Action: removed `ARCADE_JUMP_HOLD_ACCEL_PER_SEC2`/`RUNNER_JUMP_HOLD_ACCEL_PER_SEC2` (confirmed no
  other reference sites) and the continuous thrust they drove, from all 4
  `apply_arcade_player_forces`/`apply_runner_player_forces` W/UP-held blocks. Replaced with
  impulse-plus-release-cut: releasing the jump key while still rising fast
  (`dy < -JUMP_CUT_SPEED_PER_SEC`) caps the ascent via the new `jumpKeyHeldLastTick` field; holding
  through to apex lets gravity decay `dy` naturally. Removes the grounded-hold-thrust quirk as a
  direct consequence — no unconditional per-tick `dy` modification survives.
- Behavior impact: jump height is now determined by whether the key is released early, not by how
  long it's held after the initial impulse; holding the key while grounded no longer adds thrust.
- Verification performed: `make mingw` — 0 errors, 121 warnings (verified unchanged by diffing
  old vs. new `process.c` warnings line-by-line, not assumed). All 13 local targets pass.
- Rollback: `git revert 22398c0`.

### [2026-07-21] Validate game feel
- Phase: Pass 15 (commit `a734b64`)
- File(s): new `docs/verification/game_feel_test.c`, `Makefile` (new `mingw-gamefeeltest` target),
  `.github/workflows/mingw-validation.yml` (step + log-upload path added in the same commit),
  `src/process.c` (see below), `docs/game-feel-map.md`
- Action: (1) coyote time decaying naturally over `COYOTE_TICKS` real consume calls; (2) jump
  buffering on the Runner side; (3) variable jump height (release-cut, no-cut-past-threshold,
  no-cut-without-a-release-edge); (4) confirms the grounded-hold-thrust quirk is gone; (5) the
  player-2 path, both modes. Caught a real off-by-one while writing this test, not by it:
  `consume_arcade_jump_requests`/`consume_runner_jump_requests` ran the coyote refresh/decay
  *before* the jump-fire check, incorrectly denying a jump on the coyote window's last valid tick
  (the decrement consumed it before the check ever saw it). Fixed by reordering both functions in
  the same commit as the test that caught it.
- Verification performed: `make mingw` — 0 errors, 121 warnings. All 14 local targets pass (14/14
  new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert a734b64`.

## Pass 16 summary — optimization (2026-07-21)

Baseline: commit `7ed5259` (tag `refactor-pre-optimization`, `main` after Phase 15's PR #11
merge), `make mingw` 0 errors/121 warnings, all 14 existing local targets passing. Implements the
physics assessment's own last phase, "optimization" — unlike Phases 1-5, this one has no
demonstrated bug: whether any loop is actually slow depends on real gameplay load this environment
has no display to measure, so this pass adds a measurement tool and a static complexity audit
rather than speculative loop changes. Full evidence in `docs/optimization-map.md`, written before
any Pass 16 code edit. Two commits: `f102955` (audit doc) and `ab0b7af` (opt-in performance
logging), plus this documentation commit.

### [2026-07-21] Map optimization opportunities
- Phase: Pass 16 (commit `f102955`)
- File(s): new `docs/optimization-map.md`
- Action: confirmed no profiling/timing infrastructure exists anywhere beyond the raw
  `SDL_GetPerformanceCounter` calls the Phase 11 accumulator already uses (and discards). Computed
  the real, static iteration counts for every hot loop from the actual array-size constants: the
  bullet-vs-target collision loops in `process()` are the largest by an order of magnitude
  (113,000 checks/tick single-player, 226,000 multiplayer), followed by the ledge/self-collision
  loops in `collisionDetect()` (~11,600/tick), with the render loops walking the full 1000-slot
  bullet array and 100-slot ledge array every real frame. Documents explicitly why no loop is
  restructured based on this alone — no real evidence any of them is an actual bottleneck, and
  guessing would repeat the premature-optimization mistake the assessment's own framing warns
  against.
- Behavior impact: none (documentation only).
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged, no code touched). All
  14 local targets pass. `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: delete `docs/optimization-map.md`.

### [2026-07-21] Add opt-in performance logging
- Phase: Pass 16 (commit `ab0b7af`)
- File(s): `inc/header.h` (new `PERF_LOG_INTERVAL_SEC` constant), `src/main.c`
- Action: `main()` gains timing instrumentation around the fixed-tick physics loop and
  `doRender()`/`doRender2()` in both gameplay scenes, active only when the `ENDGAME_PERF_LOG`
  environment variable is set. Once per real second, prints physics ticks, average physics-tick
  time, and average render time (milliseconds) via a new `[perf] ticks=... physics_ms=...
  render_ms=...` line. Every measurement is purely additive — no existing line (the `while`
  condition, accumulator arithmetic, `arcade_simulate`/`runner_simulate`/`doRender`/`doRender2`
  calls themselves) changes. Caught before any commit, not by a test: the first-draft
  `render_ms` formula was nonsensical (divided by `perfLogTimer` instead of counting render calls);
  fixed by introducing `perfRenderCallsThisSecond` before this was ever committed.
- Behavior impact: none when disabled (the default) — only added cost is one `getenv` call at
  startup and unentered branches. When enabled, prints real timing data to the console.
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged; zero warnings
  attributed to `main.c`). All 14 local targets pass. No new `docs/verification/*.c` test —
  `main()` is structurally excluded from every existing test binary (`MINGW_SRCS_NO_MAIN`), and the
  change has no observable gameplay behavior to assert against when disabled; a real run with
  `ENDGAME_PERF_LOG=1` set would require a display not available here — stated plainly, not
  claimed. `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert ab0b7af`.

## Pass 17 summary — input snapshot isolation (2026-07-21)

Baseline: commit `b94e363` (tag `refactor-pre-input-snapshot`, `main` after Phase 16's PR #12
merge), `make mingw` 0 errors/121 warnings, all 14 existing local targets passing. Implements the
first of four phases split out from the user's proposed target frame architecture (`poll events →
build input snapshot → fixed-step loop → render`): 17 input snapshot isolation (this pass), 18
AI/forces separation, 19 collision ordering, 20 render interpolation. Full evidence in
`docs/input-snapshot-architecture-map.md`, written before any Pass 17 code edit. Four commits:
`1e2f85c` (audit doc), `eeb2923` (capture helpers), `814cee8` (cutover), `59fb5f9` (test + CI),
plus this documentation commit.

### [2026-07-21] Map input snapshot architecture
- Phase: Pass 17 (commit `1e2f85c`)
- File(s): new `docs/input-snapshot-architecture-map.md`
- Action: confirmed `apply_arcade_player_forces`/`apply_runner_player_forces` (`src/process.c`)
  each call `SDL_GetKeyboardState()` directly, inside the fixed-step path — a real frame producing
  multiple physics ticks re-samples live keyboard state a non-deterministic number of times.
  Confirmed jump was already correctly isolated via Phase 12/15's edge-triggered `InputState`
  buffer fields, needing no change. Documents five simplifications adopted over the originally
  proposed capture API, each backed by evidence for why the simpler shape already satisfies every
  stated invariant.
- Behavior impact: none (documentation only).
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged, no code touched). All
  14 local targets pass. `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: delete `docs/input-snapshot-architecture-map.md`.

### [2026-07-21] Add input snapshot capture helpers
- Phase: Pass 17 (commit `eeb2923`)
- File(s): `inc/header.h` (expanded `InputState`), new `inc/input_snapshot.h`, new
  `src/input_snapshot.c`, new `docs/verification/header_only_input_snapshot.c`, `Makefile`
  (`mingw-headertest` target)
- Action: expanded `InputState` with 6 continuous held-key fields (movement/jump-held/shoot-held
  for both players), alongside the existing edge-triggered jump-buffer ints. New
  `input_capture_arcade()`/`input_capture_runner()` are pure functions reading a caller-supplied
  `SDL_GetKeyboardState()` array into `InputState` — not yet wired into gameplay in this commit.
- Behavior impact: none (nothing new is called yet; all 14 existing targets pass unchanged).
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged, no warnings from the
  new file). All 14 local targets pass. `py -3 scripts/audit_repository_usage.py` —
  `Result: PASS`.
- Rollback: `git revert eeb2923`.

### [2026-07-21] Route player input through the snapshot
- Phase: Pass 17 (commit `814cee8`)
- File(s): `src/main.c`, `src/process.c`, `src/processEvents.c`, `src/loadGame.c`,
  `docs/verification/game_feel_test.c`
- Action: `main()` calls `SDL_GetKeyboardState()` exactly once per real frame, before the
  fixed-step accumulator loop, and calls `input_capture_arcade()`/`input_capture_runner()` to
  populate `gameState->input`. `apply_arcade_player_forces`/`apply_runner_player_forces` and
  `processEvents()`'s shoot check read `game->input.*` instead of calling
  `SDL_GetKeyboardState()` themselves. `arcade_session_reset()`/`runner_session_reset()`
  explicitly zero the 6 new fields alongside the existing jump-buffer resets. Updated
  `game_feel_test.c`'s header comment (no assertion/call-site change needed, since
  session-reset's explicit zeroing already gives every call the "no key held" state the test
  relies on).
- Behavior impact: preserving — same branch structure, same left-wins precedence, same
  variable-jump-height release-cut logic, same shoot cooldown, just reading a frame-frozen
  snapshot instead of live SDL state.
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged). All 14 local targets
  pass, including the updated `game_feel_test.c`. `py -3 scripts/audit_repository_usage.py` —
  `Result: PASS`.
- Rollback: `git revert 814cee8`.

### [2026-07-21] Validate input snapshot isolation
- Phase: Pass 17 (commit `59fb5f9`)
- File(s): new `docs/verification/input_snapshot_test.c`, `Makefile` (new
  `mingw-inputsnapshottest` target), `.github/workflows/mingw-validation.yml` (step +
  log-upload path added in the same commit)
- Action: verifies (1) `input_capture_arcade`/`input_capture_runner` set exactly the right fields
  from a fabricated key array, including that `input_capture_runner` leaves the shoot fields
  untouched; (2) a simulated multi-tick real frame applies held movement on every tick but the
  release-cut only once; (3) the pre-existing left-wins precedence on simultaneous left+right
  holds, now explicit and tested; (4) player 1/2 fields don't cross-mutate; (5) session reset
  clears every new field, both modes.
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged). All 15 local targets
  pass (20/20 new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert 59fb5f9`.

## Pass 18 summary — AI/forces separation (2026-07-21)

Baseline: commit `8258ba0` (tag `refactor-pre-ai-forces-separation`, `main` after Phase 17's
PR #13 merge), `make mingw` 0 errors/121 warnings, all 15 existing local targets passing.
Implements the second of four phases split out from the user's proposed target frame
architecture (17 input snapshot isolation, 18 AI/forces separation [this pass], 19 collision
ordering, 20 render interpolation) — a structural extraction only, no behavior change. Full
evidence in `docs/ai-forces-separation-map.md`, written before any Pass 18 code edit. Three
commits: `0fdd6cf` (audit doc), `4f3d5cf` (extraction), `77a4e1c` (test + CI), plus this
documentation commit.

### [2026-07-21] Map AI/forces separation
- Phase: Pass 18 (commit `0fdd6cf`)
- File(s): new `docs/ai-forces-separation-map.md`
- Action: confirmed boss/regular-enemy/smart-enemy movement in `process()` integrates position
  using the velocity decided on a previous tick, then adjusts velocity for the next tick — decide
  and integrate are interleaved by design, ruling out a literal 4-function decide/apply/integrate
  split without changing enemy speeds. Confirmed `process2()` (Runner) has zero AI-driven
  entities. Documents the extraction design and the one piece safely extractable as a true
  "decide intent" step (the multiplayer smart-enemy nearest-target comparison).
- Behavior impact: none (documentation only).
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged, no code touched). All
  15 local targets pass. `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: delete `docs/ai-forces-separation-map.md`.

### [2026-07-21] Extract boss/enemy/smart-enemy movement into src/ai_forces.c
- Phase: Pass 18 (commit `4f3d5cf`)
- File(s): new `inc/ai_forces.h`, new `src/ai_forces.c`, `src/process.c`, new
  `docs/verification/header_only_ai_forces.c`, `Makefile` (`mingw-headertest` target)
- Action: moved boss/regular-enemy/smart-enemy movement (previously `process.c:636-923`) verbatim
  into `move_boss_entities()`, `move_regular_enemies()`, `move_smart_enemies()` — same statement
  order and arithmetic. `process()` now calls the three functions at the exact same point. Added
  `smart_enemy_select_target()`, replicating the exact `pow()`-based nearest-target comparison
  the multiplayer smart-enemy chase used to do inline. The confirmed pre-existing multiplayer
  chase asymmetry (its inner `while` loop omits an `x += dx` line the single-player version has)
  is preserved exactly, not fixed.
- Behavior impact: none — a structural extraction only.
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged; confirmed the ~26
  warnings that moved from `process.c` to `ai_forces.c` are the same lines relocated, not new
  ones). All 15 local targets pass, including `projectile_correctness_test.c` (calls `process()`
  directly with enemy data) unchanged. `py -3 scripts/audit_repository_usage.py` —
  `Result: PASS`.
- Rollback: `git revert 4f3d5cf`.

### [2026-07-21] Validate AI/forces separation
- Phase: Pass 18 (commit `77a4e1c`)
- File(s): new `docs/verification/ai_forces_test.c`, `Makefile` (new `mingw-aiforcestest`
  target), `.github/workflows/mingw-validation.yml` (step + log-upload path added in the same
  commit)
- Action: the first-ever test assertions in this repository on the actual AI movement math —
  boss/regular-enemy band-steering (both directions), gravity clamps, and horizontal wraparound;
  smart-enemy single-player hop-impulse-and-steer chase; `smart_enemy_select_target()` picking
  the nearer of two players in multiplayer and always `man` in single-player; and that
  `process()` still delegates to the extracted functions correctly. Assertions use the exact same
  float literals/expressions the production code uses, so comparisons are bit-exact.
- Verification performed: `make mingw` — 0 errors, 121 warnings (unchanged). All 16 local targets
  pass (15/15 new assertions pass). `py -3 scripts/audit_repository_usage.py` — `Result: PASS`.
- Rollback: `git revert 77a4e1c`.
