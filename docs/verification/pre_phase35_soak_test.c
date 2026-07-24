// Deterministic, render-free restart and long-session soak coverage for the
// pre-Phase-35 whole-game audit. This intentionally uses production reset,
// simulation, replay, event-queue, and fixed-step APIs without loading assets.
#include "atomic_file.h"
#include "fixed_step.h"
#include "frame.h"
#include "game_events.h"
#include "preferences.h"
#include "replay.h"

#include <stdint.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SOAK_RESTART_COUNT 64
#define SOAK_SESSION_TICKS 4000
#define SOAK_REPLAY_RESTARTS 16
#define SOAK_CLOCK_FRAMES 20000

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            fprintf(stderr, "PRE-PHASE 35 SOAK TEST: FAIL - %s\n",        \
                    description);                                           \
            failures++;                                                     \
        }                                                                   \
    } while (0)

typedef struct EnvironmentSnapshot
{
    bool wasSet;
    bool valid;
    char *value;
} EnvironmentSnapshot;

typedef struct ClockSoakResult
{
    bool valid;
    bool sawCappedFrame;
    bool sawNonGameplayFrame;
    int totalSteps;
    uint32_t signature;
} ClockSoakResult;

typedef struct SessionDigest
{
    uint32_t signature;
    int time;
    int gameLives;
    int tempScore;
    int xScore;
    int activeBullets;
    int activeSecondBullets;
    float manX;
    float manY;
    float secondPlayerX;
    float secondPlayerY;
} SessionDigest;

static EnvironmentSnapshot environment_snapshot(const char *name)
{
    const char *value = SDL_getenv(name);
    EnvironmentSnapshot snapshot = {value != NULL, true, NULL};
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

static bool environment_matches(const char *name,
                                const EnvironmentSnapshot *snapshot)
{
    if (snapshot == NULL || !snapshot->valid)
    {
        return false;
    }
    const char *value = SDL_getenv(name);
    if (!snapshot->wasSet)
    {
        return value == NULL || value[0] == '\0';
    }
    return value != NULL && strcmp(value, snapshot->value) == 0;
}

static bool environment_restore(const char *name,
                                const EnvironmentSnapshot *snapshot)
{
    if (snapshot == NULL || !snapshot->valid)
    {
        return false;
    }
    if (snapshot->wasSet)
    {
        if (SDL_setenv(name, snapshot->value, 1) != 0)
        {
            return false;
        }
    }
    else
    {
#ifdef _WIN32
        if (SDL_setenv(name, "", 1) != 0)
        {
            return false;
        }
#else
        if (unsetenv(name) != 0)
        {
            return false;
        }
#endif
    }
    return environment_matches(name, snapshot);
}

static void environment_snapshot_destroy(EnvironmentSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }
    free(snapshot->value);
    snapshot->value = NULL;
}

static long process_id(void)
{
#ifdef _WIN32
    return (long)_getpid();
#else
    return (long)getpid();
#endif
}

static bool create_test_directory(const char *path)
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

static bool file_is_absent(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return true;
    }
    fclose(file);
    return false;
}

static bool file_equals_text(const char *path, const char *expected)
{
    size_t length = 0U;
    char *contents = (char *)SDL_LoadFile(path, &length);
    if (contents == NULL)
    {
        return false;
    }
    const size_t expectedLength = strlen(expected);
    const bool equal =
        length == expectedLength &&
        memcmp(contents, expected, expectedLength) == 0;
    SDL_free(contents);
    return equal;
}

static int active_bullet_count(const Bullet bullets[MAX_BULLETS])
{
    int count = 0;
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        if (bullets[index].active)
        {
            count++;
        }
    }
    return count;
}

static bool input_is_clear(const InputState *input)
{
    return input != NULL &&
           !input->moveLeftPlayer1 && !input->moveRightPlayer1 &&
           !input->jumpHeldPlayer1 && !input->shootHeldPlayer1 &&
           !input->moveLeftPlayer2 && !input->moveRightPlayer2 &&
           !input->jumpHeldPlayer2 && !input->shootHeldPlayer2 &&
           input->jumpBufferTicksPlayer1 == 0 &&
           input->jumpBufferTicksPlayer2 == 0;
}

static bool render_transforms_are_synced(const GameState *game)
{
    if (game == NULL ||
        game->man.prevX != game->man.x || game->man.prevY != game->man.y ||
        game->secondPlayer.prevX != game->secondPlayer.x ||
        game->secondPlayer.prevY != game->secondPlayer.y)
    {
        return false;
    }
    for (int index = 0; index < NUM_ENEMIES; index++)
    {
        if (game->enemyValues[index].prevX != game->enemyValues[index].x ||
            game->enemyValues[index].prevY != game->enemyValues[index].y)
        {
            return false;
        }
    }
    for (int index = 0; index < NUM_SMART_ENEMIES; index++)
    {
        if (game->smartEnemies[index].prevX != game->smartEnemies[index].x ||
            game->smartEnemies[index].prevY != game->smartEnemies[index].y)
        {
            return false;
        }
    }
    for (int index = 0; index < 2; index++)
    {
        if (game->boss[index].prevX != game->boss[index].x ||
            game->boss[index].prevY != game->boss[index].y)
        {
            return false;
        }
    }
    for (int index = 0; index < NUM_STARS; index++)
    {
        if (game->stars[index].prevX != (float)game->stars[index].x ||
            game->stars[index].prevY != (float)game->stars[index].y)
        {
            return false;
        }
    }
    return true;
}

