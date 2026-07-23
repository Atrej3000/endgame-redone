#include "runner_segments.h"

#define RUNNER_SEGMENT_LENGTH 1500
#define RUNNER_SEGMENT_PLATFORM_SPACING 300
#define RUNNER_LEDGE_BASE_Y 600
#define RUNNER_STAR_X_OFFSET 90

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

static void write_segment(GameState *game, int firstSlot)
{
    RunnerSegmentState *state = &game->runnerSegments;
    const RunnerSegmentType type = choose_segment_type(state);
    const RunnerSegmentDefinition *definition = runner_segments_get_definition(type);
    const int baseX = state->nextSegmentNumber * RUNNER_SEGMENT_LENGTH;

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
        game->stars[slot].mode = (state->nextSegmentNumber + i) % 2;
        game->stars[slot].phase = 2.0f * 3.14f * (float)(random() % 360L) / 360.0f;
    }

    state->lastSegmentType = (int)type;
    state->nextSegmentNumber++;
}

void runner_segments_reset(GameState *game)
{
    game->runnerSegments = (RunnerSegmentState){0, 0, 55, RUNNER_SEGMENT_RECOVERY, 0};
    for (int segment = 0; segment < NUM_STARS / RUNNER_SEGMENT_PLATFORM_COUNT; segment++)
    {
        write_segment(game, segment * RUNNER_SEGMENT_PLATFORM_COUNT);
    }
    game->runnerSegments.nextWriteSlot = 0;
}

static void extend_world(GameState *game)
{
    const int firstSlot = game->runnerSegments.nextWriteSlot;
    for (int segment = 0; segment < RUNNER_SEGMENTS_PER_EXTENSION; segment++)
    {
        write_segment(game, firstSlot + segment * RUNNER_SEGMENT_PLATFORM_COUNT);
    }
    game->runnerSegments.nextWriteSlot =
        (firstSlot + RUNNER_SEGMENTS_PER_EXTENSION * RUNNER_SEGMENT_PLATFORM_COUNT) % NUM_STARS;
    game->runnerSegments.nextExtensionScore += 50;
}

void runner_segments_update(GameState *game)
{
    // A lightweight simulation caller may use a zero-initialized GameState
    // without starting a Runner session. Only runner_segments_reset() arms
    // world streaming, so it cannot overwrite independently configured traps.
    if (game->runnerSegments.nextExtensionScore <= 0)
    {
        return;
    }

    const int distanceScore = (int)(game->man.x / 293.0f);
    if (distanceScore > game->x_score)
    {
        game->x_score = distanceScore;
    }
    while (game->x_score >= game->runnerSegments.nextExtensionScore)
    {
        extend_world(game);
    }
}
