# Phase 25 — Profile-guided optimization

## Evidence and chosen optimization

`ENDGAME_PERF_LOG=1` already reports average physics and render time. Phase 25
extends that output with the average active projectile count and the exact
number of projectile-target narrow-phase checks performed during the reporting
interval.

The old collision path scanned every one of the 1,000 slots for each of 113
targets: 113,000 slot checks per player per fixed tick, even when the pool was
almost empty. The selected, bounded optimization is an active-index snapshot:
each projectile pool is scanned once, then only active indices are used while
iterating targets in their original order.

For `A` active projectiles, the work changes from `113 * 1000` slot scans to
`1000 + 113 * A` per player per tick. The narrow-phase collision count remains
exactly `113 * A`, which is included in the performance log for real-machine
profiling. No spatial grid was introduced: the target set is only 113 entities,
and the active-pool filter eliminates the demonstrably dominant empty-slot
work without increasing system complexity.

## Reading a log line

```
[perf] ticks=60 physics_ms=0.412 render_ms=1.203 active_projectiles=4.0 projectile_checks=27120
```

`active_projectiles` is the average primary-plus-secondary active pool entries
per fixed tick. `projectile_checks` is the total number of actual swept
projectile-vs-target checks in the one-second reporting window. Run both game
modes and representative combat scenes before considering a second
optimization; if physics time remains within budget, retain this simple design.

## Regression guarantee

The active-index list retains increasing bullet-index order and the original
target-category/target-index order. The collision-pipeline regression verifies
that one active bullet produces 113 target checks rather than 113,000
empty-pool slot scans, while projectile correctness tests preserve hit and
despawn behavior.
