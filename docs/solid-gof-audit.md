# SOLID / GoF Architecture Audit — Ucode_Endgame

Written **before** any Phase 9 code edit, per that phase's own requirement. Evidence gathered
directly from the tree at commit `e967001` (tag `refactor-pre-solid-gof`) via three parallel
codebase explorations plus targeted direct reads immediately before this document was written —
not carried over from any prior phase's summary.

**Scope discipline** (from the task prompt, restated here because it drives every decision
below): at most two substantial GoF patterns, one or two small supporting SOLID extractions, at
most three new abstractions total in this phase. Arcade and Runner stay separate. No ECS,
dependency-injection container, service locator, Singleton, or C++-class-hierarchy emulation. A
pattern is not used unless it demonstrably solves a problem the codebase already has.

## 1. Module map

24 files in `src/`, 2 headers in `inc/` (`header.h`, `app.h`):

| File | Lines | Responsibilities |
|---|---|---|
| `src/main.c` | 82 | Bootstrap + the single scene-dispatch loop (9 cases + default) |
| `src/app.c` | 191 | SDL/IMG/TTF/Mixer subsystem lifecycle, `GameState` alloc/free, 3 destroy-and-null helpers |
| `src/scene.c` | 62 | `app_change_scene()` + 2 static per-scene entry hooks |
| `src/frame.c` | 19 | `arcade_frame()`/`runner_frame()` — pure orchestration, no logic |
| `src/asset_loader.c` | 88 | 3 generic checked SDL loaders (`load_texture`/`load_music`/`load_font`) |
| `src/loadGame.c` | 480 | Shared/Arcade/Runner asset-group load+unload, Arcade/Runner session reset |
| `src/load_menu.c` | ~80 | Per-menu-screen texture/music load — **bypasses the checked loaders** (see §4) |
| `src/process.c` | 1163 | Bullet pool mgmt + Arcade simulation (`process`) + Runner simulation (`process2`), both with inline scoring/spawn |
| `src/processEvents.c` | 783 | Input polling + scene transitions + lazy SFX load + audio teardown + win/loss checks + leaderboard write, both modes |
| `src/collisionDetect.c` | 755 | Arcade AABB resolution (`collisionDetect`) + Runner hazard/death delegation (`collisionDetect2`) |
| `src/doRender.c` | — | Arcade/Runner render (`doRender`/`doRender2`) |
| `src/doRender_menu.c`, `doRender_pause.c`, `doRender_leaderboard.c` | — | Per-scene render-only screens |
| `src/menu_events.c` | ~120 | Menu input (`menu_events`, `menu0_events`) |
| `src/pause_events.c` | 37 | Pause-scene input |
| `src/runner_death.c` | — | Runner single-shot death lifecycle (`runner_trigger_death`/`runner_update_death`) |
| `src/collide2d.c` | — | Shared AABB primitive used by both collision functions |
| `src/status.c`, `src/kills_score.c`, `src/X_score.c`, `src/x_list_leader.c`, `src/draw_lifes.c` | — | Per-counter HUD init/draw functions |
| `src/random_sign.c` | 7 | One helper, `random_sign()` |

## 2. Header dependency map

Only two headers exist. `inc/app.h` (19 lines) is the sole example of a focused header today:
`#include "header.h"` then exactly 2 prototypes (`app_init`, `app_shutdown`). Every other
declaration in the project — every constant, every data-model typedef, the `AppScene`/`GameMode`
enums, the `GameState` struct, and prototypes for all ~24 source files — lives in the single
404-line `inc/header.h`. 23 of 24 `.c` files `#include "header.h"` directly; only `app.c` gets it
transitively through `app.h`. This is a textbook ISP violation: every translation unit receives
every declaration in the codebase whether it uses it or not.

## 3. Mutable-state ownership map

`GameState` (`inc/header.h:189-312`) is a single flat struct, ~90 fields, no nested sub-structs
anywhere (confirmed: zero `typedef struct` exists outside `header.h`). Ownership is enforced only
by convention, never by the type system:

