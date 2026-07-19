# Design Patterns — Ucode_Endgame

Companion to `docs/solid-gof-audit.md` (the evidence and decision process) and
`docs/dependency-map.md` (dependency direction). This document is the final, implementation-level
record of every pattern this project uses, recognizes as already present, or deliberately rejects.

## Implemented this phase (Phase 9)

| Pattern | Problem solved | Implementation | Why selected | Alternatives rejected |
|---|---|---|---|---|
| Factory Method | 10 call sites across `process.c`/`loadGame.c` hand-duplicated the same 5-6-field `Enemies` struct initialization, with no bounds check anywhere | `enemy_spawn`/`smart_enemy_spawn`/`boss_spawn` (`src/entity_spawn.c`, `inc/entity_spawn.h`) — bounds-checked, exact-field-matching creation functions | Centralizes the bounds check (a genuine new correctness guarantee) alongside the field-set invariant; gives each entity kind a distinct, named, testable creation entry point | A bare (non-bounds-checked) helper function — rejected, would deduplicate text but not add the missing safety check |
| Command (translation boundary, not queued objects) | `processEvents()`/`processEvents2()` independently hand-implement the same 5 keys meaning the same 5 actions; `menu0_events()` already hand-rolls a multi-key-to-one-action pattern | `GameCommand` enum + `translate_arcade_command`/`translate_runner_command`/`translate_menu_command` (`src/input_command.c`, `inc/input_command.h`) | Pure, synchronous keycode-to-action mapping, testable independent of each mode's differing side-effect bodies; no queue/heap allocation, so continuous input timing is untouched | Per-file helper function (rejected — doesn't remove the keycode-to-action mapping duplication itself); a real Command object with `execute()`/undo (rejected — no undo feature exists, no queuing need, explicitly disallowed by the task) |

**Supporting SOLID extractions** (not GoF patterns, but part of this phase's scope):
- **ISP**: `inc/scene.h`, `inc/frame.h` — focused headers mirroring the existing `inc/app.h`
  pattern. See "Header decomposition, and its limits" below for the asymmetry between the two.
- **DIP consistency fix**: `src/load_menu.c`'s three raw `IMG_Load()` calls now go through the
  existing `load_texture()` checked wrapper — reuses an existing seam, not a new abstraction.

## Already present, informally (recognized, not re-implemented)

| Pattern | Where | Why formalizing would over-engineer this project |
|---|---|---|
| State | `AppScene` enum + `app_change_scene()` (`src/scene.c`) + 2 per-scene entry hooks | Only 2 of 10 scenes need entry side effects. A formal state-object-per-scene (each with its own `enter()`/`exit()`/`handle_input()` struct of function pointers) would add a layer of indirection with no behavioral gain — the existing enum-and-switch is smaller, is exactly what a C developer expects, and every transition already funnels through one function. |
| Facade | `app_init()`/`app_shutdown()` (`src/app.c`); `arcade_frame()`/`runner_frame()` (`src/frame.c`); `arcade_assets_load`/`runner_assets_load`/`shared_assets_load` and their `_unload` counterparts (`src/loadGame.c`) | Each already is a minimal, well-named, single-purpose simplified entry point over several lower-level SDL/gameplay calls. Wrapping these in another named "Facade" layer would be a facade over a facade — pure ceremony. |
| Flyweight | `shared_assets_load`/`sharedAssetsLoaded` (`src/loadGame.c`); the Phase 5 `manFrames[0]` consolidation | The shared-immutable-texture-and-font bucket already is the Flyweight idea — one load, referenced by both modes, gated by a boolean flag. No duplicated loading remains to fix; a reference-counted cache or hash-table registry would solve a problem this codebase doesn't have. |
| Adapter | `load_texture`/`load_music`/`load_font` (`src/asset_loader.c`) | Already the checked translation boundary between raw SDL calls and the rest of the codebase. This phase closed the one place (`load_menu.c`) that bypassed it — see the DIP fix above — rather than adding a second, redundant adapter layer. |

## Rejected

| Pattern | Why rejected |
|---|---|
| Strategy | No pair of interchangeable, same-contract implementations exists with a real runtime selection. `arcade_session_reset`/`runner_session_reset`'s internal single/multiplayer branching is one function handling both via a mode check, not two swappable strategy objects. Arcade and Runner mechanics differ too much to be modeled as interchangeable strategies (explicitly disallowed by this phase's own charter regardless). |
| Abstract Factory | Arcade/Runner asset-group loaders (`arcade_assets_load`/`runner_assets_load`) are already clear, explicit, independently named functions. There is no need for interchangeable *families* of resources — introducing an abstract-factory interface here would add a layer of indirection purely to claim the pattern was used. |
| Observer | No single event in the codebase has multiple independent, decoupled consumers today. Every apparent "multi-effect" site (e.g. death triggering both audio cleanup and a scene change) is one function doing several things sequentially in one place — not multiple registered listeners reacting independently. Introducing Observer here would replace a simple, readable sequence of calls with more indirection for zero functional gain. |
| Template Method | `src/frame.c`'s fixed simulate→collide→render→(death)→input sequence is already the shared shape between Arcade and Runner, implemented as two separate concrete functions. Unifying this via an inheritance-style template/callback structure would require the exact "imitate C++ classes"/"large function-pointer framework" this phase is explicitly told not to build, in exchange for no simplification (Arcade and Runner's individual steps already differ enough that a shared template would need overrides for nearly every step). |
| Singleton | `GameState *` passed explicitly through every function that needs it is strictly better than a global/singleton accessor: nothing is hidden, every dependency is visible at every call site, and the existing lifecycle tests can construct/destroy independent `GameState` instances freely. A singleton would hide dependencies and complicate testing for no benefit — explicitly disallowed by this phase's own charter regardless. |
| Service Locator | Same reasoning as Singleton — every dependency in this codebase is already an explicit function argument or struct field. A locator would hide the dependency graph this project has deliberately kept visible since Phase 4's asset-lifecycle work. Explicitly disallowed by this phase's own charter regardless. |
| Builder | Every struct in this codebase (`Man`, `Bullet`, `Ledge`, `Enemies`, etc.) is small and locally initialized with direct field assignment or (as of this phase) a small factory function. None have enough construction complexity (optional steps, many parameter combinations, staged construction) to justify a builder. |
| Prototype | No cloning/copying use case exists anywhere in the codebase — every entity is either bulk-reset via a loop (`arcade_session_reset`/`runner_session_reset`) or freshly spawned via a factory function (`entity_spawn.c`, this phase). Neither needs "copy an existing instance." |
| Composite | Every entity collection (`enemyValues[101]`, `smartEnemies[10]`, `boss[2]`, `bullets[1000]`) is a flat array of independent, non-hierarchical structs. There is no tree-shaped part-whole structure anywhere to justify Composite. |
| Decorator | Rendering (`doRender`/`doRender2`) has been render-pure since Phase 5 and performs static, fixed per-frame blitting. There is no dynamic, run-time-composed rendering behavior (e.g., stackable visual effects) that Decorator would simplify. |
| Mediator | The top-level call graph (`docs/dependency-map.md`) is already small and direct — `main` dispatches to `scene`/`frame`, which call a short, fixed list of mode functions. There are no many-to-many object interactions for a mediator to untangle. |
| Memento | No save-state/undo feature exists anywhere in this codebase to support. Introducing Memento would be building infrastructure for a feature that was never requested. |
| Visitor | Every entity struct (`Enemies`, `Man`, `Ledge`, etc.) is fixed and procedural, operated on by ordinary functions that already know the concrete type. There is no open-ended "add new operations without modifying types" problem that Visitor solves here. |
| Chain of Responsibility | Input handling is a small, direct `switch` per scene (formalized this phase via `GameCommand` where duplication was demonstrated — see above). There is no need for a request to pass through a dynamic chain of handlers deciding whether to consume or forward it. |
| Interpreter | Key mappings are a small, fixed table (now partially formalized as `GameCommand` translation functions this phase). A full grammar/interpreter is unjustified machinery for a fixed set of keycode-to-action mappings. |
| Proxy | No remote, lazy-initialization, security, or access-control boundary exists anywhere in this codebase for Proxy to mediate. |

