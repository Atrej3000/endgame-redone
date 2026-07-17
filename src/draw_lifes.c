#include "header.h"

void draw_status_lives2(GameState *game)
{
    SDL_Renderer *renderer = game->renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    // Clear screen
    //SDL_RenderClear(renderer);


    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {570, 28, game->labelW, game->labelH};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);
}
