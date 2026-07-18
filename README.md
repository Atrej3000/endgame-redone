# Ucode_Endgame
Ucode IT Academy C Marathon: Final team project (Serha/Slipchencko team)

Installation (macOS, primary supported platform):
    1. open your terminal in the game directory/repo and print "make"
    2. print "./endgame"
    ...
    3. profit

Windows/MinGW validation build (additive, does not replace the macOS build above):
    Used to compile and smoke-test this project on Windows, where the bundled macOS
    `.framework`s and `clang` toolchain aren't available. Requires MinGW-w64 GCC and GNU Make
    (e.g. `scoop install gcc make`), plus the official SDL2 MinGW "devel" packages:
        - SDL2-devel-<version>-mingw.tar.gz     (github.com/libsdl-org/SDL/releases)
        - SDL2_image-devel-<version>-mingw.tar.gz (github.com/libsdl-org/SDL_image/releases)
        - SDL2_ttf-devel-<version>-mingw.tar.gz   (github.com/libsdl-org/SDL_ttf/releases)
        - SDL2_mixer-devel-<version>-mingw.tar.gz (github.com/libsdl-org/SDL_mixer/releases)
    Extract each into `vendor/SDL2-mingw/<PackageName-version>/` (see `docs/refactor-plan.md`
    for the exact versions last verified and the small include-compatibility shims needed).
    Then:
        make mingw               # builds build-mingw/endgame-mingw.exe with strict warnings
        make mingw-run           # builds and runs it
        make mingw-smoketest     # non-interactive init/asset-guard/shutdown runtime check
        make mingw-scenetest     # non-interactive scene-transition check
        make mingw-lifecycletest # non-interactive asset-load/session-reset lifecycle check
        make mingw-asan          # ASan/UBSan debug build, where the toolchain supports it
    `vendor/` and `build-mingw/` are gitignored (not committed) since they're large,
    regeneratable, third-party/build artifacts.

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

    2) Runner mod: you need to run to the right side as long as possible, if you fall or touch enemies you will die.

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
