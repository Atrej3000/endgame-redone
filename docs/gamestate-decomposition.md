# GameState Decomposition Audit — Ucode_Endgame

Written **before** any Phase 10 code edit, per that phase's own requirement. Evidence gathered
directly from the tree at commit `81dda1e` (tag `refactor-pre-gamestate-decomposition`, main
after Phase 9's PR #5 merge) via three parallel codebase explorations plus targeted direct reads
immediately before this document was written.

> **Current-status addendum (pre-Phase 35, 2026-07-23):** inventory counts
> and migration decisions below are the historical Phase 10 evidence. The
> Runner leaderboard is now centrally owned by `leaderboard_*`, maintained as
> a descending top-25 table, and atomically persisted in SDL's per-user
> preference directory. No code serializes the raw `GameState` layout.

## 1. Memory-layout safety (must be established before any field moves)

Searched the entire repository (`src/*.c`, `inc/*.h`, `docs/verification/*.c`) for every operation
that could depend on `GameState`'s layout:

| Category | Result |
|---|---|
| `sizeof(GameState)` / `sizeof(*game)` | **Exactly one site**: `src/app.c:128`, `calloc(1, sizeof(GameState))` inside `app_init()`. A single top-level allocation; does not depend on field order. |
| `memcpy`/`memset` on `GameState*` or a field | **Zero matches.** |
| Pointer casts of `GameState*` | **One benign match**: `src/app.c:128`'s `(GameState *)calloc(...)`. `docs/verification/smoke_init_shutdown.c` casts `game` to `(void*)` only for `printf("%p", ...)` diagnostics — not byte reinterpretation. |
| Pointer arithmetic on `GameState*` | **Zero matches** — no array-of-`GameState`, no `game + 1`-style arithmetic; always a single heap instance passed by pointer. |
| `offsetof(` | **Zero matches** anywhere in the repository. |
| File I/O writing/reading `GameState` or fields as raw bytes | **Zero raw-layout serialization.** At Phase 10 there was no live leaderboard I/O. Current settings, display, and leaderboard persistence serialize validated named values through atomic text-file helpers; none reads or writes `sizeof(GameState)` or relies on field offsets. |
| Whole-struct `memcmp` in tests | **Zero matches.** The only `memcmp` calls in `docs/verification/` (`entity_spawn_test.c:58,75,94`) each compare a single `Enemies` sub-struct (`sizeof(Enemies)`), never `sizeof(GameState)`. |
| External tooling (`.gdbinit`, disassembly notes, offset references) | **Zero matches** anywhere in `docs/`. |

**Conclusion**: nested structs can be introduced inside `GameState` with no risk to any existing
memory-layout assumption. The single `calloc(1, sizeof(GameState))` zero-initializes the whole
struct regardless of internal nesting, and nothing else in the codebase depends on field order or
byte offsets. `GameState` already contains multiple nested structs as direct members today
(`Man`, `Enemies`, `Train`, `Cloud1`..`Cloud8`, `Star`, `Ledge`, `Bullet*`) — this phase's new
nested structs are not a new pattern for the type system, only new *for GameState's own
platform/lifecycle fields specifically*.

## 2. Full field inventory (`inc/header.h:189-312`, 405-line file)

| Field | Responsibility | Owner | Initialized | Reset | Destroyed | Candidate group |
|---|---|---|---|---|---|---|
| `scrollX` | Runner world-scroll offset | Runner simulation (`process.c`) | `calloc` (0) | `process2()`/`runner_session_reset` | n/a | Runner session (deferred) |
| `multiPlayer` | Single/multiplayer mode selection | `arcade_session_reset`/`runner_session_reset` | `calloc` (0 = single-player) | same | n/a | Session/mode (deferred, shared) |
| `gameLives` | Runner shared life pool | Runner simulation/events | `calloc` (0) | `runner_session_reset` | n/a | Runner session (deferred) |
| `sumScore` | Unclear — **no write site found anywhere** (confirmed again this phase) | none | `calloc` (0) | never | n/a | Dead field candidate — **not moved, not removed** (out of this phase's charter) |
| `tempScore` | Arcade wave-trigger counter | `process.c` (Arcade) | `calloc` (0) | `arcade_session_reset` | n/a | Arcade session (deferred) |
| `shotCount`, `shotCountMultiplayer` | Arcade shoot rate-limiter | `processEvents.c` | `calloc` (0) | `arcade_session_reset` | n/a | Arcade session (deferred) |
| `secondPlayer`, `man` (`Man`) | Both players' physics/animation state | `process.c`/`processEvents.c`/session-reset | `calloc` (zeroed `Man`) | `arcade_session_reset`/`runner_session_reset` | textures freed in `app_shutdown`/asset unloads | Shared player session (deferred, large) |
| `enemy` (`Enemies`) | Template enemy texture holder | `loadGame.c` (asset load) | `calloc` | asset unload | asset unload | Arcade assets (deferred) |
| `boss[2]`, `enemyValues[101]`, `smartEnemies[10]` | Arcade entity arrays | `process.c`/`entity_spawn.c`/`loadGame.c` | `calloc` (zeroed) | `arcade_session_reset` + wave triggers | n/a | Arcade session (deferred, large) |
| `train`, `cloud1`..`cloud8` | Arcade background decor position/texture | `loadGame.c`/`process.c` | `calloc` | `arcade_session_reset` | asset unload | Arcade assets (deferred) |
| `bullets[1000]`, `secondBullets[1000]` (`Bullet*`) | Shared bullet pool (both modes) | `process.c` (`addBullet`/`removeBullet`) | `calloc` (all NULL) | `arcade_session_reset` (via `removeBullet` loop) | `app_shutdown` (via `removeBullet` loop) | Shared session (deferred) |
| `stars[100]` (`Star`) | Runner-only hazards | `runner_session_reset`/`processEvents2` | `calloc` | `runner_session_reset` | n/a | Runner session (deferred) |
| `ledges[100]` (`Ledge`) | Shared platform layout, populated differently per mode | `arcade_session_reset`/`runner_session_reset` | `calloc` | both reset functions | n/a | Shared session (deferred) |
| `CurrentSheetBullet`, `CurrentSheetBullet2`, `CurrentSheetBoss` | Animation-frame indices | `process.c` | `calloc` (0) | not explicitly reset | n/a | Arcade session (deferred) |
| `star`, `manFrames[12]`, `secondPlayerFrames[12]`, `brick`, `menu0/1/2`, `mult`, `leaders`, `fon`, `pause`, `label`, `labelMultiplayer`, `death`, `brick_block`, `copper_block`, `bulletTexture`, `secondBulletTexture`, `bossTexture` (`SDL_Texture*`) | Shared/Arcade/Runner textures — **already informally grouped** by `shared_assets_load`/`arcade_assets_load`/`runner_assets_load` in `loadGame.c` | `loadGame.c` | `calloc` (all NULL) | n/a (load-once) | matching `*_assets_unload` | Shared/Arcade/Runner assets (deferred — informally grouped already via load/unload functions and the 3 boolean flags, but not yet as nested structs; a natural next-phase candidate) |
| `labelW`, `labelH` | HUD label texture dimensions | `kills_score.c`/`X_score.c` | `calloc` (0) | recomputed on redraw | n/a | HUD (deferred) |
| `CurrentSpriteBack`, `CurrentSpriteBack2`, `CurrentSpriteBack3` | Background animation state | `process.c` | `calloc` (0) | not explicitly reset | n/a | Arcade assets (deferred) |
| `sheetTextureBack`, `sheetTextureBack2`, `sheetTextureSun` | Arcade background textures | `loadGame.c` | `calloc` (NULL) | n/a | `arcade_assets_unload` | Arcade assets (deferred) |
| `font` | Shared HUD font | `loadGame.c` (`shared_assets_load`) | `calloc` (NULL) | n/a | `shared_assets_unload` | Shared assets (deferred) |
| `menuMus`, `battleMus`, `runnerMus` (`Mix_Music*`) | Background music per screen | `load_menu.c`/`loadGame.c` | `calloc` (NULL) | freed on scene-exit in event handlers | `app_shutdown` | Audio (deferred — **lifecycle not yet centralized**, see §3) |
| `jumpSound`, `kickSound`, `select`, `shootSound`, `damageSound` (`Mix_Chunk*`) | SFX, lazily loaded at 9 scattered sites | 4 different files | `calloc` (NULL) | freed on scene-exit in event handlers | `app_shutdown` | Audio (deferred — same reason) |
| `renderer` | SDL renderer handle | `app.c` (`app_init`/`app_shutdown`) | set once in `app_init` | never mid-run | `app_shutdown` | **Selected: `AppContext`** |
| `arcadeAssetsLoaded`, `runnerAssetsLoaded`, `sharedAssetsLoaded` | Asset-group load state flags | `loadGame.c`'s 3 load + 3 unload functions | `calloc` (false) | matching `*_assets_unload` | matching `*_assets_unload` | **Selected: `AssetLifecycleFlags`** |
| `scene` | Authoritative scene routing | `scene.c` (`app_change_scene`, sole writer) + 1 documented bootstrap in `main.c` | `calloc` (0 = `APP_SCENE_MAIN_MENU`) | n/a (transitions, not reset) | n/a | **Selected: `AppContext`** |
| `menu_status`, `menu0_status` | **DEPRECATED** — superseded by `scene`, no longer read/written anywhere in active routing (confirmed again this phase) | none (dead) | `calloc` (0) | never | n/a | Dead — flagged safe-to-remove in Phase 7, still not removed (out of this phase's charter) |
| `x_score`, `x_list[25]`, `x_i` | Runner score plus persistent descending top-25 leaderboard | gameplay events + `leaderboard_*` | `calloc`, then `leaderboard_load` from the per-user preference file | `x_score` on new Runner session; table only on load/recovery/record | atomically saved text; no dynamic destruction | Leaderboard state (grouping deferred) |
| `kills_score`, `kills_score_multi` | Arcade score | `process.c`/`loadGame.c` | `calloc` (0) | `arcade_session_reset` | n/a | Arcade session (deferred) |
| `time`, `deathCountdown` | Shared per-frame timers | `process.c`/`process2.c`/`runner_death.c` | `calloc` (0 / uninitialized meaning) | `arcade_session_reset` sets `time=0`, `deathCountdown=-1` | n/a | Session (deferred) |
| `statusState` | Secondary crude state machine (`STATUS_STATE_LIVES`/`STATUS_STATE_GAME`) | `loadGame.c`/`status.c` | `calloc` (0) | `arcade_session_reset` | n/a | HUD/menu state (deferred) |
| `iter` | Set in `runner_session_reset`; broader usage unconfirmed this pass | `loadGame.c` | `calloc` (0) | `runner_session_reset` | n/a | Runner session (deferred) |

Every field's address is taken only in the ordinary sense of C struct member access
(`&game->field` for output parameters like `load_texture(..., &game->mult)`) — no field's address
is stored long-term, compared, or used for arithmetic. No field is directly accessed by any test
via anything other than the same `game->field` syntax production code uses (confirmed in §3/§4 of
the accompanying exploration — tests use `game->scene`, `game->arcadeAssetsLoaded`, etc.,
identically to production code, so the same mechanical rename applies to both).

## 3. Group candidates considered and why exactly two were selected

- **`AppContext { renderer; scene; }`** — both fields already exist on `GameState` (no new field
  introduced); both represent platform/application-level state, not gameplay state; both have a
  single clear production owner (`app.c` for `renderer`, `scene.c` for `scene`); `window` is
  deliberately excluded — it is **not** a `GameState` field at all (confirmed: `app_init`'s
  signature takes `SDL_Window **outWindow` as a separate out-parameter, never copied onto
  `GameState`), and adding it now would mean introducing a brand-new field, not migrating an
  existing one — out of scope for a layout-preserving, behavior-preserving phase. Combining
  `renderer` with `scene` also avoids the single-field-nested-struct rule that isolating `scene`
  alone would violate.
- **`AssetLifecycleFlags { arcadeAssetsLoaded; runnerAssetsLoaded; sharedAssetsLoaded; }`** — the
  single cleanest candidate found: write surface fully contained to 3 loader + 3 unloader
  functions in one file (`loadGame.c`), read only there plus `scene.c`'s 2 entry hooks, and
  **zero test-direct-writes** (every test site is a pure read/assertion — unlike `scene`, which
  has 17 test-only direct-write preconditions, each self-documented as an intentional exception).
- **Audio fields — explicitly rejected as a candidate this phase.** Phase 9's own audit
  (`docs/solid-gof-audit.md` §5.5/§9) already found this group's lifecycle is *not* centralized —
  9 scattered ad-hoc lazy-reload sites across 4 files (`pause_events.c`, `menu_events.c` x2,
  `process.c` x2, `processEvents.c` x3/x3), each with its own `if (field == NULL) field =
  Mix_LoadWAV(...)` guard, freed inconsistently on scene-exit in event handlers rather than
  through a single owning pair of functions. This is precisely the complexity this phase's own
  rule says to avoid ("prefer fields whose lifecycle is already centralized"). Grouping these
  fields into a struct today would not simplify anything — the scattered load/free call sites
  would remain scattered, just addressing a nested field instead of a flat one. Unifying the
  lazy-reload pattern first (a separate, already-identified deferred item) is a prerequisite to
  ever grouping this field set usefully.
- **Arcade/Runner session data (entities, score, timers) — explicitly rejected as a first choice**,
  per the task's own explicit instruction not to select these merely because they are large. Real
  candidate clusters exist (see the table above), but each spans many files with both read and
  write access from simulation, collision, and event-handling code — a much larger, higher-risk
  migration than either selected group. Recorded as deferred, ready to be picked up as its own
  phase once `AppContext`/`AssetLifecycleFlags` have proven the migration pattern.
- **Leaderboard/HUD/menu state — grouping deferred.** This was lower priority
  than the two selected Phase 10 groups. The pre-Phase-35 audit has since
  centralized leaderboard load/record/save/sort/recovery behavior in
  `leaderboard_*`; moving the remaining fields into a nested struct is still a
  separate structural change.

## 4. Access-site migration plan (exact counts, verified by grep before any edit)

**`scene`**: 3 production writes (`scene.c:44`, `scene.c:49`, `main.c:29`), 8 production reads
(`main.c:31,32`, `menu_events.c:16`, `pause_events.c:23`, `processEvents.c:512,522,786`,
`scene.c:48`), plus 17 test-only writes and 30 test-only reads across `frame_pipeline_test.c`,
`lifecycle_test.c`, `runner_death_test.c`, `scene_transition_test.c` — all self-documented
preconditions, migrated mechanically alongside production code.

**`renderer`**: 2 production writes (`app.c:174` set, `app.c:95` null), and read at every
`load_texture(game->renderer, ...)` call site — concentrated in `src/loadGame.c` (~45 sites) and
`src/load_menu.c` (3 sites), plus one local-alias-then-use pattern (`SDL_Renderer *renderer =
game->renderer;`) in `src/kills_score.c`, `src/X_score.c`, `src/x_list_leader.c`, `src/status.c`,
`src/draw_lifes.c` (1 each). All are pure mechanical renames (`game->renderer` →
`game->app.renderer`), zero logic change.

**`arcadeAssetsLoaded`/`runnerAssetsLoaded`/`sharedAssetsLoaded`**: 6 production writes total (one
set + one clear per flag, all in `src/loadGame.c`), 3 production reads (one per flag's own loader,
plus `scene.c`'s 2 entry-hook reads for the mode-specific flags), and test-only reads in
`lifecycle_test.c`, `scene_transition_test.c`, `smoke_init_shutdown.c` (26 sites, all reads, zero
test writes).

## 5. `header.h` duplicate declarations found (beyond the documented `app_change_scene` case)

- `app_change_scene`: declared in both `header.h:355` and `scene.h:14` (deliberate, documented in
  Phase 9). 3 callers (`menu_events.c`, `pause_events.c`, `processEvents.c`) still rely on the
  `header.h` copy rather than including `scene.h` directly.
- **`doRender`**: declared **twice, verbatim, inside `header.h` itself** — `header.h:336` and
  `header.h:345`, both `void doRender(SDL_Renderer *renderer, GameState *game);`. This is an
  accidental, pre-existing duplicate (not part of any deliberate migration), harmless to remove
  in a single one-line edit within `header.h` — no header split or caller migration needed.