static void dirty_session_transients(GameState *game, int restart)
{
    game->input = (InputState){
        .moveLeftPlayer1 = true,
        .moveRightPlayer2 = true,
        .jumpHeldPlayer1 = true,
        .shootHeldPlayer2 = true,
        .jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS,
        .jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS,
    };
    game->events.count = 1;
    game->events.events[0] = (GameEvent){
        GAME_EVENT_ARCADE_GAME_OVER_CHECK,
        GAME_EVENT_TARGET_REGULAR_ENEMY,
        -1,
        -1,
        false,
    };
    game->app.controllerJumpHeldLastFrame = true;
    game->CurrentSheetBullet = 59;
    game->CurrentSheetBullet2 = 29;
    game->CurrentSheetBoss = 3;
    game->CurrentSpriteBack = 1;
    game->CurrentSpriteBack2 = 1;
    game->CurrentSpriteBack3 = 1;
    game->renderAlpha = NAN;
    game->perfProjectileActiveSamples = UINT64_MAX;
    game->perfProjectileTargetChecks = UINT64_MAX;
    game->simulationOnly = true;
    game->shotCount = ARCADE_SHOT_COOLDOWN_TICKS;
    game->shotCountMultiplayer = ARCADE_SHOT_COOLDOWN_TICKS;
    game->bullets[0].active = true;
    game->bullets[MAX_BULLETS - 1].active = true;
    game->secondBullets[0].active = true;
    game->secondBullets[MAX_BULLETS - 1].active = true;
    game->man.prevX = game->man.x - (float)(restart + 1);
    game->man.prevY = game->man.y + 5.0f;
    game->secondPlayer.prevX = game->secondPlayer.x + 7.0f;
    game->secondPlayer.prevY = game->secondPlayer.y - 9.0f;
    game->stars[0].prevX = (float)game->stars[0].x + 11.0f;
    game->stars[0].prevY = (float)game->stars[0].y - 13.0f;
    game->feedback.hitStopTicks = FEEDBACK_BOSS_HIT_STOP_TICKS;
    game->feedback.screenShakeTicks = FEEDBACK_SCREEN_SHAKE_TICKS;
    game->feedback.muzzleFlashTicks[0] = FEEDBACK_MUZZLE_FLASH_TICKS;
    game->feedback.muzzleFlashTicks[1] = FEEDBACK_MUZZLE_FLASH_TICKS;
}

static bool common_reset_state_is_clean(const GameState *game, GameMode mode,
                                        bool runner)
{
    const int expectedLives = runner ? 3 : 10;
    const float expectedSecondX = runner ? 100.0f : 1000.0f;
    const float expectedSecondY = runner ? 80.0f : 200.0f;
    const bool secondPlayerActive = mode == GAME_MODE_MULTIPLAYER;
    return game->multiPlayer == mode &&
           game->gameLives == expectedLives &&
           game->statusState == STATUS_STATE_LIVES &&
           game->time == 0 &&
           game->deathCountdown == -1 &&
           input_is_clear(&game->input) &&
           game->events.count == 0 &&
           !game->app.controllerJumpHeldLastFrame &&
           game->CurrentSheetBullet == 0 &&
           game->CurrentSheetBullet2 == 0 &&
           game->CurrentSheetBoss == 0 &&
           game->CurrentSpriteBack == 0 &&
           game->CurrentSpriteBack2 == 0 &&
           game->CurrentSpriteBack3 == 0 &&
           game->renderAlpha == 0.0f &&
           game->perfProjectileActiveSamples == 0U &&
           game->perfProjectileTargetChecks == 0U &&
           !game->simulationOnly &&
           game->shotCount == 0 &&
           game->shotCountMultiplayer == 0 &&
           active_bullet_count(game->bullets) == 0 &&
           active_bullet_count(game->secondBullets) == 0 &&
           game->feedback.hitStopTicks == 0 &&
           game->feedback.screenShakeTicks == 0 &&
           game->feedback.muzzleFlashTicks[0] == 0 &&
           game->feedback.muzzleFlashTicks[1] == 0 &&
           game->man.visible0 == 1 &&
           game->man.lives == 3 &&
           game->secondPlayer.visible0 == (secondPlayerActive ? 1 : 0) &&
           game->secondPlayer.lives == (secondPlayerActive ? 3 : 0) &&
           game->secondPlayer.x == expectedSecondX &&
           game->secondPlayer.y == expectedSecondY &&
           render_transforms_are_synced(game);
}

