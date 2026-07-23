# Pre-Phase-35 whole-game gap audit

Date: 2026-07-23

## Scope and outcome

This audit reviewed the application boundary, both game modes, cross-scene
state, fixed-step scheduling, persistence, packaging, and platform validation
before starting Phase 35. It was a correctness and release-readiness pass, not
a new gameplay phase.

The discovered correctness gaps have production fixes and focused regression
gates. Windows CI now compiles the production sources and 35 focused checks
with the strict MinGW warning policy, builds and verifies a runnable package,
and retains logs when a gate fails. Linux production build/smoke and Linux
ASan/UBSan are required rather than advisory.

## Fixed gaps and ownership

| Area | Gap closed | Primary regression gate |
|---|---|---|
| Input and event boundaries | Rebinding now follows configured scancodes; repeated keydown cannot refresh jump buffers; polling stops immediately after a scene transition; focus loss clears held, buffered, controller-edge, and pending keyboard state; quit is handled consistently. | `mingw-prephase35-statetest` |
| Polling versus simulation | Empty event queues and non-gameplay scenes no longer advance scenery, animation, shooting, death, or leaderboard score. Input snapshot capture rejects invalid sources and clears cross-mode state. | `mingw-prephase35-statetest` |
| Arcade consequences | Contact plus escape costs one shared life once; escape has deterministic precedence and gives no kill/progression credit; boss escape and player-two participation respect mode; inactive entities do not simulate. | `mingw-prephase35-simulationtest` |
| Projectile correctness | Sweeps select the nearest crossed target, edge-crossing hits resolve before off-screen culling, invalid indices are rejected, and fixed-tick shooting cannot burst through real-frame compatibility calls. | `mingw-prephase35-simulationtest` |
| Runner consequences | Hidden player two cannot affect single-player; shared death is applied once; lives saturate at zero; respawn clears velocity/interpolation/input; score persists once; recycled stars reset interpolation. | `mingw-prephase35-simulationtest` |
| World and public API boundaries | High-speed downward movement resolves against the first surface independent of ledge array order; invalid/null public inputs return safely. | `mingw-prephase35-simulationtest` |
| Fixed-step integration | Fractional time cannot leak between Arcade and Runner; catch-up is capped and backlog is reduced to a bounded interpolation remainder; leaving gameplay clears the scheduler. | `mingw-prephase35-integrationtest`, `linux-asan-integration` |
| Session reset integration | Both modes clear stale input/events/animation/flash/projectiles and initialize hidden player-two state deterministically; the event queue can represent both projectile pools. | `mingw-prephase35-integrationtest` |
| Asset lookup and startup | Relative resources resolve from the executable directory, its development-build parent, then the working-directory fallback. Required startup assets are checked before entering the main loop. | `mingw-prephase35-lifecycletest` |
| Renderer, audio, and music | Stored VSync intent is honored; accelerated creation falls back to software; a missing audio device is a supported silent mode; scene music changes and pause/resume behavior are centralized. | `mingw-prephase35-lifecycletest`, required Linux smoke |
| Lifecycle and failure handling | Menu load failures request a clean quit instead of calling `exit`; scene transitions clear transient input; font/audio/resource ownership has one shutdown path; no-audio shutdown is null-safe; and omitted window/renderer output containers fall back to the `GameState`-owned handles. | `mingw-prephase35-lifecycletest` |
| Determinism | `ENDGAME_SEED` provides a cross-platform deterministic startup seed and is applied after SDL backends initialize. | `mingw-prephase35-lifecycletest` |
| Preferences | Settings and display state use SDL's per-user preference directory, accept `ENDGAME_PREF_DIR` only as a test/support override, sanitize loaded values, and write through a flushed sibling temporary file before replacement. | `mingw-prephase35-persistencetest` |
| UI and leaderboard recovery | Leaderboard entries remain top-25 sorted across persistence/reload, malformed persisted data recovers safely, mode-menu selections survive redraw, missing UI textures fail without null dereferences, and multiplayer HUD caches remain separate. | `mingw-prephase35-uitest` |
| Numerical and long-session robustness | Fixed-step/replay/wave/segment/spawn/feedback/physics APIs reject invalid boundaries, counters and pools remain bounded across long synthetic sessions, and atomic/preference helpers fail safely. | `mingw-prephase35-robustnesstest`, `linux-asan-robustness` |
| Collider, event, and AI boundaries | Gameplay colliders use one authoritative model; half-open edge and high-speed sweep rules are deterministic; corrupt/full event queues recover safely; simultaneous projectiles resolve from one snapshot; counters saturate; inactive/dead entities and hidden player two cannot influence AI or hazards. | `mingw-prephase35-collisionaitest`, `linux-asan-collision-ai` |
| Restart and soak behavior | Repeated cross-mode/SP/MP resets clear every session transient without damaging app-lifetime state; maximal event batches fit; mixed-rate clocks and seeded headless sessions remain bounded and deterministic across fresh restarts. | `mingw-prephase35-soaktest`, `linux-asan-soak` |
| Build dependency correctness | Public headers participate in rebuild decisions; header self-containment now includes `atomic_file.h`, `fixed_step.h`, `leaderboard.h`, and `preferences.h`; clean/build paths and macOS architecture/rpath settings remain explicit. | `mingw-headertest`, CI production builds |
| Windows distribution | Packaging copies only the production executable, four SDL DLLs, runtime resources, license, and README. Verification compares executable and DLL sizes/SHA-256 content against the current build artifacts, compares the exact relative resource file set and content against the repository, and enforces the exact release root layout. Staging is verified before publication; replacement requires the exact `.endgame-package` marker and refuses symlinks or protected paths. | `mingw-package`, `mingw-package-verify` |

