#include "fixed_step.h"

bool fixed_step_scene_is_gameplay(AppScene scene)
{
    return scene == APP_SCENE_ARCADE_GAME || scene == APP_SCENE_RUNNER_GAME;
}

void fixed_step_clock_reset(FixedStepClock *clock, AppScene scene)
{
    if (!clock) return;
    clock->accumulator = 0.0;
    clock->scene = scene;
    clock->initialized = true;
}

void fixed_step_clock_begin_frame(FixedStepClock *clock, AppScene scene, double frameTime)
{
    if (!clock) return;
    if (!clock->initialized || clock->scene != scene || !fixed_step_scene_is_gameplay(scene))
    {
        fixed_step_clock_reset(clock, scene);
    }
    if (!fixed_step_scene_is_gameplay(scene)) return;

    if (!isfinite(clock->accumulator) || clock->accumulator < 0.0)
    {
        clock->accumulator = 0.0;
    }
    if (isnan(frameTime)) frameTime = 0.0;
    if (frameTime < 0.0) frameTime = 0.0;
    if (frameTime > MAX_FRAME_TIME) frameTime = MAX_FRAME_TIME;
    clock->accumulator += frameTime;
}

bool fixed_step_clock_should_step(const FixedStepClock *clock, AppScene scene, int stepsTaken)
{
    return clock != NULL && clock->initialized && clock->scene == scene &&
           fixed_step_scene_is_gameplay(scene) &&
           stepsTaken >= 0 &&
           isfinite(clock->accumulator) &&
           stepsTaken < MAX_PHYSICS_STEPS_PER_FRAME &&
           clock->accumulator >= (double)PHYSICS_DT;
}

void fixed_step_clock_consume_step(FixedStepClock *clock)
{
    if (!clock) return;
    if (!isfinite(clock->accumulator) || clock->accumulator < 0.0)
    {
        clock->accumulator = 0.0;
        return;
    }
    clock->accumulator -= (double)PHYSICS_DT;
    if (clock->accumulator < 0.0) clock->accumulator = 0.0;
}

void fixed_step_clock_finish_frame(FixedStepClock *clock, AppScene scene, int stepsTaken)
{
    if (!clock) return;
    if (!clock->initialized || clock->scene != scene || !fixed_step_scene_is_gameplay(scene))
    {
        fixed_step_clock_reset(clock, scene);
        return;
    }
    if (!isfinite(clock->accumulator) || clock->accumulator < 0.0)
    {
        clock->accumulator = 0.0;
        return;
    }

    // A capped frame deliberately drops whole pending ticks. Keeping that
    // backlog made every following frame catch up forever and allowed alpha
    // to exceed one. Preserve only the interpolation remainder.
    if (stepsTaken >= MAX_PHYSICS_STEPS_PER_FRAME &&
        clock->accumulator >= (double)PHYSICS_DT)
    {
        clock->accumulator = fmod(clock->accumulator, (double)PHYSICS_DT);
    }
}

float fixed_step_clock_alpha(const FixedStepClock *clock)
{
    if (!clock || !clock->initialized || !fixed_step_scene_is_gameplay(clock->scene))
    {
        return 0.0f;
    }
    if (!isfinite(clock->accumulator) || clock->accumulator <= 0.0)
    {
        return 0.0f;
    }
    double alpha = clock->accumulator / (double)PHYSICS_DT;
    if (alpha < 0.0) alpha = 0.0;
    if (alpha > 1.0) alpha = 1.0;
    return (float)alpha;
}
