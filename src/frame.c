#include "header.h"
#include "frame.h"

void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer)
{
    process(game);
    collisionDetect(game);
    doRender(renderer, game);
    processEvents(window, game);
}

void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer)
{
    process2(game);
    collisionDetect2(game);
    doRender2(renderer, game);
    runner_update_death(game); // see src/runner_death.c, docs/runner-death-lifecycle.md
    processEvents2(window, game);
}
