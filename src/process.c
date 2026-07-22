#include "header.h"
#include "entity_spawn.h"
#include "ai_forces.h"
#include "collision_pipeline.h"
#include "game_events.h"

// Fixed pool (Phase 14, see docs/projectile-correctness-map.md): no
// malloc/free -- a shot claims the first inactive slot in place.
void addBullet(GameState *game, float x, float y, float dx)
{

    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!game->bullets[i].active)
        {
            found = i;
            Mix_VolumeChunk(game->audio.shoot, 32);
            Mix_PlayChannel(-1, game->audio.shoot, 0);
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
    }
}

void removeBullet(GameState *game, int i)
{
    game->bullets[i].active = false;
}

void addSecondBullet(GameState *game, float x, float y, float dx)
{
    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!game->secondBullets[i].active)
        {
            found = i;
            Mix_VolumeChunk(game->audio.shoot, 32);
            Mix_PlayChannel(-1, game->audio.shoot, 0);
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
    }
}

void removeSecondBullet(GameState *game, int i)
{
    game->secondBullets[i].active = false;
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
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (game->bullets[i].active)
        {
            game->bullets[i].prevX = game->bullets[i].x;
            game->bullets[i].dx = (game->bullets[i].dx >= 0) ? BULLET_SPEED_PER_SEC : -BULLET_SPEED_PER_SEC;
            game->bullets[i].x += game->bullets[i].dx * dt;
            if ((game->bullets[i].x < 0) || (game->bullets[i].x > 1280))
            {
                removeBullet(game, i);
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
                game->secondBullets[i].dx = (game->secondBullets[i].dx >= 0) ? BULLET_SPEED_PER_SEC : -BULLET_SPEED_PER_SEC;
                game->secondBullets[i].x += game->secondBullets[i].dx * dt;
                if ((game->secondBullets[i].x < 0) || (game->secondBullets[i].x > 1280))
                {
                    removeSecondBullet(game, i);
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
    if (game->input.jumpBufferTicksPlayer1 > 0)
    {
        if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;
            game->man.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer1 = 0;

            Mix_VolumeChunk(game->audio.jump, 32);
            Mix_PlayChannel(-1, game->audio.jump, 0);
        }
        else
        {
            game->input.jumpBufferTicksPlayer1--;
        }
    }

    if (game->input.jumpBufferTicksPlayer2 > 0)
    {
        if (game->secondPlayer.onLedge || game->secondPlayer.coyoteTicksRemaining > 0)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.onLedge = 0;
            game->secondPlayer.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer2 = 0;

            Mix_VolumeChunk(game->audio.jump, 32);
            Mix_PlayChannel(-1, game->audio.jump, 0);
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
    if (game->secondPlayer.onLedge)
    {
        game->secondPlayer.coyoteTicksRemaining = COYOTE_TICKS;
    }
    else if (game->secondPlayer.coyoteTicksRemaining > 0)
    {
        game->secondPlayer.coyoteTicksRemaining--;
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

    if (game->input.moveLeftPlayer1)
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
    else if (game->input.moveRightPlayer1)
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

        if (game->input.moveLeftPlayer2)
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
        else if (game->input.moveRightPlayer2)
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
    // BULLET
    game->time++;

    // Contact detection produces events only; consequences execute once in
    // the dedicated Phase 24 consequence pass below.
    game_events_begin(game);
    detect_projectile_hits(game);
    game_events_apply(game);

    if (game->time == 120)
    {
        for (int i = 0; i < 10; i += 2)
        {
            enemy_spawn(game, i, 620, (float)i * -200.0f, 0, 0);
            enemy_spawn(game, i + 1, 580, (float)i * -200.0f, 0, 0);
        }
    }

    if (game->tempScore == 10)
    {
        for (int j = 0; j < 20; j += 2)
        {
            enemy_spawn(game, j, 620, (float)j * -200.0f, 0, 0);
            enemy_spawn(game, j + 1, 580, (float)j * -200.0f, 0, 0);
        }
        game->tempScore++;
    }

    if (game->tempScore == 31)
    {
        for (int j = 0; j < 30; j += 2)
        {
            enemy_spawn(game, j, 620, (float)j * -200.0f, 0, 0);
            enemy_spawn(game, j + 1, 580, (float)j * -200.0f, 0, 0);
        }

        smart_enemy_spawn(game, 0, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 1, 200, 200, 0, 0);

        game->tempScore++;
    }
    if (game->tempScore == 72)
    {
        for (int j = 0; j < 50; j += 2)
        {
            enemy_spawn(game, j, 620, (float)j * -100.0f, 0, 0);
            enemy_spawn(game, j + 1, 580, (float)j * -100.0f, 0, 0);
        }
        smart_enemy_spawn(game, 2, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 3, 200, 200, 0, 0);

        game->tempScore++;
    }

    if (game->tempScore == 133)
    {
        for (int j = 0; j < 70; j += 2)
        {
            enemy_spawn(game, j, 620, (float)j * -100.0f, 0, 0);
            enemy_spawn(game, j + 1, 580, (float)j * -100.0f, 0, 0);
        }
        smart_enemy_spawn(game, 4, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 5, 200, 200, 0, 0);
        smart_enemy_spawn(game, 6, 1000, 500, 0, 0);
        smart_enemy_spawn(game, 7, 200, 500, 0, 0);

        game->tempScore++;
    }
    if (game->tempScore == 224)
    {
        for (int j = 0; j < 100; j += 2)
        {
            enemy_spawn(game, j, 620, (float)j * -100.0f, 0, 0);
            enemy_spawn(game, j + 1, 580, (float)j * -100.0f, 0, 0);
        }
        smart_enemy_spawn(game, 0, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 1, 200, 200, 0, 0);
        smart_enemy_spawn(game, 2, 1000, 500, 0, 0);
        smart_enemy_spawn(game, 3, 200, 500, 0, 0);
        game->tempScore++;
    }
    if (game->tempScore == 345)
    {
        boss_spawn(game, 0, 1100, 0, 0, 0);
        boss_spawn(game, 1, 100, 0, 0, 0);

        game->tempScore++;
    }

    if (game->tempScore == 366)
    {
        for (int i = 0; i < NUM_ENEMIES - 1; i += 2)
        {
            enemy_spawn(game, i, 620, (float)i * -100.0f, 0, 0);
            enemy_spawn(game, i + 1, 580, (float)i * -100.0f, 0, 0);
        }
        boss_spawn(game, 0, 1100, 0, 0, 0);
        boss_spawn(game, 1, 100, 0, 0, 0);

        smart_enemy_spawn(game, 0, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 1, 200, 200, 0, 0);
        smart_enemy_spawn(game, 2, 1000, 500, 0, 0);
        smart_enemy_spawn(game, 3, 200, 500, 0, 0);

        game->tempScore++;
    }
    if (game->tempScore > 506)
    {
        game->tempScore = 365;
    }

    if (game->time > 0)
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
    if (game->input.jumpBufferTicksPlayer1 > 0)
    {
        if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;
            game->man.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer1 = 0;

            Mix_VolumeChunk(game->audio.jump, 32);
            Mix_PlayChannel(-1, game->audio.jump, 0);
        }
        else
        {
            game->input.jumpBufferTicksPlayer1--;
        }
    }

    if (game->input.jumpBufferTicksPlayer2 > 0)
    {
        if (game->secondPlayer.onLedge || game->secondPlayer.coyoteTicksRemaining > 0)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.onLedge = 0;
            game->secondPlayer.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer2 = 0;

            Mix_VolumeChunk(game->audio.jump, 32);
            Mix_PlayChannel(-1, game->audio.jump, 0);
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
    if (game->secondPlayer.onLedge)
    {
        game->secondPlayer.coyoteTicksRemaining = COYOTE_TICKS;
    }
    else if (game->secondPlayer.coyoteTicksRemaining > 0)
    {
        game->secondPlayer.coyoteTicksRemaining--;
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
    bool manJumpHeld = game->input.jumpHeldPlayer1;
    if (!manJumpHeld && game->man.jumpKeyHeldLastTick && game->man.dy < -JUMP_CUT_SPEED_PER_SEC)
    {
        // Variable jump height -- see apply_arcade_player_forces()'s man
        // block, docs/game-feel-map.md.
        game->man.dy = -JUMP_CUT_SPEED_PER_SEC;
    }
    game->man.jumpKeyHeldLastTick = manJumpHeld;

    if (game->input.moveLeftPlayer1)
    {
        game->man.dx -= RUN_ACCEL_PER_SEC2 * dt;
        if (game->man.dx < -RUN_MAX_SPEED_PER_SEC)
        {
            game->man.dx = -RUN_MAX_SPEED_PER_SEC;
        }
        game->man.facingLeft = 1;
        game->man.slowingDown = 0;
    }
    else if (game->input.moveRightPlayer1)
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

        if (game->input.moveLeftPlayer2)
        {
            game->secondPlayer.dx -= RUN_ACCEL_PER_SEC2 * dt;
            if (game->secondPlayer.dx < -RUN_MAX_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = -RUN_MAX_SPEED_PER_SEC;
            }
            game->secondPlayer.facingLeft = 1;
            game->secondPlayer.slowingDown = 0;
        }
        else if (game->input.moveRightPlayer2)
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
    game->time++;

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
        // Death-countdown progression itself is owned by runner_update_death()
        // (src/runner_death.c), called once per frame from runner_frame() --
        // see docs/runner-death-lifecycle.md. This used to also decrement
        // gameLives here, unboundedly (deathCountdown was never decremented),
        // independent of runner_resolve_death()'s own decrement; removed.
    }
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
}
