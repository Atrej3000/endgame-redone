#include "header.h"
#include "frame.h"
#include "game_events.h"

void arcade_simulate(GameState *game, float dt)
{
    capture_render_transforms(game);
    consume_arcade_jump_requests(game);
    apply_arcade_player_forces(game, dt);
    move_arcade_bullets(game, dt);
    process(game, dt);
    collisionDetect(game);
}

void runner_simulate(GameState *game, float dt)
{
    capture_render_transforms(game);
    consume_runner_jump_requests(game);
    apply_runner_player_forces(game, dt);
    process2(game, dt);
    game_events_begin(game);
    collisionDetect2(game);
    game_events_apply(game);
}

void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer, float dt)
{
    arcade_simulate(game, dt);
    // This legacy one-shot helper has no accumulator. Rendering the current
    // state is therefore the only sensible interpolation point.
    game->renderAlpha = 1.0f;
    doRender(renderer, game);
    processEvents(window, game);
}

void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer, float dt)
{
    runner_simulate(game, dt);
    game->renderAlpha = 1.0f;
    doRender2(renderer, game);
    runner_update_death(game); // see src/runner_death.c, docs/runner-death-lifecycle.md --
                                // kept after doRender2, matching the original arcade_frame/
                                // runner_frame order exactly (Phase 5/6 deliberately render
                                // the death-in-progress state before resolving it)
    processEvents2(window, game);
}
