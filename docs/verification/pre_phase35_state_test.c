// Focused pre-Phase-35 state/input regression test. This harness is kept out
// of the shipped game and intentionally has no Makefile target: it verifies
// event-polling invariants without changing the project/CI surface while the
// broader gap audit is still in progress.
#include "app.h"
#include "display.h"
#include "frame.h"
#include "input_command.h"
#include "input_snapshot.h"
#include "settings.h"

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (condition)                                                      \
        {                                                                   \
            printf("PRE-PHASE35 STATE TEST: PASS - %s\n", description);     \
        }                                                                   \
        else                                                                \
        {                                                                   \
            printf("PRE-PHASE35 STATE TEST: FAIL - %s\n", description);     \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void drain_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
    }
}

static void push_key(SDL_Keycode key, SDL_Scancode scancode, Uint8 repeat)
{
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.type = SDL_KEYDOWN;
    event.key.state = SDL_PRESSED;
    event.key.repeat = repeat;
    event.key.keysym.sym = key;
    event.key.keysym.scancode = scancode;
    (void)SDL_PushEvent(&event);
}

static bool input_is_clear(const InputState *input)
{
    return !input->moveLeftPlayer1 && !input->moveRightPlayer1 &&
           !input->jumpHeldPlayer1 && !input->shootHeldPlayer1 &&
           !input->moveLeftPlayer2 && !input->moveRightPlayer2 &&
           !input->jumpHeldPlayer2 && !input->shootHeldPlayer2 &&
           input->jumpBufferTicksPlayer1 == 0 &&
           input->jumpBufferTicksPlayer2 == 0;
}

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("PRE-PHASE35 STATE TEST: app_init failed\n");
        return 1;
    }
    drain_events();

    game->app.settings.player1.jump = SDL_SCANCODE_E;
    game->app.settings.player2.jump = SDL_SCANCODE_R;
    game->app.settings.player1.pause = SDL_SCANCODE_P;

    CHECK("configured P1 jump scancode translates to a P1 jump edge",
          translate_gameplay_command(SDLK_e, SDL_SCANCODE_E, &game->app.settings) ==
              GAME_COMMAND_JUMP_PLAYER1);
    CHECK("configured P2 jump scancode translates to a P2 jump edge",
          translate_gameplay_command(SDLK_r, SDL_SCANCODE_R, &game->app.settings) ==
              GAME_COMMAND_JUMP_PLAYER2);
    CHECK("legacy W is not a jump after P1 is rebound",
          translate_gameplay_command(SDLK_w, SDL_SCANCODE_W, &game->app.settings) ==
              GAME_COMMAND_NONE);
    CHECK("gameplay translation rejects a missing settings map",
          translate_gameplay_command(SDLK_e, SDL_SCANCODE_E, NULL) ==
              GAME_COMMAND_NONE);

    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only scene precondition
    game->multiPlayer = GAME_MODE_MULTIPLAYER;
    game->input.jumpBufferTicksPlayer1 = 0;
    push_key(SDLK_e, SDL_SCANCODE_E, 0);
    processEvents(window, game);
    CHECK("Arcade polling records the configured P1 jump edge",
          game->input.jumpBufferTicksPlayer1 == JUMP_BUFFER_TICKS);

    game->input.jumpBufferTicksPlayer1 = 0;
    push_key(SDLK_e, SDL_SCANCODE_E, 1);
    processEvents(window, game);
    CHECK("repeated keydown does not refresh a jump buffer",
          game->input.jumpBufferTicksPlayer1 == 0);

    // Once pause transitions the scene, the later jump must not mutate
    // gameplay state. Scene entry flushes stale key events but deliberately
    // leaves lifecycle events queued, so a following quit also proves this
    // polling call returned immediately.
    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only scene precondition
    game->input.jumpBufferTicksPlayer1 = 0;
    push_key(SDLK_p, SDL_SCANCODE_P, 0);
    push_key(SDLK_e, SDL_SCANCODE_E, 0);
    SDL_Event queuedQuit = {0};
    queuedQuit.type = SDL_QUIT;
    (void)SDL_PushEvent(&queuedQuit);
    processEvents(window, game);
    CHECK("pause is the first and only transition in its polling call",
          game->app.scene == APP_SCENE_ARCADE_PAUSE);
    CHECK("a jump queued after pause does not enter the gameplay buffer",
          game->input.jumpBufferTicksPlayer1 == 0);
    SDL_Event remaining = {0};
    CHECK("event polling stops immediately after the pause transition",
          SDL_PollEvent(&remaining) == 1 && remaining.type == SDL_QUIT);
    drain_events();

    // Event polling owns only events. No-event calls must not advance visual
    // cadence, scenery, shooting, hazards, or game-over consequences.
    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only scene precondition
    game->time = 0;
    game->train.x = 123;
    game->CurrentSheetBullet = 17;
    game->input.shootHeldPlayer1 = true;
    game->shotCount = 0;
    game->bullets[0].active = false;
    processEvents(window, game);
    CHECK("Arcade no-event polling does not move scenery",
          game->train.x == 123);
    CHECK("Arcade no-event polling does not advance animation cadence",
          game->CurrentSheetBullet == 17);
    CHECK("Arcade no-event polling does not fire a held shot",
          !game->bullets[0].active && game->shotCount == 0);

    game->app.scene = APP_SCENE_RUNNER_LEADERBOARD; // test-only scene precondition
    game->gameLives = 0;
    game->x_i = 4;
    game->x_list[4] = 777;
    game->man.isDead = 0;
    game->man.y = 800.0f;
    game->input.jumpBufferTicksPlayer1 = 0;
    push_key(SDLK_e, SDL_SCANCODE_E, 0);
    processEvents2(window, game);
    CHECK("Runner leaderboard polling does not duplicate a game-over score",
          game->x_i == 4 && game->x_list[4] == 777);
    CHECK("Runner leaderboard ignores gameplay jump commands",
          game->input.jumpBufferTicksPlayer1 == 0);
    CHECK("Runner leaderboard polling does not apply fall/death consequences",
          game->man.isDead == 0 && game->gameLives == 0);

    Uint8 keyboardState[SDL_NUM_SCANCODES] = {0};
    game->input.shootHeldPlayer1 = true;
    game->input.shootHeldPlayer2 = true;
    input_capture_runner(&game->input, keyboardState, &game->app.settings);
    CHECK("Runner capture clears stale Arcade shooting state",
          !game->input.shootHeldPlayer1 && !game->input.shootHeldPlayer2);
    game->input.moveLeftPlayer1 = true;
    input_capture_runner(&game->input, NULL, &game->app.settings);
    CHECK("snapshot capture rejects a missing keyboard state without mutation",
          game->input.moveLeftPlayer1);
    settings_defaults(&game->app.settings);
    game->app.settings.player1.moveLeft = (SDL_Scancode)-1;
    game->app.settings.player2.moveRight =
        (SDL_Scancode)SDL_NUM_SCANCODES;
    input_state_clear(&game->input);
    input_capture_arcade(&game->input, keyboardState,
                         &game->app.settings);
    CHECK("snapshot capture rejects corrupt scancodes without indexing out of bounds",
          !game->input.moveLeftPlayer1 &&
              !game->input.moveRightPlayer2);
    settings_defaults(&game->app.settings);
    input_state_clear(NULL);
    CHECK("event pollers reject null state",
          processEvents(NULL, NULL) == 0 &&
              processEvents2(NULL, NULL) == 0);

    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition
    game->man.x = 123.0f;
    game->man.prevX = 123.0f;
    game->gameLives = 3;
    push_key(SDLK_0, SDL_SCANCODE_0, 0);
    (void)processEvents2(window, game);
    CHECK("production Runner input does not expose the former zero-key cheat",
          game->man.x == 123.0f && game->man.prevX == 123.0f &&
              game->gameLives == 3);

    const bool fullscreenBeforeRepeat = game->app.display.fullscreen;
    SDL_Event repeatedFullscreen = {0};
    repeatedFullscreen.type = SDL_KEYDOWN;
    repeatedFullscreen.key.repeat = 1;
    repeatedFullscreen.key.keysym.sym = SDLK_F11;
    display_handle_event(game, &repeatedFullscreen);
    CHECK("repeated F11 does not toggle display state",
          game->app.display.fullscreen == fullscreenBeforeRepeat);

    game->input.moveLeftPlayer1 = true;
    game->input.jumpHeldPlayer2 = true;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;
    game->app.controllerJumpHeldLastFrame = true;
    SDL_Event focusLost = {0};
    focusLost.type = SDL_WINDOWEVENT;
    focusLost.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    display_handle_event(game, &focusLost);
    CHECK("focus loss clears continuous and buffered input",
          input_is_clear(&game->input));
    CHECK("focus loss clears the controller jump edge",
          !game->app.controllerJumpHeldLastFrame);
    CHECK("focus loss persistently pauses active Runner gameplay",
          game->app.scene == APP_SCENE_RUNNER_PAUSE);

    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only scene precondition
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    game->app.controllerJumpHeldLastFrame = true;
    (void)SDL_PushEvent(&focusLost);
    push_key(SDLK_e, SDL_SCANCODE_E, 0);
    processEvents(window, game);
    CHECK("focus loss clears state before a later queued key can re-buffer",
          input_is_clear(&game->input) && !game->app.controllerJumpHeldLastFrame);
    CHECK("focus loss persistently pauses active Arcade gameplay",
          game->app.scene == APP_SCENE_ARCADE_PAUSE);
    CHECK("focus loss flushes pending keyboard events",
          SDL_PollEvent(&remaining) == 0);

    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only scene precondition
    game->time = 321;
    game->renderAlpha = 0.375f;
    game->input.moveRightPlayer1 = true;
    (void)SDL_PushEvent(&focusLost);
    arcade_frame(game, window, renderer, PHYSICS_DT);
    CHECK("focus-loss frame skips held-input recapture and simulation",
          game->time == 321 && input_is_clear(&game->input) &&
              game->renderAlpha == 0.375f &&
              game->app.scene == APP_SCENE_ARCADE_PAUSE);

    drain_events();
    game->app.scene = APP_SCENE_MAIN_MENU; // test-only scene precondition
    SDL_Event quit = {0};
    quit.type = SDL_QUIT;
    (void)SDL_PushEvent(&quit);
    menu0_events(game);
    CHECK("SDL quit routes every polling scene to APP_SCENE_QUIT",
          game->app.scene == APP_SCENE_QUIT);

    app_shutdown(&game, &window, &renderer);
    if (failures == 0)
    {
        printf("PRE-PHASE35 STATE TEST: ALL PASS\n");
        return 0;
    }
    printf("PRE-PHASE35 STATE TEST: %d FAILURE(S)\n", failures);
    return 1;
}
