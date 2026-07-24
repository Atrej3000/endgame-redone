#include "header.h"

int doRender_leaderboard(SDL_Renderer *renderer, GameState *game)
{
    int leaders_status = 0;
    if (!renderer || !game)
    {
        return leaders_status;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect leaders = {0, 0, 1280, 720};
    if (game->leaders)
    {
        (void)SDL_RenderCopy(renderer, game->leaders, NULL, &leaders);
    }
    SDL_RenderPresent(renderer);
    return leaders_status;
}

int doRender_leaderboard2(SDL_Renderer *renderer, GameState *game)
{
    int leaders_status = 0;
    if (!renderer || !game)
    {
        return leaders_status;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect leaders = {0, 0, 1280, 720};
    if (game->leaders)
    {
        (void)SDL_RenderCopy(renderer, game->leaders, NULL, &leaders);
    }

    init_status_x_list(game);

    SDL_RenderPresent(renderer);
    return leaders_status;
}