static void verify_repeated_cross_mode_resets(void)
{
    GameState *game = calloc(1U, sizeof(*game));
    CHECK("restart soak allocates isolated state", game != NULL);
    if (game == NULL)
    {
        return;
    }

    game->app.scene = APP_SCENE_MAIN_MENU;
    game->app.display =
        (DisplaySettings){1440, 810, true};
    game->app.settings.musicVolume = 37;
    game->app.settings.effectsVolume = 41;
    game->app.settings.player1.jump = SDL_SCANCODE_F;
    game->app.settings.player2.jump = SDL_SCANCODE_KP_0;
    game->assetFlags =
        (AssetLifecycleFlags){true, true, true, true};
    game->x_i = 2;
    game->x_list[0] = 9001;
    game->x_list[1] = 42;

    bool allClean = true;
    bool persistentStateSurvived = true;
    for (int restart = 0; restart < SOAK_RESTART_COUNT; restart++)
    {
        dirty_session_transients(game, restart);
        const bool runner = restart % 4 >= 2;
        const GameMode mode =
            restart % 2 == 0 ? GAME_MODE_SINGLE_PLAYER
                             : GAME_MODE_MULTIPLAYER;
        if (runner)
        {
            runner_session_reset(game, mode);
        }
        else
        {
            arcade_session_reset(game, mode);
        }

        allClean = allClean &&
                   common_reset_state_is_clean(game, mode, runner) &&
                   (runner ? game->x_score == 0
                           : game->kills_score == 0 &&
                                 game->kills_score_multi == 0 &&
                                 game->tempScore == 0);
        persistentStateSurvived =
            persistentStateSurvived &&
            game->app.scene == APP_SCENE_MAIN_MENU &&
            game->app.display.windowWidth == 1440 &&
            game->app.display.windowHeight == 810 &&
            game->app.display.fullscreen &&
            game->app.settings.musicVolume == 37 &&
            game->app.settings.effectsVolume == 41 &&
            game->app.settings.player1.jump == SDL_SCANCODE_F &&
            game->app.settings.player2.jump == SDL_SCANCODE_KP_0 &&
            game->assetFlags.arcadeAssetsLoaded &&
            game->assetFlags.runnerAssetsLoaded &&
            game->assetFlags.sharedAssetsLoaded &&
            game->assetFlags.sharedAudioAssetsLoaded &&
            game->x_i == 2 &&
            game->x_list[0] == 9001 &&
            game->x_list[1] == 42;
    }
    CHECK("64 Arcade/Runner and SP/MP restarts clear every session transient",
          allClean);
    CHECK("session restarts preserve app settings, assets, scene, and scores",
          persistentStateSurvived);
    free(game);
}

static void verify_simultaneous_event_boundary(void)
{
    GameState *game = calloc(1U, sizeof(*game));
    CHECK("event-boundary soak allocates isolated state", game != NULL);
    if (game == NULL)
    {
        return;
    }
    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->app.scene = APP_SCENE_ARCADE_GAME;
    game_events_begin(game);

    for (int index = 0; index < MAX_BULLETS; index++)
    {
        game->bullets[index] =
            (Bullet){0.0f, 100.0f, 1.0f, 0.0f, 100.0f, true};
        game->secondBullets[index] =
            (Bullet){0.0f, 100.0f, 1.0f, 0.0f, 100.0f, true};
    }
    for (int index = 0; index < NUM_ENEMIES; index++)
    {
        game->enemyValues[index].visible = 1;
        game->enemyValues[index].x = 200.0f + (float)index;
        game->enemyValues[index].y = 100.0f;
    }

    bool allAccepted = true;
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        const int targetIndex = index % NUM_ENEMIES;
        allAccepted = allAccepted &&
                      game_events_push_projectile_hit(
                          game, false, index,
                          GAME_EVENT_TARGET_REGULAR_ENEMY, targetIndex);
    }
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        const int targetIndex = index % NUM_ENEMIES;
        allAccepted = allAccepted &&
                      game_events_push_projectile_hit(
                          game, true, index,
                          GAME_EVENT_TARGET_REGULAR_ENEMY, targetIndex);
    }
    static const GameEventType arcadePlayerEvents[2] = {
        GAME_EVENT_ARCADE_PLAYER_HIT,
        GAME_EVENT_ARCADE_PLAYER_FELL,
    };
    for (int typeIndex = 0; typeIndex < 2; typeIndex++)
    {
        for (int playerIndex = 0; playerIndex < 2; playerIndex++)
        {
            allAccepted = allAccepted &&
                          game_events_push_player_contact(
                              game, arcadePlayerEvents[typeIndex], playerIndex);
        }
    }
    for (int index = 0; index < NUM_ENEMIES; index++)
    {
        allAccepted = allAccepted &&
                      game_events_push_target_contact(
                          game, GAME_EVENT_ARCADE_ENEMY_ESCAPED, index);
    }
    for (int index = 0; index < 2; index++)
    {
        allAccepted = allAccepted &&
                      game_events_push_target_contact(
                          game, GAME_EVENT_ARCADE_BOSS_ESCAPED, index);
    }
    allAccepted =
        allAccepted &&
        game_events_push_transition_check(
            game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);

    const int expectedEvents = 2 * MAX_BULLETS + 4 + NUM_ENEMIES + 2 + 1;
    CHECK("the maximum simultaneous Arcade event set fits without starvation",
          allAccepted && game->events.count == expectedEvents &&
              expectedEvents < MAX_GAME_EVENTS);
    const int countBeforeDuplicate = game->events.count;
    CHECK("duplicates and invalid boundaries cannot grow the event queue",
          !game_events_push_projectile_hit(
              game, false, 0, GAME_EVENT_TARGET_REGULAR_ENEMY, 0) &&
              !game_events_push_target_contact(
                  game, GAME_EVENT_ARCADE_ENEMY_ESCAPED, NUM_ENEMIES) &&
              game->events.count == countBeforeDuplicate);

    game->simulationOnly = true;
    game_events_apply(game);
    CHECK("a maximal event batch drains once without negative lives",
          game->events.count == 0 && game->gameLives == 0);
    CHECK("every snapshot-hit projectile is consumed while targets score once",
          active_bullet_count(game->bullets) == 0 &&
              active_bullet_count(game->secondBullets) == 0 &&
              game->kills_score == NUM_ENEMIES &&
              game->tempScore == NUM_ENEMIES);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    CHECK("reset after a maximal event batch clears both projectile pools",
          active_bullet_count(game->bullets) == 0 &&
              active_bullet_count(game->secondBullets) == 0 &&
              game->events.count == 0);
    game->events.count = MAX_GAME_EVENTS + 1;
    CHECK("a corrupted oversized queue rejects new public API writes",
          !game_events_push_transition_check(
              game, GAME_EVENT_ARCADE_GAME_OVER_CHECK));
    game_events_apply(game);
    CHECK("a corrupted oversized queue is discarded without iteration",
          game->events.count == 0);
    game->events.count = -1;
    CHECK("a corrupted negative queue rejects writes and recovers on apply",
          !game_events_push_transition_check(
              game, GAME_EVENT_ARCADE_GAME_OVER_CHECK));
    game_events_apply(game);
    CHECK("negative queue metadata is reset to the empty boundary",
          game->events.count == 0);
    free(game);
}

