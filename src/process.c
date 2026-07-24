#include "header.h"
#include "entity_spawn.h"
#include "ai_forces.h"
#include "arcade_waves.h"
#include "collision_pipeline.h"
#include "combat_feedback.h"
#include "game_events.h"
#include "runner_segments.h"

#include <stdint.h>

static bool valid_simulation_dt(const GameState *game, float dt)
{
    return game != NULL && isfinite(dt) && dt > 0.0f &&
           dt <= (float)MAX_FRAME_TIME;
}

static void advance_simulation_time(GameState *game)
{
    if (game->time < 0 || game->time == INT_MAX)
    {
        game->time = 0;
    }
    else
    {
        game->time++;
    }
}

static void move_wrapping_coordinate(int *coordinate, int delta, int leftEdge)
{
    if (coordinate == NULL || delta < 0) return;
    const int64_t next = (int64_t)*coordinate - (int64_t)delta;
    *coordinate = next < (int64_t)leftEdge ? WIDTH : (int)next;
}

static int advance_wrapped_counter(int counter, int frameCount)
{
    if (frameCount <= 1 || counter < 0 || counter >= frameCount)
    {
        return 0;
    }
    return counter == frameCount - 1 ? 0 : counter + 1;
}

// Simulation is also used by the headless replay/sanitizer harness. Effects
// are optional there (and during a recoverable asset-load failure), while the
// authoritative movement/result must remain valid.
static void play_effect(Mix_Chunk *effect, int volume)
{
    if (effect != NULL)
    {
        Mix_VolumeChunk(effect, volume);
        Mix_PlayChannel(-1, effect, 0);
    }
}

// Fixed pool (Phase 14, see docs/projectile-correctness-map.md): no
// malloc/free -- a shot claims the first inactive slot in place.
void addBullet(GameState *game, float x, float y, float dx)
{
    if (game == NULL || !isfinite(x) || !isfinite(y) || !isfinite(dx))
    {
        return;
    }

    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!game->bullets[i].active)
        {
            found = i;
            play_effect(game->audio.shoot, 32);
            break;
        }
    }
    if (found >= 0)
    {
        int i = found;
        game->bullets[i].x = x;
        game->bullets[i].y = y;
        game->bullets[i].dx = dx;
        game->bullets[i].prevX = x;
        game->bullets[i].prevY = y;
        game->bullets[i].active = true;
        combat_feedback_trigger_shot(game, 0, x, y);
    }
}

void removeBullet(GameState *game, int i)
{
    if (game == NULL || i < 0 || i >= MAX_BULLETS)
    {
        return;
    }
    game->bullets[i].active = false;
}

void addSecondBullet(GameState *game, float x, float y, float dx)
{
    if (game == NULL || !game->multiPlayer ||
        !isfinite(x) || !isfinite(y) || !isfinite(dx))
    {
        return;
    }
    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!game->secondBullets[i].active)
        {
            found = i;
            play_effect(game->audio.shoot, 32);
            break;
        }
    }
    if (found >= 0)
    {
        int i = found;
        game->secondBullets[i].x = x;
        game->secondBullets[i].y = y;
        game->secondBullets[i].dx = dx;
        game->secondBullets[i].prevX = x;
        game->secondBullets[i].prevY = y;
        game->secondBullets[i].active = true;
        combat_feedback_trigger_shot(game, 1, x, y);
    }
}

void removeSecondBullet(GameState *game, int i)
{
    if (game == NULL || i < 0 || i >= MAX_BULLETS)
    {
        return;
    }
    game->secondBullets[i].active = false;
}

