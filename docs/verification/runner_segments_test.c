#include "runner_segments.h"

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "RUNNER SEGMENTS TEST: FAIL - %s\n", message);
        failures++;
    }
}

int main(void)
{
    GameState uninitialized = {0};
    runner_segments_update(&uninitialized);
    expect_true(uninitialized.runnerSegments.nextSegmentNumber == 0 &&
                    uninitialized.runnerSegments.nextExtensionScore == 0 &&
                    uninitialized.ledges[0].x == 0,
                "uninitialized game state does not stream a Runner world");

    for (int type = RUNNER_SEGMENT_RECOVERY; type < RUNNER_SEGMENT_COUNT; type++)
    {
        const RunnerSegmentDefinition *definition = runner_segments_get_definition((RunnerSegmentType)type);
        expect_true(definition != NULL, "every declared segment type has a definition");
        for (int i = 0; definition != NULL && i < RUNNER_SEGMENT_PLATFORM_COUNT; i++)
        {
            if (i > 0)
            {
                const int verticalStep = abs(definition->ledgeYOffsets[i] - definition->ledgeYOffsets[i - 1]);
                expect_true(verticalStep <= 100,
                            "template ledges never require an unbounded vertical transition");
            }
            expect_true(definition->starYOffsets[i] <= -120,
                        "template hazards remain above their supporting ledge");
        }
    }
    expect_true(runner_segments_get_definition((RunnerSegmentType)-1) == NULL &&
                    runner_segments_get_definition(RUNNER_SEGMENT_COUNT) == NULL,
                "out-of-range template types are rejected");

    GameState game = {0};
    srandom(12345u);
    runner_segments_reset(&game);
    expect_true(game.runnerSegments.nextSegmentNumber == 20 && game.runnerSegments.nextWriteSlot == 0 &&
                    game.runnerSegments.nextExtensionScore == 55,
                "reset fills the initial 20 tested segments and primes the first extension");
    expect_true(game.ledges[0].x == 0 && game.ledges[4].x == 1200 &&
                    game.ledges[5].x == 1500 && game.ledges[99].x == 29700,
                "initial segments cover the world in fixed safe-length chunks");
    for (int i = 1; i < NUM_STARS; i++)
    {
        expect_true(game.ledges[i].x - game.ledges[i - 1].x == 300,
                    "every adjacent generated platform keeps the validated 120-pixel gap");
    }

    game.man.x = 55.0f * 293.0f;
    runner_segments_update(&game);
    expect_true(game.x_score == 55 && game.runnerSegments.nextSegmentNumber == 30 &&
                    game.runnerSegments.nextWriteSlot == 50 && game.runnerSegments.nextExtensionScore == 105,
                "first distance milestone streams ten segments and advances its threshold");
    expect_true(game.ledges[0].x == 30000 && game.ledges[49].x == 44700 && game.ledges[50].x == 15000,
                "first extension safely reuses only the buffer behind the player");

    game.man.x = 105.0f * 293.0f;
    runner_segments_update(&game);
    expect_true(game.runnerSegments.nextSegmentNumber == 40 && game.runnerSegments.nextWriteSlot == 0 &&
                    game.runnerSegments.nextExtensionScore == 155,
                "second milestone rotates to the other half of the streaming buffer");
    expect_true(game.ledges[50].x == 45000 && game.ledges[99].x == 59700,
                "second extension writes ahead without overwriting current terrain");

    if (failures == 0) printf("RUNNER SEGMENTS TEST: ALL PASS\n");
    return failures == 0 ? 0 : 1;
}
