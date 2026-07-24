#include "settings.h"
#include "atomic_file.h"
#include "preferences.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>

#define SETTINGS_FILE "settings.ini"
#define SETTINGS_VERSION 1
#define SETTINGS_MAX_FILE_SIZE (16U * 1024U)

typedef enum SettingsKey
{
    SETTINGS_KEY_VERSION = 0,
    SETTINGS_KEY_MUSIC,
    SETTINGS_KEY_EFFECTS,
    SETTINGS_KEY_VSYNC,
    SETTINGS_KEY_SCREEN_SHAKE,
    SETTINGS_KEY_P1_LEFT,
    SETTINGS_KEY_P1_RIGHT,
    SETTINGS_KEY_P1_JUMP,
    SETTINGS_KEY_P1_SHOOT,
    SETTINGS_KEY_P2_LEFT,
    SETTINGS_KEY_P2_RIGHT,
    SETTINGS_KEY_P2_JUMP,
    SETTINGS_KEY_P2_SHOOT,
    SETTINGS_KEY_PAUSE,
    SETTINGS_KEY_COUNT,
    SETTINGS_KEY_UNKNOWN = -1
} SettingsKey;

static char *settings_path(void)
{
    return preferences_file_path(SETTINGS_FILE);
}

static bool settings_parse_assignment(char *line, char *key, size_t keySize,
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

static SettingsKey settings_key_from_name(const char *key)
{
    if (strcmp(key, "version") == 0) return SETTINGS_KEY_VERSION;
    if (strcmp(key, "music") == 0) return SETTINGS_KEY_MUSIC;
    if (strcmp(key, "effects") == 0) return SETTINGS_KEY_EFFECTS;
    if (strcmp(key, "vsync") == 0) return SETTINGS_KEY_VSYNC;
    if (strcmp(key, "screen_shake") == 0) return SETTINGS_KEY_SCREEN_SHAKE;
    if (strcmp(key, "p1_left") == 0) return SETTINGS_KEY_P1_LEFT;
    if (strcmp(key, "p1_right") == 0) return SETTINGS_KEY_P1_RIGHT;
    if (strcmp(key, "p1_jump") == 0) return SETTINGS_KEY_P1_JUMP;
    if (strcmp(key, "p1_shoot") == 0) return SETTINGS_KEY_P1_SHOOT;
    if (strcmp(key, "p2_left") == 0) return SETTINGS_KEY_P2_LEFT;
    if (strcmp(key, "p2_right") == 0) return SETTINGS_KEY_P2_RIGHT;
    if (strcmp(key, "p2_jump") == 0) return SETTINGS_KEY_P2_JUMP;
    if (strcmp(key, "p2_shoot") == 0) return SETTINGS_KEY_P2_SHOOT;
    if (strcmp(key, "pause") == 0) return SETTINGS_KEY_PAUSE;
    return SETTINGS_KEY_UNKNOWN;
}

static SDL_Scancode settings_scancode_from_int(int value)
{
    if (value <= (int)SDL_SCANCODE_UNKNOWN || value >= SDL_NUM_SCANCODES)
    {
        return SDL_SCANCODE_UNKNOWN;
    }
    return (SDL_Scancode)value;
}

static bool settings_scancode_is_reserved(SDL_Scancode scancode)
{
    return scancode == SDL_SCANCODE_ESCAPE ||
           scancode == SDL_SCANCODE_Q ||
           scancode == SDL_SCANCODE_F11;
}

void settings_defaults(GameSettings *settings)
{
    if (!settings) return;
    settings->player1 = (PlayerBindings){SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_W, SDL_SCANCODE_SPACE, SDL_SCANCODE_P};
    settings->player2 = (PlayerBindings){SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_KP_0, SDL_SCANCODE_P};
    settings->musicVolume = MIX_MAX_VOLUME;
    settings->effectsVolume = MIX_MAX_VOLUME;
    settings->vsync = true;
    settings->screenShake = true;
    settings->selectedRow = 0;
    settings->waitingBinding = -1;
}

void settings_reset(GameSettings *settings) { settings_defaults(settings); }

static void settings_clamp(GameSettings *settings)
{
    if (settings->musicVolume < 0) settings->musicVolume = 0;
    if (settings->musicVolume > MIX_MAX_VOLUME) settings->musicVolume = MIX_MAX_VOLUME;
    if (settings->effectsVolume < 0) settings->effectsVolume = 0;
    if (settings->effectsVolume > MIX_MAX_VOLUME) settings->effectsVolume = MIX_MAX_VOLUME;

    const PlayerBindings defaults1 = {SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_W, SDL_SCANCODE_SPACE, SDL_SCANCODE_P};
    const PlayerBindings defaults2 = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_KP_0, SDL_SCANCODE_P};
    SDL_Scancode *bindings[] = {&settings->player1.moveLeft, &settings->player1.moveRight, &settings->player1.jump, &settings->player1.shoot,
                                &settings->player2.moveLeft, &settings->player2.moveRight, &settings->player2.jump, &settings->player2.shoot,
                                &settings->player1.pause};
    const SDL_Scancode fallback[] = {defaults1.moveLeft, defaults1.moveRight, defaults1.jump, defaults1.shoot,
                                     defaults2.moveLeft, defaults2.moveRight, defaults2.jump, defaults2.shoot, defaults1.pause};
    for (int index = 0; index < 9; index++)
    {
        if ((int)*bindings[index] <= (int)SDL_SCANCODE_UNKNOWN ||
            (int)*bindings[index] >= SDL_NUM_SCANCODES ||
            settings_scancode_is_reserved(*bindings[index]))
        {
            *bindings[index] = fallback[index];
        }
    }
    settings->player2.pause = settings->player1.pause;
}

