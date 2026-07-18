# Ucode_Endgame
![MinGW validation](https://github.com/Atrej3000/endgame-redone/actions/workflows/mingw-validation.yml/badge.svg)

Ucode IT Academy C Marathon: Final team project (Serha/Slipchencko team)

Installation (macOS, primary supported platform):
    1. open your terminal in the game directory/repo and print "make"
    2. print "./endgame"
    ...
    3. profit

Windows/MinGW validation build (additive, does not replace the macOS build above):
    Used to compile and smoke-test this project on Windows, where the bundled macOS
    `.framework`s and `clang` toolchain aren't available. Requires MinGW-w64 GCC and GNU Make
    (e.g. `scoop install gcc make`), plus the official SDL2 MinGW "devel" packages, pinned in
    the Makefile (`SDL2_VERSION`, `SDL2_IMAGE_VERSION`, `SDL2_TTF_VERSION`, `SDL2_MIXER_VERSION`;
    run `make print-mingw-versions` to see the exact numbers currently pinned):
        ./scripts/setup-mingw-sdl2.sh
    fetches and lays out all four packages (plus the small include-compatibility shims needed)
    into `vendor/SDL2-mingw/` automatically -- idempotent, safe to re-run, and the same script
    CI uses (see below), so there's one source of truth instead of manual per-package steps.
    Then:
        make mingw               # builds build-mingw/endgame-mingw.exe with strict warnings
        make mingw-run           # builds and runs it
        make mingw-smoketest     # non-interactive init/asset-guard/shutdown runtime check
        make mingw-scenetest     # non-interactive scene-transition check
        make mingw-lifecycletest # non-interactive asset-load/session-reset lifecycle check
        make mingw-frametest     # non-interactive frame-order/render-purity/animation check
        make mingw-deathtest     # non-interactive Runner death/respawn/game-over lifecycle check
        make mingw-asan          # ASan/UBSan debug build, where the toolchain supports it
        make audit-repo          # repository usage integrity check (asset paths + prototypes)
    `vendor/` and `build-mingw/` are gitignored (not committed) since they're large,
    regeneratable, third-party/build artifacts.

Continuous Integration:
    `.github/workflows/mingw-validation.yml` runs the same build, all five non-interactive test
    targets, and the repository usage integrity check above on every pull request and every push
    to `main`, on a Windows runner (MSYS2 MINGW64 toolchain) -- see `docs/refactor-plan.md`'s
    Phase 6/Pass 6 and Phase 41-45/Pass 7 notes for the full design and reasoning.

Known platform limitations:
    - The default build targets macOS only (bundled `.framework`s under `resource/frameworks`,
      `clang`-specific flags). Linux is not currently supported by either build path.
    - AddressSanitizer/UndefinedBehaviorSanitizer are not available through the MinGW-w64
      toolchains tested on Windows (they lack the sanitizer runtime libraries for that target);
      sanitizer instrumentation on Windows would need MSVC + clang-cl instead.

Manual:
The game has two mods:

    1) Arcade battle mod: you need to kill enemies and don`t allow them to reach the Unit City,
        otherwise, you will lose one of your lives, if enemies touch you - you also will die.

    2) Runner mod: you need to run to the right side as long as possible. Falling or touching a
        trap costs one of your 3 lives (a brief death animation plays, then you respawn) --
        the game ends only once all 3 lives are gone.

    Also, every mod has its own multiplayer where you can play with your friend with one keyboard

Main Menu:

    1. Buttons "1", "2", "3" for choosing game mod in any menu window.
    2. Button "ESC" for exit from the main menu out of the game or any window in the game in the second menu.
    3. Button "q" for exit from the second menu in the main menu or exit from any other window out of the game.

Controls in the Game:

    Button "p" - pause in the game.
    Button "ESC" - exit from pause or exit in the menu without saving the result.

    1st player:
        Buttons "w", "a", "d", for control player moves.
        Button "SPACE" for shooting.

    2nd player:
        Buttons "up", "left", "right", for control player moves.
        Button "0" for shooting.

PS: Good luck, have fun.
