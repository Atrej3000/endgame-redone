#include "header.h"

void draw_status_lives2(GameState *game)
{
    if (!game || !game->app.renderer || !game->label) return;
    SDL_Renderer *renderer = game->app.renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {570, 28, game->labelW, game->labelH};
    (void)SDL_RenderCopy(renderer, game->label, NULL, &textRect);
}
