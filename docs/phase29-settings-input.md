# Phase 29 — Settings and input configuration

The main menu now exposes **Settings** with the `S` key. The screen keeps all
configuration in `GameState.app.settings`, while display mode remains in the
existing `GameState.app.display` owner.

## Controls

- Up/Down selects a setting; Left/Right adjusts volume or toggles an option.
- Enter begins key reassignment for a player action, or resets settings on the
  reset row. Escape cancels a reassignment or returns to the main menu.
- The settings file is written after each completed adjustment/rebinding and
  once more during normal shutdown. It lives in SDL's per-user preference
  directory as `settings.ini`, next to the Phase 28 `display.ini` file.

## Persisted preferences

`settings.ini` stores separate music and effects volumes, VSync, the screen
shake preference, and the nine keyboard bindings used by the two players plus the shared pause action.
Its parser starts from documented defaults, clamps audio to SDL's valid range,
and rejects invalid scancodes before they can be used to index keyboard state.

Fullscreen continues to be persisted by `display.ini`; the settings screen
uses the same fullscreen toggle implementation as F11. VSync is applied both
when the renderer is created and whenever it is changed in the settings screen.

## Controller support

The first SDL-compatible controller is opened on startup and is replaced when
the active controller is disconnected. Player 1 can use the D-pad or left
stick to move, **A** to jump, and **X** to shoot in Arcade. Controller input
is merged with keyboard input, so keyboard Player 2 remains available in local
multiplayer. A-button presses create one buffered jump request; holding the
button only controls jump height and cannot create repeated jumps.

## Verification

`docs/verification/settings_test.c`, run by `make mingw-settingstest`, checks
the documented defaults, valid/invalid rebinding, non-mutation on rejection,
and reset behavior. The header self-containment target now also includes
`settings.h`. Runtime initialization/shutdown—including opening and closing a
controller when one is attached—is covered by the existing smoke test.
