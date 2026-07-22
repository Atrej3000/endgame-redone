#include "settings_menu.h"
#include "display.h"
#include "input_snapshot.h"
#include "settings.h"
#include "scene.h"

#define SETTINGS_ROW_COUNT 15

static const char *row_names[SETTINGS_ROW_COUNT] = {
    "Music volume", "Effects volume", "Fullscreen", "VSync", "Screen shake", "Reset defaults",
    "P1 left", "P1 right", "P1 jump", "P1 shoot", "P2 left", "P2 right", "P2 jump", "P2 shoot", "Pause"
};

static SDL_Scancode binding_for_row(const GameSettings *settings, int row)
{
    SDL_Scancode values[] = {settings->player1.moveLeft, settings->player1.moveRight, settings->player1.jump, settings->player1.shoot,
                             settings->player2.moveLeft, settings->player2.moveRight, settings->player2.jump, settings->player2.shoot,
                             settings->player1.pause};
    return values[row - 6];
}

static void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color)
{
    if (!font) return;
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destination = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (!texture) return;
    SDL_RenderCopy(renderer, texture, NULL, &destination);
    SDL_DestroyTexture(texture);
}

void doRender_settings(SDL_Renderer *renderer, GameState *game)
{
    SDL_SetRenderDrawColor(renderer, 12, 18, 34, 255);
    SDL_RenderClear(renderer);
    const SDL_Color white = {240, 240, 240, 255};
    const SDL_Color selected = {255, 210, 64, 255};
    draw_text(renderer, game->font, "SETTINGS", 500, 40, white);
    draw_text(renderer, game->font, "Arrows: adjust  Enter: rebind  Esc: back", 250, 100, white);
    for (int row = 0; row < SETTINGS_ROW_COUNT; row++)
    {
        char value[96] = "";
        if (row == 0) (void)snprintf(value, sizeof(value), "%d", game->app.settings.musicVolume);
        else if (row == 1) (void)snprintf(value, sizeof(value), "%d", game->app.settings.effectsVolume);
        else if (row == 2) (void)snprintf(value, sizeof(value), "%s", game->app.display.fullscreen ? "On" : "Off");
        else if (row == 3) (void)snprintf(value, sizeof(value), "%s", game->app.settings.vsync ? "On" : "Off");
        else if (row == 4) (void)snprintf(value, sizeof(value), "%s", game->app.settings.screenShake ? "On" : "Off");
        else if (row >= 6) (void)snprintf(value, sizeof(value), "%s", SDL_GetScancodeName(binding_for_row(&game->app.settings, row)));
        draw_text(renderer, game->font, row_names[row], 320, 150 + row * 38, row == game->app.settings.selectedRow ? selected : white);
        draw_text(renderer, game->font, value, 760, 150 + row * 38, row == game->app.settings.selectedRow ? selected : white);
    }
    if (game->app.settings.waitingBinding >= 0) draw_text(renderer, game->font, "Press a key...", 480, 680, selected);
    SDL_RenderPresent(renderer);
}

void settings_events(GameState *game)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&game->app, &event);
        display_handle_event(game, &event);
        if (event.type != SDL_KEYDOWN) continue;
        if (game->app.settings.waitingBinding >= 0)
        {
            if (event.key.keysym.scancode != SDL_SCANCODE_ESCAPE)
                (void)settings_rebind(&game->app.settings, game->app.settings.waitingBinding, event.key.keysym.scancode);
            game->app.settings.waitingBinding = -1;
            (void)settings_save(&game->app.settings);
            continue;
        }
        if (event.key.keysym.sym == SDLK_ESCAPE) { app_change_scene(game, APP_SCENE_MAIN_MENU); continue; }
        if (event.key.keysym.sym == SDLK_UP) game->app.settings.selectedRow = (game->app.settings.selectedRow + SETTINGS_ROW_COUNT - 1) % SETTINGS_ROW_COUNT;
        else if (event.key.keysym.sym == SDLK_DOWN) game->app.settings.selectedRow = (game->app.settings.selectedRow + 1) % SETTINGS_ROW_COUNT;
        else if (event.key.keysym.sym == SDLK_RETURN && game->app.settings.selectedRow >= 6) game->app.settings.waitingBinding = game->app.settings.selectedRow - 6;
        else if (event.key.keysym.sym == SDLK_RETURN && game->app.settings.selectedRow == 5)
        {
            settings_reset(&game->app.settings);
            settings_apply_audio(&game->app.settings);
            (void)SDL_RenderSetVSync(game->app.renderer, game->app.settings.vsync ? 1 : 0);
            (void)settings_save(&game->app.settings);
        }
        else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT)
        {
            const int delta = event.key.keysym.sym == SDLK_RIGHT ? 8 : -8;
            const int row = game->app.settings.selectedRow;
            if (row == 0) game->app.settings.musicVolume += delta;
            else if (row == 1) game->app.settings.effectsVolume += delta;
            else if (row == 2) (void)display_toggle_fullscreen(game);
            else if (row == 3) { game->app.settings.vsync = !game->app.settings.vsync; (void)SDL_RenderSetVSync(game->app.renderer, game->app.settings.vsync ? 1 : 0); }
            else if (row == 4) game->app.settings.screenShake = !game->app.settings.screenShake;
            if (game->app.settings.musicVolume < 0) game->app.settings.musicVolume = 0;
            if (game->app.settings.musicVolume > MIX_MAX_VOLUME) game->app.settings.musicVolume = MIX_MAX_VOLUME;
            if (game->app.settings.effectsVolume < 0) game->app.settings.effectsVolume = 0;
            if (game->app.settings.effectsVolume > MIX_MAX_VOLUME) game->app.settings.effectsVolume = MIX_MAX_VOLUME;
            settings_apply_audio(&game->app.settings);
            (void)settings_save(&game->app.settings);
        }
    }
}
