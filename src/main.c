#include "header.h"
#include "app.h"
#include "scene.h"
#include "frame.h"
#include "fixed_step.h"
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

    // Bootstrap only -- the one direct assignment to `scene` outside
    // app_change_scene(), since there is no "previous scene" transition to
    // run an enter-hook for at program start. calloc already zero-initializes
    // `scene` to APP_SCENE_MAIN_MENU (its first member); set explicitly here
    // for clarity rather than relying on that implicitly.
    gameState->app.scene = APP_SCENE_MAIN_MENU;
    load_menu0(gameState);

    // Fixed-timestep physics clock -- see docs/physics-timestep-map.md.
    // Real elapsed time is measured every loop iteration regardless of
    // scene, so the timestamp stays accurate across scene changes; the
    // Its accumulator only accrues inside one gameplay scene and is reset
    // across every transition, so no mode/session/pause backlog can leak.
    Uint64 perfFrequency = SDL_GetPerformanceFrequency();
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    FixedStepClock fixedClock = {0};

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

        const AppScene frameScene = gameState->app.scene;
        if (!fixed_step_scene_is_gameplay(frameScene))
        {
            fixed_step_clock_reset(&fixedClock, frameScene);
        }

        switch (frameScene) {
            case APP_SCENE_MAIN_MENU:
                menu0_events(gameState);
                if (gameState->app.scene == frameScene) doRender_menu0(renderer, gameState);
                break;

            case APP_SCENE_SETTINGS:
                settings_events(gameState);
                if (gameState->app.scene == frameScene) doRender_settings(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_MENU:
                menu_events(gameState);
                if (gameState->app.scene == frameScene) doRender_menu1(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_GAME:
            {
                // Poll discrete input before any catch-up ticks. A pause,
                // quit, or game-mode transition must take effect before the
                // simulation can advance again.
                if (!processEvents(window, gameState))
                {
                    fixed_step_clock_reset(&fixedClock, gameState->app.scene);
                    break;
                }
                input_capture_arcade(&gameState->input, keyboardState, &gameState->app.settings);
                input_apply_controller(&gameState->input, &gameState->app, true);
                fixed_step_clock_begin_frame(&fixedClock, frameScene, frameTime);
                int steps = 0;
                Uint64 perfPhysicsStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                while (gameState->app.scene == frameScene &&
                       fixed_step_clock_should_step(&fixedClock, frameScene, steps))
                {
                    arcade_simulate(gameState, PHYSICS_DT);
                    fixed_step_clock_consume_step(&fixedClock);
                    steps++;
                }
                fixed_step_clock_finish_frame(&fixedClock, gameState->app.scene, steps);
                if (perfLoggingEnabled)
                {
                    Uint64 perfPhysicsEnd = SDL_GetPerformanceCounter();
                    perfPhysicsTimeAccum += (double)(perfPhysicsEnd - perfPhysicsStart) / (double)perfFrequency;
                    perfPhysicsStepsThisSecond += steps;
                }

                // Consequences may have changed scene during the first of
                // several catch-up ticks. Never run the remaining ticks or
                // render/process the departed gameplay scene.
                if (gameState->app.scene != frameScene) break;

                gameState->renderAlpha = fixed_step_clock_alpha(&fixedClock);
                Uint64 perfRenderStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                doRender(renderer, gameState);
                if (perfLoggingEnabled)
                {
                    Uint64 perfRenderEnd = SDL_GetPerformanceCounter();
                    perfRenderTimeAccum += (double)(perfRenderEnd - perfRenderStart) / (double)perfFrequency;
                    perfRenderCallsThisSecond++;
                }
                break;
            }

            case APP_SCENE_ARCADE_LEADERBOARD:
                if (processEvents(window, gameState) &&
                    gameState->app.scene == frameScene)
                {
                    doRender_leaderboard(renderer, gameState);
                }
                break;

            case APP_SCENE_ARCADE_PAUSE:
                pause_events(gameState);
                if (gameState->app.scene == frameScene) doRender_pause(renderer, gameState);
                break;

            case APP_SCENE_RUNNER_MENU:
                menu_events(gameState);
                if (gameState->app.scene == frameScene) doRender_menu2(renderer, gameState);
                break;

            case APP_SCENE_RUNNER_GAME:
            {
                if (!processEvents2(window, gameState))
                {
                    fixed_step_clock_reset(&fixedClock, gameState->app.scene);
                    break;
                }
                input_capture_runner(&gameState->input, keyboardState, &gameState->app.settings);
                input_apply_controller(&gameState->input, &gameState->app, false);
                fixed_step_clock_begin_frame(&fixedClock, frameScene, frameTime);
                int steps = 0;
                Uint64 perfPhysicsStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                while (gameState->app.scene == frameScene &&
                       fixed_step_clock_should_step(&fixedClock, frameScene, steps))
                {
                    runner_simulate(gameState, PHYSICS_DT);
                    fixed_step_clock_consume_step(&fixedClock);
                    steps++;
                }
                fixed_step_clock_finish_frame(&fixedClock, gameState->app.scene, steps);
                if (perfLoggingEnabled)
                {
                    Uint64 perfPhysicsEnd = SDL_GetPerformanceCounter();
                    perfPhysicsTimeAccum += (double)(perfPhysicsEnd - perfPhysicsStart) / (double)perfFrequency;
                    perfPhysicsStepsThisSecond += steps;
                }

                if (gameState->app.scene != frameScene) break;

                gameState->renderAlpha = fixed_step_clock_alpha(&fixedClock);
                Uint64 perfRenderStart = perfLoggingEnabled ? SDL_GetPerformanceCounter() : 0;
                doRender2(renderer, gameState);
                if (perfLoggingEnabled)
                {
                    Uint64 perfRenderEnd = SDL_GetPerformanceCounter();
                    perfRenderTimeAccum += (double)(perfRenderEnd - perfRenderStart) / (double)perfFrequency;
                    perfRenderCallsThisSecond++;
                }
                break;
            }

            case APP_SCENE_RUNNER_LEADERBOARD:
                if (processEvents2(window, gameState) &&
                    gameState->app.scene == frameScene)
                {
                    doRender_leaderboard2(renderer, gameState);
                }
                break;

            case APP_SCENE_RUNNER_PAUSE:
                pause_events(gameState);
                if (gameState->app.scene == frameScene) doRender_pause(renderer, gameState);
                break;

            default:
                app_change_scene(gameState, APP_SCENE_QUIT);
                break;
        }
    }

    app_shutdown(&gameState, &window, &renderer);
    return 0;
}
