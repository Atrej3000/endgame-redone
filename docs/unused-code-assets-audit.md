# Unused Code/Asset Audit — Ucode_Endgame

Written **before** any Phase 7 deletion, per that phase's own requirement. Describes exactly what
was searched, what was found, and the confidence level assigned to every candidate, at commit
`94d117b` (tag `refactor-pre-unused-cleanup`). Nothing below is deleted until this document and
the file-level plan are complete; the actual deletion commits follow in sequence after this doc
lands.

## 1. Audit methodology

Three independent research passes, each covering a distinct area, run in parallel:

1. **Source/symbol reachability** — every `.c`/`.h` file under `src/`, `inc/`, `docs/verification/`,
   `.github/`; for every non-`static` function, every call site repo-wide (production code and all
   5 test harnesses); every `header.h` prototype cross-referenced against actual call sites; every
   `GameState`/`Man` struct field's reads/writes; every large commented-out block.
2. **Asset reference-tracing** — every file under `resource/`; every `resource/...`-shaped string
   literal in `src/*.c`; SHA-256 hash of every asset file to detect byte-identical duplicates
   (`sha256sum`, available and used, no fallback needed); Makefile `-framework` flag
   cross-reference; README/docs asset mentions; license/attribution file inventory.
3. **Generated/tracked-artifact audit** — full `git ls-files` review against `.gitignore`; check
   for tracked-then-ignored leakage in `vendor/`/`build-mingw/`; `docs/verification/` file-by-file
   classification (test source vs. screenshot evidence vs. build logs); OS-metadata/backup-file
   sweep; baseline repo size numbers.

Every high-impact finding from all three passes was then **personally re-verified by direct
`grep`/`Read`** before being trusted (not taken as-is from research output) — in particular:
`leader_events`, `doRender_multiplayer`/`doRender_multiplayer2`, `kills_list`, `draw_status_x_list`,
every dead `Man`/`GameState` field listed below, and the exact boundaries of both dead
`processEvents.c` comment blocks. A dedicated validation pass then read every proposed deletion
site directly (not the summaries) specifically hunting for compile breakage or subtle indirect
use, and found **one real gap**: removing the `shooting` field requires also deleting its two
write sites in `loadGame.c`, not just the field declaration — folded into the plan below.

**Tools used**: `Glob`, `Grep` (repo-wide exact and pattern search), `Bash` (`find`, `sha256sum`,
`wc -l`, `stat`), direct file reads of every proposed deletion site. `gcc -Wunused-function
-Wunused-variable` is already part of the standing `MINGW_WARN_FLAGS` build (`-Wall -Wextra`
implies these) — the baseline build already runs with them and reports 167 warnings, none of which
flag any of the items below as compiler-detected-unused (because they're all *externally linked*
and *called from at least one place*, even if that place is itself dead — the classic
"reachable-if-called, but nothing calls it" gap that compiler warnings alone cannot catch and that
this manual audit exists to close). `nm`/`objdump` were not needed — no function-pointer tables,
no macro-renamed calls, and no platform-specific compilation branches exist anywhere in this
codebase (confirmed: `src/*.c` is a flat, non-recursive directory with one universal `#include
"header.h"`, no per-platform `#ifdef`-guarded function *definitions*, only three tiny
`#ifdef _WIN32`/`#ifdef __APPLE__` portability shims inside `header.h` itself).

## 2. Build and test baseline

Confirmed before any deletion:
- `git status --short` — clean.
- `git rev-parse --short HEAD` — `94d117b` (tag `refactor-pass-5-frame-pipeline` and earlier all
  present; `refactor-pre-unused-cleanup` created fresh on this commit as this phase's own
  rollback point).
- `make mingw-clean && make mingw` — **0 errors, 167 warnings**.
- `make mingw-smoketest` — PASS.
- `make mingw-scenetest` — PASS (16 checks).
- `make mingw-lifecycletest` — PASS (55 checks).
- `make mingw-frametest` — PASS (38 checks).
- `make mingw-deathtest` — PASS (39 checks, the newest Phase-6 target).

## 3. Active source graph (summary — full per-function detail in §5/§7 tables)

