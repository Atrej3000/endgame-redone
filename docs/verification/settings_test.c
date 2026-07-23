#include "atomic_file.h"
#include "display.h"
#include "preferences.h"
#include "settings.h"
#include "settings_menu.h"

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "SETTINGS TEST: FAIL - %s\n", message);
        failures++;
    }
}

static long test_process_id(void)
{
#ifdef _WIN32
    return (long)_getpid();
#else
    return (long)getpid();
#endif
}

static bool make_test_directory(const char *path)
{
#ifdef _WIN32
    return _mkdir(path) == 0;
#else
    return mkdir(path, 0700) == 0;
#endif
}

static void remove_test_directory(const char *path)
{
#ifdef _WIN32
    (void)_rmdir(path);
#else
    (void)rmdir(path);
#endif
}

static bool set_atomic_temp_blockers(const char *path, bool create)
{
    bool ok = true;
    char temporaryPath[512];
    for (unsigned int attempt = 0U; attempt < 128U; attempt++)
    {
        const int result = snprintf(temporaryPath, sizeof(temporaryPath),
                                    "%s.tmp.%ld.%u", path,
                                    test_process_id(), attempt);
        if (result < 0 || (size_t)result >= sizeof(temporaryPath))
        {
            ok = false;
            break;
        }
        if (create)
        {
            if (!make_test_directory(temporaryPath))
            {
                ok = false;
                break;
            }
        }
        else
        {
            remove_test_directory(temporaryPath);
        }
    }
    if (create && !ok)
    {
        (void)set_atomic_temp_blockers(path, false);
    }
    return ok;
}

static bool set_preference_override(const char *path)
{
#ifdef _WIN32
    return _putenv_s("ENDGAME_PREF_DIR", path) == 0;
#else
    return setenv("ENDGAME_PREF_DIR", path, 1) == 0;
#endif
}

static void clear_preference_override(void)
{
#ifdef _WIN32
    (void)_putenv_s("ENDGAME_PREF_DIR", "");
#else
    (void)unsetenv("ENDGAME_PREF_DIR");
#endif
}

static bool write_preference_text(const char *filename, const char *contents)
{
    char *path = preferences_file_path(filename);
    if (!path) return false;
    const bool written = atomic_write_text_file(path, contents, strlen(contents));
    free(path);
    return written;
}

static char *load_preference_text(const char *filename, size_t *length)
{
    char *path = preferences_file_path(filename);
    if (!path) return NULL;
    char *contents = (char *)SDL_LoadFile(path, length);
    free(path);
    return contents;
}

static void remove_preference_file(const char *filename)
{
    char *path = preferences_file_path(filename);
    if (path)
    {
        (void)remove(path);
        free(path);
    }
}

static void drain_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
    }
}

