// Cross-mode/session and fixed-step regression checks discovered by the
// pre-Phase-35 whole-game audit.
#include "fixed_step.h"

static int failures = 0;

#define CHECK(description, condition)                                           \
    do                                                                          \
    {                                                                           \
        if (condition)                                                          \
        {                                                                       \
            printf("PRE-PHASE35 INTEGRATION TEST: PASS - %s\n", description);   \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            printf("PRE-PHASE35 INTEGRATION TEST: FAIL - %s\n", description);   \
            failures++;                                                         \
        }                                                                       \
    } while (0)

static bool input_is_clear(const InputState *input)
{
    return !input->moveLeftPlayer1 && !input->moveRightPlayer1 &&
           !input->jumpHeldPlayer1 && !input->shootHeldPlayer1 &&
           !input->moveLeftPlayer2 && !input->moveRightPlayer2 &&
           !input->jumpHeldPlayer2 && !input->shootHeldPlayer2 &&
           input->jumpBufferTicksPlayer1 == 0 &&
           input->jumpBufferTicksPlayer2 == 0;
}

static void verify_fixed_step_clock(void)
{
    FixedStepClock clock = {0};

    fixed_step_clock_begin_frame(&clock, APP_SCENE_ARCADE_GAME,
                                 (double)PHYSICS_DT * 0.75);
    CHECK("partial Arcade tick is retained inside the same gameplay scene",
          !fixed_step_clock_should_step(&clock, APP_SCENE_ARCADE_GAME, 0) &&
              fixed_step_clock_alpha(&clock) > 0.7f);

    fixed_step_clock_begin_frame(&clock, APP_SCENE_RUNNER_GAME, 0.0);
    CHECK("Arcade fractional time is not inherited by Runner",
          clock.accumulator == 0.0 && fixed_step_clock_alpha(&clock) == 0.0f);

    fixed_step_clock_begin_frame(&clock, APP_SCENE_RUNNER_GAME, MAX_FRAME_TIME);
    int steps = 0;
    while (fixed_step_clock_should_step(&clock, APP_SCENE_RUNNER_GAME, steps))
    {
        fixed_step_clock_consume_step(&clock);
        steps++;
    }
    fixed_step_clock_finish_frame(&clock, APP_SCENE_RUNNER_GAME, steps);
    CHECK("fixed-step catch-up is capped", steps == MAX_PHYSICS_STEPS_PER_FRAME);
    CHECK("capped backlog drops whole ticks and keeps only interpolation remainder",
          clock.accumulator >= 0.0 &&
              clock.accumulator < (double)PHYSICS_DT &&
              fixed_step_clock_alpha(&clock) >= 0.0f &&
              fixed_step_clock_alpha(&clock) <= 1.0f);

    fixed_step_clock_finish_frame(&clock, APP_SCENE_RUNNER_MENU, 1);
    CHECK("leaving gameplay clears the scheduling clock immediately",
          clock.scene == APP_SCENE_RUNNER_MENU &&
              clock.accumulator == 0.0 &&
              !fixed_step_clock_should_step(&clock, APP_SCENE_RUNNER_MENU, 0));
}

static void dirty_cross_mode_state(GameState *game)
{
    game->input = (InputState){
        .moveLeftPlayer1 = true,
        .shootHeldPlayer2 = true,
        .jumpBufferTicksPlayer1 = 5,
        .jumpBufferTicksPlayer2 = 6,
    };
    game->events.count = 17;
    game->app.controllerJumpHeldLastFrame = true;
    game->secondPlayer.x = -500.0f;
    game->secondPlayer.y = 5000.0f;
    game->secondPlayer.dx = 99.0f;
    game->secondPlayer.dy = 101.0f;
    game->secondPlayer.isDead = 1;
    game->secondPlayer.hitFlashTicks = 9;
    game->secondPlayer.animation.state = ANIMATION_STATE_HIT;
    game->secondPlayer.animation.frame = 6;
    game->secondBullets[0].active = true;
    game->secondBullets[MAX_BULLETS - 1].active = true;
    game->enemyValues[0].visible = 1;
    game->smartEnemies[0].visible = 1;
    game->boss[0].visible = 1;
    game->man.hitFlashTicks = 7;
    game->man.animation.state = ANIMATION_STATE_HIT;
    game->man.animation.frame = 4;
}

static void verify_arcade_reset(void)
{
    GameState game = {0};
    dirty_cross_mode_state(&game);

    arcade_session_reset(&game, GAME_MODE_SINGLE_PLAYER);

    CHECK("Arcade single-player reset fully clears input and queued events",
          input_is_clear(&game.input) &&
              game.events.count == 0 &&
              !game.app.controllerJumpHeldLastFrame);
    CHECK("Arcade single-player reset initializes hidden P2 deterministically",
          game.secondPlayer.visible0 == 0 &&
              game.secondPlayer.lives == 0 &&
              game.secondPlayer.dx == 0.0f &&
              game.secondPlayer.dy == 0.0f &&
              game.secondPlayer.isDead == 0);
    CHECK("Arcade reset clears the P2 projectile pool even in single-player",
          !game.secondBullets[0].active &&
              !game.secondBullets[MAX_BULLETS - 1].active);
    CHECK("Arcade reset clears animation and hit-flash state for both players",
          game.man.animation.state == ANIMATION_STATE_IDLE &&
              game.man.animation.frame == 0 &&
              game.man.hitFlashTicks == 0 &&
              game.secondPlayer.animation.state == ANIMATION_STATE_IDLE &&
              game.secondPlayer.animation.frame == 0 &&
              game.secondPlayer.hitFlashTicks == 0);
    CHECK("Arcade reset marks every unspawned enemy category inactive",
          game.enemyValues[0].visible == 0 &&
              game.smartEnemies[0].visible == 0 &&
              game.boss[0].visible == 0);
}

static void verify_runner_reset(void)
{
    GameState game = {0};
    dirty_cross_mode_state(&game);

    runner_session_reset(&game, GAME_MODE_SINGLE_PLAYER);

    CHECK("Runner single-player reset initializes hidden P2 deterministically",
          game.secondPlayer.visible0 == 0 &&
              game.secondPlayer.lives == 0 &&
              game.secondPlayer.x == 100.0f &&
              game.secondPlayer.y == 80.0f &&
              game.secondPlayer.dx == 0.0f &&
              game.secondPlayer.dy == 0.0f);
    CHECK("Runner reset clears stale input, events, animation, and flash",
          input_is_clear(&game.input) &&
              game.events.count == 0 &&
              game.man.animation.state == ANIMATION_STATE_IDLE &&
              game.man.hitFlashTicks == 0 &&
              game.secondPlayer.animation.state == ANIMATION_STATE_IDLE &&
              game.secondPlayer.hitFlashTicks == 0);
}

int main(void)
{
    CHECK("event queue can represent both full projectile pools",
          MAX_GAME_EVENTS >= MAX_BULLETS * 2);
    verify_fixed_step_clock();
    verify_arcade_reset();
    verify_runner_reset();

    if (failures == 0)
    {
        printf("PRE-PHASE35 INTEGRATION TEST: ALL PASS\n");
        return 0;
    }
    printf("PRE-PHASE35 INTEGRATION TEST: %d FAILURE(S)\n", failures);
    return 1;
}
