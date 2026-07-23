#pragma once

// Deterministic simulation replay support (Phase 31). A recording owns the
// initial random seed, selected mode, and the exact fixed-tick input snapshot
// for a bounded session. It deliberately has no renderer, event-loop, or
// asset-loading dependency, so it is safe to execute in headless tests.
#include "header.h"

#define REPLAY_MAX_TICKS 600

typedef enum ReplayScene
{
    REPLAY_SCENE_ARCADE,
    REPLAY_SCENE_RUNNER
} ReplayScene;

typedef struct ReplayRecording
{
    ReplayScene scene;
    GameMode playerMode;
    unsigned int seed;
    int tickCount;
    InputState inputs[REPLAY_MAX_TICKS];
} ReplayRecording;

// A deliberately small, authoritative state snapshot. It covers the player,
// score/life/session state, and representative procedurally generated world
// values without coupling a replay assertion to render-only pointers.
typedef struct ReplayResult
{
    ReplayScene scene;
    GameMode playerMode;
    int tickCount;
    int gameLives;
    int tempScore;
    int xScore;
    int time;
    int statusState;
    float manX;
    float manY;
    float manDx;
    float manDy;
    float secondPlayerX;
    float secondPlayerY;
    float scrollX;
    int firstLedgeX;
    int firstLedgeY;
    int tenthLedgeY;
    int firstStarX;
    int firstStarY;
    unsigned int worldSignature;
} ReplayResult;

bool replay_recording_init(ReplayRecording *recording, ReplayScene scene,
                           GameMode playerMode, unsigned int seed, int tickCount);
bool replay_recording_set_input(ReplayRecording *recording, int tick,
                                InputState input);
bool replay_run(const ReplayRecording *recording, ReplayResult *result);
bool replay_results_equal(const ReplayResult *left, const ReplayResult *right);