int main(void)
{
    if (SDL_Init(SDL_INIT_EVENTS) != 0)
    {
        fprintf(stderr, "SETTINGS TEST: FAIL - SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    GameSettings settings;
    settings_defaults(&settings);
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_A &&
                    settings.player1.moveRight == SDL_SCANCODE_D &&
                    settings.player1.jump == SDL_SCANCODE_W &&
                    settings.player1.shoot == SDL_SCANCODE_SPACE && settings.player1.pause == SDL_SCANCODE_P,
                "player 1 defaults are the documented controls");
    expect_true(settings.player2.moveLeft == SDL_SCANCODE_LEFT &&
                    settings.player2.moveRight == SDL_SCANCODE_RIGHT &&
                    settings.player2.jump == SDL_SCANCODE_UP &&
                    settings.player2.shoot == SDL_SCANCODE_KP_0,
                "player 2 defaults are the documented controls");
    expect_true(settings.musicVolume == MIX_MAX_VOLUME && settings.effectsVolume == MIX_MAX_VOLUME &&
                    settings.vsync && settings.screenShake,
                "audio and accessibility defaults are enabled");

    expect_true(settings_rebind(&settings, 0, SDL_SCANCODE_J),
                "a valid player 1 binding can be changed");
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_J,
                "the selected binding receives the new scancode");
    expect_true(!settings_rebind(&settings, 0, settings.player1.moveRight) &&
                    settings.player1.moveLeft == SDL_SCANCODE_J,
                "a binding conflict is rejected without mutating controls");
    expect_true(!settings_rebind(&settings, -1, SDL_SCANCODE_K) &&
                    !settings_rebind(&settings, 9, SDL_SCANCODE_K) &&
                    !settings_rebind(&settings, 2, SDL_SCANCODE_UNKNOWN) &&
                    !settings_rebind(&settings, 2, SDL_SCANCODE_ESCAPE) &&
                    !settings_rebind(&settings, 2, SDL_SCANCODE_Q) &&
                    !settings_rebind(&settings, 2, SDL_SCANCODE_F11),
                "invalid and globally reserved binding requests are rejected");
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_J,
                "rejected changes do not mutate a binding");

    settings.musicVolume = 0;
    settings.effectsVolume = 0;
    settings.vsync = false;
    settings.screenShake = false;
    settings_reset(&settings);
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_A &&
                    settings.musicVolume == MIX_MAX_VOLUME && settings.effectsVolume == MIX_MAX_VOLUME &&
                    settings.vsync && settings.screenShake,
                "reset restores all configurable defaults");

    char testDirectory[128];
    const int directoryResult = snprintf(testDirectory, sizeof(testDirectory),
                                         "pre_phase35_settings_%ld",
                                         test_process_id());
    if (directoryResult < 0 || (size_t)directoryResult >= sizeof(testDirectory) ||
        !make_test_directory(testDirectory) ||
        !set_preference_override(testDirectory))
    {
        fprintf(stderr, "SETTINGS TEST: FAIL - isolated preference directory setup\n");
        SDL_Quit();
        return 1;
    }

    GameSettings persisted;
    settings_defaults(&persisted);
    (void)settings_rebind(&persisted, 0, SDL_SCANCODE_J);
    persisted.musicVolume = 37;
    persisted.effectsVolume = 81;
    persisted.vsync = false;
    persisted.screenShake = false;
    expect_true(settings_save(&persisted),
                "settings save succeeds through ENDGAME_PREF_DIR");

    GameSettings loaded;
    settings_load(&loaded);
    expect_true(loaded.player1.moveLeft == SDL_SCANCODE_J &&
                    loaded.musicVolume == 37 && loaded.effectsVolume == 81 &&
                    !loaded.vsync && !loaded.screenShake,
                "settings roundtrip through ENDGAME_PREF_DIR");

    size_t priorLength = 0U;
    char *priorContents = load_preference_text("settings.ini", &priorLength);
    char *settingsPath = preferences_file_path("settings.ini");
    const bool blockedTemporaryPath =
        settingsPath != NULL &&
        set_atomic_temp_blockers(settingsPath, true);
    expect_true(blockedTemporaryPath,
                "atomic-write failure fixture blocks the sibling temp path");
    persisted.musicVolume = 99;
    expect_true(blockedTemporaryPath && !settings_save(&persisted),
                "settings save reports a failed atomic temp write");
    size_t afterLength = 0U;
    char *afterContents = load_preference_text("settings.ini", &afterLength);
    expect_true(priorContents != NULL && afterContents != NULL &&
                    priorLength == afterLength &&
                    memcmp(priorContents, afterContents, priorLength) == 0,
                "failed atomic temp write preserves the prior settings file");
    SDL_free(priorContents);
    SDL_free(afterContents);
    if (settingsPath) (void)set_atomic_temp_blockers(settingsPath, false);
    free(settingsPath);

    expect_true(write_preference_text(
                    "settings.ini",
                    "version=2\nmusic=0\neffects=0\np1_left=13\n"),
                "unsupported-version fixture is written");
    settings_load(&loaded);
    expect_true(loaded.musicVolume == MIX_MAX_VOLUME &&
                    loaded.effectsVolume == MIX_MAX_VOLUME &&
                    loaded.player1.moveLeft == SDL_SCANCODE_A,
                "unsupported settings versions fall back to defaults");

    expect_true(write_preference_text(
                    "settings.ini",
                    "version=1\nmusic=12oops\ngarbage middle line\n"
                    "effects=23\nvsync=0\nscreen_shake=1\n"),
                "malformed-line fixture is written");
    settings_load(&loaded);
    expect_true(loaded.musicVolume == MIX_MAX_VOLUME &&
                    loaded.effectsVolume == 23 && !loaded.vsync &&
                    loaded.screenShake,
                "malformed settings lines are ignored without hiding later values");

    expect_true(write_preference_text(
                    "settings.ini",
                    "version=1\nmusic=999999999999999999999999999\n"
                    "effects=17\nfuture_setting=42\n"),
                "overflow and unknown-key fixture is written");
    settings_load(&loaded);
    expect_true(loaded.musicVolume == MIX_MAX_VOLUME &&
                    loaded.effectsVolume == 17,
                "overflow integers are ignored safely and unknown keys are forward-compatible");

    expect_true(write_preference_text(
                    "settings.ini",
                    "version=1\nmusic=10\nmusic=90\neffects=12\n"),
                "duplicate-key fixture is written");
    settings_load(&loaded);
    expect_true(loaded.musicVolume == MIX_MAX_VOLUME &&
                    loaded.effectsVolume == MIX_MAX_VOLUME,
                "duplicate known keys reject the ambiguous settings file");

    const size_t oversizedLength = 17U * 1024U;
    char *oversized = (char *)malloc(oversizedLength + 1U);
    expect_true(oversized != NULL, "oversized settings fixture allocates");
    if (oversized)
    {
        memset(oversized, 'x', oversizedLength);
        oversized[oversizedLength] = '\0';
        expect_true(write_preference_text("settings.ini", oversized),
                    "oversized settings fixture is written");
        settings_load(&loaded);
        expect_true(loaded.musicVolume == MIX_MAX_VOLUME &&
                        loaded.player1.moveLeft == SDL_SCANCODE_A,
                    "oversized settings files are rejected before parser allocation");
        free(oversized);
    }

    char conflictContents[256];
    const int conflictResult = snprintf(
        conflictContents, sizeof(conflictContents),
        "version=1\nmusic=19\np1_left=%d\np1_right=%d\n",
        (int)SDL_SCANCODE_A, (int)SDL_SCANCODE_A);
    expect_true(conflictResult >= 0 &&
                    (size_t)conflictResult < sizeof(conflictContents) &&
                    write_preference_text("settings.ini", conflictContents),
                "binding-conflict fixture is written");
    settings_load(&loaded);
    expect_true(loaded.player1.moveLeft == SDL_SCANCODE_A &&
                    loaded.player1.moveRight == SDL_SCANCODE_D &&
                    loaded.musicVolume == 19,
                "persisted binding conflicts restore safe default bindings");

    GameState *game = (GameState *)calloc(1U, sizeof(GameState));
    expect_true(game != NULL, "Reset Defaults test allocates GameState");
    if (game)
    {
        settings_defaults(&game->app.settings);
        game->app.settings.musicVolume = 5;
        game->app.settings.effectsVolume = 7;
        game->app.settings.vsync = false;
        game->app.settings.screenShake = false;
        game->app.settings.selectedRow = 5;
        game->app.display.windowWidth = 1600;
        game->app.display.windowHeight = 900;
        game->app.display.fullscreen = true;
        game->app.scene = APP_SCENE_SETTINGS;

        drain_events();
        SDL_Event resetEvent = {0};
        resetEvent.type = SDL_KEYDOWN;
        resetEvent.key.type = SDL_KEYDOWN;
        resetEvent.key.state = SDL_PRESSED;
        resetEvent.key.keysym.sym = SDLK_RETURN;
        resetEvent.key.keysym.scancode = SDL_SCANCODE_RETURN;
        expect_true(SDL_PushEvent(&resetEvent) == 1,
                    "Reset Defaults event is queued");
        settings_events(game);
        expect_true(game->app.settings.musicVolume == MIX_MAX_VOLUME &&
                        game->app.settings.effectsVolume == MIX_MAX_VOLUME &&
                        game->app.settings.vsync &&
                        game->app.settings.screenShake,
                    "Reset Defaults restores GameSettings in memory");
        expect_true(game->app.display.windowWidth == WIDTH &&
                        game->app.display.windowHeight == HEIGHT &&
                        !game->app.display.fullscreen,
                    "Reset Defaults restores windowed DisplaySettings in memory");

        settings_load(&loaded);
        DisplaySettings loadedDisplay;
        display_settings_load(&loadedDisplay);
        expect_true(loaded.musicVolume == MIX_MAX_VOLUME &&
                        loaded.effectsVolume == MIX_MAX_VOLUME &&
                        loaded.vsync && loaded.screenShake,
                    "Reset Defaults persists GameSettings");
        expect_true(loadedDisplay.windowWidth == WIDTH &&
                        loadedDisplay.windowHeight == HEIGHT &&
                        !loadedDisplay.fullscreen,
                    "Reset Defaults persists windowed DisplaySettings");
        free(game);
    }

    remove_preference_file("settings.ini");
    remove_preference_file("display.ini");
    clear_preference_override();
    remove_test_directory(testDirectory);
    SDL_Quit();

    if (failures == 0) printf("SETTINGS TEST: ALL PASS\n");
    return failures == 0 ? 0 : 1;
}