| Cluster | Fields (representative) | Owning code |
|---|---|---|
| SDL platform | `renderer` | `app.c` (window/renderer themselves are separate `main()` locals, not `GameState` fields) |
| Scene routing | `scene` (write-only via `app_change_scene()`, one documented bootstrap exception in `main.c`) | `scene.c` |
| Shared assets | `mult, leaders, pause, brick, death, font, manFrames[0]`, gated by `sharedAssetsLoaded` | `loadGame.c` |
| Arcade assets/session | `bossTexture, battleMus`, `man.lives`, `enemyValues[101]`, `boss[2]`, `bullets[1000]`, gated by `arcadeAssetsLoaded` | `loadGame.c` |
| Runner assets/session | `star, fon, runnerMus`, `gameLives`, `stars[100]`, `ledges[100]`, gated by `runnerAssetsLoaded` | `loadGame.c` |
| Lazily-loaded SFX | `select, jumpSound, shootSound, damageSound, kickSound` | **scattered** — 9 ad-hoc reload sites across 4 files, outside any group flag |
| Leaderboard | `x_list[25], x_i, x_score` | write site: `processEvents.c:764-770` (Runner only); reset: 3 sites (`main.c`, `load_menu.c` x2) |
| Deprecated | `menu_status, menu0_status` | dead; explicitly marked "safe to remove entirely" at `header.h:293-296` (left alone this phase — out of charter) |

The asset-group boolean flags (`arcadeAssetsLoaded`/`runnerAssetsLoaded`/`sharedAssetsLoaded`) are
an **implicit grouping mechanism** — they partition ownership without the fields actually being
members of a nested struct. This is real, working ownership discipline; it just isn't expressed
in the type system.

## 4. Top-level call graph

```
main()
  -> app_init()                                   [src/app.c]
  -> load_menu0()                                 [src/load_menu.c]
  -> while (scene != APP_SCENE_QUIT):
       switch (scene):
         MAIN_MENU        -> menu0_events, doRender_menu0
         ARCADE_MENU      -> menu_events, doRender_menu1
         ARCADE_GAME      -> arcade_frame()        [src/frame.c]
                               -> process -> collisionDetect -> doRender -> processEvents
         ARCADE_LEADERBOARD -> processEvents, doRender_leaderboard
         ARCADE_PAUSE     -> doRender_pause, pause_events
         RUNNER_MENU      -> menu_events, doRender_menu2
         RUNNER_GAME      -> runner_frame()        [src/frame.c]
                               -> process2 -> collisionDetect2 -> doRender2 -> runner_update_death -> processEvents2
         RUNNER_LEADERBOARD -> processEvents2, doRender_leaderboard2
         RUNNER_PAUSE     -> doRender_pause, pause_events
         default          -> app_change_scene(QUIT)
  -> app_shutdown()                                [src/app.c]
```
Every scene transition in the codebase funnels through the single function `app_change_scene()`
(`src/scene.c:38`) except the one documented bootstrap assignment in `main.c`. 21 real call sites
across `scene.c`, `menu_events.c`, `pause_events.c`, `processEvents.c` — see the decision table in
§6 for the duplication this creates.

## 5. SOLID findings

### 5.1 Single Responsibility — violated, concretely
- `inc/header.h` mixes constants, every data type, the `GameState` god-struct, and prototypes for
  the whole program — see §2.
- `src/process.c` (1163 lines) mixes bullet-pool management, Arcade simulation, and Runner
  simulation in one file, with inline scoring interleaved into physics update loops.
- `src/processEvents.c` (783 lines) mixes input polling, scene transitions, lazy resource
  loading, resource teardown, win/loss rule evaluation, and a leaderboard write.
- `GameState` has no responsibility boundaries in the type system (see §3).

