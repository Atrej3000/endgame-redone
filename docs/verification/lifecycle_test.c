// Standalone asset/session lifecycle test -- NOT part of the shipped game
// (main.c is excluded when building this, same pattern as
// smoke_init_shutdown.c/scene_transition_test.c). Exercises the real
// arcade_assets_load/unload, runner_assets_load/unload,
// arcade_session_reset/runner_session_reset, and the pause/new-game
// transition sequences directly, without needing keyboard/window input
// injection.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant (see docs/scene-state-map.md): a few cases
// below directly assign game->app.scene to set up a precondition. Production
// code must never do this -- only this test file does, and only for setup.
#include "app.h"
#include "scene.h"

static int failures = 0;

#define CHECK(desc, cond)                                 \
    do                                                    \
    {                                                     \
        if (cond)                                         \
        {                                                 \
            printf("LIFECYCLE TEST: PASS - %s\n", desc);  \
        }                                                 \
        else                                              \
        {                                                 \
            printf("LIFECYCLE TEST: FAIL - %s\n", desc);  \
            failures++;                                   \
        }                                                 \
    } while (0)

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("LIFECYCLE TEST: app_init failed, cannot run\n");
        return 1;
    }

    // -------------------------------------------------------------------
    // 1. Asset lifecycle -- Arcade (+ the shared bucket it cascades into)
    // -------------------------------------------------------------------
    CHECK("starts unloaded (arcadeAssetsLoaded)", game->assetFlags.arcadeAssetsLoaded == false);
    CHECK("starts unloaded (bossTexture NULL)", game->bossTexture == NULL);
    CHECK("shared menu/select/jump audio is ready after app init",
          game->assetFlags.sharedAudioAssetsLoaded && game->audio.menuMusic != NULL &&
              game->audio.select != NULL && game->audio.jump != NULL);

    CHECK("arcade_assets_load() succeeds", arcade_assets_load(game) == true);
    CHECK("arcadeAssetsLoaded is now true", game->assetFlags.arcadeAssetsLoaded == true);
    CHECK("sharedAssetsLoaded cascaded to true", game->assetFlags.sharedAssetsLoaded == true);
    CHECK("bossTexture is non-NULL", game->bossTexture != NULL);
    CHECK("Arcade music and effects are loaded before a session starts",
          game->audio.arcadeMusic != NULL && game->audio.shoot != NULL &&
              game->audio.damage != NULL && game->audio.kick != NULL);
    CHECK("shared mult texture is non-NULL", game->mult != NULL);

    // Named individually (not just covered by the blanket "load succeeded"
    // check above) so a future case-sensitivity regression on any one of
    // these three fields fails with an unambiguous assertion -- these are
    // exactly the three fields whose load literals had confirmed casing
    // defects, corrected in Phase 8 (see docs/asset-path-portability.md).
    CHECK("sheetTextureBack2 (sunset_front.png) is non-NULL", game->sheetTextureBack2 != NULL);
    CHECK("brick_block (Brick_block.png) is non-NULL", game->brick_block != NULL);
    CHECK("copper_block (Copper_block.png) is non-NULL", game->copper_block != NULL);

    // Pointer equality is valid here because a second load must be a pure
    // no-op (the arcadeAssetsLoaded guard returns true immediately without
    // touching any field) -- if it reloaded, this would be a *different*
    // SDL_Texture* even if visually identical, since load_texture() always
    // creates a fresh texture object.
    SDL_Texture *bossBefore = game->bossTexture;
    CHECK("second arcade_assets_load() call is a no-op (returns true)", arcade_assets_load(game) == true);
    CHECK("second call does not replace the live texture (pointer equality)", game->bossTexture == bossBefore);

    arcade_assets_unload(game);
    CHECK("arcadeAssetsLoaded is false after unload", game->assetFlags.arcadeAssetsLoaded == false);
    CHECK("bossTexture is NULL after unload", game->bossTexture == NULL);
    CHECK("Arcade audio is released with its mode group",
          game->audio.arcadeMusic == NULL && game->audio.shoot == NULL &&
              game->audio.damage == NULL && game->audio.kick == NULL);
    CHECK("shared assets are NOT touched by arcade_assets_unload() (mult still loaded)", game->mult != NULL);
    CHECK("sharedAssetsLoaded is still true (owned separately)", game->assetFlags.sharedAssetsLoaded == true);

    // Idempotency: unloading an already-unloaded group must not crash.
    arcade_assets_unload(game);
    CHECK("second arcade_assets_unload() completes without crash (flag still false)", game->assetFlags.arcadeAssetsLoaded == false);

    CHECK("arcade_assets_load() succeeds again after unload", arcade_assets_load(game) == true);
    CHECK("bossTexture is non-NULL again after reload", game->bossTexture != NULL);

    // -------------------------------------------------------------------
    // 2. Asset lifecycle -- Runner
    // -------------------------------------------------------------------
    CHECK("Runner starts unloaded (runnerAssetsLoaded)", game->assetFlags.runnerAssetsLoaded == false);
    CHECK("Runner starts unloaded (star NULL)", game->star == NULL);

    CHECK("runner_assets_load() succeeds", runner_assets_load(game) == true);
    CHECK("runnerAssetsLoaded is now true", game->assetFlags.runnerAssetsLoaded == true);
    CHECK("star is non-NULL", game->star != NULL);
    CHECK("Runner music is loaded before a session starts", game->audio.runnerMusic != NULL);
    // Shared bucket was already loaded by the Arcade test above; runner's
    // own call to shared_assets_load() must see that and skip reloading.
    SDL_Texture *multBefore = game->mult;
    CHECK("shared mult texture untouched by runner_assets_load() (pointer equality)", game->mult == multBefore);

    SDL_Texture *starBefore = game->star;
    CHECK("second runner_assets_load() call is a no-op (returns true)", runner_assets_load(game) == true);
    CHECK("second call does not replace the live texture (pointer equality)", game->star == starBefore);

    runner_assets_unload(game);
    CHECK("runnerAssetsLoaded is false after unload", game->assetFlags.runnerAssetsLoaded == false);
    CHECK("star is NULL after unload", game->star == NULL);
    CHECK("Runner music is released with its mode group", game->audio.runnerMusic == NULL);

    runner_assets_unload(game);
    CHECK("second runner_assets_unload() completes without crash", game->assetFlags.runnerAssetsLoaded == false);

    CHECK("runner_assets_load() succeeds again after unload", runner_assets_load(game) == true);
    CHECK("star is non-NULL again after reload", game->star != NULL);

    // -------------------------------------------------------------------
    // 3. Shared-bucket lifecycle (both modes retained assets loaded by now)
    // -------------------------------------------------------------------
    shared_assets_unload(game);
    CHECK("sharedAssetsLoaded is false after shared_assets_unload()", game->assetFlags.sharedAssetsLoaded == false);
    CHECK("mult is NULL after shared_assets_unload()", game->mult == NULL);
    CHECK("application-owned font survives shared_assets_unload()", game->font != NULL);
    // Arcade/Runner's own flags are untouched by unloading the shared bucket.
    CHECK("arcadeAssetsLoaded untouched by shared_assets_unload()", game->assetFlags.arcadeAssetsLoaded == true);
    CHECK("runnerAssetsLoaded untouched by shared_assets_unload()", game->assetFlags.runnerAssetsLoaded == true);

    // Re-loading either mode's assets re-cascades into the shared bucket.
    CHECK("arcade_assets_load() re-cascades shared bucket", arcade_assets_load(game) == true);
    CHECK("sharedAssetsLoaded true again", game->assetFlags.sharedAssetsLoaded == true);
    CHECK("mult non-NULL again", game->mult != NULL);

    // -------------------------------------------------------------------
    // 4. Session reset -- Arcade
    // -------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);

    // Deliberately mutate representative session state.
    game->tempScore = 999;
    game->gameLives = 0;
    game->man.lives = 0;
    game->man.x = 12345;
    game->enemyValues[0].x = 54321;
    addBullet(game, 10.0f, 20.0f, 3.0f);
    CHECK("addBullet() created a live bullet (setup)", game->bullets[0].active);

    SDL_Texture *bossBeforeReset = game->bossTexture;
    bool arcadeLoadedBeforeReset = game->assetFlags.arcadeAssetsLoaded;

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);

    CHECK("session reset: tempScore back to 0", game->tempScore == 0);
    CHECK("session reset: gameLives back to 10", game->gameLives == 10);
    CHECK("session reset: man.lives back to 3", game->man.lives == 3);
    CHECK("session reset: man.x back to starting position", game->man.x == 320 - 40);
    CHECK("session reset: enemyValues[0].x back to 640", game->enemyValues[0].x == 640);
    CHECK("session reset: bullets[0] deactivated (no leak)", !game->bullets[0].active);
    CHECK("session reset does not touch asset pointers (bossTexture unchanged)", game->bossTexture == bossBeforeReset);
    CHECK("session reset does not touch loaded flags", game->assetFlags.arcadeAssetsLoaded == arcadeLoadedBeforeReset);

    // -------------------------------------------------------------------
    // 5. Session reset -- Runner
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);

    game->x_score = 888;
    game->gameLives = 0;
    game->man.isDead = 1;
    game->man.x = 99999;

    SDL_Texture *starBeforeReset = game->star;
    bool runnerLoadedBeforeReset = game->assetFlags.runnerAssetsLoaded;

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);

    CHECK("Runner session reset: x_score back to 0", game->x_score == 0);
    CHECK("Runner session reset: gameLives back to 3", game->gameLives == 3);
    CHECK("Runner session reset: man.isDead back to 0", game->man.isDead == 0);
    CHECK("Runner session reset: man.x back to starting position", game->man.x == 320 - 240);
    CHECK("Runner session reset does not touch asset pointers (star unchanged)", game->star == starBeforeReset);
    CHECK("Runner session reset does not touch loaded flags", game->assetFlags.runnerAssetsLoaded == runnerLoadedBeforeReset);

    // -------------------------------------------------------------------
    // 6. Pause/resume never resets session state
    // -------------------------------------------------------------------
    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only precondition
    game->tempScore = 777;               // sentinel

    app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
    CHECK("pause: scene is ARCADE_PAUSE", game->app.scene == APP_SCENE_ARCADE_PAUSE);
    CHECK("pause: session state untouched by entering pause", game->tempScore == 777);

    // Resume (mirrors pause_events.c's exact resume expression).
    app_change_scene(game, (game->app.scene == APP_SCENE_RUNNER_PAUSE) ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);
    CHECK("resume: scene is back to ARCADE_GAME", game->app.scene == APP_SCENE_ARCADE_GAME);
    CHECK("resume: session state still untouched across pause/resume", game->tempScore == 777);

    // -------------------------------------------------------------------
    // 7. New-game transition: reset + assets retained + correct scene
    // -------------------------------------------------------------------
    game->app.scene = APP_SCENE_ARCADE_MENU; // test-only precondition
    game->tempScore = 555;               // stale value from a previous round

    // Mirrors menu_events.c's SDLK_2 handler exactly.
    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    app_change_scene(game, APP_SCENE_ARCADE_GAME);

    CHECK("new-game: session was reset (tempScore back to 0)", game->tempScore == 0);
    CHECK("new-game: selected mode applied (multiPlayer == MULTIPLAYER)", game->multiPlayer == GAME_MODE_MULTIPLAYER);
    CHECK("new-game: assets remain loaded (arcadeAssetsLoaded still true)", game->assetFlags.arcadeAssetsLoaded == true);
    CHECK("new-game: entered the correct gameplay scene", game->app.scene == APP_SCENE_ARCADE_GAME);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("LIFECYCLE TEST: ALL PASS\n");
        return 0;
    }
    printf("LIFECYCLE TEST: %d FAILURE(S)\n", failures);
    return 1;
}
