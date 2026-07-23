# Phase 34: Combat Feedback and Animation

Combat consequences now produce deterministic presentation feedback without
changing collision, scoring, or spawn rules. A successful shot creates a brief
muzzle flash; projectile hits create impact bursts and hit flashes; defeated
targets get a larger particle burst. Runner and Arcade player contacts use the
same feedback path.

Hit-stop consumes one or two fixed simulation ticks (three for a boss hit) but
leaves rendering and input polling active. Optional screen shake respects the
persisted `screen_shake` setting and is derived from the tick counter rather
than fresh randomness, so replay remains stable.

`AnimationPlayer` replaces shared enemy frame counters at render time. Regular
enemies, smart enemies, bosses, and both players own their own frame clocks;
the renderer selects sprite-sheet frames from that state. Existing sprite
sheets are sufficient; future art can replace the sheets without changing the
feedback API.

`docs/verification/combat_feedback_test.c` verifies particles, flashes,
hit-stop duration, shake bounds, and independent animation clocks.
