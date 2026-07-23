#include "runner_segments.h"

#include <stdint.h>

#define RUNNER_SEGMENT_LENGTH 1500
#define RUNNER_SEGMENT_PLATFORM_SPACING 300
#define RUNNER_LEDGE_BASE_Y 600
#define RUNNER_STAR_X_OFFSET 90
#define RUNNER_SCORE_PIXELS 293.0
#define RUNNER_MAX_EXTENSIONS_PER_TICK 2

_Static_assert(NUM_STARS % RUNNER_SEGMENT_PLATFORM_COUNT == 0,
               "Runner buffers must hold complete segments");

static const RunnerSegmentDefinition segmentDefinitions[RUNNER_SEGMENT_COUNT] = {
    {{0, 0, 0, 0, 0}, {-140, -120, -140, -120, -140}},
    {{0, -50, -100, -50, 0}, {-140, -120, -140, -120, -140}},
    {{-50, 0, 50, 0, -50}, {-130, -150, -130, -150, -130}},
    {{-100, 0, -100, 0, -100}, {-150, -130, -150, -130, -150}},
};

const RunnerSegmentDefinition *runner_segments_get_definition(RunnerSegmentType type)
{
    if (type < RUNNER_SEGMENT_RECOVERY || type >= RUNNER_SEGMENT_COUNT)
    {
        return NULL;
    }
    return &segmentDefinitions[type];
}

static RunnerSegmentType choose_segment_type(RunnerSegmentState *state)
{
    if (state->nextSegmentNumber == 0 || state->nextSegmentNumber % 4 == 0)
    {
        return RUNNER_SEGMENT_RECOVERY;
    }

    state->difficultyTier = state->nextSegmentNumber / 4;
    const int maxTemplate = state->difficultyTier >= 2 ? RUNNER_SEGMENT_HIGH_LOW : RUNNER_SEGMENT_VALLEY;
    RunnerSegmentType type = (RunnerSegmentType)(1 + (int)(random() % (long)maxTemplate));
    if ((int)type == state->lastSegmentType)
    {
        type = (RunnerSegmentType)((int)type % maxTemplate + 1);
    }
    return type;
}

static bool segment_coordinates_fit(int segmentNumber)
{
    if (segmentNumber < 0) return false;
    const int64_t finalX = (int64_t)segmentNumber * RUNNER_SEGMENT_LENGTH +
                           (RUNNER_SEGMENT_PLATFORM_COUNT - 1) *
                               RUNNER_SEGMENT_PLATFORM_SPACING +
                           RUNNER_STAR_X_OFFSET;
    return finalX <= INT_MAX;
}

static bool write_segment(GameState *game, int firstSlot)
{
    RunnerSegmentState *state = &game->runnerSegments;
    if (!segment_coordinates_fit(state->nextSegmentNumber)) return false;
    const RunnerSegmentType type = choose_segment_type(state);
    const RunnerSegmentDefinition *definition = runner_segments_get_definition(type);
    if (definition == NULL) return false;
    const int baseX = (int)((int64_t)state->nextSegmentNumber * RUNNER_SEGMENT_LENGTH);

    for (int i = 0; i < RUNNER_SEGMENT_PLATFORM_COUNT; i++)
    {
        const int slot = (firstSlot + i) % NUM_STARS;
        const int ledgeX = baseX + i * RUNNER_SEGMENT_PLATFORM_SPACING;
        const int ledgeY = RUNNER_LEDGE_BASE_Y + definition->ledgeYOffsets[i];
        game->ledges[slot] = (Ledge){ledgeX, ledgeY, 180, 60};

        const int starX = ledgeX + RUNNER_STAR_X_OFFSET;
        const int starY = ledgeY + definition->starYOffsets[i];
        game->stars[slot].baseX = starX;
        game->stars[slot].baseY = starY;
        game->stars[slot].x = starX;
        game->stars[slot].y = starY;
        game->stars[slot].prevX = (float)starX;
        game->stars[slot].prevY = (float)starY;
        game->stars[slot].mode = (state->nextSegmentNumber + i) % 2;
        game->stars[slot].phase = 2.0f * 3.14f * (float)(random() % 360L) / 360.0f;
    }

    state->lastSegmentType = (int)type;
    state->nextSegmentNumber++;
    return true;
}

