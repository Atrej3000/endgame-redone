#include "app.h"
#include "audio_assets.h"
#include "preferences.h"
#include "scene.h"

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                     \
    {                                                                      \
        if (condition)                                                     \
        {                                                                  \
            printf("PRE-PHASE35 LIFECYCLE TEST: PASS - %s\n", description); \
        }                                                                  \
        else                                                               \
        {                                                                  \
            fprintf(stderr, "PRE-PHASE35 LIFECYCLE TEST: FAIL - %s\n",      \
                    description);                                          \
            failures++;                                                    \
        }                                                                  \
    } while (0)

typedef struct EnvironmentSnapshot
{
    const char *name;
    bool wasSet;
    bool valid;
    char *value;
} EnvironmentSnapshot;

static EnvironmentSnapshot capture_environment(const char *name)
{
    const char *value = SDL_getenv(name);
    EnvironmentSnapshot snapshot = {name, value != NULL, true, NULL};
    if (value == NULL)
    {
        return snapshot;
    }
    const size_t length = strlen(value);
    snapshot.value = malloc(length + 1U);
    if (snapshot.value == NULL)
    {
        snapshot.valid = false;
        return snapshot;
    }
    memcpy(snapshot.value, value, length + 1U);
    return snapshot;
}

static bool restore_environment(EnvironmentSnapshot *snapshot)
{
    if (snapshot == NULL || !snapshot->valid)
    {
        return false;
    }
    int result = 0;
    if (snapshot->wasSet)
    {
        result = SDL_setenv(snapshot->name, snapshot->value, 1);
    }
    else
    {
#ifdef _WIN32
        // SDL2 has no SDL_unsetenv. An empty value is the supported Windows
        // equivalent for these opt-in overrides (all consumers require
        // both a non-NULL pointer and a non-empty string).
        result = SDL_setenv(snapshot->name, "", 1);
#else
        result = unsetenv(snapshot->name);
#endif
    }
    const char *restored = SDL_getenv(snapshot->name);
    const bool matches =
        result == 0 &&
        (snapshot->wasSet
             ? restored != NULL && strcmp(restored, snapshot->value) == 0
             : restored == NULL || restored[0] == '\0');
    free(snapshot->value);
    snapshot->value = NULL;
    return matches;
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

static void remove_preference_file(const char *filename)
{
    char *path = preferences_file_path(filename);
    if (path)
    {
        (void)remove(path);
        free(path);
    }
}

static bool input_is_clear(const InputState *input)
{
    return !input->moveLeftPlayer1 && !input->moveRightPlayer1 &&
           !input->jumpHeldPlayer1 && !input->shootHeldPlayer1 &&
           !input->moveLeftPlayer2 && !input->moveRightPlayer2 &&
           !input->jumpHeldPlayer2 && !input->shootHeldPlayer2 &&
           input->jumpBufferTicksPlayer1 == 0 &&
           input->jumpBufferTicksPlayer2 == 0;
}

static void test_scene_audio_and_input(GameState *game)
{
    load_menu0(game);
    CHECK("main-menu music starts", Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0);

    app_change_scene(game, APP_SCENE_ARCADE_MENU);
    CHECK("Arcade menu loads without redirecting", game->app.scene == APP_SCENE_ARCADE_MENU);
    CHECK("Arcade music is preloaded", game->audio.arcadeMusic != NULL);
    app_change_scene(game, APP_SCENE_ARCADE_GAME);
    CHECK("Arcade gameplay music is playing", Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0);
    app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
    CHECK("Arcade pause pauses music", Mix_PausedMusic() != 0);
    app_change_scene(game, APP_SCENE_ARCADE_GAME);
    CHECK("Arcade resume resumes music", Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0);
    app_change_scene(game, APP_SCENE_MAIN_MENU);
    CHECK("leaving gameplay restores menu music", Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0);

    app_change_scene(game, APP_SCENE_RUNNER_MENU);
    CHECK("Runner menu loads without redirecting", game->app.scene == APP_SCENE_RUNNER_MENU);
    CHECK("Runner music is preloaded", game->audio.runnerMusic != NULL);
    app_change_scene(game, APP_SCENE_RUNNER_GAME);
    CHECK("Runner gameplay music is playing", Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0);
    app_change_scene(game, APP_SCENE_RUNNER_PAUSE);
    CHECK("Runner pause pauses music", Mix_PausedMusic() != 0);
    app_change_scene(game, APP_SCENE_MAIN_MENU);
    CHECK("pause-to-main replaces paused gameplay music",
          Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0);

    game->input.moveLeftPlayer1 = true;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    game->app.controllerJumpHeldLastFrame = true;
    SDL_Event keyEvent;
    SDL_zero(keyEvent);
    keyEvent.type = SDL_KEYDOWN;
    keyEvent.key.keysym.sym = SDLK_q;
    (void)SDL_PushEvent(&keyEvent);
    keyEvent.type = SDL_KEYUP;
    (void)SDL_PushEvent(&keyEvent);
    app_change_scene(game, APP_SCENE_SETTINGS);
    CHECK("scene transition clears held and buffered input", input_is_clear(&game->input));
    CHECK("scene transition clears controller edge state",
          !game->app.controllerJumpHeldLastFrame);
    SDL_Event remainingKey;
    CHECK("scene transition flushes queued key edges only",
          SDL_PeepEvents(&remainingKey, 1, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYUP) == 0);
}

static void test_menu_failure_returns(GameState *game)
{
    SDL_FlushEvent(SDL_QUIT);
    SDL_Renderer *renderer = game->app.renderer;
    destroy_texture(&game->menu1);
    game->app.renderer = NULL;
    game->app.scene = APP_SCENE_MAIN_MENU;
    load_menu1(game);
    CHECK("menu texture failure requests a clean quit instead of exiting",
          game->app.scene == APP_SCENE_QUIT);
    SDL_Event queuedQuit;
    CHECK("menu failure does not depend on a queued quit workaround",
          SDL_PeepEvents(&queuedQuit, 1, SDL_PEEKEVENT,
                         SDL_QUIT, SDL_QUIT) == 0);
    game->app.renderer = renderer;
}

static void test_asset_group_recovery(GameState *game)
{
    destroy_texture(&game->bulletTexture);
    game->assetFlags.arcadeAssetsLoaded = true;
    game->app.scene = APP_SCENE_MAIN_MENU; // test-only precondition
    app_change_scene(game, APP_SCENE_ARCADE_MENU);
    CHECK("Arcade loaded-flag mismatch rebuilds the complete asset group",
          game->app.scene == APP_SCENE_ARCADE_MENU &&
              game->bulletTexture != NULL &&
              game->bossTexture != NULL);

    destroy_texture(&game->star);
    game->assetFlags.runnerAssetsLoaded = true;
    game->app.scene = APP_SCENE_MAIN_MENU; // test-only precondition
    app_change_scene(game, APP_SCENE_RUNNER_MENU);
    CHECK("Runner loaded-flag mismatch rebuilds the complete asset group",
          game->app.scene == APP_SCENE_RUNNER_MENU &&
              game->star != NULL &&
              game->fon != NULL);

    destroy_texture(&game->mult);
    game->assetFlags.sharedAssetsLoaded = true;
    CHECK("shared loaded-flag mismatch is recovered through a mode retry",
          arcade_assets_load(game) && game->mult != NULL &&
              game->man.sheetTextureIdle != NULL);

    Mix_HaltMusic();
    if (game->audio.select) Mix_FreeChunk(game->audio.select);
    game->audio.select = NULL;
    game->assetFlags.sharedAudioAssetsLoaded = true;
    CHECK("shared-audio flag mismatch rebuilds missing audio safely",
          arcade_assets_load(game) && game->audio.select != NULL &&
              game->audio.menuMusic != NULL);

    CHECK("public asset lifecycle rejects null state safely",
          !arcade_assets_load(NULL) && !runner_assets_load(NULL));
    arcade_assets_unload(NULL);
    runner_assets_unload(NULL);
    shared_assets_unload(NULL);
}

static void test_cross_mode_menu_entry(GameState *game)
{
    destroy_texture(&game->menu2);
    game->menu_status = 99;
    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only precondition
    app_change_scene(game, APP_SCENE_RUNNER_MENU);
    CHECK("direct Arcade-to-Runner entry initializes the Runner menu",
          game->app.scene == APP_SCENE_RUNNER_MENU &&
              game->menu2 != NULL && game->menu_status == 0);

    destroy_texture(&game->menu1);
    game->menu_status = 99;
    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition
    app_change_scene(game, APP_SCENE_ARCADE_MENU);
    CHECK("direct Runner-to-Arcade entry initializes the Arcade menu",
          game->app.scene == APP_SCENE_ARCADE_MENU &&
              game->menu1 != NULL && game->menu_status == 0);
}

int main(void)
{
    EnvironmentSnapshot audioDriver =
        capture_environment("SDL_AUDIODRIVER");
    EnvironmentSnapshot videoDriver =
        capture_environment("SDL_VIDEODRIVER");
    EnvironmentSnapshot seedOverride =
        capture_environment("ENDGAME_SEED");
    EnvironmentSnapshot preferenceOverride =
        capture_environment("ENDGAME_PREF_DIR");
    char originalDirectory[4096] = {0};
    const bool haveOriginalDirectory =
        getcwd(originalDirectory, sizeof(originalDirectory)) != NULL;
    char *executableDirectory = NULL;
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    char preferenceDirectory[4096] = {0};
    bool preferenceDirectoryCreated = false;
    bool preferenceOverrideSet = false;

    CHECK("capture test environment without mutating it",
          audioDriver.valid && videoDriver.valid && seedOverride.valid &&
              preferenceOverride.valid);
    if (!audioDriver.valid || !videoDriver.valid || !seedOverride.valid ||
        !preferenceOverride.valid)
    {
        goto cleanup;
    }
    CHECK("capture original working directory", haveOriginalDirectory);
    if (!haveOriginalDirectory)
    {
        goto cleanup;
    }
    const int preferenceResult = snprintf(
        preferenceDirectory, sizeof(preferenceDirectory),
        "%s/pre_phase35_lifecycle_%ld", originalDirectory,
        test_process_id());
    preferenceDirectoryCreated =
        preferenceResult >= 0 &&
        (size_t)preferenceResult < sizeof(preferenceDirectory) &&
        make_test_directory(preferenceDirectory);
    preferenceOverrideSet =
        preferenceDirectoryCreated &&
        SDL_setenv("ENDGAME_PREF_DIR", preferenceDirectory, 1) == 0;
    CHECK("create isolated lifecycle preference directory",
          preferenceDirectoryCreated && preferenceOverrideSet);
    if (!preferenceOverrideSet)
    {
        goto cleanup;
    }

    executableDirectory = SDL_GetBasePath();
    CHECK("SDL exposes the executable directory", executableDirectory != NULL);
    if (executableDirectory == NULL)
    {
        goto cleanup;
    }
    CHECK("change working directory away from repository root",
          chdir(executableDirectory) == 0);
    SDL_free(executableDirectory);
    executableDirectory = NULL;

    (void)SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    (void)SDL_setenv("ENDGAME_SEED", "424242", 1);

    CHECK("app_init resolves assets from executable parent when CWD differs",
          app_init(&game, &window, &renderer));
    if (game == NULL)
    {
        goto cleanup;
    }
    CHECK("startup preloads the required main-menu texture", game->menu0 != NULL);
    GameState *const initializedGame = game;
    SDL_Window *const initializedWindow = window;
    SDL_Renderer *const initializedRenderer = renderer;
    CHECK("app_init refuses occupied outputs without losing their ownership",
          !app_init(&game, &window, &renderer) &&
              game == initializedGame && window == initializedWindow &&
              renderer == initializedRenderer);
    const long firstRandom = random();
    test_scene_audio_and_input(game);
    test_asset_group_recovery(game);
    test_cross_mode_menu_entry(game);
    test_menu_failure_returns(game);
    app_shutdown(&game, &window, &renderer);

    CHECK("second deterministic app_init succeeds", app_init(&game, &window, &renderer));
    if (game != NULL)
    {
        const long secondRandom = random();
        CHECK("ENDGAME_SEED reproduces the first random value", firstRandom == secondRandom);
        app_shutdown(&game, NULL, NULL);
        window = NULL;
        renderer = NULL;
        CHECK("shutdown falls back to GameState-owned window and renderer handles",
              game == NULL && SDL_WasInit(0U) == 0U);
    }

    CHECK("mismatched-handle app_init succeeds",
          app_init(&game, &window, &renderer));
    if (game != NULL)
    {
        SDL_Window *const originalWindow = window;
        SDL_Renderer *const originalRenderer = renderer;
        SDL_Window *extraWindow = SDL_CreateWindow(
            "lifecycle mismatch", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, 64, 64, SDL_WINDOW_HIDDEN);
        SDL_Renderer *extraRenderer =
            extraWindow ? SDL_CreateRenderer(
                              extraWindow, -1, SDL_RENDERER_SOFTWARE)
                        : NULL;
        CHECK("distinct shutdown-handle fixture initializes",
              extraWindow != NULL && extraRenderer != NULL);
        if (extraWindow != NULL && extraRenderer != NULL)
        {
            window = extraWindow;
            renderer = extraRenderer;
            app_shutdown(&game, &window, &renderer);
            CHECK("shutdown destroys and nulls both distinct handle pairs",
                  game == NULL && window == NULL && renderer == NULL &&
                      SDL_WasInit(0U) == 0U);
        }
        else
        {
            if (extraRenderer) SDL_DestroyRenderer(extraRenderer);
            if (extraWindow) SDL_DestroyWindow(extraWindow);
            window = originalWindow;
            renderer = originalRenderer;
            app_shutdown(&game, &window, &renderer);
        }
    }

    (void)SDL_setenv("SDL_VIDEODRIVER",
                     "definitely-not-a-video-driver", 1);
    CHECK("failed app_init cleans every partial output",
          !app_init(&game, &window, &renderer) &&
              game == NULL && window == NULL && renderer == NULL &&
              SDL_WasInit(0U) == 0U);

    (void)SDL_setenv("SDL_AUDIODRIVER", "definitely-not-a-driver", 1);
    (void)SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    CHECK("app_init falls back to software rendering and silent audio",
          app_init(&game, &window, &renderer));
    if (game != NULL)
    {
        SDL_RendererInfo info;
        SDL_zero(info);
        CHECK("fallback renderer reports software capability",
              SDL_GetRendererInfo(renderer, &info) == 0 &&
                  (info.flags & SDL_RENDERER_SOFTWARE) != 0U);
        CHECK("silent audio lifecycle is intentional and pointer-safe",
              game->assetFlags.sharedAudioAssetsLoaded &&
                  game->audio.menuMusic == NULL && game->audio.select == NULL);
        load_menu0(game);
        app_change_scene(game, APP_SCENE_ARCADE_MENU);
        CHECK("mode assets remain usable without an audio device",
              game->app.scene == APP_SCENE_ARCADE_MENU &&
                  game->assetFlags.arcadeAssetsLoaded);
        app_shutdown(&game, &window, &renderer);
    }

cleanup:
    SDL_free(executableDirectory);
    if (game != NULL || window != NULL || renderer != NULL)
    {
        app_shutdown(&game, &window, &renderer);
    }
    if (haveOriginalDirectory)
    {
        CHECK("restore original working directory",
              chdir(originalDirectory) == 0);
    }
    if (preferenceDirectoryCreated)
    {
        if (preferenceOverrideSet)
        {
            remove_preference_file("settings.ini");
            remove_preference_file("display.ini");
            remove_preference_file("leaderboard.ini");
        }
        remove_test_directory(preferenceDirectory);
    }
    const bool audioRestored = restore_environment(&audioDriver);
    const bool videoRestored = restore_environment(&videoDriver);
    const bool seedRestored = restore_environment(&seedOverride);
    const bool preferenceRestored =
        restore_environment(&preferenceOverride);
    CHECK("restore SDL and ENDGAME environment overrides",
          audioRestored && videoRestored && seedRestored &&
              preferenceRestored);

    if (failures == 0)
    {
        printf("PRE-PHASE35 LIFECYCLE TEST: ALL PASS\n");
        return 0;
    }
    fprintf(stderr, "PRE-PHASE35 LIFECYCLE TEST: %d FAILURE(S)\n", failures);
    return 1;
}
