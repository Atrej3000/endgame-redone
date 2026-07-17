#include "header.h"

void init_status_x_list(GameState *game) {
    for (int i = 0; i < 20; i++) {
        char str[32] = "";
        sprintf(str, "%.2d. %10d points", i + 1, game->x_list[i]);

        SDL_Color white = {255, 255, 255, 255};

        //SDL_Surface *tmp = TTF_RenderText_Blended(game->font, "TRIPLE AAA PROJECT!!!", white);
        SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
        game->labelW = tmp->w;
        game->labelH = tmp->h;
        if (game->label) {
            SDL_DestroyTexture(game->label);
            game->label = NULL;
        }
        game->label = SDL_CreateTextureFromSurface(game->renderer, tmp);
        SDL_FreeSurface(tmp);
        //}
//}
//void draw_status_x_list(GameState *game) {
//    for (int i = 0; i < 25; i++) {
        SDL_Renderer *renderer = game->renderer;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_Rect textRect = {370, 50 + i * 28, game->labelW, game->labelH};
        SDL_RenderCopy(renderer, game->label, NULL, &textRect);
    }
}
void shutdown_status_x_list(GameState *game) {
    SDL_DestroyTexture(game->label);
    game->label = NULL;
}