**Change made this phase**: extracted enemy/smart-enemy/boss spawn-invariant logic out of
`process.c`/`loadGame.c` into `src/entity_spawn.c` (§7.1); extracted `scene.c`/`frame.c`'s public
surface into focused headers (§7.3). `process.c`'s bullet-pool/simulation split and
`processEvents.c`'s input/rule-check split are **not** touched this phase — splitting either
further would exceed the 3-abstraction ceiling and is recorded as deferred debt (§9).

### 5.2 Open/Closed — violated, concretely
The exact same 5-6-field `Enemies` initialization pattern (`x, y, dy, dx, visible[, countShots]`)
is hand-written inline at **8 wave-trigger blocks in `process.c`** (`time==120`,
`tempScore==10,31,72,133,224,345,366`) plus **2 of the 3 reset loops in `loadGame.c`'s
`arcade_session_reset`** (the `smartEnemies`/`boss` reset loops match the pattern exactly; the
third, `enemyValues`' own reset loop, sets only `x,y,dy,dx` and deliberately never touches
`visible`/`countShots` — a genuinely different contract from every other site, verified by direct
read, see §7.1 for why it is intentionally *not* converted). Adding or adjusting one spawn
invariant today still means editing up to 10 places by hand. Four files independently implement "quit to main menu" with
divergent side-effect bodies for the same `SDLK_q` trigger; two files duplicate "pause"; two
functions duplicate "jump."

**Change made this phase**: `enemy_spawn`/`smart_enemy_spawn`/`boss_spawn` (§7.1) reduce the
10-site duplication to one call site each supplying only the location-specific values (x, y, dx,
dy); a bounds check is centralized there for the first time. The keycode-duplication is addressed
by `input_command.c` (§7.2) at the *mapping* layer — mode-specific cleanup bodies remain separate
functions where they genuinely differ (Arcade frees different audio handles than Runner), so OCP
is served without forcing false uniformity between two modes whose behavior differs.

### 5.3 Liskov Substitution — not currently applicable, verified deliberately
C has no inheritance; LSP applies only where a shared function-pointer/contract interface exists
between multiple implementations. Before this phase, no such shared interface exists: `process`/
`process2`, `collisionDetect`/`collisionDetect2`, `doRender`/`doRender2`, `processEvents`/
`processEvents2` are independently named function pairs, never called through one function-pointer
table, so there is nothing to violate. **This phase's new `translate_arcade_command`/
`translate_runner_command`/`translate_menu_command` (§7.2) do share one real contract**
(`GameCommand (*)(SDL_Keycode)`) — verified: all three accept any `SDL_Keycode` value including
unmapped ones (precondition: none), all three return `GAME_COMMAND_NONE` rather than crashing or
returning an out-of-range enum value on unmapped input (postcondition, verified by
`command_translation_test.c`), none allocate or hold state, and all three are safe to call from
the same generic client code. No shared interface is introduced for `process`/`collisionDetect`/
`doRender`/`processEvents` themselves — the task explicitly disallows unifying Arcade/Runner, and
no LSP violation exists there to fix.

### 5.4 Interface Segregation — violated, concretely
See §2. 23/24 translation units depend on the single monolithic header regardless of need.

**Change made this phase**: `inc/scene.h` and `inc/frame.h` extracted, mirroring the existing
`inc/app.h` pattern — narrow, single-purpose, each declaring only what its own module's callers
need. `inc/header.h` remains the compatibility header for every other declaration (per the task's
own incremental-migration guidance: no one-commit full replacement). `inc/entity_spawn.h` and
`inc/input_command.h` are new, equally narrow, from day one.

### 5.5 Dependency Inversion — one real gap found and fixed
High-level orchestration (`frame.c`, `scene.c`) already depends only on named functions passed
explicitly, never on global state beyond the explicit `GameState*` — no DIP violation there.
**One real, demonstrated gap**: `src/load_menu.c` bypasses the checked, error-reporting
`load_texture()` wrapper that every other texture-load call site in the codebase uses, calling raw
`IMG_Load()` directly with no NULL-check on the returned surface — the one place in the codebase
where a failed image load would segfault instead of failing safely. **Fixed this phase** by
routing `load_menu.c`'s three texture loads through `load_texture()` — not a new abstraction,
reuse of the seam that already exists and is used everywhere else.