`src/main.c`'s scene dispatch is the sole root. Confirmed-live call chains from it:
`app_init`/`app_shutdown` (`app.c`) → `arcade_frame`/`runner_frame` (`frame.c`) →
`process`/`process2`, `collisionDetect`/`collisionDetect2`, `doRender`/`doRender2`,
`processEvents`/`processEvents2` → `runner_trigger_death`/`runner_update_death` (`runner_death.c`),
`collide2d`, `addBullet`/`removeBullet`/`addSecondBullet`/`removeSecondBullet`,
`draw_status_lives`/`init_status_lives`/`shutdown_status_lives` (`status.c`),
`draw_status_lives2` (`draw_lifes.c` — confirmed live, not to be confused with the *dead*
`doRender_multiplayer.c`), `init_status_x`/`draw_status_x` (`X_score.c`),
`init_status_x_list` (`x_list_leader.c`), `init_status_kills`/`draw_status_kills`
(`kills_score.c`), `random_sign`. Scene routing: `menu0_events`/`menu_events`/`pause_events`
(return value discarded by `main.c` at both call sites — the function itself is live, its return
value simply isn't consumed; not a deletion candidate, just documented for completeness),
`app_change_scene`/`arcade_menu_enter`/`runner_menu_enter` (`scene.c`),
`load_menu0`/`load_menu1`/`load_menu2` (`load_menu.c`),
`shared_assets_load/unload`/`arcade_assets_load/unload`/`arcade_session_reset`/
`runner_assets_load/unload`/`runner_session_reset` (`loadGame.c`),
`load_texture`/`load_music`/`load_font` (`asset_loader.c` — **not** `load_chunk`, see §5),
`destroy_texture`/`free_chunk`/`free_music` (`app.c`).
`docs/verification/*.c`'s 5 harnesses each call into this same graph directly (no separate code
path) — confirmed each test file's exact coverage in the prior research pass, listed in §7.

## 4. Active asset graph (summary — full per-file table in §5)

Every asset load in this codebase is a **single string literal** passed directly to
`load_texture`/`load_music`/`load_chunk`/`load_font` (`asset_loader.c`) or a raw
`Mix_LoadWAV`/`Mix_LoadMUS`/`TTF_OpenFont` call — confirmed via full read of `asset_loader.c` and
every load call site. **No dynamic path construction exists anywhere** (`sprintf`/`snprintf`/
`strcat`/`mx_strcat`/`mx_strjoin` hits are all unrelated to file paths — score-label text
formatting only). This means static string-literal search is fully dispositive for "is this asset
ever loaded" — there is no runtime-only load path a static grep could miss. 66 of 88 game-asset
files (excluding the 1,205-file `resource/frameworks/` third-party bundle, audited separately) are
referenced by at least one live literal; 22 are not (§5); 3 are referenced only with mismatched
case (§7, kept, documented as a bug, not touched).

## 5. Proposed safe deletions (Proven unused)

### 5.1 Whole source files

| Item | Type | Evidence | Confidence | Risk | Proposed action |
|---|---|---|---|---|---|
| `src/mx_atoi.c` (+`mx_isdigit`/`mx_isspace`) | file, 3 fns | Zero callers outside file; not called by any of 5 test harnesses | Proven unused | None — self-contained | Delete file |
| `src/mx_count_words.c` | file, 1 fn | Only caller is `mx_strsplit` (itself dead) | Proven unused | None | Delete file |
| `src/mx_file_to_str.c` | file, 1 fn | Zero live callers (own demo `main()` commented) | Proven unused | None | Delete file |
| `src/mx_itoa.c` | file, 1 fn | Only apparent call site is inside a `/* */` comment (`processEvents.c:810-822`) | Proven unused | None | Delete file |
| `src/mx_sort_arr_char.c` | file, 1 fn | Zero callers | Proven unused | None | Delete file |
| `src/mx_sort_arr_int.c` | file, 1 fn | Only reference is commented (`doRender_leaderboard.c:19`) | Proven unused | None | Delete file |
| `src/mx_strcat.c` | file, 1 fn | Only caller is `mx_strjoin` (dead) | Proven unused | None | Delete file |
| `src/mx_strcmp.c` | file, 1 fn | Only caller is `mx_sort_arr_char` (dead) | Proven unused | None | Delete file |
| `src/mx_strcpy.c` | file, 1 fn | Only caller is `mx_strdup` (dead) | Proven unused | None | Delete file |
| `src/mx_strdel.c` | file, 1 fn | Zero live callers | Proven unused | None | Delete file |
| `src/mx_strdup.c` | file, 1 fn | Only caller is `mx_strjoin` (dead) | Proven unused | None | Delete file |
| `src/mx_strjoin.c` | file, 1 fn | Zero live callers | Proven unused | None | Delete file |
| `src/mx_strlen.c` | file, 1 fn | Only callers are dead (`mx_strcat`, `mx_strdup`); one apparent site is commented (`processEvents.c:819`) | Proven unused | None | Delete file |
| `src/mx_strncpy.c` | file, 1 fn | Only caller is `mx_strsplit` (dead) | Proven unused | None | Delete file |
| `src/mx_strnew.c` | file, 1 fn | All 4 callers are themselves dead | Proven unused | None | Delete file |
| `src/mx_strsplit.c` | file, 1 fn | Zero live callers (own demo `main()` commented) | Proven unused | None | Delete file |
| `src/leader_events.c` | file, 1 fn | Zero callers anywhere; matches prior `docs/scene-state-map.md` finding, re-verified fresh | Proven unused | None | Delete file |
| `src/doRender_multiplayer.c` | file, 2 fns | Zero callers anywhere for either function | Proven unused | None | Delete file |

