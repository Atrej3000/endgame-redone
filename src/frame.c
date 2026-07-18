#include "header.h"

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
    processEvents2(window, game);
}
