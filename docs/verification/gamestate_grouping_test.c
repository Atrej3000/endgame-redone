// Standalone GameState nested-struct grouping test -- NOT part of the
// shipped game (main.c is excluded when building this, same pattern as the
// other docs/verification/*.c harnesses). Exercises AppContext
// (game->app.renderer/game->app.scene) and AssetLifecycleFlags
// (game->assetFlags.*), introduced in Phase 10 -- see
// docs/gamestate-decomposition.md.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant: none needed here -- every scene check below
// either reads the zero-initialized value or goes through app_change_scene()
// itself.
#include "app.h"
#include "scene.h"

static int failures = 0;

#define CHECK(desc, cond)                                          \
    do                                                              \
    {                                                                \
        if (cond)                                                     \
        {                                                              \
            printf("GAMESTATE GROUPING TEST: PASS - %s\n", desc);       \
        }                                                                \
        else                                                              \
        {                                                                  \
            printf("GAMESTATE GROUPING TEST: FAIL - %s\n", desc);           \
            failures++;                                                     \
        }                                                                    \
    } while (0)

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("GAMESTATE GROUPING TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. Zero-initialization -- calloc still zero-initializes both new
    //    nested structs correctly regardless of internal layout.
    // ------------------------------------------------------------------
    CHECK("app.scene zero-initializes to APP_SCENE_MAIN_MENU (enum value 0)",
          game->app.scene == APP_SCENE_MAIN_MENU);
    CHECK("assetFlags.arcadeAssetsLoaded zero-initializes to false",
          game->assetFlags.arcadeAssetsLoaded == false);
    CHECK("assetFlags.runnerAssetsLoaded zero-initializes to false",
          game->assetFlags.runnerAssetsLoaded == false);
    CHECK("assetFlags.sharedAssetsLoaded zero-initializes to false",
          game->assetFlags.sharedAssetsLoaded == false);
    CHECK("shared audio is initialized during app startup",
          game->assetFlags.sharedAudioAssetsLoaded == true && game->audio.menuMusic != NULL &&
              game->audio.select != NULL && game->audio.jump != NULL);

    // ------------------------------------------------------------------
    // 2. app.renderer -- set once by app_init(), matches the out-param.
    // ------------------------------------------------------------------
    CHECK("app.renderer is non-NULL after app_init()", game->app.renderer != NULL);
    CHECK("app.renderer matches the app_init() out-parameter", game->app.renderer == renderer);

    // ------------------------------------------------------------------
    // 3. Explicit write -- app_change_scene() updates app.scene correctly.
    // ------------------------------------------------------------------
    app_change_scene(game, APP_SCENE_ARCADE_LEADERBOARD);
    CHECK("app_change_scene() updates app.scene", game->app.scene == APP_SCENE_ARCADE_LEADERBOARD);

    // ------------------------------------------------------------------
    // 4. Reset/cleanup -- assetFlags flips true -> false through the real
    //    production load/unload functions, unrelated groups untouched.
    // ------------------------------------------------------------------
    int manLivesBefore = game->man.lives;

    CHECK("arcade_assets_load() succeeds", arcade_assets_load(game) == true);
    CHECK("assetFlags.arcadeAssetsLoaded is true after load",
          game->assetFlags.arcadeAssetsLoaded == true);
    CHECK("assetFlags.sharedAssetsLoaded cascaded to true", game->assetFlags.sharedAssetsLoaded == true);

    arcade_assets_unload(game);
    CHECK("assetFlags.arcadeAssetsLoaded is false after unload",
          game->assetFlags.arcadeAssetsLoaded == false);
    CHECK("unrelated group (man.lives) untouched by asset load/unload",
          game->man.lives == manLivesBefore);
    CHECK("unrelated group (app.scene) untouched by asset load/unload",
          game->app.scene == APP_SCENE_ARCADE_LEADERBOARD);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("GAMESTATE GROUPING TEST: ALL PASS\n");
        return 0;
    }
    printf("GAMESTATE GROUPING TEST: %d FAILURE(S)\n", failures);
    return 1;
}