## Required validation matrix

| Environment | Required evidence |
|---|---|
| Windows / MinGW | Strict production build, repository audit, all 35 focused checks, all nine pre-Phase-35 audit gates, package build, and package verification. Each audit gate has an individual log uploaded when the job fails. |
| Linux production | Case-sensitive repository audit, production executable build, and headless smoke test using SDL dummy drivers. No step is `continue-on-error`. |
| Linux sanitizers | Deterministic replay plus the pre-Phase-35 simulation, integration, robustness, collision/AI, and soak harnesses under AddressSanitizer and UndefinedBehaviorSanitizer with leak detection and halt-on-error enabled. |
| Local audit evidence | Strict MinGW production compilation passed. The state, simulation, integration, lifecycle, UI, robustness, collision/AI, soak, expanded-header, settings, and display gates passed. Package create/verify/safe-replace, unmarked-directory refusal, exact resource/binary manifests, and altered-content rejection passed. The complete independent matrix remains the CI source of truth. |

Run the nine whole-game gates together:

```sh
make mingw-prephase35
```

Create and verify a package at an explicit destination:

```sh
make mingw-package MINGW_PACKAGE_DIR=/tmp/endgame-windows
make mingw-package-verify MINGW_PACKAGE_DIR=/tmp/endgame-windows
```

## Remaining platform limitations

- The bundled-framework macOS target is preserved, including explicit
  architecture and executable-relative framework rpaths, but there is no
  macOS CI runner in this workflow and it was not runtime-tested from the
  Windows audit environment.
- The validated MinGW distributions do not include native ASan/UBSan runtime
  libraries. Required sanitizer coverage therefore runs on Linux; it does not
  prove Windows-specific allocator or driver behavior.
- Dummy video and audio drivers prove headless initialization, ownership, and
  fallback behavior. They do not replace manual validation on a real GPU,
  display, controller, and audio endpoint.
- The Windows output is a verified runnable directory, not a signed installer.
  Code signing, installer UX, automatic updates, and release-version metadata
  remain release-engineering work.
- Asset and package checks prove presence, casing, ownership, and loadability;
  they do not assess visual composition, audio mastering, or third-party
  license suitability beyond copying the repository's current license.

These limitations do not block Phase 35 architecture work, but they should not
be described as solved by the pre-Phase-35 audit.
