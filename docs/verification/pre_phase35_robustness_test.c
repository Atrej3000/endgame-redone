#include "arcade_waves.h"
#include "atomic_file.h"
#include "combat_feedback.h"
#include "entity_spawn.h"
#include "fixed_step.h"
#include "physics_body.h"
#include "preferences.h"
#include "replay.h"
#include "runner_segments.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            fprintf(stderr, "PRE-PHASE 35 ROBUSTNESS TEST: FAIL - %s\n",  \
                    description);                                           \
            failures++;                                                     \
        }                                                                   \
    } while (0)

typedef struct AtomicWriter
{
    const char *path;
    const char *payload;
    size_t length;
    int failures;
} AtomicWriter;

typedef struct EnvironmentSnapshot
{
    bool wasSet;
    bool valid;
    char *value;
} EnvironmentSnapshot;

static EnvironmentSnapshot capture_environment(const char *name)
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

static bool restore_environment(const char *name,
                                EnvironmentSnapshot *snapshot)
{
    if (snapshot == NULL || !snapshot->valid)
    {
        return false;
    }
    int result = 0;
    if (snapshot->wasSet)
    {
        result = SDL_setenv(name, snapshot->value, 1);
    }
    else
    {
#ifdef _WIN32
        // SDL2 has no SDL_unsetenv. An empty value is the supported Windows
        // equivalent for ENDGAME_PREF_DIR, whose consumer ignores empty
        // strings.
        result = SDL_setenv(name, "", 1);
#else
        result = unsetenv(name);
#endif
    }
    const char *restored = SDL_getenv(name);
    const bool matches =
        result == 0 &&
        (snapshot->wasSet
             ? restored != NULL && strcmp(restored, snapshot->value) == 0
             : restored == NULL || restored[0] == '\0');
    free(snapshot->value);
    snapshot->value = NULL;
    return matches;
}

static int write_repeatedly(void *contextPointer)
{
    AtomicWriter *context = (AtomicWriter *)contextPointer;
    for (int iteration = 0; iteration < 64; iteration++)
    {
        if (!atomic_write_text_file(context->path, context->payload, context->length))
        {
            context->failures++;
        }
    }
    return 0;
}

static bool file_equals_either_payload(const char *path,
                                       const AtomicWriter *first,
                                       const AtomicWriter *second)
{
    size_t length = 0U;
    char *contents = (char *)SDL_LoadFile(path, &length);
    if (contents == NULL) return false;
    const bool matchesFirst = length == first->length &&
                              memcmp(contents, first->payload, length) == 0;
    const bool matchesSecond = length == second->length &&
                               memcmp(contents, second->payload, length) == 0;
    SDL_free(contents);
    return matchesFirst || matchesSecond;
}

static bool temporary_files_are_absent(const char *path)
{
    char candidate[512];
    for (unsigned int attempt = 0U; attempt < 128U; attempt++)
    {
#ifdef _WIN32
        (void)snprintf(candidate, sizeof(candidate), "%s.tmp.%lu.%u", path,
                       (unsigned long)GetCurrentProcessId(), attempt);
#else
        (void)snprintf(candidate, sizeof(candidate), "%s.tmp.%ld.%u", path,
                       (long)getpid(), attempt);
#endif
        FILE *file = fopen(candidate, "rb");
        if (file != NULL)
        {
            fclose(file);
            return false;
        }
    }
    return true;
}

static void verify_atomic_writes_and_preferences(void)
{
    CHECK("preference override is installed for the isolated file test",
          SDL_setenv("ENDGAME_PREF_DIR", ".", 1) == 0);
    CHECK("preference paths reject traversal and empty leaf names",
          preferences_file_path("../settings.ini") == NULL &&
              preferences_file_path("folder/settings.ini") == NULL &&
              preferences_file_path("") == NULL);

    char *path = preferences_file_path(".pre-phase35-atomic-robustness.ini");
    CHECK("a valid preference leaf produces a path", path != NULL);
    if (path == NULL) return;
    (void)remove(path);

    static char firstPayload[8192];
    static char secondPayload[8192];
    memset(firstPayload, 'A', sizeof(firstPayload));
    memset(secondPayload, 'B', sizeof(secondPayload));
    firstPayload[sizeof(firstPayload) - 1U] = '\n';
    secondPayload[sizeof(secondPayload) - 1U] = '\n';
    AtomicWriter first = {path, firstPayload, sizeof(firstPayload), 0};
    AtomicWriter second = {path, secondPayload, sizeof(secondPayload), 0};

    SDL_Thread *firstThread = SDL_CreateThread(write_repeatedly, "atomic-a", &first);
    SDL_Thread *secondThread = SDL_CreateThread(write_repeatedly, "atomic-b", &second);
    CHECK("concurrent writer threads start", firstThread != NULL && secondThread != NULL);
    if (firstThread != NULL) SDL_WaitThread(firstThread, NULL);
    if (secondThread != NULL) SDL_WaitThread(secondThread, NULL);
    CHECK("all concurrent atomic writes succeed",
          first.failures == 0 && second.failures == 0);
    CHECK("concurrent writers leave one complete payload",
          file_equals_either_payload(path, &first, &second));
    CHECK("successful writes leave no same-process temporary files",
          temporary_files_are_absent(path));
    CHECK("invalid atomic destinations are rejected",
          !atomic_write_text_file(NULL, "x", 1U) &&
              !atomic_write_text_file("", "x", 1U));

    (void)remove(path);
    free(path);
}