static uint32_t signature_mix(uint32_t signature, uint32_t value)
{
    return (signature ^ value) * UINT32_C(16777619);
}

static uint32_t signature_float(uint32_t signature, float value)
{
    uint32_t bits = 0U;
    memcpy(&bits, &value, sizeof(bits));
    return signature_mix(signature, bits);
}

static ClockSoakResult run_clock_soak(void)
{
    FixedStepClock clock = {0};
    ClockSoakResult result = {true, false, false, 0, UINT32_C(2166136261)};
    for (int frame = 0; frame < SOAK_CLOCK_FRAMES; frame++)
    {
        AppScene scene = APP_SCENE_ARCADE_GAME;
        const int sceneBand = (frame / 251) % 4;
        if (sceneBand == 1)
        {
            scene = APP_SCENE_ARCADE_PAUSE;
        }
        else if (sceneBand == 2)
        {
            scene = APP_SCENE_RUNNER_GAME;
        }
        else if (sceneBand == 3)
        {
            scene = APP_SCENE_MAIN_MENU;
        }

        const double frameTime =
            frame % 113 == 0
                ? MAX_FRAME_TIME
                : (double)((frame % 7) + 1) * (double)PHYSICS_DT * 0.25;
        fixed_step_clock_begin_frame(&clock, scene, frameTime);
        int steps = 0;
        while (fixed_step_clock_should_step(&clock, scene, steps))
        {
            fixed_step_clock_consume_step(&clock);
            steps++;
        }
        fixed_step_clock_finish_frame(&clock, scene, steps);
        const float alpha = fixed_step_clock_alpha(&clock);

        result.valid =
            result.valid &&
            steps >= 0 && steps <= MAX_PHYSICS_STEPS_PER_FRAME &&
            isfinite(alpha) && alpha >= 0.0f && alpha <= 1.0f &&
            (fixed_step_scene_is_gameplay(scene) ||
             (steps == 0 && alpha == 0.0f && clock.accumulator == 0.0));
        result.sawCappedFrame =
            result.sawCappedFrame ||
            steps == MAX_PHYSICS_STEPS_PER_FRAME;
        result.sawNonGameplayFrame =
            result.sawNonGameplayFrame ||
            !fixed_step_scene_is_gameplay(scene);
        result.totalSteps += steps;
        result.signature =
            signature_mix(result.signature, (uint32_t)scene);
        result.signature =
            signature_mix(result.signature, (uint32_t)steps);
        result.signature = signature_float(result.signature, alpha);
    }
    return result;
}

static void verify_fixed_step_soak(void)
{
    const ClockSoakResult first = run_clock_soak();
    const ClockSoakResult restarted = run_clock_soak();
    CHECK("20,000 mixed-rate frames keep steps and interpolation bounded",
          first.valid && first.sawCappedFrame &&
              first.sawNonGameplayFrame && first.totalSteps > 0);
    CHECK("fixed-step scheduling is deterministic across a fresh restart",
          first.totalSteps == restarted.totalSteps &&
              first.signature == restarted.signature &&
              restarted.valid);
}

