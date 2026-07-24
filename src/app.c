#include "app.h"
#include "audio_assets.h"
#include "display.h"
#include "input_snapshot.h"
#include "settings.h"

#include <errno.h>
#include <inttypes.h>

static unsigned int application_seed(void)
{
    const char *configuredSeed = SDL_getenv("ENDGAME_SEED");
    if (configuredSeed != NULL && configuredSeed[0] != '\0')
    {
        errno = 0;
        char *end = NULL;
        const uintmax_t parsed = strtoumax(configuredSeed, &end, 10);
        if (errno == 0 && end != configuredSeed && end != NULL && end[0] == '\0' &&
            parsed <= (uintmax_t)UINT_MAX)
        {
            const unsigned int seed = (unsigned int)parsed;
            printf("[seed] %u (ENDGAME_SEED)\n", seed);
            return seed;
        }
        fprintf(stderr, "app_init: invalid ENDGAME_SEED '%s'; using wall-clock seed\n",
                configuredSeed);
    }

    const unsigned int seed = (unsigned int)time(NULL);
    printf("[seed] %u\n", seed);
    return seed;
}

static SDL_Renderer *create_renderer(SDL_Window *window, bool vsync)
{
    const Uint32 acceleratedFlags = SDL_RENDERER_ACCELERATED |
                                    (vsync ? SDL_RENDERER_PRESENTVSYNC : 0U);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, acceleratedFlags);
    if (renderer != NULL)
    {
        return renderer;
    }

    if (vsync)
    {
        fprintf(stderr, "app_init: accelerated VSync renderer unavailable (%s); "
                        "trying accelerated renderer without a creation-time VSync requirement\n",
                SDL_GetError());
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer != NULL)
        {
            return renderer;
        }
    }

    fprintf(stderr, "app_init: accelerated renderer unavailable (%s); trying software renderer\n",
            SDL_GetError());
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    return renderer;
}

void destroy_texture(SDL_Texture **tex)
{
    if (tex && *tex)
    {
        SDL_DestroyTexture(*tex);
        *tex = NULL;
    }
}

void app_shutdown(GameState **outGame, SDL_Window **outWindow, SDL_Renderer **outRenderer)
{
    GameState *game = outGame ? *outGame : NULL;
    const bool audioAvailable = audio_assets_output_available();

    if (game)
    {
        // 1. stop active playback before freeing anything it might reference
        if (audioAvailable)
        {
            Mix_HaltChannel(-1);
            Mix_HaltMusic();
        }

        // 2. gameplay-owned dynamic allocations
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            removeBullet(game, i);
            removeSecondBullet(game, i);
        }

        // 3. generated (HUD) textures -- not part of any asset group, kept inline
        destroy_texture(&game->label);
        destroy_texture(&game->labelMultiplayer);
        destroy_texture(&game->scoreLabel);
        destroy_texture(&game->killsLabel);

        // mode-owned asset groups, delegated (see docs/game-session-lifecycle.md)
        arcade_assets_unload(game);
        runner_assets_unload(game);
        shared_assets_unload(game);
        audio_assets_unload_arcade(game);
        audio_assets_unload_runner(game);
        audio_assets_unload_shared(game);
        if (game->font)
        {
            TTF_CloseFont(game->font);
            game->font = NULL;
        }

        // manFrames[12] is fully covered by the calls above: index 0 by
        // shared_assets_unload() (both modes load the identical file, see
        // docs/game-session-lifecycle.md) and indices 1-11 by
        // runner_assets_unload() -- no centralized destroy needed here.

        // menu0/1/2 are never assigned by either mode's asset-load (see
        // docs/game-session-lifecycle.md) but kept here for completeness --
        // null-safe, harmless if always NULL.
        destroy_texture(&game->menu0);
        destroy_texture(&game->menu1);
        destroy_texture(&game->menu2);

        destroy_texture(&game->man.sheetTextureIdle);
        destroy_texture(&game->man.sheetTextureRun2);
        destroy_texture(&game->man.sheetTextureJump2);

        if (game->app.controller)
        {
            SDL_GameControllerClose(game->app.controller);
            game->app.controller = NULL;
            game->app.controllerJumpHeldLastFrame = false;
        }
    }

    // 6. renderer. The explicit output container and GameState normally
    // alias the same handle; fall back to the state-owned handle when callers
    // intentionally omit the optional container.
    SDL_Renderer *stateRenderer = game ? game->app.renderer : NULL;
    SDL_Renderer *outputRenderer = outRenderer ? *outRenderer : NULL;
    if (stateRenderer && outputRenderer && outputRenderer != stateRenderer)
    {
        fprintf(stderr,
                "app_shutdown: renderer output did not match GameState; "
                "destroying both owned renderer handles\n");
    }
    if (stateRenderer)
    {
        SDL_DestroyRenderer(stateRenderer);
    }
    if (outputRenderer && outputRenderer != stateRenderer)
    {
        SDL_DestroyRenderer(outputRenderer);
    }
    if (outRenderer)
    {
        *outRenderer = NULL;
    }
    if (game)
    {
        game->app.renderer = NULL;
    }

    // 7. persist the windowed display preference before destroying its
    // window. As with the renderer, the GameState handle is authoritative
    // when the optional output container is omitted.
    SDL_Window *stateWindow = game ? game->app.window : NULL;
    SDL_Window *outputWindow = outWindow ? *outWindow : NULL;
    if (stateWindow && outputWindow && outputWindow != stateWindow)
    {
        fprintf(stderr,
                "app_shutdown: window output did not match GameState; "
                "destroying both owned window handles\n");
    }
    if (stateWindow)
    {
        display_capture_window_settings(game);
        if (game && !settings_save(&game->app.settings))
        {
            fprintf(stderr, "app_shutdown: could not save game settings\n");
        }
        if (game && !display_settings_save(&game->app.display))
        {
            fprintf(stderr, "app_shutdown: could not save display settings\n");
        }
        SDL_DestroyWindow(stateWindow);
    }
    if (outputWindow && outputWindow != stateWindow)
    {
        SDL_DestroyWindow(outputWindow);
    }
    if (outWindow)
    {
        *outWindow = NULL;
    }
    if (game)
    {
        game->app.window = NULL;
    }

    // 8. audio + extension subsystems (each is documented safe to call even
    // if the matching init never ran or already ran once this process)
    if (audioAvailable)
    {
        Mix_CloseAudio();
    }
    Mix_Quit();
    IMG_Quit();
    TTF_Quit();

    // 9. SDL itself
    SDL_Quit();

    // 10. application state
    if (outGame && *outGame)
    {
        free(*outGame);
        *outGame = NULL;
    }
}

