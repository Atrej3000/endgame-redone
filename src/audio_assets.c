#include "audio_assets.h"

static void unload_chunk(Mix_Chunk **chunk)
{
    if (chunk && *chunk)
    {
        Mix_FreeChunk(*chunk);
        *chunk = NULL;
    }
}

static void unload_music(Mix_Music **music)
{
    if (music && *music)
    {
        Mix_FreeMusic(*music);
        *music = NULL;
    }
}

bool audio_assets_load_shared(GameState *game)
{
    if (!game) return false;
    if (game->assetFlags.sharedAudioAssetsLoaded) return true;

    const bool ok = load_music("resource/sounds/menuMus.mp3", &game->audio.menuMusic) &&
                    load_chunk("resource/sounds/select.wav", &game->audio.select) &&
                    load_chunk("resource/sounds/jump.wav", &game->audio.jump);
    if (!ok)
    {
        audio_assets_unload_shared(game);
        return false;
    }
    game->assetFlags.sharedAudioAssetsLoaded = true;
    return true;
}

bool audio_assets_load_arcade(GameState *game)
{
    if (!game || !audio_assets_load_shared(game)) return false;
    if (game->audio.arcadeMusic && game->audio.shoot && game->audio.damage && game->audio.kick) return true;

    const bool ok = load_music("resource/sounds/battleMus.mp3", &game->audio.arcadeMusic) &&
                    load_chunk("resource/sounds/shoot.wav", &game->audio.shoot) &&
                    load_chunk("resource/sounds/damage.wav", &game->audio.damage) &&
                    load_chunk("resource/sounds/kick.wav", &game->audio.kick);
    if (!ok)
    {
        audio_assets_unload_arcade(game);
        return false;
    }
    return true;
}

bool audio_assets_load_runner(GameState *game)
{
    if (!game || !audio_assets_load_shared(game)) return false;
    if (game->audio.runnerMusic) return true;
    if (!load_music("resource/sounds/runnerMus.mp3", &game->audio.runnerMusic))
    {
        audio_assets_unload_runner(game);
        return false;
    }
    return true;
}

void audio_assets_unload_arcade(GameState *game)
{
    if (!game) return;
    unload_music(&game->audio.arcadeMusic);
    unload_chunk(&game->audio.shoot);
    unload_chunk(&game->audio.damage);
    unload_chunk(&game->audio.kick);
}

void audio_assets_unload_runner(GameState *game)
{
    if (game) unload_music(&game->audio.runnerMusic);
}

void audio_assets_unload_shared(GameState *game)
{
    if (!game) return;
    unload_music(&game->audio.menuMusic);
    unload_chunk(&game->audio.select);
    unload_chunk(&game->audio.jump);
    game->assetFlags.sharedAudioAssetsLoaded = false;
}