static bool bindings_are_unique(const GameSettings *settings)
{
    const SDL_Scancode bindings[] = {
        settings->player1.moveLeft, settings->player1.moveRight,
        settings->player1.jump, settings->player1.shoot,
        settings->player2.moveLeft, settings->player2.moveRight,
        settings->player2.jump, settings->player2.shoot,
        settings->player1.pause,
    };
    for (int left = 0; left < 9; left++)
    {
        for (int right = left + 1; right < 9; right++)
        {
            if (bindings[left] == bindings[right]) return false;
        }
    }
    return true;
}

void settings_load(GameSettings *settings)
{
    if (!settings) return;
    settings_defaults(settings);
    size_t contentLength = 0U;
    char *content = preferences_load_bounded_text(
        SETTINGS_FILE, SETTINGS_MAX_FILE_SIZE, &contentLength);
    if (!content) return;

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
        if (lineEnd) *lineEnd = '\0';
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
        if (!settings_parse_assignment(cursor, key, sizeof(key), &value))
        {
            cursor = lineEnd ? lineEnd + 1 : (char *)end;
            continue;
        }

        const SettingsKey settingKey = settings_key_from_name(key);
        if (settingKey != SETTINGS_KEY_UNKNOWN)
        {
            const unsigned int keyBit = 1U << (unsigned int)settingKey;
            if ((seenKeys & keyBit) != 0U)
            {
                duplicateKey = true;
            }
            else
            {
                seenKeys |= keyBit;
                switch (settingKey)
                {
                case SETTINGS_KEY_VERSION:
                    version = value;
                    break;
                case SETTINGS_KEY_MUSIC:
                    settings->musicVolume = value;
                    break;
                case SETTINGS_KEY_EFFECTS:
                    settings->effectsVolume = value;
                    break;
                case SETTINGS_KEY_VSYNC:
                    if (value == 0 || value == 1) settings->vsync = value != 0;
                    break;
                case SETTINGS_KEY_SCREEN_SHAKE:
                    if (value == 0 || value == 1) settings->screenShake = value != 0;
                    break;
                case SETTINGS_KEY_P1_LEFT:
                    settings->player1.moveLeft = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P1_RIGHT:
                    settings->player1.moveRight = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P1_JUMP:
                    settings->player1.jump = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P1_SHOOT:
                    settings->player1.shoot = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P2_LEFT:
                    settings->player2.moveLeft = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P2_RIGHT:
                    settings->player2.moveRight = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P2_JUMP:
                    settings->player2.jump = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_P2_SHOOT:
                    settings->player2.shoot = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_PAUSE:
                    settings->player1.pause = settings_scancode_from_int(value);
                    break;
                case SETTINGS_KEY_COUNT:
                case SETTINGS_KEY_UNKNOWN:
                default:
                    break;
                }
            }
        }
        cursor = lineEnd ? lineEnd + 1 : (char *)end;
    }
    SDL_free(content);
    if (version != SETTINGS_VERSION || duplicateKey || invalidFile)
    {
        settings_defaults(settings);
        return;
    }
    settings_clamp(settings);
    if (!bindings_are_unique(settings))
    {
        const int selectedRow = settings->selectedRow;
        const int waitingBinding = settings->waitingBinding;
        const int musicVolume = settings->musicVolume;
        const int effectsVolume = settings->effectsVolume;
        const bool vsync = settings->vsync;
        const bool screenShake = settings->screenShake;
        settings_defaults(settings);
        settings->selectedRow = selectedRow;
        settings->waitingBinding = waitingBinding;
        settings->musicVolume = musicVolume;
        settings->effectsVolume = effectsVolume;
        settings->vsync = vsync;
        settings->screenShake = screenShake;
    }
}