bool app_init(GameState **outGame, SDL_Window **outWindow, SDL_Renderer **outRenderer)
{
    if (outGame == NULL || outWindow == NULL || outRenderer == NULL)
    {
        fprintf(stderr, "app_init: output pointers must not be NULL\n");
        return false;
    }
    if (*outGame != NULL || *outWindow != NULL || *outRenderer != NULL)
    {
        fprintf(stderr,
                "app_init: output containers must be empty to preserve existing ownership\n");
        return false;
    }

    *outGame = NULL;
    *outWindow = NULL;
    *outRenderer = NULL;

    GameState *game = (GameState *)calloc(1, sizeof(GameState));
    if (!game)
    {
        fprintf(stderr, "app_init: failed to allocate GameState\n");
        return false;
    }
    *outGame = game;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        fprintf(stderr, "app_init: SDL_Init failed: %s\n", SDL_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }

    const int wantedImgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    int gotImgFlags = IMG_Init(wantedImgFlags);
    if ((gotImgFlags & wantedImgFlags) != wantedImgFlags)
    {
        fprintf(stderr, "app_init: IMG_Init failed: %s\n", IMG_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }

    display_settings_defaults(&game->app.display);
    display_settings_load(&game->app.display);
    settings_load(&game->app.settings);
    const Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
        (game->app.display.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0U);

    *outWindow = SDL_CreateWindow("Endgame",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   game->app.display.windowWidth,
                                   game->app.display.windowHeight,
                                   windowFlags);
    if (!*outWindow)
    {
        fprintf(stderr, "app_init: SDL_CreateWindow failed: %s\n", SDL_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    game->app.window = *outWindow;
    const Uint32 activeFullscreenFlags =
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP;
    game->app.display.fullscreen =
        (SDL_GetWindowFlags(*outWindow) & activeFullscreenFlags) != 0U;

    *outRenderer = create_renderer(*outWindow, game->app.settings.vsync);
    if (!*outRenderer)
    {
        fprintf(stderr, "app_init: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    game->app.renderer = *outRenderer;
    if (!display_configure_renderer(*outRenderer))
    {
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }

    if (TTF_Init() != 0)
    {
        fprintf(stderr, "app_init: TTF_Init failed: %s\n", TTF_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) != 0)
    {
        fprintf(stderr, "app_init: audio unavailable; continuing silently: %s\n", Mix_GetError());
    }
    if (!audio_assets_load_shared(game))
    {
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    const bool requestedVsync = game->app.settings.vsync;
    (void)display_set_vsync(game, requestedVsync);
    if (!load_font("./resource/text/Fonts/crazy-pixel.ttf", 32, &game->font))
    {
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu0.png", &game->menu0))
    {
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    if (audio_assets_output_available())
    {
        settings_apply_audio(&game->app.settings);
    }
    input_controller_open_first(&game->app);
    // Seed only after SDL and its extension libraries have initialized. Some
    // platform backends use the C random stream internally; seeding earlier
    // would make the first gameplay value depend on backend initialization.
    srandom(application_seed());

    return true;
}