static void verify_fixed_step_and_replay_boundaries(void)
{
    FixedStepClock clock = {NAN, APP_SCENE_ARCADE_GAME, true};
    fixed_step_clock_begin_frame(&clock, APP_SCENE_ARCADE_GAME, NAN);
    CHECK("NaN frame state is recovered without poisoning interpolation",
          clock.accumulator == 0.0 && fixed_step_clock_alpha(&clock) == 0.0f);
    fixed_step_clock_begin_frame(&clock, APP_SCENE_ARCADE_GAME, INFINITY);
    CHECK("infinite frame time is capped",
          clock.accumulator == MAX_FRAME_TIME);
    CHECK("negative step counts are rejected",
          !fixed_step_clock_should_step(&clock, APP_SCENE_ARCADE_GAME, -1));

    ReplayRecording recording;
    CHECK("baseline replay initializes",
          replay_recording_init(&recording, REPLAY_SCENE_RUNNER,
                                GAME_MODE_SINGLE_PLAYER, 1U, 1));
    recording.tickCount = REPLAY_MAX_TICKS + 1;
    CHECK("corrupted replay counts cannot index beyond the input array",
          !replay_recording_set_input(&recording, 0, (InputState){0}));
}

static void clear_arcade_entities(GameState *game)
{
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        game->enemyValues[i].visible = 0;
        game->enemyValues[i].y = 1000.0f;
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        game->smartEnemies[i].visible = 0;
        game->smartEnemies[i].y = 1000.0f;
    }
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].visible = 0;
        game->boss[i].y = 1000.0f;
    }
}

static void verify_progression_boundaries(void)
{
    arcade_waves_reset(NULL);
    arcade_waves_update(NULL);
    GameState game = {0};
    clear_arcade_entities(&game);
    arcade_waves_reset(&game);
    game.arcadeWaves.regularSpawned = INT_MAX;
    arcade_waves_update(&game);
    CHECK("corrupted wave counters recover to a valid wave-one state",
          game.arcadeWaves.waveNumber == 1 &&
              game.arcadeWaves.regularSpawned == 0);

    const WaveDefinition *definition = arcade_waves_get_definition(INT_MAX);
    game.arcadeWaves.waveNumber = INT_MAX;
    game.arcadeWaves.regularSpawned = definition->regularEnemies;
    game.arcadeWaves.smartSpawned = definition->smartEnemies;
    game.arcadeWaves.bossesSpawned = definition->bosses;
    arcade_waves_update(&game);
    CHECK("wave progression does not overflow at INT_MAX",
          game.arcadeWaves.waveNumber == INT_MAX &&
              game.arcadeWaves.restTicksRemaining > 0);

    runner_segments_reset(&game);
    const int startingSegment = game.runnerSegments.nextSegmentNumber;
    game.man.x = INFINITY;
    runner_segments_update(&game);
    CHECK("non-finite runner distance saturates safely with bounded work",
          game.x_score == INT_MAX &&
              game.runnerSegments.nextSegmentNumber ==
                  startingSegment + 2 * RUNNER_SEGMENTS_PER_EXTENSION);
    const int ledgeBeforeSaturation = game.ledges[0].x;
    game.runnerSegments.nextExtensionScore = INT_MAX;
    runner_segments_update(&game);
    runner_segments_update(&game);
    CHECK("saturated runner progression disables repeated extension work",
          game.runnerSegments.nextSegmentNumber == -1 &&
              game.ledges[0].x == ledgeBeforeSaturation);

    runner_segments_reset(&game);
    game.x_score = 100;
    game.runnerSegments.nextExtensionScore = 1;
    game.runnerSegments.nextWriteSlot = NUM_STARS - 1;
    const int ledgeBeforeCorruption = game.ledges[0].x;
    runner_segments_update(&game);
    CHECK("misaligned runner write slots are rejected before mutation",
          game.runnerSegments.nextSegmentNumber == -1 &&
              game.ledges[0].x == ledgeBeforeCorruption);

    runner_segments_reset(&game);
    game.x_score = INT_MAX;
    game.runnerSegments.nextSegmentNumber = INT_MAX;
    game.runnerSegments.nextExtensionScore = 1;
    game.runnerSegments.nextWriteSlot = 0;
    runner_segments_update(&game);
    CHECK("unrepresentable runner coordinates disable streaming without overflow",
          game.runnerSegments.nextSegmentNumber == -1);
    runner_segments_reset(NULL);
    runner_segments_update(NULL);
}

