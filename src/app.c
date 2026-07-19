#include "app.h"

void destroy_texture(SDL_Texture **tex)
{
    if (tex && *tex)
    {
        SDL_DestroyTexture(*tex);
        *tex = NULL;
    }
}

void free_chunk(Mix_Chunk **chunk)
{
    if (chunk && *chunk)
    {
        Mix_FreeChunk(*chunk);
        *chunk = NULL;
    }
}

void free_music(Mix_Music **music)
{
    if (music && *music)
    {
        Mix_FreeMusic(*music);
        *music = NULL;
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

        // 4. chunks and music not owned by any asset group (loaded lazily
        // elsewhere -- menu_events.c/pause_events.c/processEvents.c/
        // process.c/load_menu.c -- out of scope for the asset-group split)
        free_chunk(&game->jumpSound);
        free_chunk(&game->kickSound);
        free_chunk(&game->select);
        free_chunk(&game->shootSound);
        free_chunk(&game->damageSound);
        free_music(&game->menuMus);
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

    // 7. window
    if (outWindow && *outWindow)
    {
        SDL_DestroyWindow(*outWindow);
        *outWindow = NULL;
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

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
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

    *outWindow = SDL_CreateWindow("Game Window",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   WIDTH,
                                   HEIGHT,
                                   0);
    if (!*outWindow)
    {
        fprintf(stderr, "app_init: SDL_CreateWindow failed: %s\n", SDL_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }

    *outRenderer = SDL_CreateRenderer(*outWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*outRenderer)
    {
        fprintf(stderr, "app_init: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        app_shutdown(outGame, outWindow, outRenderer);
        return false;
    }
    game->app.renderer = *outRenderer;

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

    return true;
}