// Authoritative shooting cadence belongs to the fixed simulation tick. The
// counters are remaining cooldown ticks rather than a render-frame latch, so
// zero-step and multi-step render frames produce the same projectile stream.
void arcade_shooting_step(GameState *game)
{
    if (game == NULL || game->gameLives <= 0)
    {
        return;
    }

    if (game->shotCount > 0)
    {
        game->shotCount--;
    }
    if (!game->man.isDead && game->input.shootHeldPlayer1 &&
        game->shotCount <= 0)
    {
        if (!game->man.facingLeft)
        {
            addBullet(game, game->man.x + 40, game->man.y + 15, 3);
        }
        else
        {
            addBullet(game, game->man.x, game->man.y + 15, -3);
        }
        game->shotCount = ARCADE_SHOT_COOLDOWN_TICKS;
    }

    if (game->multiPlayer)
    {
        if (game->shotCountMultiplayer > 0)
        {
            game->shotCountMultiplayer--;
        }
        if (!game->secondPlayer.isDead && game->input.shootHeldPlayer2 &&
            game->shotCountMultiplayer <= 0)
        {
            if (!game->secondPlayer.facingLeft)
            {
                addSecondBullet(game, game->secondPlayer.x + 40, game->secondPlayer.y + 15, 3);
            }
            else
            {
                addSecondBullet(game, game->secondPlayer.x, game->secondPlayer.y + 15, -3);
            }
            game->shotCountMultiplayer = ARCADE_SHOT_COOLDOWN_TICKS;
        }
    }
}

// Kept for processEvents() ABI compatibility. Real-frame event polling must
// never advance gameplay; arcade_simulate() calls arcade_shooting_step().
void process_arcade_shooting(GameState *game)
{
    (void)game;
}

void arcade_presentation_step(GameState *game)
{
    if (game == NULL)
    {
        return;
    }

    game->CurrentSheetBullet =
        advance_wrapped_counter(game->CurrentSheetBullet, 60);
    if (game->time % 2 == 0)
    {
        game->CurrentSheetBullet2 =
            advance_wrapped_counter(game->CurrentSheetBullet2, 30);
        move_wrapping_coordinate(&game->cloud1.x, 2, -312);
        move_wrapping_coordinate(&game->cloud2.x, 3, -416);
        move_wrapping_coordinate(&game->cloud3.x, 2, -280);
        move_wrapping_coordinate(&game->cloud4.x, 3, -292);
        move_wrapping_coordinate(&game->cloud5.x, 2, -216);
        move_wrapping_coordinate(&game->cloud6.x, 3, -162);
        move_wrapping_coordinate(&game->cloud7.x, 2, -264);
        move_wrapping_coordinate(&game->cloud8.x, 3, -352);
    }

    // The legacy render-frame path advanced the train twice (+4 and +2).
    // Keep one authoritative fixed-tick movement at the primary +4 speed.
    if (game->train.x > WIDTH - 4)
    {
        game->train.x = -480;
    }
    else
    {
        game->train.x += 4;
    }

    if (game->time % 4 == 0)
    {
        game->CurrentSpriteBack =
            advance_wrapped_counter(game->CurrentSpriteBack, 2);
        game->enemy.currentSpriteRun =
            advance_wrapped_counter(game->enemy.currentSpriteRun, 4);
        game->enemy.currentSpriteRun2 =
            advance_wrapped_counter(game->enemy.currentSpriteRun2, 6);
    }
    if (game->time % 5 == 0)
    {
        game->CurrentSheetBoss =
            advance_wrapped_counter(game->CurrentSheetBoss, 4);
    }
}