## Header decomposition, and its limits

`inc/scene.h` and `inc/frame.h` were added this phase (mirroring the existing `inc/app.h`
pattern), but their scope differs based on real call-site counts discovered during
implementation, not assumed in advance:

- `arcade_frame`/`runner_frame` have exactly one definer (`frame.c`) and one production caller
  (`main.c`) — their prototypes were fully removed from `inc/header.h` and now live only in
  `inc/frame.h`. Two test harnesses that also call them directly were updated to include the new
  header (caught immediately by a failed build, not silently missed).
- `app_change_scene()` has 5 real call sites (`scene.c`, `main.c`, `menu_events.c`,
  `pause_events.c`, `processEvents.c`) — its prototype is declared in **both** `inc/header.h`
  (unchanged, so the 3 call sites not touched this phase need no edit) and the new `inc/scene.h`
  (a real, standalone-usable header for `scene.c`'s own public surface). This follows the task's
  own explicit guidance: "leave unresolved declarations in the compatibility header ... migrate
  source files gradually ... do not perform a one-commit replacement of every include."
- `AppScene`'s enum definition itself remains in `inc/header.h` and cannot move without a deeper
  restructuring: `GameState.scene` is typed `AppScene`, so `header.h` must see the enum before
  `GameState`'s own definition. Moving `AppScene` into `scene.h` would require `header.h` to
  include `scene.h`, but `scene.h` already needs `GameState` (via `header.h`) for
  `app_change_scene`'s signature — a circular include. `inc/entity_spawn.h` and
  `inc/input_command.h` (this phase's other two new headers) have no such constraint and are
  fully self-sufficient.

## Deferred (evidence-supported, not attempted this phase)

- **`GameState` decomposition into nested structs** (`AppContext`, `AudioAssets`, per-mode session
  clusters) — real candidate clusters exist (module/ownership map in the audit), but combining
  this with the Command/Factory work risked exceeding this phase's abstraction ceiling.
- **Unifying the 9 scattered lazy-SFX-reload sites** into the `_assets_load`/`_assets_unload`
  ownership model.
- **Converting `menu_events()`/`pause_events()` to `GameCommand`** — neither duplicates its own
  keymap the way `processEvents`/`processEvents2` duplicate with each other; the shared
  `SDLK_q` → "quit to main menu" trigger across these two files remains a small, independent
  inline handler in each.
- **Further header decomposition** beyond the four new focused headers added this phase —
  `process.c`/`processEvents.c`/`collisionDetect.c` and the rest remain on `inc/header.h`.
