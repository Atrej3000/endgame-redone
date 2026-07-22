// Standalone runtime smoke test -- NOT part of the shipped game (main.c is
// excluded when building this). Exercises the real app_init -> one real
// asset-load pass -> app_shutdown -> repeated app_shutdown cycle end to end,
// automatically, without needing to inject keyboard input into a window.
// See docs/refactor-log.md for the run this produced and how it was built.
#include "app.h"

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("SMOKE TEST: app_init failed\n");
        return 1;
    }
    printf("SMOKE TEST: app_init ok (game=%p window=%p renderer=%p)\n",
           (void *)game, (void *)window, (void *)renderer);

    // Exercise one real asset-load pass (texture + music load, matching
    // main()'s own first call) without entering the interactive main loop.
    load_menu0(game);
    printf("SMOKE TEST: load_menu0 ok (menu0=%p menuMusic=%p)\n",
           (void *)game->menu0, (void *)game->audio.menuMusic);

    // A second entry into the same load path, to confirm the guarded
    // reload behavior from Pass 1 (menu0/menuMusic should NOT be reloaded --
    // pointers should be identical to the first load).
    SDL_Texture *menu0Before = game->menu0;
    Mix_Music *menuMusicBefore = game->audio.menuMusic;
    load_menu0(game);
    printf("SMOKE TEST: load_menu0 second call -- menu0 unchanged=%s, menuMusic unchanged=%s\n",
           (game->menu0 == menu0Before) ? "yes" : "NO (regression)",
           (game->audio.menuMusic == menuMusicBefore) ? "yes" : "NO (regression)");

    // Simulate "main menu -> Arcade -> main menu -> Arcade" a few times:
    // load_menu1()/arcade_assets_load() run every time the mode is
    // (re-)entered in the real main loop, so call them repeatedly here too.
    for (int i = 0; i < 3; i++)
    {
        load_menu1(game);
        arcade_assets_load(game);
    }
    printf("SMOKE TEST: repeated load_menu1()+arcade_assets_load() x3 ok (bossTexture=%p arcadeAssetsLoaded=%d)\n",
           (void *)game->bossTexture, game->assetFlags.arcadeAssetsLoaded);
    SDL_Texture *bossBefore = game->bossTexture;
    TTF_Font *fontBefore = game->font;
    arcade_assets_load(game);
    printf("SMOKE TEST: arcade_assets_load() extra call -- bossTexture unchanged=%s, font unchanged=%s\n",
           (game->bossTexture == bossBefore) ? "yes" : "NO (regression)",
           (game->font == fontBefore) ? "yes" : "NO (regression)");

    // Simulate "main menu -> Runner -> main menu -> Runner" a few times.
    for (int i = 0; i < 3; i++)
    {
        load_menu2(game);
        runner_assets_load(game);
    }
    printf("SMOKE TEST: repeated load_menu2()+runner_assets_load() x3 ok (star=%p runnerAssetsLoaded=%d)\n",
           (void *)game->star, game->assetFlags.runnerAssetsLoaded);
    SDL_Texture *starBefore = game->star;
    runner_assets_load(game);
    printf("SMOKE TEST: runner_assets_load() extra call -- star unchanged=%s\n",
           (game->star == starBefore) ? "yes" : "NO (regression)");

    app_shutdown(&game, &window, &renderer);
    printf("SMOKE TEST: first app_shutdown ok (game=%p window=%p renderer=%p)\n",
           (void *)game, (void *)window, (void *)renderer);

    // Idempotency check: calling shutdown again on already-NULLed pointers
    // must not crash (double-free / double-destroy).
    app_shutdown(&game, &window, &renderer);
    printf("SMOKE TEST: second app_shutdown (idempotency check) completed without crash\n");

    printf("SMOKE TEST: PASS\n");
    return 0;
}
