#include "header.h"

int doRender_leaderboard(SDL_Renderer *renderer, GameState *game)
{
    int leaders_status = 0;
    SDL_Rect leaders = {0, 0, 1280, 720};
    SDL_RenderCopy(renderer, game->leaders, NULL, &leaders);
    SDL_RenderPresent(renderer);
    return leaders_status;
}

int doRender_leaderboard2(SDL_Renderer *renderer, GameState *game)
{
    int leaders_status = 0;
    SDL_Rect leaders = {0, 0, 1280, 720};
    SDL_RenderCopy(renderer, game->leaders, NULL, &leaders);

    //leader list
    //mx_sort_arr_int(game->x_list, 20);
    init_status_x_list(game);
    //draw_status_x_list(game);

    SDL_RenderPresent(renderer);
    return leaders_status;
}
