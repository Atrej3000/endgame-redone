#include "display.h"
#include "scene.h"

#define DISPLAY_SETTINGS_FILE "display.ini"

static char *display_settings_path(void)
{
    char *preferencePath = SDL_GetPrefPath("Ucode", "Endgame");
    if (!preferencePath)
    {
        return NULL;
    }

    const size_t pathLength = strlen(preferencePath) + strlen(DISPLAY_SETTINGS_FILE) + 1U;
    char *settingsPath = malloc(pathLength);
    if (settingsPath)
    {
        (void)snprintf(settingsPath, pathLength, "%s%s", preferencePath, DISPLAY_SETTINGS_FILE);
    }
    SDL_free(preferencePath);
    return settingsPath;
}

void display_settings_defaults(DisplaySettings *settings)
{
    if (!settings)
    {
        return;
    }
    settings->windowWidth = WIDTH;
    settings->windowHeight = HEIGHT;
    settings->fullscreen = false;
}

void display_settings_sanitize(DisplaySettings *settings)
{
    if (!settings)
    {
        return;
    }
    if (settings->windowWidth < DISPLAY_MIN_WIDTH || settings->windowWidth > DISPLAY_MAX_WIDTH)
    {
        settings->windowWidth = WIDTH;
    }
    if (settings->windowHeight < DISPLAY_MIN_HEIGHT || settings->windowHeight > DISPLAY_MAX_HEIGHT)
    {
        settings->windowHeight = HEIGHT;
    }
}

void display_settings_load(DisplaySettings *settings)
{
    if (!settings)
    {
        return;
    }

    display_settings_defaults(settings);

    char *settingsPath = display_settings_path();
    if (!settingsPath)
    {
        return;
    }

    FILE *file = fopen(settingsPath, "r");
    free(settingsPath);
    if (!file)
    {
        return;
    }

    char line[64];
    int value = 0;
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "width=%d", &value) == 1)
        {
            settings->windowWidth = value;
        }
        else if (sscanf(line, "height=%d", &value) == 1)
        {
            settings->windowHeight = value;
        }
        else if (sscanf(line, "fullscreen=%d", &value) == 1)
        {
            settings->fullscreen = (value != 0);
        }
    }
    (void)fclose(file);
    display_settings_sanitize(settings);
}

bool display_settings_save(const DisplaySettings *settings)
{
    if (!settings)
    {
        return false;
    }

    DisplaySettings sanitized = *settings;
    display_settings_sanitize(&sanitized);

    char *settingsPath = display_settings_path();
    if (!settingsPath)
    {
        return false;
    }

    FILE *file = fopen(settingsPath, "w");
    free(settingsPath);
    if (!file)
    {
        return false;
    }

    const int result = fprintf(file, "version=1\nwidth=%d\nheight=%d\nfullscreen=%d\n",
                               sanitized.windowWidth, sanitized.windowHeight,
                               sanitized.fullscreen ? 1 : 0);
    const int closeResult = fclose(file);
    return result >= 0 && closeResult == 0;
}

bool display_configure_renderer(SDL_Renderer *renderer)
{
    if (!renderer)
    {
        return false;
    }
    if (SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT) != 0)
    {
        fprintf(stderr, "display_configure_renderer: SDL_RenderSetLogicalSize failed: %s\n", SDL_GetError());
        return false;
    }
    if (SDL_RenderSetIntegerScale(renderer, SDL_FALSE) != 0)
    {
        fprintf(stderr, "display_configure_renderer: SDL_RenderSetIntegerScale failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void display_capture_window_settings(GameState *game)
{
    if (!game || !game->app.window || game->app.display.fullscreen)
    {
        return;
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(game->app.window, &width, &height);
    game->app.display.windowWidth = width;
    game->app.display.windowHeight = height;
    display_settings_sanitize(&game->app.display);
}

bool display_toggle_fullscreen(GameState *game)
{
    if (!game || !game->app.window)
    {
        return false;
    }

    const bool enteringFullscreen = !game->app.display.fullscreen;
    if (enteringFullscreen)
    {
        display_capture_window_settings(game);
    }

    const Uint32 mode = enteringFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0U;
    if (SDL_SetWindowFullscreen(game->app.window, mode) != 0)
    {
        fprintf(stderr, "display_toggle_fullscreen: SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        return false;
    }

    game->app.display.fullscreen = enteringFullscreen;
    if (!enteringFullscreen)
    {
        SDL_SetWindowSize(game->app.window, game->app.display.windowWidth, game->app.display.windowHeight);
    }
    (void)display_settings_save(&game->app.display);
    return true;
}

void display_handle_event(GameState *game, const SDL_Event *event)
{
    if (!game || !event)
    {
        return;
    }
    if (event->type == SDL_QUIT ||
        (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_CLOSE))
    {
        app_change_scene(game, APP_SCENE_QUIT);
        return;
    }
    if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_F11)
    {
        (void)display_toggle_fullscreen(game);
        return;
    }
    if (event->type == SDL_WINDOWEVENT &&
        (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
        !game->app.display.fullscreen)
    {
        game->app.display.windowWidth = event->window.data1;
        game->app.display.windowHeight = event->window.data2;
        display_settings_sanitize(&game->app.display);
    }
}
