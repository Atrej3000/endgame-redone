#include "settings.h"

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "SETTINGS TEST: FAIL - %s\n", message);
        failures++;
    }
}

int main(void)
{
    GameSettings settings;
    settings_defaults(&settings);
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_A &&
                    settings.player1.moveRight == SDL_SCANCODE_D &&
                    settings.player1.jump == SDL_SCANCODE_W &&
                    settings.player1.shoot == SDL_SCANCODE_SPACE && settings.player1.pause == SDL_SCANCODE_P,
                "player 1 defaults are the documented controls");
    expect_true(settings.player2.moveLeft == SDL_SCANCODE_LEFT &&
                    settings.player2.moveRight == SDL_SCANCODE_RIGHT &&
                    settings.player2.jump == SDL_SCANCODE_UP &&
                    settings.player2.shoot == SDL_SCANCODE_KP_0,
                "player 2 defaults are the documented controls");
    expect_true(settings.musicVolume == MIX_MAX_VOLUME && settings.effectsVolume == MIX_MAX_VOLUME &&
                    settings.vsync && settings.screenShake,
                "audio and accessibility defaults are enabled");

    expect_true(settings_rebind(&settings, 0, SDL_SCANCODE_J),
                "a valid player 1 binding can be changed");
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_J,
                "the selected binding receives the new scancode");
    expect_true(!settings_rebind(&settings, -1, SDL_SCANCODE_K) &&
                    !settings_rebind(&settings, 9, SDL_SCANCODE_K) &&
                    !settings_rebind(&settings, 2, SDL_SCANCODE_UNKNOWN),
                "invalid binding indices and unknown scancodes are rejected");
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_J,
                "rejected changes do not mutate a binding");

    settings.musicVolume = 0;
    settings.effectsVolume = 0;
    settings.vsync = false;
    settings.screenShake = false;
    settings_reset(&settings);
    expect_true(settings.player1.moveLeft == SDL_SCANCODE_A &&
                    settings.musicVolume == MIX_MAX_VOLUME && settings.effectsVolume == MIX_MAX_VOLUME &&
                    settings.vsync && settings.screenShake,
                "reset restores all configurable defaults");

    if (failures == 0) printf("SETTINGS TEST: ALL PASS\n");
    return failures == 0 ? 0 : 1;
}
