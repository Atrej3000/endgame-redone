#include "header.h"
#include "audio_assets.h"
#include "leaderboard.h"
#include "scene.h"

static void request_safe_quit(GameState *game, const char *menuName)
{
    fprintf(stderr, "%s: required menu texture could not be loaded; requesting clean shutdown\n",
            menuName);
    if (game != NULL)
    {
        app_change_scene(game, APP_SCENE_QUIT);
    }

}

void load_menu0(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    game->menu0_status = 0;
    if (game->menu0 == NULL)
    {
        if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu0.png",
                          &game->menu0))
        {
            request_safe_quit(game, "load_menu0");
            return;
        }
    }
    leaderboard_load(game);
    (void)audio_assets_play_menu(game);
}

void load_menu1(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    game->menu_status = 0;
    if (game->menu1 == NULL)
    {
        if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu1.png",
                          &game->menu1))
        {
            request_safe_quit(game, "load_menu1");
            return;
        }
    }

}

void load_menu2(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    game->menu_status = 0;
    if (game->menu2 == NULL)
    {
        if (!load_texture(game->app.renderer, "./resource/images/backgrounds/menu2.png",
                          &game->menu2))
        {
            request_safe_quit(game, "load_menu2");
            return;
        }
    }

}