// Moves every active bullet exactly once per tick (Phase 14, see
// docs/projectile-correctness-map.md) -- called by arcade_simulate()
// (src/frame.c) before process(), which previously re-ran this same
// movement up to 113 times per tick (once per enemy/smart-enemy/boss
// collision-loop iteration). Captures prevX before moving, for the swept
// collision test in process()'s own collision loops.
//
// Normalizes dx to +-BULLET_SPEED_PER_SEC by sign, not a max-clamp: the
// legacy per-application clamp (0.1) was smaller than the spawn dx (+-3),
// so it always shrank toward the bound regardless of direction; the new
// bound (678 px/s, chosen to preserve the legacy 11.3 px/tick speed at
// 60 Hz) is *larger* than the spawn dx, so
// a max-clamp would never fire and the bullet would silently move at the
// spawn speed (3/tick) instead of the intended, speed-preserving one.
// Normalizing by sign reproduces the old code's actual effect (spawn
// direction, fixed magnitude every tick) regardless of which bound is
// bigger.
void move_arcade_bullets(GameState *game, float dt)
{
    if (!valid_simulation_dt(game, dt)) return;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (game->bullets[i].active)
        {
            game->bullets[i].prevX = game->bullets[i].x;
            game->bullets[i].prevY = game->bullets[i].y;
            if (game->bullets[i].dx > 0.0f)
            {
                game->bullets[i].dx = BULLET_SPEED_PER_SEC;
            }
            else if (game->bullets[i].dx < 0.0f)
            {
                game->bullets[i].dx = -BULLET_SPEED_PER_SEC;
            }
            const float nextX = game->bullets[i].x + game->bullets[i].dx * dt;
            if (!isfinite(game->bullets[i].x) || !isfinite(game->bullets[i].y) ||
                !isfinite(game->bullets[i].dx) || !isfinite(nextX))
            {
                removeBullet(game, i);
            }
            else
            {
                game->bullets[i].x = nextX;
            }
        }
    }

    if (game->multiPlayer)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (game->secondBullets[i].active)
            {
                game->secondBullets[i].prevX = game->secondBullets[i].x;
                game->secondBullets[i].prevY = game->secondBullets[i].y;
                if (game->secondBullets[i].dx > 0.0f)
                {
                    game->secondBullets[i].dx = BULLET_SPEED_PER_SEC;
                }
                else if (game->secondBullets[i].dx < 0.0f)
                {
                    game->secondBullets[i].dx = -BULLET_SPEED_PER_SEC;
                }
                const float nextX =
                    game->secondBullets[i].x + game->secondBullets[i].dx * dt;
                if (!isfinite(game->secondBullets[i].x) ||
                    !isfinite(game->secondBullets[i].y) ||
                    !isfinite(game->secondBullets[i].dx) || !isfinite(nextX))
                {
                    removeSecondBullet(game, i);
                }
                else
                {
                    game->secondBullets[i].x = nextX;
                }
            }
        }
    }
}

// Consumes the edge-triggered, buffered jump-request countdowns (Phase 12,
// extended with coyote time + jump buffering in Phase 15, see
// docs/game-feel-map.md) at the fixed physics tick rate -- called by
// arcade_simulate() (src/frame.c) before apply_arcade_player_forces, matching
// the original relative ordering. Coyote time (coyoteTicksRemaining) keeps a
// short grace window open after leaving a ledge; jump buffering
// (jumpBufferTicksPlayer1/2) keeps a jump request alive for a short window if
// pressed slightly before landing. Zeroing coyoteTicksRemaining the instant a
// jump fires prevents a second buffered request from also succeeding within
// the same still-open coyote window (an unintended free double-jump).
//
// The jump-fire check runs BEFORE this tick's coyote refresh/decay, not
// after: coyoteTicksRemaining reflects "grace ticks left as of last tick's
// grounding state" and must still be usable on the very tick it counts down
// to its last value, not consumed by the decrement before the check ever
// sees it (an off-by-one caught while writing docs/verification/game_feel_test.c,
// fixed here, not shipped).
void consume_arcade_jump_requests(GameState *game)
{
    if (game == NULL) return;
    Man *players[2] = {&game->man, &game->secondPlayer};
    const int playerCount = game->multiPlayer ? 2 : 1;
    for (int i = 0; i < playerCount; i++)
    {
        if (players[i]->onLedge) players[i]->airJumpsRemaining = 1;
        if (players[i]->doubleJumpAnimationTicks > 0) players[i]->doubleJumpAnimationTicks--;
        if (players[i]->damageInvulnerabilityTicks > 0) players[i]->damageInvulnerabilityTicks--;
    }

    if (game->input.jumpBufferTicksPlayer1 > 0)
    {
        if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;
            game->man.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer1 = 0;

            play_effect(game->audio.jump, 32);
        }
        else if (game->man.airJumpsRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.airJumpsRemaining--;
            game->man.doubleJumpAnimationTicks = DOUBLE_JUMP_ANIMATION_TICKS;
            game->input.jumpBufferTicksPlayer1 = 0;
            play_effect(game->audio.jump, 32);
        }
        else
        {
            game->input.jumpBufferTicksPlayer1--;
        }
    }

    if (game->multiPlayer && game->input.jumpBufferTicksPlayer2 > 0)
    {
        if (game->secondPlayer.onLedge || game->secondPlayer.coyoteTicksRemaining > 0)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.onLedge = 0;
            game->secondPlayer.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer2 = 0;

            play_effect(game->audio.jump, 32);
        }
        else if (game->secondPlayer.airJumpsRemaining > 0)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.airJumpsRemaining--;
            game->secondPlayer.doubleJumpAnimationTicks = DOUBLE_JUMP_ANIMATION_TICKS;
            game->input.jumpBufferTicksPlayer2 = 0;
            play_effect(game->audio.jump, 32);
        }
        else
        {
            game->input.jumpBufferTicksPlayer2--;
        }
    }

    if (game->man.onLedge)
    {
        game->man.coyoteTicksRemaining = COYOTE_TICKS;
    }
    else if (game->man.coyoteTicksRemaining > 0)
    {
        game->man.coyoteTicksRemaining--;
    }
    if (game->multiPlayer)
    {
        if (game->secondPlayer.onLedge)
        {
            game->secondPlayer.coyoteTicksRemaining = COYOTE_TICKS;
        }
        else if (game->secondPlayer.coyoteTicksRemaining > 0)
        {
            game->secondPlayer.coyoteTicksRemaining--;
        }
    }
}

