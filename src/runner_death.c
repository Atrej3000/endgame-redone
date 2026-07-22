#include "header.h"

// See docs/runner-death-lifecycle.md for the full trace and reasoning behind this design.

// Idempotent: calling this while already dying is a no-op -- no re-trigger, no restarted
// countdown, no extra decrement. Safe to call from multiple collision checks in the same frame.
void runner_trigger_death(GameState *game)
{
    if (game->man.isDead)
    {
        return;
    }
    game->man.isDead = 1;
    game->deathCountdown = 120; // existing constant, reused verbatim -- not a new value
}

// Called once per frame from runner_frame(), after doRender2() (same relative position
// runner_resolve_death() held in Phase 5), before processEvents2().
void runner_update_death(GameState *game)
{
    if (!game->man.isDead)
    {
        return;
    }
    if (game->deathCountdown > 0)
    {
        game->deathCountdown--;
        return; // still mid-animation; nothing else happens yet
    }
    // Countdown finished this frame: exactly one decrement, then respawn both players safely.
    game->gameLives--;
    game->man.isDead = 0;
    game->man.y = 0;
    game->man.dy = 0;
    if (game->man.x < 0) // only clamp the specific condition that would otherwise re-trigger
    {
        game->man.x = 0;
    }
    game->man.prevX = game->man.x;
    game->man.prevY = game->man.y;
    if (game->multiPlayer)
    {
        game->secondPlayer.y = 0;
        game->secondPlayer.dy = 0;
        if (game->secondPlayer.x < 0)
        {
            game->secondPlayer.x = 0;
        }
        game->secondPlayer.prevX = game->secondPlayer.x;
        game->secondPlayer.prevY = game->secondPlayer.y;
    }
    game->deathCountdown = -1; // idle sentinel, matches runner_session_reset()'s own value
}
