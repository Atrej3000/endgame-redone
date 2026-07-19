#include "header.h"

void load_menu0(GameState *game) {
    if (game->menuMus == NULL) {
        game->menuMus = Mix_LoadMUS("resource/sounds/menuMus.mp3");
    }

    game->menu0_status = 0;
    //меню0_________________________________________________________________________
    if (game->menu0 == NULL) {
        if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu0.png", &game->menu0)) {
            printf("Cannot find menu0.png\n");
            SDL_Quit();
            exit(1);
        }
    }
    Mix_PlayMusic(game->menuMus, -1);
}

void load_menu1(GameState *game) {
    if (game->menuMus == NULL) {
        game->menuMus = Mix_LoadMUS("resource/sounds/menuMus.mp3");
    }

    game->menu_status = 0;
    //меню_________________________________________________________________________
    if (game->menu1 == NULL) {
        if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu1.png", &game->menu1)) {
            printf("Cannot find menu1.png");
            SDL_Quit();
            exit(1);
        }
    }
    Mix_PlayMusic(game->menuMus, -1);

    game->x_score = 0;
    for(game->x_i = 0; game->x_i < 25; game->x_i++) {
        game->x_list[game->x_i] = 0;
    }
    game->x_i = 0;
}

void load_menu2(GameState *game) {
    if (game->menuMus == NULL) {
        game->menuMus = Mix_LoadMUS("resource/sounds/menuMus.mp3");
    }

    game->menu_status = 0;
    //меню_________________________________________________________________________
    if (game->menu2 == NULL) {
        if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu2.png", &game->menu2)) {
            printf("Cannot find menu2.png");
            SDL_Quit();
            exit(1);
        }
    }
    Mix_PlayMusic(game->menuMus, -1);

    game->x_score = 0;
    for(game->x_i = 0; game->x_i < 25; game->x_i++) {
        game->x_list[game->x_i] = 0;
    }
    game->x_i = 0;
}
