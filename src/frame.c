#include "header.h"
#include "combat_feedback.h"
#include "collision_pipeline.h"
#include "frame.h"
#include "game_events.h"
#include "input_snapshot.h"

static bool simulation_dt_is_valid(float dt)
{
    return isfinite(dt) && dt > 0.0f && dt <= (float)MAX_FRAME_TIME;
}

void arcade_simulate(GameState *game, float dt)
{
    if (game == NULL || !simulation_dt_is_valid(dt)) return;
    if (combat_feedback_step(game)) return;
    capture_render_transforms(game);
    consume_arcade_jump_requests(game);
    apply_arcade_player_forces(game, dt);
    move_arcade_bullets(game, dt);
    process(game, dt);
    arcade_presentation_step(game);
    collisionDetect(game);
    arcade_shooting_step(game);
    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
}

void runner_simulate(GameState *game, float dt)
{
    if (game == NULL || !simulation_dt_is_valid(dt)) return;
    if (combat_feedback_step(game)) return;
    runner_death_step(game);
    capture_render_transforms(game);
    consume_runner_jump_requests(game);
    apply_runner_player_forces(game, dt);
    process2(game, dt);
    game_events_begin(game);
    collisionDetect2(game);
    detect_runner_fixed_hazards(game);
    game_events_apply(game);
}

void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer, float dt)
{
    if (!game) return;
    const AppScene sceneBeforeInput = game->app.scene;
    if (!processEvents(window, game) ||
        game->app.scene != sceneBeforeInput)
    {
        return;
    }
    const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);
    input_capture_arcade(&game->input, keyboardState, &game->app.settings);
    input_apply_controller(&game->input, &game->app, true);
    arcade_simulate(game, dt);
    if (game->app.scene != sceneBeforeInput)
    {
        return;
    }
    // This legacy one-shot helper has no accumulator. Rendering the current
    // state is therefore the only sensible interpolation point.
    game->renderAlpha = 1.0f;
    doRender(renderer, game);
}

void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer, float dt)
{
    if (!game) return;
    const AppScene sceneBeforeInput = game->app.scene;
    if (!processEvents2(window, game) ||
        game->app.scene != sceneBeforeInput)
    {
        return;
    }
    const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);
    input_capture_runner(&game->input, keyboardState, &game->app.settings);
    input_apply_controller(&game->input, &game->app, false);
    runner_simulate(game, dt);
    if (game->app.scene != sceneBeforeInput)
    {
        return;
    }
    game->renderAlpha = 1.0f;
    doRender2(renderer, game);
}
