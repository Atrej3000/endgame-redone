#include "header.h"

void init_status_x(GameState *game)
{
    if (!game || !game->font || !game->app.renderer) return;
    if (game->cachedScoreValid && game->cachedScoreValue == game->x_score &&
        game->scoreLabel)
    {
        return;
    }
    char str[32] = "";
    (void)snprintf(str, sizeof(str), "Score: %d", game->x_score);

    SDL_Color white = {0, 255, 255, 255};

    SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
    if (!tmp) return;
    SDL_Texture *replacement = SDL_CreateTextureFromSurface(game->app.renderer, tmp);
    if (replacement)
    {
        destroy_texture(&game->scoreLabel);
        game->scoreLabel = replacement;
        game->scoreLabelW = tmp->w;
        game->scoreLabelH = tmp->h;
        game->cachedScoreValue = game->x_score;
        game->cachedScoreValid = true;
    }
    SDL_FreeSurface(tmp);
}

void draw_status_x(GameState *game)
{
    if (!game || !game->app.renderer || !game->scoreLabel) return;
    SDL_Renderer *renderer = game->app.renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {WIDTH - game->scoreLabelW - 24, 20,
                         game->scoreLabelW, game->scoreLabelH};
    (void)SDL_RenderCopy(renderer, game->scoreLabel, NULL, &textRect);
}
