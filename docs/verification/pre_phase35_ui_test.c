#include "app.h"
#include "atomic_file.h"
#include "leaderboard.h"
#include "preferences.h"
#include "settings_menu.h"

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                     \
    {                                                                      \
        if (condition)                                                     \
        {                                                                  \
            printf("PRE-PHASE35 UI TEST: PASS - %s\n", description);       \
        }                                                                  \
        else                                                               \
        {                                                                  \
            printf("PRE-PHASE35 UI TEST: FAIL - %s\n", description);       \
            failures++;                                                    \
        }                                                                  \
    } while (0)

static long process_id(void)
{
#ifdef _WIN32
    return (long)_getpid();
#else
    return (long)getpid();
#endif
}

static bool create_directory(const char *path)
{
#ifdef _WIN32
    return _mkdir(path) == 0;
#else
    return mkdir(path, 0700) == 0;
#endif
}

static void remove_directory(const char *path)
{
#ifdef _WIN32
    (void)_rmdir(path);
#else
    (void)rmdir(path);
#endif
}

static bool set_preference_directory(const char *path)
{
#ifdef _WIN32
    return _putenv_s("ENDGAME_PREF_DIR", path) == 0;
#else
    return setenv("ENDGAME_PREF_DIR", path, 1) == 0;
#endif
}

static void clear_preference_directory(void)
{
#ifdef _WIN32
    (void)_putenv_s("ENDGAME_PREF_DIR", "");
#else
    (void)unsetenv("ENDGAME_PREF_DIR");
#endif
}

static bool write_leaderboard_text(const char *contents)
{
    char *path = preferences_file_path("leaderboard.ini");
    if (!path) return false;
    const bool saved = atomic_write_text_file(path, contents, strlen(contents));
    free(path);
    return saved;
}

static void remove_preference_file(const char *filename)
{
    char *path = preferences_file_path(filename);
    if (path)
    {
        (void)remove(path);
        free(path);
    }
}

