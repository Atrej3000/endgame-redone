#include "atomic_file.h"
#include "display.h"
#include "preferences.h"

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
        fprintf(stderr, "DISPLAY SETTINGS TEST: FAIL - %s\n", message);
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

static bool write_display_fixture(const char *contents)
{
    char *path = preferences_file_path("display.ini");
    if (!path) return false;
    const bool written = atomic_write_text_file(path, contents, strlen(contents));
    free(path);
    return written;
}

static char *load_display_file(size_t *length)
{
    char *path = preferences_file_path("display.ini");
    if (!path) return NULL;
    char *contents = (char *)SDL_LoadFile(path, length);
    free(path);
    return contents;
}

int main(void)
{
    DisplaySettings settings;
    display_settings_defaults(&settings);
    expect_true(settings.windowWidth == WIDTH && settings.windowHeight == HEIGHT,
                "defaults retain the logical 1280x720 presentation size");
    expect_true(!settings.fullscreen, "default mode is windowed");

    settings.windowWidth = 1600;
    settings.windowHeight = 900;
    settings.fullscreen = true;
    display_settings_sanitize(&settings);
    expect_true(settings.windowWidth == 1600 && settings.windowHeight == 900,
                "valid persisted dimensions are retained");
    expect_true(settings.fullscreen, "fullscreen preference is retained");

    settings.windowWidth = 1;
    settings.windowHeight = DISPLAY_MAX_HEIGHT + 1;
    display_settings_sanitize(&settings);
    expect_true(settings.windowWidth == WIDTH && settings.windowHeight == HEIGHT,
                "invalid persisted dimensions fall back to the logical default");

    GameState missingRuntime = {0};
    missingRuntime.app.display.fullscreen = true;
    missingRuntime.app.settings.vsync = false;
    expect_true(!display_toggle_fullscreen(&missingRuntime) &&
                    missingRuntime.app.display.fullscreen,
                "fullscreen failure leaves the recorded runtime state unchanged");
    expect_true(!display_set_vsync(&missingRuntime, true) &&
                    !missingRuntime.app.settings.vsync,
                "VSync failure leaves the recorded runtime state unchanged");

    char testDirectory[128];
    const int directoryResult = snprintf(testDirectory, sizeof(testDirectory),
                                         "pre_phase35_display_%ld",
                                         test_process_id());
    if (directoryResult < 0 || (size_t)directoryResult >= sizeof(testDirectory) ||
        !make_test_directory(testDirectory) ||
        !set_preference_override(testDirectory))
    {
        fprintf(stderr,
                "DISPLAY SETTINGS TEST: FAIL - isolated preference directory setup\n");
        return 1;
    }

    DisplaySettings persisted = {1600, 900, true};
    expect_true(display_settings_save(&persisted),
                "display save succeeds through ENDGAME_PREF_DIR");
    DisplaySettings loaded;
    display_settings_load(&loaded);
    expect_true(loaded.windowWidth == 1600 && loaded.windowHeight == 900 &&
                    loaded.fullscreen,
                "display settings roundtrip through ENDGAME_PREF_DIR");

    size_t priorLength = 0U;
    char *priorContents = load_display_file(&priorLength);
    char *displayPath = preferences_file_path("display.ini");
    const bool blockedTemporaryPath =
        displayPath != NULL &&
        set_atomic_temp_blockers(displayPath, true);
    expect_true(blockedTemporaryPath,
                "atomic-write failure fixture blocks the display temp path");
    persisted.windowWidth = 1920;
    expect_true(blockedTemporaryPath && !display_settings_save(&persisted),
                "display save reports a failed atomic temp write");
    size_t afterLength = 0U;
    char *afterContents = load_display_file(&afterLength);
    expect_true(priorContents != NULL && afterContents != NULL &&
                    priorLength == afterLength &&
                    memcmp(priorContents, afterContents, priorLength) == 0,
                "failed atomic temp write preserves the prior display file");
    SDL_free(priorContents);
    SDL_free(afterContents);
    if (displayPath) (void)set_atomic_temp_blockers(displayPath, false);
    free(displayPath);

    expect_true(write_display_fixture(
                    "version=2\nwidth=1920\nheight=1080\nfullscreen=1\n"),
                "unsupported display-version fixture is written");
    display_settings_load(&loaded);
    expect_true(loaded.windowWidth == WIDTH && loaded.windowHeight == HEIGHT &&
                    !loaded.fullscreen,
                "unsupported display versions fall back to defaults");

    expect_true(write_display_fixture(
                    "version=1\nwidth=1600oops\n"
                    "height=999999999999999999999999\n"
                    "malformed middle line\nheight=900\nfullscreen=1\n"),
                "malformed display-line fixture is written");
    display_settings_load(&loaded);
    expect_true(loaded.windowWidth == WIDTH && loaded.windowHeight == 900 &&
                    loaded.fullscreen,
                "malformed display lines are ignored while later values load");

    expect_true(write_display_fixture(
                    "version=1\nwidth=1600\nwidth=1920\nheight=900\n"),
                "duplicate display-key fixture is written");
    display_settings_load(&loaded);
    expect_true(loaded.windowWidth == WIDTH &&
                    loaded.windowHeight == HEIGHT && !loaded.fullscreen,
                "duplicate known display keys reject the ambiguous file");

    expect_true(write_display_fixture(
                    "version=1\nwidth=1600\nheight=900\n"
                    "fullscreen=999999999999999999999999\nfuture=1\n"),
                "overflow and unknown display-key fixture is written");
    display_settings_load(&loaded);
    expect_true(loaded.windowWidth == 1600 && loaded.windowHeight == 900 &&
                    !loaded.fullscreen,
                "overflow display integers are ignored safely and unknown keys are tolerated");

    const size_t oversizedLength = 5U * 1024U;
    char *oversized = (char *)malloc(oversizedLength + 1U);
    expect_true(oversized != NULL, "oversized display fixture allocates");
    if (oversized)
    {
        memset(oversized, 'x', oversizedLength);
        oversized[oversizedLength] = '\0';
        expect_true(write_display_fixture(oversized),
                    "oversized display fixture is written");
        display_settings_load(&loaded);
        expect_true(loaded.windowWidth == WIDTH &&
                        loaded.windowHeight == HEIGHT && !loaded.fullscreen,
                    "oversized display files are rejected before parser allocation");
        free(oversized);
    }

    expect_true(write_display_fixture(
                    "width=1600\nheight=900\nfullscreen=1\n"),
                "missing-version display fixture is written");
    display_settings_load(&loaded);
    expect_true(loaded.windowWidth == WIDTH && loaded.windowHeight == HEIGHT &&
                    !loaded.fullscreen,
                "display files without version=1 fall back to defaults");

    char *cleanupPath = preferences_file_path("display.ini");
    if (cleanupPath)
    {
        (void)remove(cleanupPath);
        free(cleanupPath);
    }
    clear_preference_override();
    remove_test_directory(testDirectory);

    if (failures == 0)
    {
        printf("DISPLAY SETTINGS TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
