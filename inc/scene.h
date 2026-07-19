#pragma once

// Focused header for scene routing (src/scene.c) -- see docs/scene-state-map.md
// and docs/solid-gof-audit.md section 7.3. AppScene itself remains defined in
// header.h (GameState.scene depends on it, so header.h cannot depend on this
// file without a circular include); this header exists so a consumer whose
// only concern is scene routing can include one narrow, standalone-sufficient
// header instead of the whole project header. app_change_scene()'s prototype
// is intentionally ALSO still declared in header.h (a legal duplicate) so
// menu_events.c/pause_events.c/processEvents.c -- which also call it -- do not
// need to change in this same commit; see the audit doc for the reasoning.
#include "header.h"

void app_change_scene(GameState *game, AppScene next_scene);