// Continuous held-key forces for Arcade's `man`/`secondPlayer` (horizontal
// accel/clamp, friction/snap) plus variable-jump-height release-cut -- called
// by arcade_simulate() (src/frame.c) before process(), at the fixed physics
// tick rate. Kept as its own function, separate from process() itself
// (Phase 11, see docs/physics-timestep-map.md section 4): process() must
// remain callable directly with a manually-set dx/dy/slowingDown, unaffected
// by keyboard state, for docs/verification/*.c's existing direct-call tests
// to keep working -- if this logic lived inside process() itself, every call
// would silently re-read (and overwrite) whatever the test had just set up.
//
// Reads game->input's continuous fields (Phase 17, see
// docs/input-snapshot-architecture-map.md) instead of calling
// SDL_GetKeyboardState() itself -- main.c captures one snapshot per real
// frame, before the fixed-step loop runs, so every physics tick a frame
// produces sees identical held-key state.
void apply_arcade_player_forces(GameState *game, float dt)
{
    if (!valid_simulation_dt(game, dt)) return;
    bool manJumpHeld = game->input.jumpHeldPlayer1;
    if (manJumpHeld)
    {
        game->man.slowingDown = 0;

        if (game->time % 6 == 0)
        {
            game->man.currentSpriteJump++;
            game->man.currentSpriteJump %= 3;
        }
    }
    else if (game->man.jumpKeyHeldLastTick && game->man.dy < -JUMP_CUT_SPEED_PER_SEC)
    {
        // Variable jump height (Phase 15, see docs/game-feel-map.md):
        // released the jump key while still rising fast -- cut the ascent
        // short, giving a shorter hop for a tap and the full
        // JUMP_SPEED_PER_SEC arc for a held press.
        game->man.dy = -JUMP_CUT_SPEED_PER_SEC;
    }
    game->man.jumpKeyHeldLastTick = manJumpHeld;

    if (game->input.moveLeftPlayer1 && !game->input.moveRightPlayer1)
    {
        game->man.dx -= RUN_ACCEL_PER_SEC2 * dt;
        if (game->man.dx < -RUN_MAX_SPEED_PER_SEC)
        {
            game->man.dx = -RUN_MAX_SPEED_PER_SEC;
        }
        game->man.facingLeft = 1;
        game->man.slowingDown = 0;

        if (game->time % 6 == 0)
        {
            game->man.currentSpriteRun++;
            game->man.currentSpriteRun %= 4;
        }
    }
    else if (game->input.moveRightPlayer1 && !game->input.moveLeftPlayer1)
    {
        game->man.dx += RUN_ACCEL_PER_SEC2 * dt;
        if (game->man.dx > RUN_MAX_SPEED_PER_SEC)
        {
            game->man.dx = RUN_MAX_SPEED_PER_SEC;
        }
        game->man.facingLeft = 0;
        game->man.slowingDown = 0;

        if (game->time % 6 == 0)
        {
            game->man.currentSpriteRun++;
            game->man.currentSpriteRun %= 4;
        }
    }
    else
    {
        game->man.dx *= powf(RUN_FRICTION_DECAY_PER_TICK, dt * (float)PHYSICS_HZ);
        game->man.slowingDown = 1;
        game->man.currentSpriteRun = 1;
        if (fabsf(game->man.dx) < RUN_SNAP_ZERO_SPEED_PER_SEC)
        {
            game->man.dx = 0;
        }
    }

    if (game->multiPlayer)
    {
        bool secondPlayerJumpHeld = game->input.jumpHeldPlayer2;
        if (secondPlayerJumpHeld)
        {
            game->secondPlayer.slowingDown = 0;

            if (game->time % 6 == 0)
            {
                game->secondPlayer.currentSpriteJump2++;
                game->secondPlayer.currentSpriteJump2 %= 3;
            }
        }
        else if (game->secondPlayer.jumpKeyHeldLastTick && game->secondPlayer.dy < -JUMP_CUT_SPEED_PER_SEC)
        {
            // Variable jump height -- see the man block above.
            game->secondPlayer.dy = -JUMP_CUT_SPEED_PER_SEC;
        }
        game->secondPlayer.jumpKeyHeldLastTick = secondPlayerJumpHeld;

        if (game->input.moveLeftPlayer2 && !game->input.moveRightPlayer2)
        {
            game->secondPlayer.dx -= RUN_ACCEL_PER_SEC2 * dt;
            if (game->secondPlayer.dx < -RUN_MAX_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = -RUN_MAX_SPEED_PER_SEC;
            }
            game->secondPlayer.facingLeft = 1;
            game->secondPlayer.slowingDown = 0;

            if (game->time % 6 == 0)
            {
                game->secondPlayer.currentSpriteRun2++;
                game->secondPlayer.currentSpriteRun2 %= 4;
            }
        }
        else if (game->input.moveRightPlayer2 && !game->input.moveLeftPlayer2)
        {
            game->secondPlayer.dx += RUN_ACCEL_PER_SEC2 * dt;
            if (game->secondPlayer.dx > RUN_MAX_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = RUN_MAX_SPEED_PER_SEC;
            }
            game->secondPlayer.facingLeft = 0;
            game->secondPlayer.slowingDown = 0;

            if (game->time % 6 == 0)
            {
                game->secondPlayer.currentSpriteRun2++;
                game->secondPlayer.currentSpriteRun2 %= 4;
            }
        }
        else
        {
            game->secondPlayer.dx *= powf(RUN_FRICTION_DECAY_PER_TICK, dt * (float)PHYSICS_HZ);
            game->secondPlayer.slowingDown = 1;
            game->secondPlayer.currentSpriteRun2 = 1;
            if (fabsf(game->secondPlayer.dx) < RUN_SNAP_ZERO_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = 0;
            }
        }
    }
}

