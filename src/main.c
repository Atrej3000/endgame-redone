#include "header.h"
#include "app.h"
#include "scene.h"
#include "frame.h"
#include "input_snapshot.h"
#include "settings_menu.h"

int main()
{
    GameState *gameState = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&gameState, &window, &renderer))
    {
        return 1;
    }

    gameState->x_score = 0;
    for(gameState->x_i = 0; gameState->x_i < 25; gameState->x_i++) {
        gameState->x_list[gameState->x_i] = 0;
    }

    load_menu0(gameState);
    // Bootstrap only -- the one direct assignment to `scene` outside
    // app_change_scene(), since there is no "previous scene" transition to
    // run an enter-hook for at program start. calloc already zero-initializes
    // `scene` to APP_SCENE_MAIN_MENU (its first member); set explicitly here
    // for clarity rather than relying on that implicitly.
    gameState->app.scene = APP_SCENE_MAIN_MENU;

    // Fixed-timestep physics (Phase 11) -- see docs/physics-timestep-map.md.
    // Real elapsed time is measured every loop iteration regardless of
    // scene, so the timestamp stays accurate across scene changes; the
    // accumulator itself only accrues time while a gameplay scene is
    // active, so no backlog builds up while paused/in menus/leaderboards.
    Uint64 perfFrequency = SDL_GetPerformanceFrequency();
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    double accumulator = 0.0;

    // Opt-in performance logging (Phase 16, see docs/optimization-map.md) --
    // off unless ENDGAME_PERF_LOG is set, in which case a summary line prints
    // once per real second. Every perfLoggingEnabled-gated block below is
    // pure measurement: none of it changes accumulator, steps, or any
    // simulate()/render() call, so disabled behavior is bit-for-bit identical
    // to before this phase.
    bool perfLoggingEnabled = (getenv("ENDGAME_PERF_LOG") != NULL);
    gameState->perfLoggingEnabled = perfLoggingEnabled;
    double perfPhysicsTimeAccum = 0.0;
    double perfRenderTimeAccum = 0.0;
    double perfLogTimer = 0.0;
    int perfPhysicsStepsThisSecond = 0;
    int perfRenderCallsThisSecond = 0;

    while (gameState->app.scene != APP_SCENE_QUIT) {
        Uint64 currentCounter = SDL_GetPerformanceCounter();
        double frameTime = (double)(currentCounter - previousCounter) / (double)perfFrequency;
        previousCounter = currentCounter;
        if (frameTime > MAX_FRAME_TIME)
        {
            frameTime = MAX_FRAME_TIME;
        }

        // Input snapshot (Phase 17, see docs/input-snapshot-architecture-map.md):
        // read live keyboard state exactly once per real frame, here, outside
        // the fixed-step physics loop -- input_capture_arcade()/
        // input_capture_runner() (below) populate gameState->input's
        // continuous fields from this single snapshot, so every physics tick
        // and event-poll this frame produces sees identical held-key state.
        const Uint8 *keyboardState = SDL_GetKeyboardState(NULL);

        if (perfLoggingEnabled)
        {
            perfLogTimer += frameTime;
            if (perfLogTimer >= PERF_LOG_INTERVAL_SEC)
            {
                double physicsMs = (perfPhysicsStepsThisSecond > 0)
                    ? (perfPhysicsTimeAccum / perfPhysicsStepsThisSecond) * 1000.0
                    : 0.0;
                double renderMs = (perfRenderCallsThisSecond > 0)
                    ? (perfRenderTimeAccum / perfRenderCallsThisSecond) * 1000.0
                    : 0.0;
                double activeProjectilesPerTick = (perfPhysicsStepsThisSecond > 0)
                    ? (double)gameState->perfProjectileActiveSamples / perfPhysicsStepsThisSecond
                    : 0.0;
                printf("[perf] ticks=%d physics_ms=%.3f render_ms=%.3f active_projectiles=%.1f projectile_checks=%llu\n",
                       perfPhysicsStepsThisSecond, physicsMs, renderMs, activeProjectilesPerTick,
                       (unsigned long long)gameState->perfProjectileTargetChecks);
                perfPhysicsTimeAccum = 0.0;
                perfRenderTimeAccum = 0.0;
                perfPhysicsStepsThisSecond = 0;
                perfRenderCallsThisSecond = 0;
                gameState->perfProjectileActiveSamples = 0;
                gameState->perfProjectileTargetChecks = 0;
                perfLogTimer = 0.0;
            }
        }

        switch (gameState->app.scene) {
            case APP_SCENE_MAIN_MENU:
                menu0_events(gameState);
                doRender_menu0(renderer, gameState);
                break;

            case APP_SCENE_SETTINGS:
                settings_events(gameState);
                doRender_settings(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_MENU:
                menu_events(gameState);
                doRender_menu1(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_GAME:
            {
                input_capture_arcade(&gameState->input, keyboardState, &gameState->app.settings);
                input_apply_controller(&gameState->input, &gameState->app, true);
                accumulator += frameTime;
                int steps = 0;
                Uint64 perfPhysicsStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                while (accumulator >= (double)PHYSICS_DT && steps < MAX_PHYSICS_STEPS_PER_FRAME)
                {
                    arcade_simulate(gameState, PHYSICS_DT);
                    accumulator -= (double)PHYSICS_DT;
                    steps++;
                }
                if (perfLoggingEnabled)
                {
                    Uint64 perfPhysicsEnd = SDL_GetPerformanceCounter();
                    perfPhysicsTimeAccum += (double)(perfPhysicsEnd - perfPhysicsStart) / (double)perfFrequency;
                    perfPhysicsStepsThisSecond += steps;
                }

                gameState->renderAlpha = (float)(accumulator / (double)PHYSICS_DT);
                Uint64 perfRenderStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                doRender(renderer, gameState);
                if (perfLoggingEnabled)
                {
                    Uint64 perfRenderEnd = SDL_GetPerformanceCounter();
                    perfRenderTimeAccum += (double)(perfRenderEnd - perfRenderStart) / (double)perfFrequency;
                    perfRenderCallsThisSecond++;
                }
                processEvents(window, gameState);
                break;
            }

            case APP_SCENE_ARCADE_LEADERBOARD:
                processEvents(window, gameState);
                doRender_leaderboard(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_PAUSE:
                doRender_pause(renderer, gameState);
                pause_events(gameState);
                break;

            case APP_SCENE_RUNNER_MENU:
                menu_events(gameState);
                doRender_menu2(renderer, gameState);
                break;

            case APP_SCENE_RUNNER_GAME:
            {
                input_capture_runner(&gameState->input, keyboardState, &gameState->app.settings);
                input_apply_controller(&gameState->input, &gameState->app, false);
                accumulator += frameTime;
                int steps = 0;
                Uint64 perfPhysicsStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                while (accumulator >= (double)PHYSICS_DT && steps < MAX_PHYSICS_STEPS_PER_FRAME)
                {
                    runner_simulate(gameState, PHYSICS_DT);
                    accumulator -= (double)PHYSICS_DT;
                    steps++;
                }
                if (perfLoggingEnabled)
                {
                    Uint64 perfPhysicsEnd = SDL_GetPerformanceCounter();
                    perfPhysicsTimeAccum += (double)(perfPhysicsEnd - perfPhysicsStart) / (double)perfFrequency;
                    perfPhysicsStepsThisSecond += steps;
                }

                gameState->renderAlpha = (float)(accumulator / (double)PHYSICS_DT);
                Uint64 perfRenderStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                doRender2(renderer, gameState);
                if (perfLoggingEnabled)
                {
                    Uint64 perfRenderEnd = SDL_GetPerformanceCounter();
                    perfRenderTimeAccum += (double)(perfRenderEnd - perfRenderStart) / (double)perfFrequency;
                    perfRenderCallsThisSecond++;
                }
                runner_update_death(gameState); // see src/runner_death.c, docs/runner-death-lifecycle.md --
                                                 // kept after doRender2, matching the pre-Phase-11
                                                 // arcade_frame/runner_frame order exactly
                processEvents2(window, gameState);
                break;
            }

            case APP_SCENE_RUNNER_LEADERBOARD:
                processEvents2(window, gameState);
                doRender_leaderboard2(renderer, gameState);
                break;

            case APP_SCENE_RUNNER_PAUSE:
                doRender_pause(renderer, gameState);
                pause_events(gameState);
                break;

            default:
                app_change_scene(gameState, APP_SCENE_QUIT);
                break;
        }
    }

    app_shutdown(&gameState, &window, &renderer);
    return 0;
}
