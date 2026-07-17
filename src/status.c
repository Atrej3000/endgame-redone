#include "header.h"

void init_status_lives(GameState *game) 
{
    char str[128] = "";
    sprintf(str, "Lifes %d", (int)game->gameLives);

    SDL_Color white = {255, 255, 255, 255};

  // SDL_Surface *tmp = TTF_RenderText_Blended(game->font, "TRIPLE AAA PROJECT!!!", white);
   SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
    game->labelW = tmp->w;
    game->labelH = tmp->h;
    if (game->label) {
        SDL_DestroyTexture(game->label);
        game->label = NULL;
    }
    game->label = SDL_CreateTextureFromSurface(game->renderer, tmp);
    SDL_FreeSurface(tmp);
}

void draw_status_lives(GameState *game) 
{
    SDL_Renderer *renderer = game->renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    // Clear screen
    SDL_RenderClear(renderer);

    SDL_Rect rect = {640, 360, 54, 54};
    SDL_RenderCopyEx(renderer, game->manFrames[0], NULL, &rect, 
    0, NULL, (game->man.facingLeft == 1));

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {700, 360, game->labelW, game->labelH};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);
}

void shutdown_status_lives (GameState *game)
{
    SDL_DestroyTexture(game->label);
    game->label = NULL;
}