void process(GameState *game, float dt)
{
    if (!valid_simulation_dt(game, dt)) return;
    // BULLET
    advance_simulation_time(game);

    // Schedule the next wave entity before this tick's projectile/contact
    // consequences. A target destroyed this tick therefore remains gone for
    // the whole tick; the following tick owns the next spawn/transition.
    arcade_waves_update(game);

    // Contact detection produces events only; consequences execute once in
    // the dedicated Phase 24 consequence pass below.
    game_events_begin(game);
    detect_projectile_hits(game);
    game_events_apply(game);
    // Cull only after swept contact consequences have consumed the final
    // in-bounds-to-out-of-bounds segment.
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (game->bullets[i].active &&
            (game->bullets[i].x < 0.0f || game->bullets[i].x > (float)WIDTH))
        {
            removeBullet(game, i);
        }
        if (game->multiPlayer && game->secondBullets[i].active &&
            (game->secondBullets[i].x < 0.0f || game->secondBullets[i].x > (float)WIDTH))
        {
            removeSecondBullet(game, i);
        }
    }

    if (game->statusState == STATUS_STATE_LIVES)
    {
        shutdown_status_lives(game);
        game->statusState = STATUS_STATE_GAME;
    }
    // AI movement (Phase 18, see docs/ai-forces-separation-map.md) -- extracted
    // verbatim from process() into src/ai_forces.c, same relative position,
    // same statement order/math.
    move_boss_entities(game, dt);
    move_regular_enemies(game, dt);
    move_smart_enemies(game, dt);

    if (game->man.x > 1280)
    {
        game->man.x = -30;
        game->man.prevX = game->man.x;
        game->man.prevY = game->man.y;
    }
    if (game->man.x < -30)
    {
        game->man.x = 1270;
        game->man.prevX = game->man.x;
        game->man.prevY = game->man.y;
    }

    if (game->statusState == STATUS_STATE_GAME)
    {
        if (!game->man.isDead)
        {
            // Man MOVEMENT
            Man *man = &game->man;
            man->x += man->dx * dt;
            man->y += man->dy * dt;

            if (man->dx != 0 && man->onLedge && !man->slowingDown)
            {
                if (game->time % 3 == 0)
                {
                    if (man->animFrame < 11)
                    {
                        man->animFrame++;
                    }
                    else
                    {
                        man->animFrame = 0;
                    }
                }
            }
            man->dy += GRAVITY_PER_SEC2 * dt;
        }
        if (game->man.isDead && game->deathCountdown < 0)
        {
            game->deathCountdown = 120;
        }
        if (game->deathCountdown > 0)
        {
            game->deathCountdown--;
            if (game->deathCountdown < 0)
            {
                init_status_lives(game);
            }
        }
    }
    if (game->multiPlayer)
    {
        if (game->secondPlayer.x > 1280)
        {
            game->secondPlayer.x = -30;
            game->secondPlayer.prevX = game->secondPlayer.x;
            game->secondPlayer.prevY = game->secondPlayer.y;
        }
        if (game->secondPlayer.x < -30)
        {
            game->secondPlayer.x = 1270;
            game->secondPlayer.prevX = game->secondPlayer.x;
            game->secondPlayer.prevY = game->secondPlayer.y;
        }
        if (game->statusState == STATUS_STATE_GAME)
        {
            if (!game->secondPlayer.isDead)
            {
                // Man MOVEMENT
                Man *secondPlayer = &game->secondPlayer;
                secondPlayer->x += secondPlayer->dx * dt;
                secondPlayer->y += secondPlayer->dy * dt;

                secondPlayer->dy += GRAVITY_PER_SEC2 * dt;
            }
            if (game->secondPlayer.isDead && game->deathCountdown < 0)
            {
                game->deathCountdown = 120;
            }
            if (game->deathCountdown > 0)
            {
                game->deathCountdown--;
                if (game->deathCountdown < 0)
                {
                    init_status_lives(game);
                }
            }
        }
    }
    combat_feedback_update_animations(game);
}

