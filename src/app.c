#include "app.h"
#include "audio_assets.h"
#include "display.h"
#include "input_snapshot.h"
#include "settings.h"

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

    if (game)
    {
        // 1. stop active playback before freeing anything it might reference
        Mix_HaltChannel(-1);
        Mix_HaltMusic();

        // 2. gameplay-owned dynamic allocations
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            removeBullet(game, i);
            removeSecondBullet(game, i);
        }

        // 3. generated (HUD) textures -- not part of any asset group, kept inline
        destroy_texture(&game->label);
        destroy_texture(&game->labelMultiplayer);

        // mode-owned asset groups, delegated (see docs/game-session-lifecycle.md)
        arcade_assets_unload(game);
        runner_assets_unload(game);
        shared_assets_unload(game);
        audio_assets_unload_arcade(game);
        audio_assets_unload_runner(game);
        audio_assets_unload_shared(game);

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

        destroy_texture(&game->secondPlayer.sheetTextureIdle);
        destroy_texture(&game->secondPlayer.sheetTextureRun);
        destroy_texture(&game->secondPlayer.sheetTextureJump);

        if (game->app.controller)
        {
            SDL_GameControllerClose(game->app.controller);
            game->app.controller = NULL;
            game->app.controllerJumpHeldLastFrame = false;
        }
    }

    // 6. renderer
    if (outRenderer && *outRenderer)
    {
        SDL_DestroyRenderer(*outRenderer);
        *outRenderer = NULL;
    }
    if (game)
    {
        game->app.renderer = NULL;
    }

    // 7. persist the windowed display preference before destroying its window
    if (outWindow && *outWindow)
    {
        display_capture_window_settings(game);
        (void)settings_save(&game->app.settings);
        if (game && !display_settings_save(&game->app.display))
        {
            fprintf(stderr, "app_shutdown: could not save display settings\n");
        }
        SDL_DestroyWindow(*outWindow);
        *outWindow = NULL;
    }
    if (game)
    {
        game->app.window = NULL;
    }

    // 8. audio + extension subsystems (each is documented safe to call even
    // if the matching init never ran or already ran once this process)
    Mix_CloseAudio();
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

    srandom((unsigned int)time(NULL));

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

    *outRenderer = SDL_CreateRenderer(*outWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
        fprintf(stderr, "app_init: Mix_OpenAudio failed: %s\n", Mix_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    if (!audio_assets_load_shared(game))
    {
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    (void)SDL_RenderSetVSync(*outRenderer, game->app.settings.vsync ? 1 : 0);
    if (!load_font("./resource/text/Fonts/crazy-pixel.ttf", 32, &game->font))
    {
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    settings_apply_audio(&game->app.settings);
    input_controller_open_first(&game->app);

    return true;
}