int main(void)
{
    char testDirectory[256] = "";
    (void)snprintf(testDirectory, sizeof(testDirectory),
                   "build-mingw/pre-phase35-ui-%ld", process_id());
    CHECK("isolated preference directory created",
          create_directory(testDirectory));
    CHECK("preference override installed",
          set_preference_directory(testDirectory));

    GameState leaderboard = {0};
    leaderboard_record_score(&leaderboard, 10);
    leaderboard_record_score(&leaderboard, 100);
    leaderboard_record_score(&leaderboard, 50);
    CHECK("recording keeps scores descending",
          leaderboard.x_i == 3 && leaderboard.x_list[0] == 100 &&
              leaderboard.x_list[1] == 50 && leaderboard.x_list[2] == 10);

    leaderboard_reset(&leaderboard);
    for (int score = 0; score < 30; score++)
    {
        leaderboard_record_score(&leaderboard, score);
    }
    CHECK("full table retains exactly the best 25 scores",
          leaderboard.x_i == LEADERBOARD_CAPACITY &&
              leaderboard.x_list[0] == 29 &&
              leaderboard.x_list[LEADERBOARD_CAPACITY - 1] == 5);
    leaderboard_record_score(&leaderboard, 4);
    CHECK("a below-cutoff score cannot displace the top 25",
          leaderboard.x_list[LEADERBOARD_CAPACITY - 1] == 5);
    leaderboard_record_score(&leaderboard, 1000);
    CHECK("a new high score displaces the old cutoff",
          leaderboard.x_list[0] == 1000 &&
              leaderboard.x_list[LEADERBOARD_CAPACITY - 1] == 6);

    leaderboard.x_i = 4;
    leaderboard.x_list[0] = 2;
    leaderboard.x_list[1] = 9;
    leaderboard.x_list[2] = 1;
    leaderboard.x_list[3] = 9;
    int sorted[LEADERBOARD_CAPACITY] = {0};
    const size_t copied = leaderboard_copy_top_scores(
        &leaderboard, sorted, LEADERBOARD_CAPACITY);
    CHECK("display copy sorts equal and unequal values descending",
          copied == 4U && sorted[0] == 9 && sorted[1] == 9 &&
              sorted[2] == 2 && sorted[3] == 1);
    CHECK("display sorting does not mutate authoritative storage",
          leaderboard.x_list[0] == 2 && leaderboard.x_list[1] == 9 &&
              leaderboard.x_list[2] == 1 && leaderboard.x_list[3] == 9);

    CHECK("leaderboard saves atomically", leaderboard_save(&leaderboard));
    GameState loaded = {0};
    leaderboard_load(&loaded);
    CHECK("leaderboard round-trip restores a sorted table",
          loaded.x_i == 4 && loaded.x_list[0] == 9 &&
              loaded.x_list[1] == 9 && loaded.x_list[2] == 2 &&
              loaded.x_list[3] == 1);

    CHECK("truncated leaderboard fixture written",
          write_leaderboard_text("version=1\ncount=2\nscore0=5\n"));
    loaded.x_i = 7;
    leaderboard_load(&loaded);
    CHECK("truncated leaderboard recovers to an empty table",
          loaded.x_i == 0 && loaded.x_list[0] == 0);
    CHECK("unsupported leaderboard fixture written",
          write_leaderboard_text("version=99\ncount=1\nscore0=5\n"));
    loaded.x_i = 7;
    leaderboard_load(&loaded);
    CHECK("unsupported leaderboard version recovers to defaults",
          loaded.x_i == 0 && loaded.x_list[0] == 0);
    CHECK("negative leaderboard fixture written",
          write_leaderboard_text("version=1\ncount=1\nscore0=-5\n"));
    loaded.x_i = 7;
    leaderboard_load(&loaded);
    CHECK("negative persisted scores are rejected",
          loaded.x_i == 0 && loaded.x_list[0] == 0);
    CHECK("overflowing leaderboard fixture written",
          write_leaderboard_text(
              "version=1\ncount=1\nscore0=999999999999999999999\n"));
    loaded.x_i = 7;
    leaderboard_load(&loaded);
    CHECK("overflowing persisted scores recover without conversion UB",
          loaded.x_i == 0 && loaded.x_list[0] == 0);

    // Mode menu loading must not erase a completed Runner score merely
    // because the player visits Arcade or returns to Runner.
    loaded.x_i = 1;
    loaded.x_list[0] = 777;
    loaded.menu1 = (SDL_Texture *)(void *)&loaded;
    loaded.menu2 = (SDL_Texture *)(void *)&loaded;
    load_menu1(&loaded);
    load_menu2(&loaded);
    CHECK("mode menu entry preserves the in-session leaderboard",
          loaded.x_i == 1 && loaded.x_list[0] == 777);

    // Public UI/event entry points recover safely from missing context.
    menu_events(NULL);
    menu0_events(NULL);
    settings_events(NULL);
    doRender_settings(NULL, NULL);
    CHECK("settings UI entry points reject null context", true);
    CHECK("pause event polling rejects a null game", pause_events(NULL) == 0);
    CHECK("menu renderer rejects null context", doRender_menu1(NULL, NULL) == 0);
    CHECK("pause renderer rejects null context", doRender_pause(NULL, NULL) == 0);
    CHECK("leaderboard renderer rejects null context",
          doRender_leaderboard2(NULL, NULL) == 0);
    doRender(NULL, NULL);
    doRender2(NULL, NULL);

    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    CHECK("application initializes for HUD cache verification",
          app_init(&game, &window, &renderer));
    if (game && renderer)
    {
        game->gameLives = 3;
        init_status_lives(game);
        CHECK("lives label is created", game->label != NULL);
        if (game->label)
        {
            (void)SDL_SetTextureAlphaMod(game->label, 17U);
            init_status_lives(game);
            Uint8 alpha = 0U;
            (void)SDL_GetTextureAlphaMod(game->label, &alpha);
            CHECK("unchanged lives reuse the cached texture", alpha == 17U);

            game->statusState = STATUS_STATE_GAME;
            game->time = 5;
            (void)SDL_SetTextureAlphaMod(game->label, 23U);
            process(game, PHYSICS_DT);
            alpha = 0U;
            (void)SDL_GetTextureAlphaMod(game->label, &alpha);
            CHECK("Arcade ticks do not destroy an active cached lives label",
                  alpha == 23U);
        }

        game->kills_score = 4;
        game->kills_score_multi = 6;
        game->multiPlayer = GAME_MODE_MULTIPLAYER;
        init_status_kills(game);
        CHECK("multiplayer HUD creates independent P1 and P2 labels",
              game->killsLabel != NULL && game->labelMultiplayer != NULL &&
                  game->killsLabelW > 0 &&
                  game->multiplayerKillsLabelW > 0);

        game->x_score = 123;
        init_status_x(game);
        CHECK("Runner score label does not replace the lives label",
              game->scoreLabel != NULL && game->label != NULL &&
                  game->scoreLabel != game->label);
    }
    app_shutdown(&game, &window, &renderer);

    remove_preference_file("leaderboard.ini");
    remove_preference_file("leaderboard.ini.tmp");
    remove_preference_file("settings.ini");
    remove_preference_file("settings.ini.tmp");
    remove_preference_file("display.ini");
    remove_preference_file("display.ini.tmp");
    clear_preference_directory();
    remove_directory(testDirectory);

    if (failures == 0)
    {
        printf("PRE-PHASE35 UI TEST: ALL PASS\n");
        return 0;
    }
    printf("PRE-PHASE35 UI TEST: %d FAILURE(S)\n", failures);
    return 1;
}