**Not deleted from this cluster**: `src/random_sign.c` — confirmed live (`loadGame.c:467,474`,
`processEvents.c:770,777`), despite being grouped under the same "//Leo functions" header comment
as the dead `mx_sort_arr_char`.

### 5.2 Individual dead functions (file otherwise active)

| Item | Type | Evidence | Confidence | Risk | Proposed action |
|---|---|---|---|---|---|
| `shutdown_status_x` (`X_score.c`) | fn | Zero callers; sibling `init_status_x`/`draw_status_x` are live | Proven unused | None — fully self-contained (destroy+null a texture) | Remove function only |
| `shutdown_status_x_list` (`x_list_leader.c`) | fn | Zero callers; sibling `init_status_x_list` is live | Proven unused | None | Remove function only |
| `shutdown_status_kills` (`kills_score.c`) | fn | Zero callers; siblings `init_status_kills`/`draw_status_kills` are live | Proven unused | None | Remove function + its one `header.h:402` prototype |
| `load_chunk` (`asset_loader.c`) | fn | Zero callers — every real SFX load site (8 total, checked) uses raw `Mix_LoadWAV()` with its own equivalent NULL-guard already in place | Proven unused | None — siblings `load_texture`/`load_music`/`load_font` are heavily used, untouched | Remove function only |

### 5.3 Declaration with no definition anywhere

| Item | Type | Evidence | Confidence | Risk | Proposed action |
|---|---|---|---|---|---|
| `void draw_status_x_list(GameState*);` (`header.h:404`) | prototype only | Never defined in any `.c` file; would-be body appears merged into `init_status_x_list()`; only "caller" is a commented-out line (`doRender_leaderboard.c:21`) | Proven unused | None — removing an orphaned prototype cannot break a link that never existed | Remove prototype |

### 5.4 Dead `GameState`/`Man` fields and macros