void runner_segments_reset(GameState *game)
{
    if (game == NULL) return;
    game->runnerSegments = (RunnerSegmentState){0, 0, 55, RUNNER_SEGMENT_RECOVERY, 0};
    for (int segment = 0; segment < NUM_STARS / RUNNER_SEGMENT_PLATFORM_COUNT; segment++)
    {
        if (!write_segment(game, segment * RUNNER_SEGMENT_PLATFORM_COUNT))
        {
            game->runnerSegments.nextSegmentNumber = -1;
            game->runnerSegments.nextExtensionScore = INT_MAX;
            break;
        }
    }
    game->runnerSegments.nextWriteSlot = 0;
}

static bool extend_world(GameState *game)
{
    RunnerSegmentState *state = &game->runnerSegments;
    const int firstSlot = state->nextWriteSlot;
    const int extensionSlots = RUNNER_SEGMENTS_PER_EXTENSION *
                               RUNNER_SEGMENT_PLATFORM_COUNT;
    const int64_t lastSegmentValue = (int64_t)state->nextSegmentNumber +
                                     RUNNER_SEGMENTS_PER_EXTENSION - 1;
    if (firstSlot < 0 || firstSlot > NUM_STARS - extensionSlots ||
        firstSlot % extensionSlots != 0 ||
        state->nextSegmentNumber < 0 ||
        lastSegmentValue > INT_MAX ||
        !segment_coordinates_fit((int)lastSegmentValue))
    {
        state->nextSegmentNumber = -1;
        state->nextExtensionScore = INT_MAX;
        return false;
    }
    for (int segment = 0; segment < RUNNER_SEGMENTS_PER_EXTENSION; segment++)
    {
        if (!write_segment(game, firstSlot + segment * RUNNER_SEGMENT_PLATFORM_COUNT))
        {
            state->nextSegmentNumber = -1;
            state->nextExtensionScore = INT_MAX;
            return false;
        }
    }
    state->nextWriteSlot =
        (firstSlot + RUNNER_SEGMENTS_PER_EXTENSION * RUNNER_SEGMENT_PLATFORM_COUNT) % NUM_STARS;
    if (state->nextExtensionScore > INT_MAX - 50)
    {
        state->nextExtensionScore = INT_MAX;
        state->nextSegmentNumber = -1;
    }
    else
    {
        state->nextExtensionScore += 50;
    }
    return true;
}

static int runner_distance_score(float x)
{
    if (isnan(x) || x <= 0.0f) return 0;
    const double score = (double)x / RUNNER_SCORE_PIXELS;
    if (!isfinite(score) || score >= (double)INT_MAX) return INT_MAX;
    return (int)score;
}

void runner_segments_update(GameState *game)
{
    // A lightweight simulation caller may use a zero-initialized GameState
    // without starting a Runner session. Only runner_segments_reset() arms
    // world streaming, so it cannot overwrite independently configured traps.
    if (game == NULL || game->runnerSegments.nextExtensionScore <= 0 ||
        game->runnerSegments.nextSegmentNumber < 0)
    {
        return;
    }

    const int distanceScore = runner_distance_score(game->man.x);
    if (distanceScore > game->x_score)
    {
        game->x_score = distanceScore;
    }
    if (game->x_score == INT_MAX &&
        game->runnerSegments.nextExtensionScore == INT_MAX)
    {
        game->runnerSegments.nextSegmentNumber = -1;
        return;
    }
    int extensions = 0;
    while (game->x_score >= game->runnerSegments.nextExtensionScore &&
           extensions < RUNNER_MAX_EXTENSIONS_PER_TICK)
    {
        if (!extend_world(game)) break;
        extensions++;
    }
}
