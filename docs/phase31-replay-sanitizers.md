# Phase 31: Deterministic Replay and Linux Sanitizers

## Replay boundary

`ReplayRecording` records the gameplay scene (Arcade or Runner), player mode,
initial PRNG seed, and one `InputState` per fixed physics tick.
`replay_run()` creates an isolated zeroed `GameState`, seeds the PRNG
immediately before session reset, and advances the selected real simulation
once per stored tick using `PHYSICS_DT`.

Simulation sound effects are null-safe, so a recording containing buffered
jump or shoot input has the same gameplay result in headless replay as it
does with loaded audio assets; only the optional sound emission is skipped.
Arcade's snapshot-driven shooting action is shared between the event path and
replay; Runner has no shooting action in its gameplay loop.

The replay result deliberately snapshots authoritative state only: player
movement, score/life/session counters, camera position, and a signature of
the procedurally generated Runner world. It excludes renderer and asset
pointers, so it is suitable for headless CI and detects deterministic gameplay
regressions rather than pointer-address variation.

The initial harness covers Runner single-player and multiplayer recordings,
an Arcade recording, repeated execution of an identical recording, a changed
input, a changed seed, and API bounds validation. It provides a stable seam
for later recorded gameplay fixtures without taking ownership of the live SDL
event loop.

## Sanitizer validation

`make linux-asan` compiles the production sources plus the replay test with
AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make linux-asan LINUX_COMPAT_INCLUDE=/tmp/linux-compat-include
```

The dedicated required `linux-sanitizers` GitHub Actions job installs the SDL2
development packages, creates the include-layout shims used by the existing
Linux job, and runs that target with leak detection and halt-on-error options.
It replaces the previous Windows-only sanitizer limitation with an executable
sanitizer signal while keeping the native MinGW build unchanged.
