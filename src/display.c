#include "display.h"
#include "atomic_file.h"
#include "input_snapshot.h"
#include "preferences.h"
#include "scene.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>

#define DISPLAY_SETTINGS_FILE "display.ini"
#define DISPLAY_SETTINGS_VERSION 1
#define DISPLAY_SETTINGS_MAX_FILE_SIZE (4U * 1024U)

typedef enum DisplaySettingKey
{
    DISPLAY_SETTING_VERSION = 0,
    DISPLAY_SETTING_WIDTH,
    DISPLAY_SETTING_HEIGHT,
    DISPLAY_SETTING_FULLSCREEN,
    DISPLAY_SETTING_UNKNOWN = -1
} DisplaySettingKey;

static char *display_settings_path(void)
{
    return preferences_file_path(DISPLAY_SETTINGS_FILE);
}

static bool display_setting_parse(char *line, char *key, size_t keySize,
                                  int *value)
{
    char *separator = strchr(line, '=');
    if (!separator || separator == line || separator[1] == '\0')
    {
        return false;
    }
    const size_t keyLength = (size_t)(separator - line);
    if (keyLength >= keySize)
    {
        return false;
    }

    errno = 0;
    char *valueEnd = NULL;
    const intmax_t parsed = strtoimax(separator + 1, &valueEnd, 10);
    if (errno == ERANGE || valueEnd == separator + 1 || *valueEnd != '\0' ||
        parsed < (intmax_t)INT_MIN || parsed > (intmax_t)INT_MAX)
    {
        return false;
    }

    memcpy(key, line, keyLength);
    key[keyLength] = '\0';
    *value = (int)parsed;
    return true;
}

static DisplaySettingKey display_setting_key_from_name(const char *key)
{
    if (strcmp(key, "version") == 0) return DISPLAY_SETTING_VERSION;
    if (strcmp(key, "width") == 0) return DISPLAY_SETTING_WIDTH;
    if (strcmp(key, "height") == 0) return DISPLAY_SETTING_HEIGHT;
    if (strcmp(key, "fullscreen") == 0) return DISPLAY_SETTING_FULLSCREEN;
    return DISPLAY_SETTING_UNKNOWN;
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

    size_t contentLength = 0U;
    char *content = preferences_load_bounded_text(
        DISPLAY_SETTINGS_FILE, DISPLAY_SETTINGS_MAX_FILE_SIZE,
        &contentLength);
    if (!content)
    {
        return;
    }

    int version = -1;
    unsigned int seenKeys = 0U;
    bool duplicateKey = false;
    bool invalidFile = false;
    char *cursor = content;
    const char *end = content + contentLength;
    while (cursor < end)
    {
        char *lineEnd = memchr(cursor, '\n', (size_t)(end - cursor));
        const size_t lineLength =
            lineEnd ? (size_t)(lineEnd - cursor)
                    : (size_t)(end - cursor);
        if (memchr(cursor, '\0', lineLength) != NULL)
        {
            invalidFile = true;
            break;
        }
        if (lineEnd)
        {
            *lineEnd = '\0';
        }
        char *carriageReturn = strchr(cursor, '\r');
        if (carriageReturn)
        {
            if (carriageReturn[1] != '\0')
            {
                invalidFile = true;
                break;
            }
            *carriageReturn = '\0';
        }

        char key[32] = "";
        int value = 0;
        if (display_setting_parse(cursor, key, sizeof(key), &value))
        {
            const DisplaySettingKey settingKey =
                display_setting_key_from_name(key);
            if (settingKey != DISPLAY_SETTING_UNKNOWN)
            {
                const unsigned int keyBit =
                    1U << (unsigned int)settingKey;
                if ((seenKeys & keyBit) != 0U)
                {
                    duplicateKey = true;
                }
                else
                {
                    seenKeys |= keyBit;
                    if (settingKey == DISPLAY_SETTING_VERSION)
                    {
                        version = value;
                    }
                    else if (settingKey == DISPLAY_SETTING_WIDTH)
                    {
                        settings->windowWidth = value;
                    }
                    else if (settingKey == DISPLAY_SETTING_HEIGHT)
                    {
                        settings->windowHeight = value;
                    }
                    else if (settingKey == DISPLAY_SETTING_FULLSCREEN &&
                             (value == 0 || value == 1))
                    {
                        settings->fullscreen = value != 0;
                    }
                }
            }
        }
        cursor = lineEnd ? lineEnd + 1 : (char *)end;
    }
    SDL_free(content);
    if (version != DISPLAY_SETTINGS_VERSION || duplicateKey || invalidFile)
    {
        display_settings_defaults(settings);
        return;
    }
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

    char contents[128];
    const int result = snprintf(contents, sizeof(contents),
                                "version=%d\nwidth=%d\nheight=%d\nfullscreen=%d\n",
                                DISPLAY_SETTINGS_VERSION, sanitized.windowWidth,
                                sanitized.windowHeight,
                                sanitized.fullscreen ? 1 : 0);
    const bool saved = result >= 0 && (size_t)result < sizeof(contents) &&
                       atomic_write_text_file(settingsPath, contents,
                                              (size_t)result);
    free(settingsPath);
    return saved;
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

bool display_set_vsync(GameState *game, bool enabled)
{
    if (!game || !game->app.renderer) return false;
    if (SDL_RenderSetVSync(game->app.renderer, enabled ? 1 : 0) == 0)
    {
        game->app.settings.vsync = enabled;
        return true;
    }

    fprintf(stderr, "display_set_vsync: could not apply VSync=%d: %s\n",
            enabled ? 1 : 0, SDL_GetError());
    SDL_RendererInfo info;
    SDL_zero(info);
    if (SDL_GetRendererInfo(game->app.renderer, &info) == 0)
    {
        game->app.settings.vsync =
            (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0U;
    }
    return false;
}

void display_capture_window_settings(GameState *game)
{
    if (!game || !game->app.window)
    {
        return;
    }
    const Uint32 fullscreenFlags =
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP;
    game->app.display.fullscreen =
        (SDL_GetWindowFlags(game->app.window) & fullscreenFlags) != 0U;
    if (game->app.display.fullscreen) return;

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

    const Uint32 fullscreenFlags =
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP;
    const bool currentlyFullscreen =
        (SDL_GetWindowFlags(game->app.window) & fullscreenFlags) != 0U;
    game->app.display.fullscreen = currentlyFullscreen;
    const bool enteringFullscreen = !currentlyFullscreen;
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
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
    {
        input_state_clear(&game->input);
        input_controller_sync_jump_edge(&game->app);
        // SDL may already have keyboard events queued behind FOCUS_LOST.
        // Discard them now so the next scene/frame cannot rebuild a buffer
        // from input that occurred while the window was losing focus.
        SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);
        if (game->app.scene == APP_SCENE_ARCADE_GAME)
        {
            app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
        }
        else if (game->app.scene == APP_SCENE_RUNNER_GAME)
        {
            app_change_scene(game, APP_SCENE_RUNNER_PAUSE);
        }
        return;
    }
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0 &&
        event->key.keysym.sym == SDLK_F11)
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
