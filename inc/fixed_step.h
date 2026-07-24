#pragma once

#include "header.h"

// State owned by the real-time loop, kept outside GameState because it is
// presentation scheduling rather than authoritative gameplay state.
typedef struct FixedStepClock
{
    double accumulator;
    AppScene scene;
    bool initialized;
} FixedStepClock;

bool fixed_step_scene_is_gameplay(AppScene scene);
void fixed_step_clock_reset(FixedStepClock *clock, AppScene scene);
// Invalid/NaN timing input is recovered to a safe empty accumulator.
void fixed_step_clock_begin_frame(FixedStepClock *clock, AppScene scene, double frameTime);
bool fixed_step_clock_should_step(const FixedStepClock *clock, AppScene scene, int stepsTaken);
void fixed_step_clock_consume_step(FixedStepClock *clock);
void fixed_step_clock_finish_frame(FixedStepClock *clock, AppScene scene, int stepsTaken);
float fixed_step_clock_alpha(const FixedStepClock *clock);
