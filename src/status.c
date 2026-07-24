#include "header.h"

void init_status_lives(GameState *game) 
{
    if (!game || !game->font || !game->app.renderer) return;
    if (game->cachedLivesValid && game->cachedLivesValue == game->gameLives &&
        game->label)
    {
        return;
    }
    char str[128] = "";
    (void)snprintf(str, sizeof(str), "Lives: %d", game->gameLives);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
    if (!tmp) return;
    SDL_Texture *replacement = SDL_CreateTextureFromSurface(game->app.renderer, tmp);
    if (replacement)
    {
        destroy_texture(&game->label);
        game->label = replacement;
        game->labelW = tmp->w;
        game->labelH = tmp->h;
        game->cachedLivesValue = game->gameLives;
        game->cachedLivesValid = true;
    }
    SDL_FreeSurface(tmp);
}

void draw_status_lives(GameState *game) 
{
    if (!game || !game->app.renderer || !game->label) return;
    SDL_Renderer *renderer = game->app.renderer;
    SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

    // Clear screen
    SDL_RenderClear(renderer);

    SDL_Rect rect = {530, 333, 54, 54};
    if (game->manFrames[0])
    {
        (void)SDL_RenderCopyEx(renderer, game->manFrames[0], NULL, &rect,
                              0, NULL, game->man.facingLeft == 1
                                           ? SDL_FLIP_HORIZONTAL
                                           : SDL_FLIP_NONE);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect textRect = {594, 345, game->labelW, game->labelH};
    (void)SDL_RenderCopy(renderer, game->label, NULL, &textRect);
}

void shutdown_status_lives (GameState *game)
{
    if (game)
    {
        destroy_texture(&game->label);
        game->cachedLivesValid = false;
    }
}
