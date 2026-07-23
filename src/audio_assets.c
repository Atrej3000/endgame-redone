#include "audio_assets.h"

typedef enum
{
    MUSIC_SCENE_NONE,
    MUSIC_SCENE_MENU,
    MUSIC_SCENE_ARCADE,
    MUSIC_SCENE_RUNNER
} MusicScene;

bool audio_assets_output_available(void)
{
    int frequency = 0;
    Uint16 format = 0;
    int channels = 0;
    return Mix_QuerySpec(&frequency, &format, &channels) != 0;
}

static void unload_chunk(Mix_Chunk **chunk)
{
    if (chunk && *chunk)
    {
        if (audio_assets_output_available())
        {
            Mix_HaltChannel(-1);
        }
        Mix_FreeChunk(*chunk);
        *chunk = NULL;
    }
}

static void unload_music(Mix_Music **music)
{
    if (music && *music)
    {
        if (audio_assets_output_available())
        {
            Mix_HaltMusic();
        }
        Mix_FreeMusic(*music);
        *music = NULL;
    }
}

static bool shared_audio_complete(const GameState *game)
{
    return game != NULL && game->audio.menuMusic != NULL &&
           game->audio.select != NULL && game->audio.jump != NULL;
}

bool audio_assets_load_shared(GameState *game)
{
    if (!game) return false;
    if (!audio_assets_output_available())
    {
        // "Loaded" here means the audio lifecycle is ready. A machine with no
        // usable output device intentionally has a ready, silent lifecycle;
        // an open device still requires every asset below to load successfully.
        game->assetFlags.sharedAudioAssetsLoaded = true;
        return true;
    }
    if (shared_audio_complete(game))
    {
        game->assetFlags.sharedAudioAssetsLoaded = true;
        return true;
    }
    audio_assets_unload_shared(game);

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
    if (!audio_assets_output_available()) return true;
    if (game->audio.arcadeMusic && game->audio.shoot && game->audio.damage && game->audio.kick) return true;
    audio_assets_unload_arcade(game);

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
    if (!audio_assets_output_available()) return true;
    if (game->audio.runnerMusic) return true;
    audio_assets_unload_runner(game);
    if (!load_music("resource/sounds/runnerMus.mp3", &game->audio.runnerMusic))
    {
        audio_assets_unload_runner(game);
        return false;
    }
    return true;
}

static MusicScene music_scene_for_app_scene(AppScene scene)
{
    switch (scene)
    {
    case APP_SCENE_ARCADE_GAME:
    case APP_SCENE_ARCADE_PAUSE:
        return MUSIC_SCENE_ARCADE;
    case APP_SCENE_RUNNER_GAME:
    case APP_SCENE_RUNNER_PAUSE:
        return MUSIC_SCENE_RUNNER;
    case APP_SCENE_MAIN_MENU:
    case APP_SCENE_SETTINGS:
    case APP_SCENE_ARCADE_MENU:
    case APP_SCENE_ARCADE_LEADERBOARD:
    case APP_SCENE_RUNNER_MENU:
    case APP_SCENE_RUNNER_LEADERBOARD:
        return MUSIC_SCENE_MENU;
    case APP_SCENE_QUIT:
    default:
        return MUSIC_SCENE_NONE;
    }
}

static Mix_Music *music_for_scene(GameState *game, MusicScene scene)
{
    if (game == NULL)
    {
        return NULL;
    }
    if (scene == MUSIC_SCENE_MENU) return game->audio.menuMusic;
    if (scene == MUSIC_SCENE_ARCADE) return game->audio.arcadeMusic;
    if (scene == MUSIC_SCENE_RUNNER) return game->audio.runnerMusic;
    return NULL;
}

static bool play_music(GameState *game, Mix_Music *music)
{
    if (game == NULL || !audio_assets_output_available())
    {
        return true;
    }
    if (music == NULL)
    {
        fprintf(stderr, "audio: requested scene music is not loaded\n");
        return false;
    }
    if (Mix_PlayMusic(music, -1) != 0)
    {
        fprintf(stderr, "audio: Mix_PlayMusic failed: %s\n", Mix_GetError());
        return false;
    }
    return true;
}

bool audio_assets_play_menu(GameState *game)
{
    if (game == NULL || !audio_assets_output_available())
    {
        return true;
    }
    if (Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0)
    {
        return true;
    }
    return play_music(game, game->audio.menuMusic);
}

void audio_assets_sync_scene(GameState *game, AppScene previousScene, AppScene nextScene)
{
    if (game == NULL || !audio_assets_output_available())
    {
        return;
    }

    if (nextScene == APP_SCENE_ARCADE_PAUSE || nextScene == APP_SCENE_RUNNER_PAUSE)
    {
        if (Mix_PlayingMusic() != 0)
        {
            Mix_PauseMusic();
        }
        return;
    }

    const MusicScene previousMusic = music_scene_for_app_scene(previousScene);
    const MusicScene nextMusic = music_scene_for_app_scene(nextScene);
    if (nextMusic == MUSIC_SCENE_NONE)
    {
        return;
    }

    const bool resumingMatchingGameplay =
        ((previousScene == APP_SCENE_ARCADE_PAUSE && nextScene == APP_SCENE_ARCADE_GAME) ||
         (previousScene == APP_SCENE_RUNNER_PAUSE && nextScene == APP_SCENE_RUNNER_GAME)) &&
        Mix_PausedMusic() != 0;
    if (resumingMatchingGameplay)
    {
        Mix_ResumeMusic();
        return;
    }

    if (previousMusic == nextMusic && Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0)
    {
        return;
    }

    (void)play_music(game, music_for_scene(game, nextMusic));
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
