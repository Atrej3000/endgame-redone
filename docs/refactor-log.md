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
