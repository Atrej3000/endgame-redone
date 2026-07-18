#include "header.h"

void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer)
{
    process(game);
    collisionDetect(game);
    doRender(renderer, game);
    processEvents(window, game);
}

// Relocated from doRender2() (see docs/frame-pipeline-map.md): the death
// sprite draw stays in doRender2(), gated on the same `man.isDead` check --
// only the state mutation moves here, run after doRender2() so it still
// sees isDead==1 for that one frame's draw.
static void runner_resolve_death(GameState *game)
{
    if (game->man.isDead)
    {
        game->gameLives--;
        game->man.isDead = 0;
        game->man.y = 0;
    }
}

void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer)
{
    process2(game);
    collisionDetect2(game);
    doRender2(renderer, game);
    runner_resolve_death(game);
    processEvents2(window, game);
}
