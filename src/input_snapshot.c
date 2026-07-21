#include "input_snapshot.h"

void input_capture_arcade(InputState *input, const Uint8 *keyboardState)
{
    input->moveLeftPlayer1 = keyboardState[SDL_SCANCODE_A];
    input->moveRightPlayer1 = keyboardState[SDL_SCANCODE_D];
    input->jumpHeldPlayer1 = keyboardState[SDL_SCANCODE_W];
    input->shootHeldPlayer1 = keyboardState[SDL_SCANCODE_SPACE];

    input->moveLeftPlayer2 = keyboardState[SDL_SCANCODE_LEFT];
    input->moveRightPlayer2 = keyboardState[SDL_SCANCODE_RIGHT];
    input->jumpHeldPlayer2 = keyboardState[SDL_SCANCODE_UP];
    input->shootHeldPlayer2 = keyboardState[SDL_SCANCODE_KP_0];
}

void input_capture_runner(InputState *input, const Uint8 *keyboardState)
{
    input->moveLeftPlayer1 = keyboardState[SDL_SCANCODE_A];
    input->moveRightPlayer1 = keyboardState[SDL_SCANCODE_D];
    input->jumpHeldPlayer1 = keyboardState[SDL_SCANCODE_W];

    input->moveLeftPlayer2 = keyboardState[SDL_SCANCODE_LEFT];
    input->moveRightPlayer2 = keyboardState[SDL_SCANCODE_RIGHT];
    input->jumpHeldPlayer2 = keyboardState[SDL_SCANCODE_UP];
}
