# Phase 33: Data-Driven Runner Segments

Runner terrain and hazards now come from a small catalog of authored segment
templates: recovery, gentle rise, valley, and high-low. Every template has five
platforms at a fixed safe horizontal cadence and bounded vertical transitions;
hazard offsets are data rather than independently randomized positions.

Template selection remains seeded through `random()`, preserving deterministic
replay while preventing consecutive repeats and inserting a recovery segment
every fourth segment. Difficulty unlocks the high-low template after the
opening segments.

The 100 existing ledge/star slots act as a two-half streaming buffer. The
initial 20 segments cover 0-30,000px. At each 50-point distance milestone,
ten new segments replace only the buffer half behind the player, alternating
between slots 0–49 and 50–99. Streaming is now updated by `process2()` at the
fixed simulation rate, not `processEvents2()` at render/event rate.

`docs/verification/runner_segments_test.c` verifies template bounds, initial
coverage, the extension milestone, and safe buffer rotation.
