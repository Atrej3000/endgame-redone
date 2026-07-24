#include "header.h"
#include "combat_feedback.h"

// See docs/runner-death-lifecycle.md for the full trace and reasoning behind this design.

// Idempotent: calling this while already dying is a no-op -- no re-trigger, no restarted
// countdown, no extra decrement. Safe to call from multiple collision checks in the same frame.
void runner_trigger_death(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    if (game->multiPlayer)
    {
        game->secondPlayer.isDead = 1;
    }
    if (game->man.isDead)
    {
        return;
    }
    game->man.isDead = 1;
    game->deathCountdown = 120; // existing constant, reused verbatim -- not a new value
}

static void reset_runner_player(GameState *game, Man *player, int playerIndex)
{
    if (player->x < 0.0f)
    {
        player->x = 0.0f;
    }
    player->y = 0.0f;
    player->dx = 0.0f;
    player->dy = 0.0f;
    player->prevX = player->x;
    player->prevY = player->y;
    player->onLedge = 0;
    player->isDead = 0;
    player->coyoteTicksRemaining = 0;
    player->airJumpsRemaining = 0;
    player->doubleJumpAnimationTicks = 0;
    player->damageInvulnerabilityTicks = 0;
    player->jumpKeyHeldLastTick = false;
    player->slowingDown = 0;
    player->facingLeft = 0;
    player->visible0 = 1;
    player->animFrame = 0;
    player->animFrameSecond = 0;
    player->currentSpriteIdle = 0;
    player->currentSpriteRun = 0;
    player->currentSpriteRun2 = 0;
    player->currentSpriteJump = 0;
    player->currentSpriteJump2 = 0;
    player->hitFlashTicks = 0;
    player->animation = (AnimationPlayer){ANIMATION_STATE_IDLE, 0,
                                           FEEDBACK_ANIMATION_TICKS, 1};
    game->feedback.disappearingTicks[playerIndex] = 0;
    if (playerIndex == 0)
    {
        game->input.jumpBufferTicksPlayer1 = 0;
        game->input.moveLeftPlayer1 = false;
        game->input.moveRightPlayer1 = false;
        game->input.jumpHeldPlayer1 = false;
        game->input.shootHeldPlayer1 = false;
    }
    else
    {
        game->input.jumpBufferTicksPlayer2 = 0;
        game->input.moveLeftPlayer2 = false;
        game->input.moveRightPlayer2 = false;
        game->input.jumpHeldPlayer2 = false;
        game->input.shootHeldPlayer2 = false;
    }
    combat_feedback_trigger_player_spawn(game, playerIndex);
}

void runner_death_step(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    if (!game->man.isDead)
    {
        return;
    }
    if (game->deathCountdown > 0)
    {
        game->deathCountdown--;
        if (game->deathCountdown > 0)
        {
            return;
        }
    }
    if (game->gameLives > 0)
    {
        game->gameLives--;
    }
    if (game->gameLives < 0)
    {
        game->gameLives = 0;
    }
    if (game->gameLives == 0)
    {
        game->deathCountdown = -1;
        return;
    }
    reset_runner_player(game, &game->man, 0);
    if (game->multiPlayer)
    {
        reset_runner_player(game, &game->secondPlayer, 1);
    }
    game->deathCountdown = -1;
}

// Compatibility no-op for callers that still invoke the former render-frame
// update hook. runner_simulate() owns authoritative progression.
void runner_update_death(GameState *game)
{
    (void)game;
}
