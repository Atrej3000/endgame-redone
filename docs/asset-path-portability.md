# Asset Path Portability — Ucode_Endgame

Written **before** any Phase 8 code edit, per that phase's own requirement. Documents every
known case-sensitivity defect, re-verified directly against the current tree (commit `8634928`,
tag `refactor-pass-7-unused-cleanup`) rather than relied on from Phase 7's summary, plus the
canonical-path policy decision and the search for any additional mismatches.

## Known mismatches (re-verified directly)

| Source file and line | Requested path | Tracked path | Windows behavior | Case-sensitive behavior |
|---|---|---|---|---|
| `src/loadGame.c:116` | `./resource/images/background/Sunset_front.png` | `resource/images/background/sunset_front.png` | Loads successfully (case-insensitive lookup) | `IMG_Load` fails — no file named `Sunset_front.png` exists |
| `src/loadGame.c:118` | `./resource/images/terrain/brick_block.png` | `resource/images/terrain/Brick_block.png` | Loads successfully | `IMG_Load` fails — no file named `brick_block.png` exists |
| `src/loadGame.c:119` | `./resource/images/terrain/copper_block.png` | `resource/images/terrain/Copper_block.png` | Loads successfully | `IMG_Load` fails — no file named `copper_block.png` exists |

Each verified via direct `grep` against the current `src/loadGame.c` and `git ls-files`'s exact
tracked casing — not assumed from Phase 7's prose.

## Canonical path policy

Adopted policy (per this phase's own preference): **production source literals must match the
exact tracked filename and directory casing.** For each of the three defects, the smaller-safe
correction was chosen by consulting sibling-file convention within the same directory (git
history is uninformative here — all three files were introduced in the single initial commit
`cfb7ca7` that bulk-committed the whole asset tree at once, so there is no rename history to
consult for "which case came first").

### `resource/images/terrain/` — decision: fix the source literals
```
Brick_block.png    (tracked)
Clay_block.png     (tracked, already referenced correctly at loadGame.c:31)
Copper_block.png   (tracked)
```
All three tracked files use identical PascalCase (`Xxx_block.png`). `Clay_block.png` is already
loaded with matching case and known-correct. This makes the capitalized convention the
directory's clear, consistent, established pattern — the two **lowercase source literals**
(`brick_block.png`, `copper_block.png`) are the actual defects. **Decision: update the two source
literals to `Brick_block.png`/`Copper_block.png`. Do not rename any tracked file.**

### `resource/images/background/` — decision: fix the source literal
```
Sunset.png            (tracked, capitalized)
Sunset_sun.png        (tracked, capitalized)
sunset_cloud01.png    (tracked, lowercase)
sunset_cloud02.png    (tracked, lowercase)
sunset_cloud03.png    (tracked, lowercase)
sunset_cloud04.png    (tracked, lowercase)
sunset_cloud05.png    (tracked, lowercase)
sunset_cloud06.png    (tracked, lowercase)
sunset_cloud07.png    (tracked, lowercase)
sunset_cloud08.png    (tracked, lowercase)
sunset_front.png      (tracked, lowercase)
sunset_train.bmp      (tracked, lowercase)
```
This directory has a **split** convention: 2 files capitalize "Sunset", 10 do not. The
lowercase-prefixed pattern is both the majority (10 vs 2) and what the actual tracked file
`sunset_front.png` already uses — the capitalized **source literal** `Sunset_front.png` is the
mismatch. **Decision: update the source literal to `sunset_front.png`. Do not rename the tracked
file.**

**All three corrections touch only `src/loadGame.c` — zero file renames.** This is confirmed as
the smaller, safer change in every case: a repo-wide search (below) found each of the three paths
has exactly one load-call site, and the C field identifiers `game->brick_block`/
`game->copper_block` (declared in `inc/header.h`, used in `src/doRender.c`, described in prose in
`docs/game-session-lifecycle.md`) are ordinary C variable names, unrelated to the file path
strings — they need no change regardless of which correction path was chosen.

## Repo-wide search for additional mismatches

Searched (per this phase's required categories):
- **`src/*.c`** — every `load_texture`/`load_music`/`load_font`/raw `Mix_LoadWAV`/`Mix_LoadMUS`/
  `IMG_Load` call site (48 total asset-load call sites across `loadGame.c`, `load_menu.c`,
  `process.c`, `processEvents.c`, `menu_events.c`, `pause_events.c`) cross-checked against
  `git ls-files` output for the corresponding directories (`resource/sounds/`, `resource/text/`,
  `resource/images/backgrounds/`, `resource/images/main_hero/`, `resource/images/secondplayer/`,
  `resource/images/enemies/`, `resource/images/bullets/`, `resource/images/traps/`) — every other
  literal matches its tracked file exactly, case for case.
- **Headers** (`inc/header.h`, `inc/app.h`) — contain no path string literals at all.
- **Test harnesses** (`docs/verification/*.c`) — confirmed zero `resource/`-shaped string literals
  in any of the 5 files (each is a structural/state-machine test with no direct SDL asset I/O).
- **`Makefile`** — only references `resource/frameworks` (the macOS `.framework` bundle
  directory, via `-F`/`-framework`/`-rpath` flags) — untouched, explicitly out of scope for this
  phase (framework internal duplication is Phase 7's documented, deferred finding).
- **Setup script** (`scripts/setup-mingw-sdl2.sh`) and **CI workflow**
  (`.github/workflows/mingw-validation.yml`) — reference no `resource/...` game-asset paths at
  all (only `vendor/SDL2-mingw/...` MinGW dependency paths, an unrelated concern).
- **README.md and other docs** — no stale `resource/...` path examples found (grep confirmed the
  only "brick_block"/"copper_block" mentions in docs are C-field-name prose, not path literals).

Also checked explicitly, per this phase's checklist, and found **none** anywhere in `src/*.c`:
- Backslash path separators (`\` inside a string literal).
- Repeated separators (`resource//...`).
- Adjacent C string literals split across a path (e.g. `"resource/" "images/" "x.png"`).
- Unicode or visually similar look-alike characters in any path literal.
- Path aliases (two different literals resolving to the same file) beyond the already-known,
  harmless, intentional case of `Sunset.png`/`fon.png` (byte-identical content, different files,
  documented in Phase 5/7 — not a path-literal alias, a duplicate-content coincidence).

**One stylistic (non-defect) observation**: sound/music loads use a bare `"resource/sounds/..."`
literal while image/font loads use a `"./resource/images/..."` literal (leading `./` present or
absent). Both resolve identically on every OS (a leading `./` is purely cosmetic in a relative
path) — not a portability defect, not touched this phase (fixing a stylistic inconsistency is
outside this phase's narrow charter).

## Conclusion

Exactly three case-sensitivity defects exist in the entire codebase, all three in
`src/loadGame.c`, all three resolved by correcting the source literal (never the tracked file).
No other mismatch of any kind was found across source, headers, tests, build files, scripts, CI,
or documentation.