// Consumes the edge-triggered, buffered jump-request countdowns for Runner
// -- same design as consume_arcade_jump_requests() above (Phase 12, extended
// with coyote time + jump buffering in Phase 15, see docs/game-feel-map.md).
// Also fixes a regression found during Phase 12's audit: Runner's jump
// impulse had never been converted off the bare frame-tuned `-10` during
// Phase 11's timestep conversion, producing a jump 60x weaker than intended
// at the fixed-timestep integration now in use -- this function uses
// JUMP_SPEED_PER_SEC, matching Arcade and header.h's own documented intent.
// See consume_arcade_jump_requests()'s comment above for why the jump-fire
// check runs before this tick's coyote refresh/decay, not after.
void consume_runner_jump_requests(GameState *game)
{
    if (game == NULL) return;
    Man *players[2] = {&game->man, &game->secondPlayer};
    const int playerCount = game->multiPlayer ? 2 : 1;
    for (int i = 0; i < playerCount; i++)
    {
        if (players[i]->onLedge) players[i]->airJumpsRemaining = 1;
        if (players[i]->doubleJumpAnimationTicks > 0) players[i]->doubleJumpAnimationTicks--;
        if (players[i]->damageInvulnerabilityTicks > 0) players[i]->damageInvulnerabilityTicks--;
    }

    if (game->input.jumpBufferTicksPlayer1 > 0)
    {
        if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;
            game->man.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer1 = 0;

            play_effect(game->audio.jump, 32);
        }
        else if (game->man.airJumpsRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.airJumpsRemaining--;
            game->man.doubleJumpAnimationTicks = DOUBLE_JUMP_ANIMATION_TICKS;
            game->input.jumpBufferTicksPlayer1 = 0;
            play_effect(game->audio.jump, 32);
        }
        else
        {
            game->input.jumpBufferTicksPlayer1--;
        }
    }

    if (game->multiPlayer && game->input.jumpBufferTicksPlayer2 > 0)
    {
        if (game->secondPlayer.onLedge || game->secondPlayer.coyoteTicksRemaining > 0)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.onLedge = 0;
            game->secondPlayer.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer2 = 0;

            play_effect(game->audio.jump, 32);
        }
        else if (game->secondPlayer.airJumpsRemaining > 0)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.airJumpsRemaining--;
            game->secondPlayer.doubleJumpAnimationTicks = DOUBLE_JUMP_ANIMATION_TICKS;
            game->input.jumpBufferTicksPlayer2 = 0;
            play_effect(game->audio.jump, 32);
        }
        else
        {
            game->input.jumpBufferTicksPlayer2--;
        }
    }

    if (game->man.onLedge)
    {
        game->man.coyoteTicksRemaining = COYOTE_TICKS;
    }
    else if (game->man.coyoteTicksRemaining > 0)
    {
        game->man.coyoteTicksRemaining--;
    }
    if (game->multiPlayer)
    {
        if (game->secondPlayer.onLedge)
        {
            game->secondPlayer.coyoteTicksRemaining = COYOTE_TICKS;
        }
        else if (game->secondPlayer.coyoteTicksRemaining > 0)
        {
            game->secondPlayer.coyoteTicksRemaining--;
        }
    }
}

