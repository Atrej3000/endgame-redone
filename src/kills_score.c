#include "header.h"

void init_status_kills(GameState *game)
{
    char str[32] = "";
    sprintf(str, "Kills: %d", (int)game->kills_score);

    SDL_Color white = {0, 255, 255, 255};

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

    if(game->multiPlayer)
    {
    char str[128] = "";
    sprintf(str, "Kills: %d", (int)game->kills_score_multi);

    SDL_Color white = {0, 255, 255, 255};

    //SDL_Surface *tmp = TTF_RenderText_Blended(game->font, "TRIPLE AAA PROJECT!!!", white);
    SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
    game->labelW = tmp->w;
    game->labelH = tmp->h;
    if (game->labelMultiplayer) {
        SDL_DestroyTexture(game->labelMultiplayer);
        game->labelMultiplayer = NULL;
    }
    game->labelMultiplayer = SDL_CreateTextureFromSurface(game->renderer, tmp);
    SDL_FreeSurface(tmp);
    }

}

void draw_status_kills(GameState *game)
{
    SDL_Renderer *renderer = game->renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {50, 00, game->labelW, game->labelH};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);

    if (game->multiPlayer)
    {
    SDL_Renderer *renderer = game->renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {1150, 00, game->labelW, game->labelH};
    SDL_RenderCopy(renderer, game->labelMultiplayer, NULL, &textRect);
    }

}
