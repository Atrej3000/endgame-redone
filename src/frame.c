#include "header.h"

void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer)
{
    process(game);
    collisionDetect(game);
    doRender(renderer, game);
    processEvents(window, game);
}
