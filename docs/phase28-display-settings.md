# Phase 28 — Resolution independence and display settings

Gameplay continues to use the fixed logical coordinate system `1280×720`. The SDL renderer is
configured with `SDL_RenderSetLogicalSize`, which scales that surface to the drawable size while
preserving the aspect ratio and letterboxing excess space. This keeps physics, collision, camera,
and UI coordinates independent of the actual monitor or window resolution.

`app_init()` creates an `SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI` window. It restores the
last valid windowed dimensions and fullscreen preference from SDL's platform-specific preference
directory (`SDL_GetPrefPath("Ucode", "Endgame")`); malformed or out-of-range values fall back to
the logical default. `app_shutdown()` saves the current windowed size and fullscreen preference.

Press `F11` in any menu, game, leaderboard, or pause scene to switch between windowed mode and
desktop fullscreen. The display module owns this event handling, so gameplay input and simulation
never depend on the physical presentation size.

`docs/verification/display_settings_test.c` verifies defaults and settings bounds through
`mingw-displaytest`. The normal smoke test additionally covers `app_init()` and `app_shutdown()`,
including the persistence path, against SDL's dummy drivers in CI.