bool settings_save(const GameSettings *settings)
{
    if (!settings) return false;
    GameSettings sanitized = *settings;
    settings_clamp(&sanitized);
    if (!bindings_are_unique(&sanitized)) return false;
    char *path = settings_path();
    if (!path) return false;
    char contents[512];
    const int result = snprintf(contents, sizeof(contents),
        "version=%d\nmusic=%d\neffects=%d\nvsync=%d\nscreen_shake=%d\np1_left=%d\np1_right=%d\np1_jump=%d\np1_shoot=%d\np2_left=%d\np2_right=%d\np2_jump=%d\np2_shoot=%d\npause=%d\n",
        SETTINGS_VERSION,
        sanitized.musicVolume, sanitized.effectsVolume, sanitized.vsync ? 1 : 0,
        sanitized.screenShake ? 1 : 0,
        (int)sanitized.player1.moveLeft, (int)sanitized.player1.moveRight,
        (int)sanitized.player1.jump, (int)sanitized.player1.shoot,
        (int)sanitized.player2.moveLeft, (int)sanitized.player2.moveRight,
        (int)sanitized.player2.jump, (int)sanitized.player2.shoot,
        (int)sanitized.player1.pause);
    const bool saved = result >= 0 && (size_t)result < sizeof(contents) &&
                       atomic_write_text_file(path, contents, (size_t)result);
    free(path);
    return saved;
}

void settings_apply_audio(const GameSettings *settings)
{
    if (!settings) return;
    int frequency = 0;
    Uint16 format = 0U;
    int channels = 0;
    if (Mix_QuerySpec(&frequency, &format, &channels) == 0) return;
    Mix_VolumeMusic(settings->musicVolume);
    Mix_Volume(-1, settings->effectsVolume);
}

bool settings_rebind(GameSettings *settings, int bindingIndex, SDL_Scancode scancode)
{
    if (!settings || (int)scancode <= (int)SDL_SCANCODE_UNKNOWN ||
        (int)scancode >= SDL_NUM_SCANCODES ||
        settings_scancode_is_reserved(scancode)) return false;
    SDL_Scancode *bindings[] = {&settings->player1.moveLeft, &settings->player1.moveRight, &settings->player1.jump, &settings->player1.shoot, &settings->player2.moveLeft, &settings->player2.moveRight, &settings->player2.jump, &settings->player2.shoot, &settings->player1.pause};
    if (bindingIndex < 0 || bindingIndex >= 9) return false;
    for (int index = 0; index < 9; index++)
    {
        if (index != bindingIndex && *bindings[index] == scancode) return false;
    }
    *bindings[bindingIndex] = scancode;
    settings->player2.pause = settings->player1.pause;
    return true;
}