| Item | Type | Evidence | Confidence | Risk | Proposed action |
|---|---|---|---|---|---|
| `kills_list[25]` | `GameState` field | Zero references outside declaration + doc prose | Proven unused | None (confirmed: only one `sizeof(GameState)` in the project, the single `calloc` in `main.c` — no offset/serialization dependency on field order) | Remove |
| `visible2` | `Man` field | Zero references outside declaration | Proven unused | None | Remove (surgical edit — packed comma-list, see §9) |
| `countWaves` | `GameState` field | Zero references outside declaration | Proven unused | None | Remove |
| `musicChannel` | `GameState` field | Zero references outside declaration | Proven unused | None | Remove |
| `attack` | `Man` field | Only inside `//`-commented lines | Proven unused | None | Remove (packed comma-list) |
| `currentSpriteAttack1`, `currentSpriteSkill` | `Man` fields | Only inside the commented render block `doRender.c:167-172` | Proven unused | None | Remove (packed comma-list) + delete the block |
| `sheetTextureAttack1`, `sheetTextureSkill` | `Man` fields (`SDL_Texture*`) | Never loaded anywhere; only `app.c`'s defensive `destroy_texture()` no-ops (always NULL) + the same commented block | Proven unused | None | Remove (packed comma-list) + remove `app.c:75-76,81-82` |
| `shooting` | `Man` field | Write-only: set `0` in `loadGame.c:181,222` every session reset; zero reads (only commented lines) | Proven unused | **Requires deleting the 2 write sites in `loadGame.c` in the same commit** — validation-pass-caught gap | Remove field + both writes |
| `secondPlayerImage`, `enemyFrame`, `background`, `enemyTexture2` | `GameState` fields (`SDL_Texture*`) | Never assigned by any asset-load function (confirmed: no `load_texture` call targets any of the four); `app.c`'s own existing comment already calls these "never assigned... kept for completeness" | Proven unused | None — remove exactly `app.c` lines 64, 68, 69, 70 (**not** 65-67, the adjacent live `menu0`/`menu1`/`menu2` calls, confirmed live via `load_menu.c` writes + `doRender_menu.c` reads) | Remove fields + their 4 `destroy_texture` calls |
| `#define STATUS_STATE_GAMEOVER 2` | macro | Only inside two `//`-commented lines (`process.c:921,980`) | Proven unused | None — comments aren't preprocessed, removing the macro cannot affect them either way; those two comment lines are left untouched (historical context) | Remove macro only |
| `#define LEADERBOARD_TXT "..."` | macro | Only used inside the dead block `processEvents.c:809-822` being deleted in §5.5 | Proven unused | None (deleted in the same commit as the block that used it) | Remove macro |

### 5.5 Obsolete commented-out implementation