static InputState deterministic_input(ReplayScene scene, GameMode mode,
                                      int tick)
{
    const int phase = tick % 120;
    InputState input = {0};
    input.moveRightPlayer1 = phase < 42;
    input.moveLeftPlayer1 = phase >= 70 && phase < 96;
    input.jumpHeldPlayer1 = phase >= 8 && phase < 23;
    input.jumpBufferTicksPlayer1 =
        tick % 47 == 0 ? JUMP_BUFFER_TICKS : 0;
    input.shootHeldPlayer1 =
        scene == REPLAY_SCENE_ARCADE && tick % 61 < 26;
    if (mode == GAME_MODE_MULTIPLAYER)
    {
        input.moveRightPlayer2 = phase >= 30 && phase < 66;
        input.moveLeftPlayer2 = phase >= 98;
        input.jumpHeldPlayer2 = phase >= 45 && phase < 58;
        input.jumpBufferTicksPlayer2 =
            tick % 71 == 0 ? JUMP_BUFFER_TICKS : 0;
        input.shootHeldPlayer2 =
            scene == REPLAY_SCENE_ARCADE && tick % 79 < 31;
    }
    return input;
}

static bool prepare_recording(ReplayRecording *recording, ReplayScene scene,
                              GameMode mode, unsigned int seed, int tickCount)
{
    if (!replay_recording_init(recording, scene, mode, seed, tickCount))
    {
        return false;
    }
    for (int tick = 0; tick < tickCount; tick++)
    {
        if (!replay_recording_set_input(
                recording, tick, deterministic_input(scene, mode, tick)))
        {
            return false;
        }
    }
    return true;
}

static void verify_replay_external_side_effect_isolation(
    const EnvironmentSnapshot *preferenceSnapshot)
{
    char directory[256] = "";
    (void)snprintf(directory, sizeof(directory),
                   ".pre-phase35-replay-%ld", process_id());
    const bool directoryCreated = create_test_directory(directory);
    CHECK("replay side-effect test creates an isolated preference directory",
          directoryCreated);
    const bool overrideInstalled =
        directoryCreated &&
        SDL_setenv("ENDGAME_PREF_DIR", directory, 1) == 0;
    CHECK("replay side-effect test installs its isolated preference path",
          overrideInstalled);

    char *leaderboardPath =
        overrideInstalled ? preferences_file_path("leaderboard.ini") : NULL;
    CHECK("replay side-effect test resolves the isolated leaderboard path",
          leaderboardPath != NULL);
    if (leaderboardPath != NULL)
    {
        (void)remove(leaderboardPath);
        ReplayRecording recording;
        bool prepared = replay_recording_init(
            &recording, REPLAY_SCENE_RUNNER,
            GAME_MODE_SINGLE_PLAYER, 8191U, REPLAY_MAX_TICKS);
        for (int tick = 0; tick < REPLAY_MAX_TICKS && prepared; tick++)
        {
            InputState input = {.moveLeftPlayer1 = true};
            prepared =
                replay_recording_set_input(&recording, tick, input);
        }
        ReplayResult first = {0};
        const bool firstRan =
            prepared && replay_run(&recording, &first);
        CHECK("a replay reaches authoritative game-over in simulation-only mode",
              firstRan && first.gameLives == 0);
        CHECK("replay game-over leaves menu and asset state untouched",
              firstRan && first.externalStateUntouched);
        CHECK("replay game-over does not create a leaderboard preference",
              file_is_absent(leaderboardPath));

        static const char sentinel[] = "sentinel leaderboard\n";
        const bool sentinelWritten =
            atomic_write_text_file(leaderboardPath, sentinel,
                                   sizeof(sentinel) - 1U);
        CHECK("replay isolation fixture writes its sentinel leaderboard",
              sentinelWritten);
        ReplayResult second = {0};
        const bool secondRan =
            sentinelWritten && replay_run(&recording, &second);
        CHECK("restarted replay remains application-side-effect free",
              secondRan && second.externalStateUntouched &&
                  replay_results_equal(&first, &second));
        CHECK("replay cannot modify an existing leaderboard preference",
              file_equals_text(leaderboardPath, sentinel));
        (void)remove(leaderboardPath);
        free(leaderboardPath);
    }

    CHECK("replay side-effect test restores ENDGAME_PREF_DIR",
          environment_restore("ENDGAME_PREF_DIR", preferenceSnapshot));
    if (directoryCreated)
    {
        remove_test_directory(directory);
    }
}

static bool replay_result_is_bounded(const ReplayResult *result)
{
    const int initialLives =
        result->scene == REPLAY_SCENE_ARCADE ? 10 : 3;
    const float hiddenSecondX =
        result->scene == REPLAY_SCENE_ARCADE ? 1000.0f : 100.0f;
    const float hiddenSecondY =
        result->scene == REPLAY_SCENE_ARCADE ? 200.0f : 80.0f;
    return result->tickCount == REPLAY_MAX_TICKS &&
           result->externalStateUntouched &&
           result->gameLives >= 0 && result->gameLives <= initialLives &&
           result->tempScore >= 0 && result->xScore >= 0 &&
           result->time > 0 && result->time <= result->tickCount &&
           isfinite(result->manX) && isfinite(result->manY) &&
           isfinite(result->manDx) && isfinite(result->manDy) &&
           isfinite(result->secondPlayerX) &&
           isfinite(result->secondPlayerY) &&
           isfinite(result->scrollX) &&
           (result->playerMode == GAME_MODE_MULTIPLAYER ||
            (result->secondPlayerX == hiddenSecondX &&
             result->secondPlayerY == hiddenSecondY));
}