// Continuous held-key forces for Runner's `man`/`secondPlayer` (horizontal
// accel/clamp, friction/snap) plus variable-jump-height release-cut -- called
// by runner_simulate() (src/frame.c) before process2(), at the fixed physics
// tick rate. Kept separate from process2() itself, same reasoning as
// apply_arcade_player_forces() above (Phase 11, see
// docs/physics-timestep-map.md section 4): process2() must remain callable
// directly with a manually-set dx/dy/slowingDown, unaffected by keyboard
// state, for docs/verification/*.c's existing direct-call tests.
//
// Reads game->input's continuous fields (Phase 17, see
// docs/input-snapshot-architecture-map.md) instead of calling
// SDL_GetKeyboardState() itself -- see apply_arcade_player_forces() above.
void apply_runner_player_forces(GameState *game, float dt)
{
    if (!valid_simulation_dt(game, dt)) return;
    bool manJumpHeld = game->input.jumpHeldPlayer1;
    if (!manJumpHeld && game->man.jumpKeyHeldLastTick && game->man.dy < -JUMP_CUT_SPEED_PER_SEC)
    {
        // Variable jump height -- see apply_arcade_player_forces()'s man
        // block, docs/game-feel-map.md.
        game->man.dy = -JUMP_CUT_SPEED_PER_SEC;
    }
    game->man.jumpKeyHeldLastTick = manJumpHeld;

    if (game->input.moveLeftPlayer1 && !game->input.moveRightPlayer1)
    {
        game->man.dx -= RUN_ACCEL_PER_SEC2 * dt;
        if (game->man.dx < -RUN_MAX_SPEED_PER_SEC)
        {
            game->man.dx = -RUN_MAX_SPEED_PER_SEC;
        }
        game->man.facingLeft = 1;
        game->man.slowingDown = 0;
    }
    else if (game->input.moveRightPlayer1 && !game->input.moveLeftPlayer1)
    {
        game->man.dx += RUN_ACCEL_PER_SEC2 * dt;
        if (game->man.dx > RUN_MAX_SPEED_PER_SEC)
        {
            game->man.dx = RUN_MAX_SPEED_PER_SEC;
        }
        game->man.facingLeft = 0;
        game->man.slowingDown = 0;
    }
    else
    {
        game->man.animFrame = 0;
        game->man.dx *= powf(RUN_FRICTION_DECAY_PER_TICK, dt * (float)PHYSICS_HZ);
        game->man.slowingDown = 1;
        if (fabsf(game->man.dx) < RUN_SNAP_ZERO_SPEED_PER_SEC)
        {
            game->man.dx = 0;
        }
    }

    if (game->multiPlayer)
    {
        bool secondPlayerJumpHeld = game->input.jumpHeldPlayer2;
        if (!secondPlayerJumpHeld && game->secondPlayer.jumpKeyHeldLastTick && game->secondPlayer.dy < -JUMP_CUT_SPEED_PER_SEC)
        {
            game->secondPlayer.dy = -JUMP_CUT_SPEED_PER_SEC;
        }
        game->secondPlayer.jumpKeyHeldLastTick = secondPlayerJumpHeld;

        if (game->input.moveLeftPlayer2 && !game->input.moveRightPlayer2)
        {
            game->secondPlayer.dx -= RUN_ACCEL_PER_SEC2 * dt;
            if (game->secondPlayer.dx < -RUN_MAX_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = -RUN_MAX_SPEED_PER_SEC;
            }
            game->secondPlayer.facingLeft = 1;
            game->secondPlayer.slowingDown = 0;
        }
        else if (game->input.moveRightPlayer2 && !game->input.moveLeftPlayer2)
        {
            game->secondPlayer.dx += RUN_ACCEL_PER_SEC2 * dt;
            if (game->secondPlayer.dx > RUN_MAX_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = RUN_MAX_SPEED_PER_SEC;
            }
            game->secondPlayer.facingLeft = 0;
            game->secondPlayer.slowingDown = 0;
        }
        else
        {
            game->secondPlayer.animFrameSecond = 0;
            game->secondPlayer.dx *= powf(RUN_FRICTION_DECAY_PER_TICK, dt * (float)PHYSICS_HZ);
            game->secondPlayer.slowingDown = 1;
            if (fabsf(game->secondPlayer.dx) < RUN_SNAP_ZERO_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = 0;
            }
        }
    }
}

