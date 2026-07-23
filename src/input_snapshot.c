#include "input_snapshot.h"

static bool scancode_is_valid(SDL_Scancode scancode)
{
    return (int)scancode > (int)SDL_SCANCODE_UNKNOWN &&
           (int)scancode < SDL_NUM_SCANCODES;
}

static bool keyboard_key_is_held(const Uint8 *keyboardState,
                                 SDL_Scancode scancode)
{
    return scancode_is_valid(scancode) && keyboardState[scancode] != 0U;
}

void input_state_clear(InputState *input)
{
    if (input != NULL)
    {
        *input = (InputState){0};
    }
}

void input_capture_arcade(InputState *input, const Uint8 *keyboardState, const GameSettings *settings)
{
    if (input == NULL || keyboardState == NULL || settings == NULL)
    {
        return;
    }
    input->moveLeftPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.moveLeft);
    input->moveRightPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.moveRight);
    input->jumpHeldPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.jump);
    input->shootHeldPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.shoot);

    input->moveLeftPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.moveLeft);
    input->moveRightPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.moveRight);
    input->jumpHeldPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.jump);
    input->shootHeldPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.shoot);
}

void input_capture_runner(InputState *input, const Uint8 *keyboardState, const GameSettings *settings)
{
    if (input == NULL || keyboardState == NULL || settings == NULL)
    {
        return;
    }
    input->moveLeftPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.moveLeft);
    input->moveRightPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.moveRight);
    input->jumpHeldPlayer1 =
        keyboard_key_is_held(keyboardState, settings->player1.jump);

    input->moveLeftPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.moveLeft);
    input->moveRightPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.moveRight);
    input->jumpHeldPlayer2 =
        keyboard_key_is_held(keyboardState, settings->player2.jump);
    input->shootHeldPlayer1 = false;
    input->shootHeldPlayer2 = false;
}

void input_apply_controller(InputState *input, AppContext *app, bool includeShoot)
{
    if (!input || !app)
    {
        return;
    }
    if (app->controller &&
        SDL_GameControllerGetAttached(app->controller) == SDL_FALSE)
    {
        input_controller_open_first(app);
    }
    if (!app->controller ||
        SDL_GameControllerGetAttached(app->controller) == SDL_FALSE)
    {
        app->controllerJumpHeldLastFrame = false;
        return;
    }

    SDL_GameController *controller = app->controller;

    const Sint16 horizontalAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    const bool movesLeft = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) != 0 || horizontalAxis <= -8000;
    const bool movesRight = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0 || horizontalAxis >= 8000;
    input->moveLeftPlayer1 = input->moveLeftPlayer1 || movesLeft;
    input->moveRightPlayer1 = input->moveRightPlayer1 || movesRight;
    const bool jumpHeld = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) != 0;
    if (jumpHeld && !app->controllerJumpHeldLastFrame)
    {
        input->jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    }
    app->controllerJumpHeldLastFrame = jumpHeld;
    input->jumpHeldPlayer1 = input->jumpHeldPlayer1 || jumpHeld;
    if (includeShoot)
    {
        input->shootHeldPlayer1 = input->shootHeldPlayer1 || SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X) != 0;
    }
}

void input_controller_open_first(AppContext *app)
{
    if (!app) return;
    if (app->controller)
    {
        if (SDL_GameControllerGetAttached(app->controller) == SDL_TRUE)
        {
            return;
        }
        SDL_GameControllerClose(app->controller);
        app->controller = NULL;
        app->controllerJumpHeldLastFrame = false;
    }
    const int joystickCount = SDL_NumJoysticks();
    for (int joystickIndex = 0; joystickIndex < joystickCount; joystickIndex++)
    {
        if (SDL_IsGameController(joystickIndex) == SDL_TRUE)
        {
            app->controller = SDL_GameControllerOpen(joystickIndex);
            if (app->controller)
            {
                input_controller_sync_jump_edge(app);
                return;
            }
        }
    }
}

void input_controller_sync_jump_edge(AppContext *app)
{
    if (!app || !app->controller ||
        SDL_GameControllerGetAttached(app->controller) == SDL_FALSE)
    {
        if (app) app->controllerJumpHeldLastFrame = false;
        return;
    }
    app->controllerJumpHeldLastFrame =
        SDL_GameControllerGetButton(app->controller,
                                    SDL_CONTROLLER_BUTTON_A) != 0;
}

void input_controller_handle_event(AppContext *app, const SDL_Event *event)
{
    if (!app || !event) return;
    if (event->type == SDL_CONTROLLERDEVICEADDED)
    {
        input_controller_open_first(app);
        return;
    }
    if (event->type == SDL_CONTROLLERDEVICEREMOVED && app->controller)
    {
        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(app->controller);
        if (joystick && SDL_JoystickInstanceID(joystick) == event->cdevice.which)
        {
            SDL_GameControllerClose(app->controller);
            app->controller = NULL;
            app->controllerJumpHeldLastFrame = false;
            input_controller_open_first(app);
        }
    }
}