static void verify_replay_restart_isolation(void)
{
    bool allRunsSucceeded = true;
    bool allRunsBounded = true;
    bool allRestartsEqual = true;
    bool sawDifferentRunnerWorld = false;
    unsigned int previousRunnerSignature = 0U;
    bool haveRunnerSignature = false;

    for (int restart = 0; restart < SOAK_REPLAY_RESTARTS; restart++)
    {
        const ReplayScene scene =
            restart % 2 == 0 ? REPLAY_SCENE_ARCADE : REPLAY_SCENE_RUNNER;
        const GameMode mode =
            (restart / 2) % 2 == 0 ? GAME_MODE_SINGLE_PLAYER
                                   : GAME_MODE_MULTIPLAYER;
        const unsigned int seed =
            UINT32_C(1009) + (unsigned int)restart * UINT32_C(7919);
        ReplayRecording recording;
        ReplayRecording perturbation;
        ReplayResult first = {0};
        ReplayResult perturbed = {0};
        ReplayResult restarted = {0};

        const bool prepared =
            prepare_recording(&recording, scene, mode, seed,
                              REPLAY_MAX_TICKS) &&
            prepare_recording(
                &perturbation,
                scene == REPLAY_SCENE_ARCADE ? REPLAY_SCENE_RUNNER
                                             : REPLAY_SCENE_ARCADE,
                mode == GAME_MODE_SINGLE_PLAYER
                    ? GAME_MODE_MULTIPLAYER
                    : GAME_MODE_SINGLE_PLAYER,
                seed ^ UINT32_C(0xa5a5a5a5), 97);
        const bool ran =
            prepared &&
            replay_run(&recording, &first) &&
            replay_run(&perturbation, &perturbed) &&
            replay_run(&recording, &restarted);
        allRunsSucceeded = allRunsSucceeded && ran;
        if (!ran)
        {
            continue;
        }
        allRunsBounded =
            allRunsBounded && replay_result_is_bounded(&first) &&
            replay_result_is_bounded(&restarted);
        allRestartsEqual =
            allRestartsEqual && replay_results_equal(&first, &restarted);
        if (scene == REPLAY_SCENE_RUNNER)
        {
            if (haveRunnerSignature &&
                first.worldSignature != previousRunnerSignature)
            {
                sawDifferentRunnerWorld = true;
            }
            previousRunnerSignature = first.worldSignature;
            haveRunnerSignature = true;
        }
    }

    CHECK("16 maximum-length replays survive alternating scene/mode restarts",
          allRunsSucceeded && allRunsBounded);
    CHECK("unrelated replays cannot contaminate a restarted recording",
          allRestartsEqual);
    CHECK("different Runner seeds still produce distinct bounded worlds",
          sawDifferentRunnerWorld);
}

static bool player_state_is_finite(const Man *player)
{
    return isfinite(player->x) && isfinite(player->y) &&
           isfinite(player->prevX) && isfinite(player->prevY) &&
           isfinite(player->dx) && isfinite(player->dy);
}

static bool enemy_state_is_finite(const Enemies *enemy)
{
    return isfinite(enemy->x) && isfinite(enemy->y) &&
           isfinite(enemy->prevX) && isfinite(enemy->prevY) &&
           isfinite(enemy->dx) && isfinite(enemy->dy);
}

