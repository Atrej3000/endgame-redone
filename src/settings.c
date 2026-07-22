#include "settings.h"

#define SETTINGS_FILE "settings.ini"

static char *settings_path(void)
{
    char *preferencePath = SDL_GetPrefPath("Ucode", "Endgame");
    if (!preferencePath) return NULL;
    const size_t length = strlen(preferencePath) + strlen(SETTINGS_FILE) + 1U;
    char *path = malloc(length);
    if (path) (void)snprintf(path, length, "%s%s", preferencePath, SETTINGS_FILE);
    SDL_free(preferencePath);
    return path;
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
        if ((int)*bindings[index] <= (int)SDL_SCANCODE_UNKNOWN || (int)*bindings[index] >= SDL_NUM_SCANCODES)
        {
            *bindings[index] = fallback[index];
        }
    }
}

void settings_load(GameSettings *settings)
{
    if (!settings) return;
    settings_defaults(settings);
    char *path = settings_path();
    if (!path) return;
    FILE *file = fopen(path, "r");
    free(path);
    if (!file) return;
    char key[32]; int value = 0;
    while (fscanf(file, "%31[^=]=%d\n", key, &value) == 2)
    {
        if (strcmp(key, "music") == 0) settings->musicVolume = value;
        else if (strcmp(key, "effects") == 0) settings->effectsVolume = value;
        else if (strcmp(key, "vsync") == 0) settings->vsync = value != 0;
        else if (strcmp(key, "screen_shake") == 0) settings->screenShake = value != 0;
        else if (strcmp(key, "p1_left") == 0) settings->player1.moveLeft = (SDL_Scancode)value;
        else if (strcmp(key, "p1_right") == 0) settings->player1.moveRight = (SDL_Scancode)value;
        else if (strcmp(key, "p1_jump") == 0) settings->player1.jump = (SDL_Scancode)value;
        else if (strcmp(key, "p1_shoot") == 0) settings->player1.shoot = (SDL_Scancode)value;
        else if (strcmp(key, "p2_left") == 0) settings->player2.moveLeft = (SDL_Scancode)value;
        else if (strcmp(key, "p2_right") == 0) settings->player2.moveRight = (SDL_Scancode)value;
        else if (strcmp(key, "p2_jump") == 0) settings->player2.jump = (SDL_Scancode)value;
        else if (strcmp(key, "p2_shoot") == 0) settings->player2.shoot = (SDL_Scancode)value;
        else if (strcmp(key, "pause") == 0) settings->player1.pause = (SDL_Scancode)value;
    }
    (void)fclose(file);
    settings_clamp(settings);
}

bool settings_save(const GameSettings *settings)
{
    if (!settings) return false;
    char *path = settings_path();
    if (!path) return false;
    FILE *file = fopen(path, "w");
    free(path);
    if (!file) return false;
    const int result = fprintf(file, "version=1\nmusic=%d\neffects=%d\nvsync=%d\nscreen_shake=%d\np1_left=%d\np1_right=%d\np1_jump=%d\np1_shoot=%d\np2_left=%d\np2_right=%d\np2_jump=%d\np2_shoot=%d\npause=%d\n",
        settings->musicVolume, settings->effectsVolume, settings->vsync ? 1 : 0, settings->screenShake ? 1 : 0,
        (int)settings->player1.moveLeft, (int)settings->player1.moveRight, (int)settings->player1.jump, (int)settings->player1.shoot,
        (int)settings->player2.moveLeft, (int)settings->player2.moveRight, (int)settings->player2.jump, (int)settings->player2.shoot,
        (int)settings->player1.pause);
    return result >= 0 && fclose(file) == 0;
}

void settings_apply_audio(const GameSettings *settings)
{
    if (!settings) return;
    Mix_VolumeMusic(settings->musicVolume);
    Mix_Volume(-1, settings->effectsVolume);
}

bool settings_rebind(GameSettings *settings, int bindingIndex, SDL_Scancode scancode)
{
    if (!settings || (int)scancode <= (int)SDL_SCANCODE_UNKNOWN || (int)scancode >= SDL_NUM_SCANCODES) return false;
    SDL_Scancode *bindings[] = {&settings->player1.moveLeft, &settings->player1.moveRight, &settings->player1.jump, &settings->player1.shoot, &settings->player2.moveLeft, &settings->player2.moveRight, &settings->player2.jump, &settings->player2.shoot, &settings->player1.pause};
    if (bindingIndex < 0 || bindingIndex >= 9) return false;
    *bindings[bindingIndex] = scancode;
    return true;
}
