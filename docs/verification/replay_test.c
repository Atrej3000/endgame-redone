#include "replay.h"

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "REPLAY TEST: FAIL - %s\n", message);
        failures++;
    }
}

int main(void)
{
    ReplayRecording recording;
    expect_true(replay_recording_init(&recording, REPLAY_SCENE_RUNNER, GAME_MODE_SINGLE_PLAYER, 12345u, 180),
                "a bounded single-player recording initializes");
    expect_true(recording.seed == 12345u && recording.scene == REPLAY_SCENE_RUNNER &&
                    recording.playerMode == GAME_MODE_SINGLE_PLAYER &&
                    recording.tickCount == 180,
                "the recording retains seed, scene, player mode, and tick count");

    for (int tick = 0; tick < recording.tickCount; tick++)
    {
        InputState input = {0};
        input.moveRightPlayer1 = tick >= 20 && tick < 100;
        input.moveLeftPlayer1 = tick >= 130 && tick < 150;
        input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
        expect_true(replay_recording_set_input(&recording, tick, input),
                    "each in-range tick accepts an input snapshot");
    }

    ReplayResult first;
    ReplayResult second;
    expect_true(replay_run(&recording, &first), "the first replay run succeeds");
    expect_true(replay_run(&recording, &second), "the same replay can run again");
    expect_true(replay_results_equal(&first, &second),
                "same seed, mode, and per-tick input reach exactly the same final state");

    ReplayRecording changedInput = recording;
    InputState divergent = changedInput.inputs[20];
    divergent.moveRightPlayer1 = false;
    divergent.moveLeftPlayer1 = true;
    expect_true(replay_recording_set_input(&changedInput, 20, divergent),
                "an existing tick can be intentionally changed");
    ReplayResult changedResult;
    expect_true(replay_run(&changedInput, &changedResult), "the changed-input replay succeeds");
    expect_true(!replay_results_equal(&first, &changedResult),
                "a changed per-tick input changes the resulting state");

    ReplayRecording changedSeed = recording;
    changedSeed.seed = 54321u;
    ReplayResult changedSeedResult;
    expect_true(replay_run(&changedSeed, &changedSeedResult), "the changed-seed replay succeeds");
    expect_true(first.worldSignature != changedSeedResult.worldSignature,
                "the stored seed controls Runner's generated world");

    ReplayRecording multiplayer;
    expect_true(replay_recording_init(&multiplayer, REPLAY_SCENE_RUNNER, GAME_MODE_MULTIPLAYER, 77u, 60),
                "a multiplayer recording initializes");
    for (int tick = 0; tick < multiplayer.tickCount; tick++)
    {
        InputState input = {0};
        input.moveRightPlayer2 = true;
        expect_true(replay_recording_set_input(&multiplayer, tick, input),
                    "multiplayer input snapshots are accepted");
    }
    ReplayResult multiplayerFirst;
    ReplayResult multiplayerSecond;
    expect_true(replay_run(&multiplayer, &multiplayerFirst) && replay_run(&multiplayer, &multiplayerSecond) &&
                    replay_results_equal(&multiplayerFirst, &multiplayerSecond),
                "multiplayer replay is deterministic too");

    ReplayRecording arcade;
    expect_true(replay_recording_init(&arcade, REPLAY_SCENE_ARCADE, GAME_MODE_SINGLE_PLAYER, 99u, 90),
                "an Arcade recording initializes");
    for (int tick = 0; tick < arcade.tickCount; tick++)
    {
        InputState input = {0};
        input.moveRightPlayer1 = tick < 45;
        input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
        input.shootHeldPlayer1 = tick >= 10 && tick < 70;
        expect_true(replay_recording_set_input(&arcade, tick, input),
                    "Arcade input snapshots are accepted");
    }
    ReplayResult arcadeFirst;
    ReplayResult arcadeSecond;
    expect_true(replay_run(&arcade, &arcadeFirst) && replay_run(&arcade, &arcadeSecond) &&
                    replay_results_equal(&arcadeFirst, &arcadeSecond),
                "Arcade replay uses the real Arcade simulation deterministically");

    expect_true(!replay_recording_init(NULL, REPLAY_SCENE_RUNNER, GAME_MODE_SINGLE_PLAYER, 1u, 1) &&
                    !replay_recording_init(&recording, REPLAY_SCENE_RUNNER, GAME_MODE_SINGLE_PLAYER, 1u, -1) &&
                    !replay_recording_init(&recording, REPLAY_SCENE_RUNNER, GAME_MODE_SINGLE_PLAYER, 1u,
                                           REPLAY_MAX_TICKS + 1) &&
                    !replay_recording_init(&recording, (ReplayScene)2, GAME_MODE_SINGLE_PLAYER, 1u, 1),
                "invalid recording initialization is rejected");
    expect_true(!replay_recording_set_input(&recording, -1, (InputState){0}) &&
                    !replay_recording_set_input(&recording, recording.tickCount, (InputState){0}) &&
                    !replay_run(NULL, &first) && !replay_run(&recording, NULL),
                "invalid replay API boundaries are rejected");

    if (failures == 0)
    {
        printf("REPLAY TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
