#include "replay.h"

#include "frame.h"

static bool replay_mode_is_valid(GameMode mode)
{
    return mode == GAME_MODE_SINGLE_PLAYER || mode == GAME_MODE_MULTIPLAYER;
}

static bool replay_scene_is_valid(ReplayScene scene)
{
    return scene == REPLAY_SCENE_ARCADE || scene == REPLAY_SCENE_RUNNER;
}

static unsigned int replay_world_signature(const GameState *game)
{
    // FNV-1a over the procedural Runner world. Arithmetic is unsigned by
    // definition, so overflow is intentional and portable.
    unsigned int signature = 2166136261u;
    for (int i = 0; i < NUM_STARS; i++)
    {
        signature = (signature ^ (unsigned int)game->ledges[i].x) * 16777619u;
        signature = (signature ^ (unsigned int)game->ledges[i].y) * 16777619u;
        signature = (signature ^ (unsigned int)game->stars[i].baseX) * 16777619u;
        signature = (signature ^ (unsigned int)game->stars[i].baseY) * 16777619u;
        signature = (signature ^ (unsigned int)game->stars[i].mode) * 16777619u;
    }
    return signature;
}

static bool replay_external_state_is_untouched(const GameState *game,
                                               ReplayScene scene)
{
    const AppScene expectedScene =
        scene == REPLAY_SCENE_ARCADE ? APP_SCENE_ARCADE_GAME
                                     : APP_SCENE_RUNNER_GAME;
    return game->simulationOnly && game->app.scene == expectedScene &&
           game->app.window == NULL && game->app.renderer == NULL &&
           game->app.controller == NULL &&
           game->menu0 == NULL && game->menu1 == NULL &&
           game->menu2 == NULL && game->mult == NULL &&
           game->leaders == NULL && game->pause == NULL &&
           game->font == NULL &&
           game->audio.menuMusic == NULL &&
           game->audio.arcadeMusic == NULL &&
           game->audio.runnerMusic == NULL &&
           game->audio.select == NULL && game->audio.jump == NULL &&
           game->audio.shoot == NULL && game->audio.damage == NULL &&
           game->audio.kick == NULL &&
           !game->assetFlags.arcadeAssetsLoaded &&
           !game->assetFlags.runnerAssetsLoaded &&
           !game->assetFlags.sharedAssetsLoaded &&
           !game->assetFlags.sharedAudioAssetsLoaded &&
           game->x_i == 0;
}

static void replay_capture_result(const GameState *game, ReplayScene scene, GameMode playerMode, int tickCount,
                                  ReplayResult *result)
{
    *result = (ReplayResult){
        .scene = scene,
        .playerMode = playerMode,
        .tickCount = tickCount,
        .gameLives = game->gameLives,
        .tempScore = game->tempScore,
        .xScore = game->x_score,
        .time = game->time,
        .statusState = game->statusState,
        .manX = game->man.x,
        .manY = game->man.y,
        .manDx = game->man.dx,
        .manDy = game->man.dy,
        .secondPlayerX = game->secondPlayer.x,
        .secondPlayerY = game->secondPlayer.y,
        .scrollX = game->scrollX,
        .firstLedgeX = game->ledges[0].x,
        .firstLedgeY = game->ledges[0].y,
        .tenthLedgeY = game->ledges[9].y,
        .firstStarX = game->stars[0].baseX,
        .firstStarY = game->stars[0].baseY,
        .worldSignature = replay_world_signature(game),
        .externalStateUntouched =
            replay_external_state_is_untouched(game, scene),
    };
}

bool replay_recording_init(ReplayRecording *recording, ReplayScene scene,
                           GameMode playerMode, unsigned int seed, int tickCount)
{
    if (recording == NULL || !replay_scene_is_valid(scene) || !replay_mode_is_valid(playerMode) || tickCount < 0 ||
        tickCount > REPLAY_MAX_TICKS)
    {
        return false;
    }

    *recording = (ReplayRecording){0};
    recording->scene = scene;
    recording->playerMode = playerMode;
    recording->seed = seed;
    recording->tickCount = tickCount;
    return true;
}

bool replay_recording_set_input(ReplayRecording *recording, int tick,
                                InputState input)
{
    if (recording == NULL || recording->tickCount < 0 ||
        recording->tickCount > REPLAY_MAX_TICKS ||
        !replay_scene_is_valid(recording->scene) ||
        !replay_mode_is_valid(recording->playerMode) ||
        tick < 0 || tick >= recording->tickCount)
    {
        return false;
    }

    recording->inputs[tick] = input;
    return true;
}

bool replay_run(const ReplayRecording *recording, ReplayResult *result)
{
    if (recording == NULL || result == NULL || !replay_scene_is_valid(recording->scene) ||
        !replay_mode_is_valid(recording->playerMode) ||
        recording->tickCount < 0 || recording->tickCount > REPLAY_MAX_TICKS)
    {
        return false;
    }

    // Every run owns its random stream. This is the replay boundary that
    // prevents a preceding test or gameplay session from influencing it.
    srandom(recording->seed);

    GameState game = {0};
    if (recording->scene == REPLAY_SCENE_ARCADE)
    {
        arcade_session_reset(&game, recording->playerMode);
        game.app.scene = APP_SCENE_ARCADE_GAME;
    }
    else
    {
        runner_session_reset(&game, recording->playerMode);
        game.app.scene = APP_SCENE_RUNNER_GAME;
    }
    game.simulationOnly = true;

    // A reset begins in the lives display; the real game moves into gameplay
    // on its first processing tick. Start there explicitly so every recorded
    // tick has the same simulation contract regardless of display timing.
    game.statusState = STATUS_STATE_GAME;
    for (int tick = 0; tick < recording->tickCount; tick++)
    {
        game.input = recording->inputs[tick];
        if (recording->scene == REPLAY_SCENE_ARCADE)
        {
            arcade_simulate(&game, PHYSICS_DT);
        }
        else
        {
            runner_simulate(&game, PHYSICS_DT);
        }
    }

    replay_capture_result(&game, recording->scene, recording->playerMode, recording->tickCount, result);
    return true;
}

bool replay_results_equal(const ReplayResult *left, const ReplayResult *right)
{
    if (left == NULL || right == NULL)
    {
        return false;
    }

    return left->scene == right->scene && left->playerMode == right->playerMode &&
           left->tickCount == right->tickCount &&
           left->gameLives == right->gameLives && left->tempScore == right->tempScore &&
           left->xScore == right->xScore && left->time == right->time &&
           left->statusState == right->statusState && left->manX == right->manX &&
           left->manY == right->manY && left->manDx == right->manDx &&
           left->manDy == right->manDy && left->secondPlayerX == right->secondPlayerX &&
           left->secondPlayerY == right->secondPlayerY && left->scrollX == right->scrollX &&
           left->firstLedgeX == right->firstLedgeX && left->firstLedgeY == right->firstLedgeY &&
           left->tenthLedgeY == right->tenthLedgeY && left->firstStarX == right->firstStarX &&
           left->firstStarY == right->firstStarY && left->worldSignature == right->worldSignature &&
           left->externalStateUntouched == right->externalStateUntouched;
}