| Item | Type | Evidence | Confidence | Risk | Proposed action |
|---|---|---|---|---|---|
| `processEvents.c:539-559` | commented block (Arcade) | Dead pointer-arithmetic pseudocode referencing an undeclared `leader` variable (wouldn't compile if ever uncommented) + a file-write attempt against `resource/files/leaderboard.txt`; superseded by the live `x_list[25]`-based persistence used elsewhere | Proven unused | None — pure comment, live guard code immediately before/after confirmed intact | Delete block |
| `processEvents.c:809-822` | commented block (Runner) | File-write attempt against `LEADERBOARD_TXT` using the now-deleted `mx_itoa`/`mx_strlen`; superseded by the live `x_list[game->x_i] = game->x_score` line immediately above | Proven unused | None — live score-persist and the Phase-5 double-transition-guard comment/code immediately after confirmed intact | Delete block |

### 5.6 Assets (22 files, ~7.68 MB, zero references)

| Asset | Referenced from | Reference type | Status |
|---|---|---|---|
| `background/Blue.png` (3,703,049 B) | — | none | Unused (part of an unused 8-color palette set) |
| `background/Brown.png` (3,703,049 B) | — | none | Unused |
| `background/Brown_a.png` (98,996 B) | — | none | Unused |
| `background/Green.png` (30,272 B) | — | none | Unused |
| `background/Grey.png` (17,831 B) | — | none | Unused |
| `background/Idle_(32x32).png` (1,563 B) | — | none | Unused (byte-identical to 2 other unused copies, §5.7) |
| `background/Pink.png` (21,783 B) | — | none | Unused |
| `background/Purple.png` (6,030 B) | — | none | Unused |
| `background/Yellow.png` (27,032 B) | — | none | Unused |
| `background/train.png` (2,426 B) | — | none | Unused (superseded by live `sunset_train.bmp`) |
| `background/train_anim.png` (379,353 B) | — | none | Unused |
| `bullets/bullet.png` (3,715 B) | — | none | Unused (superseded by live `bullet_fireball.png`) |
| `bullets/secondbullet.png` (7,380 B) | — | none | Unused (superseded by live `bullet_fireball2.png`) |
| `main_hero/attack1/Attack1_(128x32).png` (4,288 B) | — | none | Unused (tied to dead `sheetTextureAttack1`) |
| `main_hero/idle/Idle_(32x32).png` (1,563 B) | — | none | Unused (tied to dead `sheetTextureIdle` usage — never loaded) |
| `secondplayer/attack1/Attack1_(128x32).png` (4,288 B) | — | none | Unused |
| `secondplayer/death/fire.png` (1,128 B) | — | none | Unused (no per-second-player death field exists) |
| `secondplayer/idle/Idle_(32x32).png` (1,563 B) | — | none | Unused |
| `terrain/mud_block.png` (606 B) | — | none | Unused (a 4th terrain tile, never wired to any load call) |
| `sounds/land.wav` (33,746 B) | — | none | Unused |
| `text/Fonts/leaderboard` (217 B) | `processEvents.c:814` | **comment-only** (inside the block deleted in §5.5) | Unused |
| `files/leaderboard.txt` (13 B) | `processEvents.c:548` | **comment-only** (inside the block deleted in §5.5) | Unused |

**Total: 22 files, 8,049,891 bytes (≈7.68 MB).** None is test-only (confirmed: none of the 5
`docs/verification/*.c` harnesses loads any asset by path at all — they're pure structural/
state-machine tests with zero SDL asset I/O).

### 5.7 Duplicate-hash groups (for context — all qualifying members already listed above)

| Hash group | Files | Active references | Canonical candidate | Action |
|---|---|---|---|---|
| `ded44a6e…` | `background/Idle_(32x32).png`, `main_hero/idle/Idle_(32x32).png`, `secondplayer/idle/Idle_(32x32).png` | **None of the three** | n/a — all unused | Delete all 3 (no consolidation needed since nothing references any of them) |
| `8769da11…` | `main_hero/attack1/Attack1_(128x32).png`, `secondplayer/attack1/Attack1_(128x32).png` | **None** | n/a | Delete both |
| `968d059c…` | `main_hero/death/fire.png` (live), `secondplayer/death/fire.png` (unused) | Only the `main_hero` copy | `main_hero/death/fire.png` | Delete only the `secondplayer` copy (already listed in §5.6) |
| `87542ea1…` | `background/Sunset.png`, `backgrounds/fon.png` | **Both** live (Arcade vs. Runner background, different modes, same bytes) | n/a — both actively used | **Not touched** — per §4.7's own guidance, do not deduplicate actively-referenced files |
| 8 `V_g(rn)N`/`V_g(rn)N+1` pairs (main_hero + secondplayer run-cycle frames) | all 16 files | **All** live (each is a distinct animation frame, coincidentally byte-identical to its neighbor) | n/a | **Not touched** |

No commit for "consolidate byte-identical duplicate assets" is needed — every duplicate group
either has all members already covered by §5.6 (both/all copies unused) or has every member
actively referenced (no consolidation candidate exists).

## 6. Uncertain candidates (Highly likely unused / Unclear — NOT deleted this pass)

| Item | Why uncertain | Required manual check |
|---|---|---|
| `menu_status`/`menu0_status` (`GameState`, deprecated routing ints) | 3 live writes (`load_menu.c`'s `load_menu0/1/2`, zeroing on every menu entry) + 1 live read (`pause_events.c`, discarded by both `main.c` call sites) — fails the "zero active reads AND zero active writes" bar even though the effect is fully inert | Decide whether to also change `pause_events()`'s return type to `void` and delete the 3 write sites + the field, in a dedicated future phase |
| `docs/verification/mingw-smoketest-final.log` | Not byte-identical to any sibling log (`sha256sum`-checked against `run1.log`/`run2-expanded.log`); not cited by filename in any doc (unlike its siblings, which are at least narrated even where not filename-quoted) | Confirm whether it documents a real 3rd run worth citing, or is safe to delete as redundant — recommend just adding the missing doc citation rather than deleting |

## 7. Confirmed active items that initially appeared unused

| Item | Why it looked unused | Why it's actually active |
|---|---|---|
| `menu0`/`menu1`/`menu2` texture fields | Adjacent, in `app.c`'s own comment, to the 4 truly-dead texture fields in §5.4 | Loaded by `load_menu.c` (`load_menu0/1/2`), read by `doRender_menu.c` — genuinely live, just lazily loaded outside `arcade_assets_load`/`runner_assets_load` |
| `draw_status_lives2` (`draw_lifes.c`) | Superficially similar name to the *dead* `doRender_multiplayer`/`doRender_multiplayer2` and to the live `draw_status_lives` (`status.c`) — easy to confuse three similarly-named HUD functions | Called from `doRender.c:194,254` (once per render, both modes) — genuinely live, distinct function |
| `collide2d.c` | One of the four files this phase's own brief flagged as "previously mentioned/suspicious" | Heavily used: 2 call sites in `collisionDetect.c`, 6 in `processEvents.c` — core, active AABB collision logic |
| `pause_events()`'s `int` return value | Discarded at both `main.c` call sites, which superficially looks like a dead code path | The *function* is very much live (called every pause-scene frame); only its *return value* is unconsumed — not a deletion candidate, the field it returns (`menu_status`) is the §6 uncertain item, not the function |

## 8. Dynamic-reference risks (found during audit, explicitly out of this phase's scope)

- **Case-mismatched asset literals — a real, confirmed cross-platform bug, NOT fixed this phase**:
  `src/loadGame.c:116` requests `"./resource/images/background/Sunset_front.png"` but the on-disk
  file is `sunset_front.png` (lowercase); `:118` requests `brick_block.png` but the file is
  `Brick_block.png`; `:119` requests `copper_block.png` but the file is `Copper_block.png`. All
  three currently work only because Windows' filesystem is case-insensitive — every one of them
  would fail `IMG_Load` on case-sensitive macOS/Linux, this project's stated primary target
  (`README.md`). These files ARE referenced (case-insensitively matched), so they are **not**
  deletion candidates — but fixing the casing is a bug fix, a different kind of change than
  removing unused content, and explicitly out of this phase's charter ("do not modify gameplay",
  "do not rename... assets merely for consistency" — renaming here would be a correctness fix, not
  a consistency one, but still a distinct class of change warranting its own reviewed phase).
  Documented here prominently so it isn't lost.
- **`resource/frameworks/` internal duplication — a real, quantifiable size opportunity, NOT acted
  on this phase**: 1,205 tracked file paths resolve to only 182 unique blobs (~63 MB of duplicate
  storage) — confirmed via SHA-256 hashing. This happened because the macOS `.framework` bundles'
  internal symlinks (`Versions/Current` → `Versions/A`, etc.) were flattened into real duplicate
  file copies at some past commit (zero symlinks are tracked anywhere in this repo). Every single
  one of these files IS referenced (the macOS `Makefile`'s `-framework SDL2` etc. flags need the
  whole bundle structure to resolve) — this is a duplicate-*storage* question, not an
  unused-*content* one, and safely restructuring it (e.g., re-vendoring with real symlinks) would
  need real macOS build validation, unavailable in this Windows-only environment. Flagged as a
  future opportunity only.

## 9. Expected impact

- **Source**: 42 → 24 files under `src/` (−18: 16 `mx_*.c` + `leader_events.c` +
  `doRender_multiplayer.c`). Approximate line reduction: 396 (mx_* cluster) + 11 + 17 (the two
  single-purpose files) + ~4+4+6+25 (the four small dead functions) + 35 (two processEvents.c
  blocks) + 6 (doRender.c commented block) + 2 (loadGame.c writes) + 4 (app.c destroy calls) +
  ~20-25 (header.h prototypes/fields/macros) ≈ **530 lines**, out of 4,960 total `src/*.c` lines
  today (~10.7%).
- **Assets**: 88 game-asset files (excluding `resource/frameworks/`) → 66 (−22), 8,049,891 bytes
  (≈7.68 MB) removed.
- **Struct footprint**: `GameState`/`Man` shrink by 13 fields total across the two structs (exact
  removal is a comma-list edit, not a byte-count claim, since C struct padding makes a precise
  `sizeof` reduction implementation-defined and not worth asserting in advance).
- **Warnings**: expected unchanged or reduced (167 baseline) — none of the deleted code currently
  contributes a warning (confirmed: the 167 baseline warnings were already cross-referenced by
  file:line in Phase 2's audit and none fall on any line being touched here), so no reduction is
  actually expected, but the count must not increase.
- No game mode, control, sprite (beyond the confirmed-dead ones being removed), sound, collision,
  or score/leaderboard *behavior* changes — every deletion target has zero live reference.

## 10. Rollback strategy

Tag `refactor-pre-unused-cleanup` marks the exact pre-cleanup state (commit `94d117b`). Each
deletion commit in the sequence below is small and independently revertible via `git revert
<sha>`; the plan's commit order (fields/macros → helper functions → whole files → commented
blocks → assets → build references → integrity script → docs) means an early revert doesn't
require unwinding later, unrelated commits. Full `git reset --hard refactor-pre-unused-cleanup`
remains available as a last resort but is not expected to be needed given the granularity of the
commit plan and the fact every item here was independently verified twice before being listed.

## 11. Post-cleanup results

All cleanup landed exactly as planned in §5; every commit was followed by a full clean rebuild and
all six local test targets (`smoketest`/`scenetest`/`lifecycletest`/`frametest`/`deathtest`, plus
the new `audit-repo` target once it existed) — every run passed, zero failures at any step.

### Deleted source

| File/symbol | Evidence | Validation |
|---|---|---|
| 16 `mx_*.c` files (see §5.1) | Zero live callers anywhere, incl. all 5 test harnesses | Build + all tests pass; 6 pre-existing warnings from inside these files disappeared (167→161), zero new warnings |
| `leader_events.c` | Zero callers | Build + all tests pass |
| `doRender_multiplayer.c` (2 fns) | Zero callers | Build + all tests pass |
| `shutdown_status_x`, `shutdown_status_x_list`, `shutdown_status_kills`, `load_chunk` | Zero callers each | Build + all tests pass |
| `draw_status_x_list` prototype | Never defined anywhere | Build + all tests pass |
| 13 `GameState`/`Man` fields, 2 macros (§5.4) | Zero live reads (or, for `shooting`, zero reads at all) | Build + all tests pass; confirmed no memory-layout dependency (single `calloc` in `main.c`) |
| 2 `processEvents.c` comment blocks (§5.5) | Superseded by live in-memory `x_list` persistence | Build + all tests pass; live guards before/after each block confirmed intact |
| 2 stale Makefile comment lines | Leftover, unrelated to current glob-based `SRCS` | Build unaffected |

### Deleted assets

| Asset | Evidence | Former purpose | Validation |
|---|---|---|---|
| 22 files, ~7.68MB (full list in §5.6) | Zero references via exhaustive string-literal grep (no dynamic path construction exists anywhere in this codebase) | Unused background palette, superseded prototype sprites, dead attack/idle textures, an orphaned terrain tile, an unused sound effect, 2 dead leaderboard data files | All 5 asset-loading test suites (which exercise `arcade_assets_load`/`runner_assets_load` end to end) pass unmodified after deletion, confirming none was ever actually loaded |

### Retained uncertain items

| Item | Why uncertain | Required manual check |
|---|---|---|
| `menu_status`/`menu0_status` | 3 live (if inert) writes + 1 live (if discarded) read — fails the zero-reads-and-zero-writes bar | Future phase: decide whether to also change `pause_events()`'s return type and delete the 3 write sites |
| `docs/verification/mingw-smoketest-final.log` | Not a byte-identical duplicate of any sibling log; not cited by filename in any doc | Confirm whether it documents a real run worth citing, or add the missing citation |

### Repository reduction

| Metric | Before (commit `94d117b`) | After | Change |
|---|---|---|---|
| `src/*.c` files | 42 | 24 | −18 |
| `src/*.c` lines | 4,960 | 4,420 | −540 |
| Game assets (excl. `resource/frameworks/`) | 88 | 66 | −22 |
| Asset bytes removed | — | — | ≈7.68 MB |
| Tracked files (`git ls-files`) | 1,366 | 1,328 | −38 (40 deletions, +2 new files: the audit doc and the integrity script) |
| Total tracked size (`git ls-tree -r -l`) | 89,211,117 B (≈85.08 MB) | 81,179,466 B (≈77.4 MB) | −8,031,651 B (≈7.66 MB) |
| Build warnings | 167 | 161 | −6 (all from inside the deleted `mx_*` files; zero new warnings anywhere) |

No exaggeration: the reduction is modest relative to the repository's total size, since the bulk of
tracked content (`resource/frameworks/`, ~1,205 files) was explicitly out of scope (§8) and
untouched. The reduction that did happen is precisely the set of items independently proven
unused by two research passes, personal re-verification, and a dedicated validation pass — nothing
more, nothing less.
