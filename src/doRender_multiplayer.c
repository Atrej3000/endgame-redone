#include "header.h"

void doRender_multiplayer(SDL_Renderer *renderer, GameState *game)
{
    SDL_Rect mult = {0, 0, 1280, 720};
    SDL_RenderCopy(renderer, game->mult, NULL, &mult);
    SDL_RenderPresent(renderer);

}

void doRender_multiplayer2(SDL_Renderer *renderer, GameState *game)
{
    SDL_Rect mult = {0, 0, 1280, 720};
    SDL_RenderCopy(renderer, game->mult, NULL, &mult);
    SDL_RenderPresent(renderer);

}
