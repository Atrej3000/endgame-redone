#include "header.h"

int doRender_menu0(SDL_Renderer *renderer, GameState *game)
{
    int menu0_status = 0;
    if (!renderer || !game)
    {
        return menu0_status;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect menu0 = {0, 0, 1280, 720};
    if (game->menu0)
    {
        (void)SDL_RenderCopy(renderer, game->menu0, NULL, &menu0);
    }
    SDL_RenderPresent(renderer);//_________________________________________menu

    return menu0_status;
}

int doRender_menu1(SDL_Renderer *renderer, GameState *game)
{
    int menu_status = 0;
    if (!renderer || !game)
    {
        return menu_status;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect menu1 = {0, 0, 1280, 720};
    if (game->menu1)
    {
        (void)SDL_RenderCopy(renderer, game->menu1, NULL, &menu1);
    }
    SDL_RenderPresent(renderer);//_________________________________________menu

    return menu_status;
}

int doRender_menu2(SDL_Renderer *renderer, GameState *game)
{
    int menu_status = 0;
    if (!renderer || !game)
    {
        return menu_status;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect menu2 = {0, 0, 1280, 720};
    if (game->menu2)
    {
        (void)SDL_RenderCopy(renderer, game->menu2, NULL, &menu2);
    }
    SDL_RenderPresent(renderer);//_________________________________________menu

    return menu_status;
}
