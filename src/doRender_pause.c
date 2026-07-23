#include "header.h"

int doRender_pause(SDL_Renderer *renderer, GameState *game)
{
    int paus = 0;
    if (!renderer || !game)
    {
        return paus;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect pause = {0, 0, WIDTH, HEIGHT};
    if (game->pause)
    {
        (void)SDL_RenderCopy(renderer, game->pause, NULL, &pause);
    }
    SDL_RenderPresent(renderer);
    return paus;
}
