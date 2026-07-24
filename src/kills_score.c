#include "header.h"

void init_status_kills(GameState *game)
{
    if (!game || !game->font || !game->app.renderer) return;
    SDL_Color white = {0, 255, 255, 255};
    if (!game->cachedKillsValid ||
        game->cachedKillsValue != game->kills_score ||
        !game->killsLabel)
    {
        char str[32] = "";
        (void)snprintf(str, sizeof(str), "P1 Kills: %d", game->kills_score);
        SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
        if (tmp)
        {
            SDL_Texture *replacement =
                SDL_CreateTextureFromSurface(game->app.renderer, tmp);
            if (replacement)
            {
                destroy_texture(&game->killsLabel);
                game->killsLabel = replacement;
                game->killsLabelW = tmp->w;
                game->killsLabelH = tmp->h;
                game->cachedKillsValue = game->kills_score;
                game->cachedKillsValid = true;
            }
            SDL_FreeSurface(tmp);
        }
    }

    if(game->multiPlayer)
    {
        if (!game->cachedMultiplayerKillsValid ||
            game->cachedMultiplayerKillsValue != game->kills_score_multi ||
            !game->labelMultiplayer)
        {
            char multiplayerStr[32] = "";
            (void)snprintf(multiplayerStr, sizeof(multiplayerStr),
                           "P2 Kills: %d", game->kills_score_multi);
            SDL_Surface *multiplayerSurface =
                TTF_RenderText_Blended(game->font, multiplayerStr, white);
            if (multiplayerSurface)
            {
                SDL_Texture *replacement =
                    SDL_CreateTextureFromSurface(game->app.renderer,
                                                 multiplayerSurface);
                if (replacement)
                {
                    destroy_texture(&game->labelMultiplayer);
                    game->labelMultiplayer = replacement;
                    game->multiplayerKillsLabelW = multiplayerSurface->w;
                    game->multiplayerKillsLabelH = multiplayerSurface->h;
                    game->cachedMultiplayerKillsValue =
                        game->kills_score_multi;
                    game->cachedMultiplayerKillsValid = true;
                }
                SDL_FreeSurface(multiplayerSurface);
            }
        }
    }
}

void draw_status_kills(GameState *game)
{
    if (!game || !game->app.renderer) return;
    SDL_Renderer *renderer = game->app.renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    if (game->killsLabel)
    {
        SDL_Rect textRect = {24, 20, game->killsLabelW,
                             game->killsLabelH};
        (void)SDL_RenderCopy(renderer, game->killsLabel, NULL, &textRect);
    }

    if (game->multiPlayer && game->labelMultiplayer)
    {
        SDL_Rect multiplayerTextRect = {
            WIDTH - game->multiplayerKillsLabelW - 24, 20,
            game->multiplayerKillsLabelW, game->multiplayerKillsLabelH};
        (void)SDL_RenderCopy(renderer, game->labelMultiplayer, NULL,
                            &multiplayerTextRect);
    }
}
