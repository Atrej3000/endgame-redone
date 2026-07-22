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
    game->label = SDL_CreateTextureFromSurface(game->app.renderer, tmp);
    SDL_FreeSurface(tmp);

    if(game->multiPlayer)
    {
    char multiplayerStr[128] = "";
    sprintf(multiplayerStr, "Kills: %d", (int)game->kills_score_multi);

    SDL_Color multiplayerWhite = {0, 255, 255, 255};

    //SDL_Surface *tmp = TTF_RenderText_Blended(game->font, "TRIPLE AAA PROJECT!!!", multiplayerWhite);
    SDL_Surface *multiplayerSurface = TTF_RenderText_Blended(game->font, multiplayerStr, multiplayerWhite);
    game->labelW = multiplayerSurface->w;
    game->labelH = multiplayerSurface->h;
    if (game->labelMultiplayer) {
        SDL_DestroyTexture(game->labelMultiplayer);
        game->labelMultiplayer = NULL;
    }
    game->labelMultiplayer = SDL_CreateTextureFromSurface(game->app.renderer, multiplayerSurface);
    SDL_FreeSurface(multiplayerSurface);
    }

}

void draw_status_kills(GameState *game)
{
    SDL_Renderer *renderer = game->app.renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {50, 00, game->labelW, game->labelH};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);

    if (game->multiPlayer)
    {
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect multiplayerTextRect = {1150, 00, game->labelW, game->labelH};
    SDL_RenderCopy(renderer, game->labelMultiplayer, NULL, &multiplayerTextRect);
    }

}