static bool long_session_state_is_bounded(const GameState *game,
                                          ReplayScene scene,
                                          GameMode mode)
{
    if (!player_state_is_finite(&game->man) ||
        !player_state_is_finite(&game->secondPlayer) ||
        !isfinite(game->scrollX) || !isfinite(game->prevScrollX) ||
        game->renderAlpha != 0.0f ||
        game->events.count != 0 ||
        game->gameLives < 0 || game->gameLives > 1000 ||
        game->time < 0 || game->time > SOAK_SESSION_TICKS ||
        game->tempScore < 0 || game->x_score < 0 ||
        game->shotCount < 0 ||
        game->shotCount > ARCADE_SHOT_COOLDOWN_TICKS ||
        game->shotCountMultiplayer < 0 ||
        game->shotCountMultiplayer > ARCADE_SHOT_COOLDOWN_TICKS ||
        game->CurrentSheetBullet < 0 || game->CurrentSheetBullet >= 60 ||
        game->CurrentSheetBullet2 < 0 || game->CurrentSheetBullet2 >= 30 ||
        game->CurrentSheetBoss < 0 || game->CurrentSheetBoss >= 4 ||
        game->input.jumpBufferTicksPlayer1 < 0 ||
        game->input.jumpBufferTicksPlayer1 > JUMP_BUFFER_TICKS ||
        game->input.jumpBufferTicksPlayer2 < 0 ||
        game->input.jumpBufferTicksPlayer2 > JUMP_BUFFER_TICKS)
    {
        return false;
    }
    if (mode == GAME_MODE_SINGLE_PLAYER &&
        (game->secondPlayer.visible0 != 0 ||
         game->secondPlayer.lives != 0 ||
         active_bullet_count(game->secondBullets) != 0))
    {
        return false;
    }
    for (int index = 0; index < NUM_ENEMIES; index++)
    {
        if (!enemy_state_is_finite(&game->enemyValues[index]))
        {
            return false;
        }
    }
    for (int index = 0; index < NUM_SMART_ENEMIES; index++)
    {
        if (!enemy_state_is_finite(&game->smartEnemies[index]))
        {
            return false;
        }
    }
    for (int index = 0; index < 2; index++)
    {
        if (!enemy_state_is_finite(&game->boss[index]))
        {
            return false;
        }
    }
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        const Bullet *first = &game->bullets[index];
        const Bullet *second = &game->secondBullets[index];
        if ((first->active &&
             (!isfinite(first->x) || !isfinite(first->y) ||
              !isfinite(first->prevX) || !isfinite(first->prevY) ||
              !isfinite(first->dx))) ||
            (second->active &&
             (!isfinite(second->x) || !isfinite(second->y) ||
              !isfinite(second->prevX) || !isfinite(second->prevY) ||
              !isfinite(second->dx))))
        {
            return false;
        }
    }
    for (int index = 0; index < NUM_STARS; index++)
    {
        if (!isfinite(game->stars[index].dx) ||
            !isfinite(game->stars[index].dy) ||
            !isfinite(game->stars[index].phase) ||
            !isfinite(game->stars[index].prevX) ||
            !isfinite(game->stars[index].prevY))
        {
            return false;
        }
    }
    return scene == REPLAY_SCENE_ARCADE ||
           game->runnerSegments.nextWriteSlot >= 0;
}

static uint32_t game_state_signature(const GameState *game)
{
    uint32_t signature = UINT32_C(2166136261);
    signature = signature_mix(signature, (uint32_t)game->multiPlayer);
    signature = signature_mix(signature, (uint32_t)game->gameLives);
    signature = signature_mix(signature, (uint32_t)game->tempScore);
    signature = signature_mix(signature, (uint32_t)game->x_score);
    signature = signature_mix(signature, (uint32_t)game->time);
    signature = signature_float(signature, game->man.x);
    signature = signature_float(signature, game->man.y);
    signature = signature_float(signature, game->man.dx);
    signature = signature_float(signature, game->man.dy);
    signature = signature_float(signature, game->secondPlayer.x);
    signature = signature_float(signature, game->secondPlayer.y);
    signature = signature_float(signature, game->scrollX);
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        signature = signature_mix(
            signature, game->bullets[index].active ? 1U : 0U);
        signature = signature_float(signature, game->bullets[index].x);
        signature = signature_mix(
            signature, game->secondBullets[index].active ? 1U : 0U);
        signature =
            signature_float(signature, game->secondBullets[index].x);
    }
    for (int index = 0; index < NUM_ENEMIES; index++)
    {
        signature =
            signature_float(signature, game->enemyValues[index].x);
        signature =
            signature_float(signature, game->enemyValues[index].y);
        signature = signature_mix(
            signature, (uint32_t)game->enemyValues[index].visible);
    }
    for (int index = 0; index < NUM_STARS; index++)
    {
        signature =
            signature_mix(signature, (uint32_t)game->stars[index].x);
        signature =
            signature_mix(signature, (uint32_t)game->stars[index].y);
        signature =
            signature_mix(signature, (uint32_t)game->ledges[index].x);
        signature =
            signature_mix(signature, (uint32_t)game->ledges[index].y);
    }
    return signature;
}

static bool run_long_session(ReplayScene scene, GameMode mode,
                             unsigned int seed, SessionDigest *digest,
                             bool *stateChanged)
{
    GameState *game = calloc(1U, sizeof(*game));
    if (game == NULL || digest == NULL || stateChanged == NULL)
    {
        free(game);
        return false;
    }
    srandom(seed);
    if (scene == REPLAY_SCENE_ARCADE)
    {
        arcade_session_reset(game, mode);
    }
    else
    {
        runner_session_reset(game, mode);
    }
    game->app.scene =
        scene == REPLAY_SCENE_ARCADE ? APP_SCENE_ARCADE_GAME
                                     : APP_SCENE_RUNNER_GAME;
    game->simulationOnly = true;
    game->statusState = STATUS_STATE_GAME;
    game->gameLives = 1000;
    if (scene == REPLAY_SCENE_ARCADE)
    {
        // Keep this a timer/pool/player soak. Wave/collision behavior has its
        // own exhaustive harness; suppressing spawns prevents a boss escape
        // from intentionally ending this otherwise long-lived session.
        game->arcadeWaves.restTicksRemaining = INT_MAX;
    }

    bool bounded = true;
    bool sawProgress = false;
    bool sawProjectile = false;
    for (int tick = 0; tick < SOAK_SESSION_TICKS; tick++)
    {
        game->input = deterministic_input(scene, mode, tick);
        if (scene == REPLAY_SCENE_ARCADE)
        {
            arcade_simulate(game, PHYSICS_DT);
            sawProjectile =
                sawProjectile || active_bullet_count(game->bullets) > 0;
        }
        else
        {
            runner_simulate(game, PHYSICS_DT);
        }
        sawProgress = sawProgress || game->time > 0;
        bounded = bounded &&
                  long_session_state_is_bounded(game, scene, mode) &&
                  game->app.scene ==
                      (scene == REPLAY_SCENE_ARCADE
                           ? APP_SCENE_ARCADE_GAME
                           : APP_SCENE_RUNNER_GAME);
    }

    *digest = (SessionDigest){
        .signature = game_state_signature(game),
        .time = game->time,
        .gameLives = game->gameLives,
        .tempScore = game->tempScore,
        .xScore = game->x_score,
        .activeBullets = active_bullet_count(game->bullets),
        .activeSecondBullets =
            active_bullet_count(game->secondBullets),
        .manX = game->man.x,
        .manY = game->man.y,
        .secondPlayerX = game->secondPlayer.x,
        .secondPlayerY = game->secondPlayer.y,
    };
    *stateChanged =
        sawProgress &&
        (scene == REPLAY_SCENE_RUNNER || sawProjectile);
    free(game);
    return bounded;
}