static void verify_spawn_feedback_and_collision_contracts(void)
{
    GameState game = {0};
    CHECK("spawn APIs reject null state and non-finite values",
          !enemy_spawn(NULL, 0, 0.0f, 0.0f, 0.0f, 0.0f) &&
              !enemy_spawn(&game, 0, NAN, 0.0f, 0.0f, 0.0f));
    game.enemyValues[0].isdead = 1;
    game.enemyValues[0].onLedge = 1;
    game.enemyValues[0].hitFlashTicks = 99;
    CHECK("recycled spawns clear stale runtime state",
          enemy_spawn(&game, 0, 10.0f, 20.0f, 1.0f, 2.0f) &&
              game.enemyValues[0].isdead == 0 &&
              game.enemyValues[0].onLedge == 0 &&
              game.enemyValues[0].hitFlashTicks == 0);

    combat_feedback_reset(&game);
    combat_feedback_trigger_player_hit(&game, -1);
    combat_feedback_trigger_player_hit(&game, 2);
    game.secondPlayer.facingLeft = 1;
    combat_feedback_trigger_shot(&game, 1, 10.0f, 20.0f);
    CHECK("player-two muzzle feedback honors player-two facing",
          game.feedback.particles[0].active &&
              game.feedback.particles[0].dx < 0.0f);
    game.feedback.particles[0].ticksRemaining = INT_MIN;
    CHECK("corrupted feedback timers deactivate without integer underflow",
          !combat_feedback_step(&game) &&
              !game.feedback.particles[0].active);
    CHECK("null feedback entry points are harmless",
          !combat_feedback_step(NULL));

    Collider collider = player_world_collider();
    KinematicBody invalidBody = {NAN, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Ledge ledge = {0, 100, 100, 20};
    CHECK("collision APIs reject null and non-finite operands",
          !colliders_can_interact(NULL, &collider) &&
              !collider_bounds_overlap(&invalidBody, &collider,
                                       &invalidBody, &collider) &&
              !resolve_kinematic_world(NULL, &collider, &ledge, 1).grounded);
    Ledge walls[2] = {{200, 0, 20, 200}, {100, 0, 20, 200}};
    KinematicBody fastBody = {300.0f, 40.0f, 0.0f, 40.0f, 18000.0f, 0.0f};
    const WorldCollisionResult fastResult =
        resolve_kinematic_world(&fastBody, &collider, walls, 2);
    CHECK("high-speed horizontal collision uses the nearest swept wall",
          fastResult.hitRight && fastBody.x == 100.0f - collider.width);
    collisionDetect(NULL);
    collisionDetect2(NULL);

    srandom(7U);
    const int saturated = random_sign(INT_MAX, INT_MAX);
    CHECK("random signed multiplication saturates instead of overflowing",
          saturated == INT_MAX || saturated == INT_MIN);
}

int main(void)
{
    EnvironmentSnapshot preferenceDirectory =
        capture_environment("ENDGAME_PREF_DIR");
    CHECK("capture ENDGAME_PREF_DIR without mutating it",
          preferenceDirectory.valid);
    if (!preferenceDirectory.valid)
    {
        return 1;
    }
    if (SDL_Init(0) != 0)
    {
        fprintf(stderr, "PRE-PHASE 35 ROBUSTNESS TEST: SDL_Init failed: %s\n",
                SDL_GetError());
        (void)restore_environment("ENDGAME_PREF_DIR",
                                  &preferenceDirectory);
        return 1;
    }
    verify_atomic_writes_and_preferences();
    verify_fixed_step_and_replay_boundaries();
    verify_progression_boundaries();
    verify_spawn_feedback_and_collision_contracts();
    SDL_Quit();
    CHECK("restore ENDGAME_PREF_DIR after the robustness harness",
          restore_environment("ENDGAME_PREF_DIR",
                              &preferenceDirectory));

    if (failures == 0)
    {
        printf("PRE-PHASE 35 ROBUSTNESS TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
