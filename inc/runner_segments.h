#pragma once

// Authored, bounded Runner world segments (Phase 33). Templates describe
// safe ledge/trap arrangements; selection remains seed-driven but never uses
// unconstrained per-platform random placement.
#include "header.h"

#define RUNNER_SEGMENT_PLATFORM_COUNT 5
#define RUNNER_SEGMENTS_PER_EXTENSION 10

typedef enum RunnerSegmentType
{
    RUNNER_SEGMENT_RECOVERY,
    RUNNER_SEGMENT_GENTLE_RISE,
    RUNNER_SEGMENT_VALLEY,
    RUNNER_SEGMENT_HIGH_LOW,
    RUNNER_SEGMENT_COUNT
} RunnerSegmentType;

typedef struct RunnerSegmentDefinition
{
    int ledgeYOffsets[RUNNER_SEGMENT_PLATFORM_COUNT];
    int starYOffsets[RUNNER_SEGMENT_PLATFORM_COUNT];
} RunnerSegmentDefinition;

const RunnerSegmentDefinition *runner_segments_get_definition(RunnerSegmentType type);
void runner_segments_reset(GameState *game);
void runner_segments_update(GameState *game);