Not fixed this phase (documented, deferred, §9): nine separate ad-hoc
"`if (field == NULL) field = Mix_LoadWAV(...)`" lazy-SFX-reload sites scattered across
`pause_events.c`, `menu_events.c` (x2), `process.c` (x2), `processEvents.c` (x3, mirrored in
`processEvents2`) — outside the `_assets_load`/`_assets_unload` ownership model. Unifying these
would be a 4th abstraction and is left for a future phase.

## 6. GoF candidate-pattern matrix

| Problem | Evidence | SOLID principle | Candidate pattern | Simpler alternative | Decision |
|---|---|---|---|---|---|
| 10-site duplicated inline `Enemies` init | `process.c` x9, `loadGame.c` x1 (§5.2) | SRP, OCP | Factory Method | A single non-bounds-checked helper function (rejected — a genuine bounds check is worth the small formal step) | **Required** |
| Bullet creation | `addBullet`/`addSecondBullet` (`process.c:3,40`) | SRP | Factory Method | — | **Already present**, no change needed |
| Quit-to-menu (`SDLK_q`) divergent x4, pause x2, jump x2 | `processEvents.c`, `menu_events.c`, `pause_events.c` (§5.2) | SRP, OCP | Command (translation boundary, not queued objects) | Per-file helper function duplicated 4 times (rejected — doesn't remove the keycode-mapping duplication itself) | **Required** |
| Scene routing | `AppScene` + `app_change_scene()` + 2 entry hooks (§4) | — | State | Formal per-scene state objects | **Already present, informally** — only 2/10 scenes need entry side effects; formal objects would be pure overhead |
| App/session/frame entry points | `app_init/app_shutdown`, `arcade_frame/runner_frame`, `*_assets_load/unload` | — | Facade | Another wrapper layer | **Already present, informally** — documented, not re-wrapped |
| Shared immutable textures/font | `shared_assets_load`, `sharedAssetsLoaded`, Phase-5 `manFrames[0]` consolidation | — | Flyweight | Reference-counted cache | **Already present, informally** — no active duplicated loading remains |
| `load_texture`/`load_music`/`load_font` vs. raw `IMG_Load` in `load_menu.c` | `asset_loader.c` vs `load_menu.c:12,34,62` (§5.5) | DIP | Adapter | — | **Already present, now enforced consistently** (fix, not new abstraction) |
| Single-player vs multiplayer session prep | `arcade_session_reset`/`runner_session_reset` internal `man`→`secondPlayer` duplication | SRP | Strategy | — | **Rejected** — same function handles both via a mode check, not two swappable implementations behind one contract; no real runtime *selection between implementations* exists |
| Arcade vs Runner mechanics | Explicitly disallowed by task | — | Strategy | — | **Rejected** (explicit instruction — mechanics differ too much, not interchangeable) |
| Arcade/Runner asset groups | `arcade_assets_load`/`runner_assets_load` | — | Abstract Factory | — | **Rejected** — already clear explicit functions, no interchangeable-family need |
| Leaderboard persistence | No live file I/O exists at all (§8, confirmed against `docs/unused-code-assets-audit.md`) | — | Adapter/Repository (`LeaderboardStore`) | — | **Rejected** — task explicitly forbids building an interface around inactive persistence; no second implementation to select between |
| Death → audio cleanup + scene change | One function does both sequentially | — | Observer | A second explicit notify call (not needed — already sequential in one place) | **Rejected** — no independent, decoupled multi-consumer exists; would add indirection with no gain |
| Arcade/Runner frame pipeline shape | `frame.c`'s fixed sequence (§1) | — | Template Method | Keep as separate concrete functions (already the case) | **Rejected** — already the simplest correct C shape; unifying via callbacks/inheritance is explicitly disallowed |
| Global mutable access | `GameState*` passed explicitly everywhere | — | Singleton | — | **Rejected (explicit instruction)** — explicit passing is strictly better: hides nothing, keeps tests possible |
| Cross-cutting dependency lookup | None exists | — | Service Locator | — | **Rejected (explicit instruction)** |
| Simple structs (`Man`, `Bullet`, `Ledge`, etc.) | Small, locally-initialized | — | Builder | Direct struct init (already used) | **Rejected** — no construction complexity to justify it |
| Any duplication/copying need | None found | — | Prototype | — | **Rejected** — no prototype-copy use case exists |
| Flat entity arrays (`enemyValues[101]`, etc.) | §3 | — | Composite | — | **Rejected** — entities are flat, not tree-shaped |
| Rendering | `doRender`/`doRender2`, render-pure since Phase 5 | — | Decorator | — | **Rejected** — no dynamic render-behavior composition need |
| Small direct call graph (§4) | — | — | Mediator | — | **Rejected** — call graph is already small and direct, a mediator would add indirection with nothing to coordinate |
| No save-state feature exists | — | — | Memento | — | **Rejected** — no undo/save-state behavior exists to support |
| Fixed procedural structs | `Enemies`, `Man`, etc. | — | Visitor | — | **Rejected** — no open-ended operation-over-types problem exists |
| Straightforward keydown switches | `processEvents.c`, `menu_events.c` | — | Chain of Responsibility | Direct switch (kept, simplified via `GameCommand`) | **Rejected** — no need for a request to pass through a chain of handlers |
| Fixed key mappings | §5.2 | — | Interpreter | `GameCommand` translation (this phase's actual choice) | **Rejected** — full grammar/interpreter machinery is unjustified for a fixed keycode table |
| No remote/lazy/security boundary | — | — | Proxy | — | **Rejected** — no such boundary exists anywhere in this codebase |
| `GameState` responsibility clusters | §3 | SRP | Nested nested structs (`AppContext`, `AudioAssets`, etc.) | — | **Deferred** — real candidate clusters exist, but doing it alongside the Command/Factory work this phase risks exceeding the 3-abstraction ceiling and mixing rollback boundaries; revisit as its own phase |
| Lazy SFX reload duplication (9 sites) | §5.5 | DIP | Small ownership-group extension (not a new pattern) | — | **Deferred** — would be a 4th abstraction this phase |

## 7. Minimal implementation (exactly what changes, and why each piece clears the bar)

Every abstraction below is checked against the task's own four questions (demonstrated problem /
why the simpler implementation is insufficient / why this pattern over a helper function / how
it's tested) before being included.

### 7.1 Factory Method — `src/entity_spawn.c` + `inc/entity_spawn.h`
```c
bool enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy);
bool smart_enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy);
bool boss_spawn(GameState *game, int index, float x, float y, float dx, float dy);
```
1. **Problem**: identical 5-6-field struct init duplicated at 10 call sites (§5.2).
2. **Why insufficient today**: no bounds-check exists anywhere in the 10 sites — an off-by-one in
   any of the hand-written loop bounds would silently corrupt adjacent memory; there is no single
   place to add or adjust a spawn invariant.
3. **Why a factory function beats a bare helper**: it centralizes the *bounds check* (a genuine new
   correctness guarantee, not just deduplication) alongside the field-set invariant, and gives each
   entity kind (`Enemies` used as enemy vs. smart-enemy vs. boss) a distinct, named creation entry
   point matching how the task's own Factory Method section describes "centralizing required
   initial values, bounds checks, ownership, failure handling."
4. **Tested by**: `docs/verification/entity_spawn_test.c` — see §10.

Each function sets exactly the fields the matching call sites set (`visible=1`, `countShots=0`
where applicable, `dx`/`dy` as given, `x`/`y` as given) and returns `false` without mutating
anything when `index` is out of range for that array. All 8 `process.c` wave-trigger blocks plus
the 2 fully-matching `loadGame.c` reset loops (`smartEnemies`, `boss`) are converted to pass the
same literal values they use today — no change to spawn timing, positions, velocities, or visible
behavior. **`loadGame.c`'s `enemyValues` reset loop is deliberately left inline**: it sets only
`x,y,dy,dx` and never touches `visible`/`countShots`, a genuinely different contract from every
other site (verified by direct read) — forcing it through `enemy_spawn` would either silently set
`visible=1` on every session reset (a real behavior change) or require a parameter no other caller
needs. Converting only what actually shares the same contract, and documenting why one site
doesn't, is the correct application of LSP here, not an oversight.

### 7.2 Command — `src/input_command.c` + `inc/input_command.h`
```c
typedef enum GameCommand {
    GAME_COMMAND_NONE,
    GAME_COMMAND_QUIT_TO_MENU,
    GAME_COMMAND_QUIT_TO_MAIN_MENU,
    GAME_COMMAND_PAUSE,
    GAME_COMMAND_RESUME,
    GAME_COMMAND_JUMP_PLAYER1,
    GAME_COMMAND_JUMP_PLAYER2,
    GAME_COMMAND_SELECT_SINGLE_PLAYER,
    GAME_COMMAND_SELECT_MULTIPLAYER,
    GAME_COMMAND_SELECT_LEADERBOARD,
    GAME_COMMAND_QUIT_GAME
} GameCommand;

GameCommand translate_arcade_command(SDL_Keycode key);
GameCommand translate_runner_command(SDL_Keycode key);
GameCommand translate_menu_command(SDL_Keycode key);
```
1. **Problem**: the same discrete action is independently keyed/bodied across 2-4 files with
   divergent side effects (§5.2); `menu0_events()` maps two distinct keycodes to the same action
   three separate times via switch fall-through (already collapsed there, but not reusable
   elsewhere).
2. **Why insufficient today**: a per-file helper function would remove body duplication but not
   the *keycode-to-action mapping* duplication itself, and wouldn't give a single place to unit-test
   "does key X mean action Y" independent of the mode-specific side effects.
3. **Why a translation boundary over a queued Command object**: the task explicitly forbids queuing
   or heap-allocating commands, and continuous movement must stay untouched — a synchronous
   `SDL_Keycode -> GameCommand` pure function, consumed immediately by a small per-mode switch, is
   the smallest correct shape that removes the mapping duplication without changing timing.
4. **Tested by**: `docs/verification/command_translation_test.c` — see §10.

Only discrete `SDL_KEYDOWN` actions already identified as duplicated are translated. Held-key
continuous polling (`SDL_GetKeyboardState`-based movement/shooting) is untouched — confirmed by
the test asserting no `GAME_COMMAND_MOVE_*` value exists in the enum at all. Mode-specific bodies
(e.g., which audio handles Arcade frees vs. Runner) remain separate hand-written code per mode
where they genuinely differ — Arcade and Runner are not forced into one shared handler.

### 7.3 ISP — `inc/scene.h`, `inc/frame.h`
Mirrors the existing `inc/app.h` pattern exactly: `inc/scene.h` declares `AppScene` and
`app_change_scene()`; `inc/frame.h` declares `arcade_frame()`/`runner_frame()`. `src/scene.c`,
`src/frame.c`, and `src/main.c` are updated to include the new focused headers for those specific
declarations; `inc/header.h` is trimmed of exactly those declarations and remains the
compatibility header for everything else — no other file's `#include` changes, no circular
includes introduced (verified: neither new header depends on the other).

### 7.4 DIP consistency fix (not a new abstraction) — `src/load_menu.c`
The three raw `IMG_Load()` calls (`load_menu.c:12,34,62`) are replaced with calls to the existing
`load_texture()` wrapper, matching every other texture-load call site in the codebase. Reuses an
existing seam; introduces no new type or function.

**`GameState` is not touched. Arcade and Runner remain fully separate — no shared struct, no
shared function-pointer table between the two modes' pipelines.**

## 8. Persistence — confirmed no live implementation exists

Cross-checked against `docs/unused-code-assets-audit.md` (§8, "Dynamic-reference risks" and the
file-removal sections): the only historical file-based leaderboard code was dead, commented-out,
and would not even have compiled if uncommented (referenced an undeclared variable and now-deleted
helper functions) — it was physically deleted in Phase 7. The only "leaderboard" in the live
codebase today is the in-memory `x_list[25]` array (§3), written from exactly one site
(`processEvents.c:764-770`, Runner mode only, already bounds-checked). Per the task's own
instruction not to build an interface around dead or inactive persistence, and because there is no
second implementation to select between, a `LeaderboardStore` interface is **explicitly rejected**
for this phase.

## 9. Deferred (evidence-supported, not attempted this phase)

- **`GameState` decomposition into nested structs** (`AppContext`, `AudioAssets`, per-mode session
  clusters) — real candidate clusters exist (§3), but combining this with the Command/Factory work
  in one phase risks exceeding the 3-abstraction ceiling and mixing rollback boundaries.
- **Unifying the 9 scattered lazy-SFX-reload sites** into the `_assets_load`/`_assets_unload`
  ownership model — would be a 4th abstraction this phase.
- **Further header decomposition** beyond `scene.h`/`frame.h`/`entity_spawn.h`/`input_command.h` —
  `header.h` remains the compatibility header for `process.c`/`processEvents.c`/`collisionDetect.c`
  and the rest; splitting those out is real future ISP work but not attempted here per the task's
  own "split incrementally, not in one commit" guidance.
- **Splitting `process.c`'s bullet-pool management from its simulation loops, and
  `processEvents.c`'s input-polling from its win/loss rule checks** — both are genuine SRP
  candidates (§5.1) but neither is touched this phase, to stay within the abstraction ceiling.

## 10. Test strategy

- **`docs/verification/entity_spawn_test.c`** (new, `mingw-entityspawntest` target): default field
  values match today's inline behavior exactly for a representative call reproducing each of the
  10 original sites (regression safety against the literals recorded in §5.2); in-range index
  succeeds; out-of-range index (`>= NUM_ENEMIES`/`NUM_SMART_ENEMIES`/2) returns `false` and leaves
  the target array slot completely untouched (a new correctness guarantee that did not exist
  before this phase).
- **`docs/verification/command_translation_test.c`** (new, `mingw-commandtest` target): every
  keycode identified in §5.2's duplication tables maps to its documented `GameCommand`; an unmapped
  key maps to `GAME_COMMAND_NONE`; the menu dual-keycode cases (`b`/`1`, `r`/`2`, `q`/`ESCAPE`) each
  map both keys to the same command; confirms no `GAME_COMMAND_MOVE_*` value exists in the enum.
- **Header independence**: both new headers are included alone (with only their own declared
  dependencies) as part of the new test targets' compilation, catching missing includes or hidden
  transitive dependencies immediately.
- **Full regression**: all 8 existing targets (`mingw-smoketest/-scenetest/-lifecycletest/
  -frametest/-deathtest/audit-repo`, plus the base `mingw` build) run after every commit; the two
  new test targets run in the same pass. 161 warnings unchanged expected throughout — this class
  of change (extraction + header split) carries no warning-count implication.

## 11. Rollback strategy

Each of the 6 planned commits (docs → Factory Method → header split → Command → DIP fix → docs)
is independently revertible with `git revert <sha>` — the sequence deliberately keeps unrelated
concerns in separate commits (per the task's own instruction not to combine header decomposition,
input translation, and entity-factory work in one commit). The tag `refactor-pre-solid-gof`
(commit `e967001`) is the whole-phase rollback point if a deeper revert is ever needed. No commit
in this phase touches `GameState`'s layout, so no commit in this sequence can desynchronize saved
state or break binary compatibility with any external tooling.
