# Phase 32: Data-Driven Arcade Waves

Arcade progression is now an explicit `WaveDefinition` table rather than a
chain of `tempScore == N` conditions. Each definition declares regular, smart,
and boss counts plus a spawn cadence. The table repeats after its final entry,
so a successful run has no score-threshold dead end.

`arcade_waves_update()` is the single progression owner. It spawns one entity
at the configured cadence, waits for every spawned entity to leave play, then
inserts a two-second rest period before the following wave. Boss waves are
telegraphed during that rest period and displayed in a contrasting HUD label.

This phase deliberately leaves the established movement constants unchanged.
Per-wave speed multipliers require per-entity speed ownership and should be
added with the planned AI/state work rather than by changing global force
constants.

`docs/verification/arcade_waves_test.c` covers the opening definition,
cadence, rest interval, boss telegraph, active-enemy completion guard, and
invalid lookup.
