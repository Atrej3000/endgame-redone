#include "header.h"

int leader_events(GameState *gameState)
{

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_ESCAPE]) {
        gameState->menu_status = 0;
    }
    return gameState->menu_status;
}