void process2(GameState *game, float dt)
{
    if (!valid_simulation_dt(game, dt)) return;
    advance_simulation_time(game);

    if (game->time > 00)
    {
        game->statusState = STATUS_STATE_GAME; //show amount of lives
    }

    if (game->statusState == STATUS_STATE_GAME)
    {
        if (!game->man.isDead)
        {
            // Man MOVEMENT
            Man *man = &game->man;
            man->x += man->dx * dt;
            man->y += man->dy * dt;

            if (man->dx != 0 && man->onLedge && !man->slowingDown)
            {
                if (game->time % 3 == 0)
                {
                    if (man->animFrame < 11)
                    {
                        man->animFrame++;
                    }
                    else
                    {
                        man->animFrame = 0;
                    }
                }
            }
            man->dy += GRAVITY_PER_SEC2 * dt;
        }

        // Second player
        if (game->multiPlayer)
        {
            // Gated on the same shared death flag as `man` -- see
            // docs/runner-death-lifecycle.md: death is a shared, "pause the
            // world" event for both players in Runner (one gameLives pool).
            if (!game->man.isDead)
            {
                game->secondPlayer.x += game->secondPlayer.dx * dt;
                game->secondPlayer.y += game->secondPlayer.dy * dt;

                if (game->secondPlayer.dx != 0 && game->secondPlayer.onLedge && !game->secondPlayer.slowingDown)
                {
                    if (game->time % 3 == 0)
                    {
                        if (game->secondPlayer.animFrameSecond < 11)
                        {
                            game->secondPlayer.animFrameSecond++;
                        }
                        else
                        {
                            game->secondPlayer.animFrameSecond = 0;
                        }
                    }
                }
                game->secondPlayer.dy += GRAVITY_PER_SEC2 * dt;
            }
        }

        //moving traps
        for(int i = 0; i < NUM_STARS; i++)
      {
        game->stars[i].x = game->stars[i].baseX;
        game->stars[i].y = game->stars[i].baseY;
        
        if(game->stars[i].mode == 0)
        {
          game->stars[i].x = (int)((float)game->stars[i].baseX + sinf(game->stars[i].phase +
              (float)game->time * PHYSICS_DT * TRAP_ANGULAR_SPEED_PER_SEC) * 75.0f);
        }
        else
        {
          game->stars[i].y = (int)((float)game->stars[i].baseY + cosf(game->stars[i].phase +
              (float)game->time * PHYSICS_DT * TRAP_ANGULAR_SPEED_PER_SEC) * 75.0f);
        }
      }
        // Death-countdown progression itself is owned by runner_death_step()
        // (src/runner_death.c), called once per fixed simulation tick --
        // see docs/runner-death-lifecycle.md. This used to also decrement
        // gameLives here, unboundedly (deathCountdown was never decremented),
        // independent of runner_resolve_death()'s own decrement; removed.
    }
    // World streaming belongs to the fixed simulation path so live play and
    // deterministic replay generate the same tested segment sequence.
    runner_segments_update(game);
    if (game->multiPlayer)
    {
        if (game->time >= 200)
            game->scrollX -= RUNNER_MULTIPLAYER_CAMERA_SPEED_PER_SEC * dt;
    }
    if (!game->multiPlayer)
    {
        game->scrollX = -game->man.x + 500;

        if (game->scrollX > 0)
            game->scrollX = 0;
    }
    combat_feedback_update_animations(game);
}