static bool session_digests_equal(const SessionDigest *first,
                                  const SessionDigest *second)
{
    return first->signature == second->signature &&
           first->time == second->time &&
           first->gameLives == second->gameLives &&
           first->tempScore == second->tempScore &&
           first->xScore == second->xScore &&
           first->activeBullets == second->activeBullets &&
           first->activeSecondBullets == second->activeSecondBullets &&
           first->manX == second->manX &&
           first->manY == second->manY &&
           first->secondPlayerX == second->secondPlayerX &&
           first->secondPlayerY == second->secondPlayerY;
}

static void verify_render_free_long_sessions(void)
{
    bool allBounded = true;
    bool allDeterministic = true;
    bool allChanged = true;
    for (int scenario = 0; scenario < 4; scenario++)
    {
        const ReplayScene scene =
            scenario % 2 == 0 ? REPLAY_SCENE_ARCADE : REPLAY_SCENE_RUNNER;
        const GameMode mode =
            scenario < 2 ? GAME_MODE_SINGLE_PLAYER
                         : GAME_MODE_MULTIPLAYER;
        const unsigned int seed =
            UINT32_C(7001) + (unsigned int)scenario * UINT32_C(101);
        SessionDigest first = {0};
        SessionDigest restarted = {0};
        bool firstChanged = false;
        bool restartedChanged = false;
        const bool firstBounded =
            run_long_session(scene, mode, seed, &first, &firstChanged);
        srandom(seed ^ UINT32_C(0x5a5a5a5a));
        (void)random();
        const bool restartedBounded =
            run_long_session(scene, mode, seed, &restarted,
                             &restartedChanged);
        allBounded =
            allBounded && firstBounded && restartedBounded;
        allDeterministic =
            allDeterministic &&
            session_digests_equal(&first, &restarted);
        allChanged =
            allChanged && firstChanged && restartedChanged;
    }
    CHECK("four 4,000-tick headless sessions remain finite and bounded",
          allBounded);
    CHECK("long simulations change authoritative state without rendering",
          allChanged);
    CHECK("long sessions reproduce after RNG perturbation and fresh reset",
          allDeterministic);
}

int main(void)
{
    EnvironmentSnapshot preferenceDirectory =
        environment_snapshot("ENDGAME_PREF_DIR");
    EnvironmentSnapshot seedOverride =
        environment_snapshot("ENDGAME_SEED");
    EnvironmentSnapshot performanceLogging =
        environment_snapshot("ENDGAME_PERF_LOG");
    CHECK("environment snapshots allocate successfully",
          preferenceDirectory.valid && seedOverride.valid &&
              performanceLogging.valid);

    if (SDL_Init(0) != 0)
    {
        fprintf(stderr, "PRE-PHASE 35 SOAK TEST: SDL_Init failed: %s\n",
                SDL_GetError());
        environment_snapshot_destroy(&preferenceDirectory);
        environment_snapshot_destroy(&seedOverride);
        environment_snapshot_destroy(&performanceLogging);
        return 1;
    }

    verify_repeated_cross_mode_resets();
    verify_simultaneous_event_boundary();
    verify_fixed_step_soak();
    verify_replay_restart_isolation();
    verify_replay_external_side_effect_isolation(
        &preferenceDirectory);
    verify_render_free_long_sessions();

    SDL_Quit();
    CHECK("headless soak leaves all ENDGAME environment overrides untouched",
          environment_matches("ENDGAME_PREF_DIR", &preferenceDirectory) &&
              environment_matches("ENDGAME_SEED", &seedOverride) &&
              environment_matches("ENDGAME_PERF_LOG",
                                  &performanceLogging));
    environment_snapshot_destroy(&preferenceDirectory);
    environment_snapshot_destroy(&seedOverride);
    environment_snapshot_destroy(&performanceLogging);

    if (failures == 0)
    {
        printf("PRE-PHASE 35 SOAK TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
