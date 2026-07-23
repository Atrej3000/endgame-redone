#include "combat_feedback.h"

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "COMBAT FEEDBACK TEST: FAIL - %s\n", message);
        failures++;
    }
}

static int active_particle_count(const GameState *game)
{
    int count = 0;
    for (int i = 0; i < MAX_FEEDBACK_PARTICLES; i++)
    {
        if (game->feedback.particles[i].active) count++;
    }
    return count;
}

int main(void)
{
    GameState game = {0};
    game.app.settings.screenShake = true;
    game.man.facingLeft = 0;

    combat_feedback_trigger_shot(&game, 0, 100.0f, 200.0f);
    expect_true(game.feedback.muzzleFlashTicks[0] == FEEDBACK_MUZZLE_FLASH_TICKS &&
                    active_particle_count(&game) == 1,
                "shot creates a bounded muzzle flash and particle");

    Enemies enemy = {.x = 300.0f, .y = 150.0f, .w = 55.0f, .h = 55.0f, .visible = 1};
    combat_feedback_trigger_enemy_hit(&game, &enemy, false, false);
    expect_true(enemy.hitFlashTicks == FEEDBACK_HIT_FLASH_TICKS &&
                    game.feedback.hitStopTicks == FEEDBACK_HIT_STOP_TICKS &&
                    game.feedback.screenShakeTicks == FEEDBACK_SCREEN_SHAKE_TICKS,
                "regular hit arms flash, hit-stop, and opted-in shake");
    expect_true(active_particle_count(&game) == 4,
                "regular impact adds a small deterministic burst");

    expect_true(combat_feedback_step(&game) && combat_feedback_step(&game) &&
                    !combat_feedback_step(&game),
                "hit-stop freezes exactly its configured number of fixed ticks");
    expect_true(game.feedback.screenShakeOffsetX == 4 || game.feedback.screenShakeOffsetX == -4,
                "screen shake exposes a bounded render offset while active");

    combat_feedback_trigger_enemy_hit(&game, &enemy, true, true);
    expect_true(game.feedback.hitStopTicks == FEEDBACK_BOSS_HIT_STOP_TICKS &&
                    active_particle_count(&game) >= 6,
                "boss defeat receives stronger hit-stop and a death burst");

    game.app.scene = APP_SCENE_ARCADE_GAME;
    game.man.onLedge = 1;
    game.man.dx = 60.0f;
    combat_feedback_update_animations(&game);
    expect_true(game.man.animation.state == ANIMATION_STATE_RUN && game.man.animation.frameCount == 12,
                "Arcade player uses an independent running animation clock");
    for (int tick = 0; tick < FEEDBACK_ANIMATION_TICKS; tick++)
    {
        combat_feedback_update_animations(&game);
    }
    expect_true(game.man.animation.frame == 1,
                "animation advances at its fixed, per-entity cadence");

    game.multiPlayer = GAME_MODE_MULTIPLAYER;
    game.secondPlayer.onLedge = 1;
    game.secondPlayer.dx = 60.0f;
    combat_feedback_update_animations(&game);
    expect_true(game.secondPlayer.animation.state == ANIMATION_STATE_RUN &&
                    game.secondPlayer.animation.frameCount == 12,
                "Arcade Pink Man uses its own 12-frame run sheet");

    game.secondPlayer.onLedge = 0;
    game.secondPlayer.dy = -1.0f;
    game.secondPlayer.doubleJumpAnimationTicks = DOUBLE_JUMP_ANIMATION_TICKS;
    combat_feedback_update_animations(&game);
    expect_true(game.secondPlayer.animation.state == ANIMATION_STATE_DOUBLE_JUMP &&
                    game.secondPlayer.animation.frameCount == 6,
                "Arcade Pink Man selects the 6-frame double-jump sheet");

    game.app.scene = APP_SCENE_RUNNER_GAME;
    game.man.onLedge = 0;
    game.man.dy = -1.0f;
    combat_feedback_update_animations(&game);
    expect_true(game.man.animation.state == ANIMATION_STATE_JUMP && game.man.animation.frameCount == 1,
                "Runner uses its full per-player animation sequence");

    game.enemyValues[0] = enemy;
    combat_feedback_update_animations(&game);
    expect_true(game.enemyValues[0].animation.state == ANIMATION_STATE_RUN &&
                    game.enemyValues[0].animation.frameCount == 4,
                "regular enemies own animation state instead of sharing a global frame");

    if (failures == 0) printf("COMBAT FEEDBACK TEST: ALL PASS\n");
    return failures == 0 ? 0 : 1;
}
