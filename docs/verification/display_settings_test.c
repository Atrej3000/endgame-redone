#include "display.h"

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "DISPLAY SETTINGS TEST: FAIL - %s\n", message);
        failures++;
    }
}

int main(void)
{
    DisplaySettings settings;
    display_settings_defaults(&settings);
    expect_true(settings.windowWidth == WIDTH && settings.windowHeight == HEIGHT,
                "defaults retain the logical 1280x720 presentation size");
    expect_true(!settings.fullscreen, "default mode is windowed");

    settings.windowWidth = 1600;
    settings.windowHeight = 900;
    settings.fullscreen = true;
    display_settings_sanitize(&settings);
    expect_true(settings.windowWidth == 1600 && settings.windowHeight == 900,
                "valid persisted dimensions are retained");
    expect_true(settings.fullscreen, "fullscreen preference is retained");

    settings.windowWidth = 1;
    settings.windowHeight = DISPLAY_MAX_HEIGHT + 1;
    display_settings_sanitize(&settings);
    expect_true(settings.windowWidth == WIDTH && settings.windowHeight == HEIGHT,
                "invalid persisted dimensions fall back to the logical default");

    if (failures == 0)
    {
        printf("DISPLAY SETTINGS TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
