#include "header.h"
#include "entity_spawn.h"

// Fixed pool (Phase 14, see docs/projectile-correctness-map.md): no
// malloc/free -- a shot claims the first inactive slot in place.
void addBullet(GameState *game, float x, float y, float dx)
{

    if (game->shootSound == NULL) {
        game->shootSound = Mix_LoadWAV("resource/sounds/shoot.wav");
    }

    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!game->bullets[i].active)
        {
            found = i;
            Mix_VolumeChunk(game->shootSound, 32);
            Mix_PlayChannel(-1, game->shootSound, 0);
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
        game->bullets[i].active = true;
    }
}

void removeBullet(GameState *game, int i)
{
    game->bullets[i].active = false;
}

void addSecondBullet(GameState *game, float x, float y, float dx)
{
    if (game->shootSound == NULL) {
        game->shootSound = Mix_LoadWAV("resource/sounds/shoot.wav");
    }
    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!game->secondBullets[i].active)
        {
            found = i;
            Mix_VolumeChunk(game->shootSound, 32);
            Mix_PlayChannel(-1, game->shootSound, 0);
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
// Normalizes dx to +-BULLET_SPEED_PER_TICK by sign, not a max-clamp: the
// legacy per-application clamp (0.1) was smaller than the spawn dx (+-3),
// so it always shrank toward the bound regardless of direction; the new
// bound (11.3, chosen to preserve the legacy steady-state speed -- see the
// constant's own comment in inc/header.h) is *larger* than the spawn dx, so
// a max-clamp would never fire and the bullet would silently move at the
// spawn speed (3/tick) instead of the intended, speed-preserving one.
// Normalizing by sign reproduces the old code's actual effect (spawn
// direction, fixed magnitude every tick) regardless of which bound is
// bigger.
void move_arcade_bullets(GameState *game)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (game->bullets[i].active)
        {
            game->bullets[i].prevX = game->bullets[i].x;
            game->bullets[i].dx = (game->bullets[i].dx >= 0) ? BULLET_SPEED_PER_TICK : -BULLET_SPEED_PER_TICK;
            game->bullets[i].x += game->bullets[i].dx;
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
                game->secondBullets[i].dx = (game->secondBullets[i].dx >= 0) ? BULLET_SPEED_PER_TICK : -BULLET_SPEED_PER_TICK;
                game->secondBullets[i].x += game->secondBullets[i].dx;
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
void consume_arcade_jump_requests(GameState *game)
{
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

    if (game->input.jumpBufferTicksPlayer1 > 0)
    {
        if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;
            game->man.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer1 = 0;

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
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

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
        }
        else
        {
            game->input.jumpBufferTicksPlayer2--;
        }
    }
}

// Continuous held-key forces for Arcade's `man`/`secondPlayer` (horizontal
// accel/clamp, friction/snap) plus variable-jump-height release-cut -- called
// by arcade_simulate() (src/frame.c) before process(), at the fixed physics
// tick rate. Kept as its own function, separate from process() itself
// (Phase 11, see docs/physics-timestep-map.md section 4): process() must
// remain callable directly with a manually-set dx/dy/slowingDown, unaffected
// by real keyboard state, for docs/verification/*.c's existing direct-call
// tests to keep working -- if this logic lived inside process() itself,
// every call would silently re-read (and in a headless test, always-absent)
// keyboard state and overwrite whatever the test had just set up.
void apply_arcade_player_forces(GameState *game, float dt)
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    bool manJumpHeld = state[SDL_SCANCODE_W];
    if (manJumpHeld)
    {
        //game->man.facingLeft = 1;
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

    if (state[SDL_SCANCODE_A])
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
    else if (state[SDL_SCANCODE_D])
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
        //game->enemy.currentSpriteRun = 1;
        if (fabsf(game->man.dx) < RUN_SNAP_ZERO_SPEED_PER_SEC)
        {
            game->man.dx = 0;
        }
    }

    if (game->multiPlayer)
    {
        bool secondPlayerJumpHeld = state[SDL_SCANCODE_UP];
        if (secondPlayerJumpHeld)
        {
            //game->man.facingLeft = 1;
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

        if (state[SDL_SCANCODE_LEFT])
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
        else if (state[SDL_SCANCODE_RIGHT])
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
            //game->enemy.currentSpriteRun = 1;
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

    if (game->damageSound == NULL) {
        game->damageSound = Mix_LoadWAV("resource/sounds/damage.wav");
    }
    if (game->kickSound == NULL) {
        game->kickSound = Mix_LoadWAV("resource/sounds/kick.wav");
    }

    for (int j = 0; j < NUM_ENEMIES; j++)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i].active)
            {
                // Swept collision (Phase 14): the bullet's path this tick
                // (prevX -> x), not just its end-of-tick position, is
                // tested against the target -- movement itself now happens
                // once per tick in move_arcade_bullets(), not here. See
                // docs/projectile-correctness-map.md section 6.
                float bx0 = fminf(game->bullets[i].prevX, game->bullets[i].x);
                float bx1 = fmaxf(game->bullets[i].prevX, game->bullets[i].x);
                if (bx1 > game->enemyValues[j].x && bx0 < game->enemyValues[j].x + 40 &&
                    game->bullets[i].y > game->enemyValues[j].y && game->bullets[i].y < game->enemyValues[j].y + 50)
                {
                    Mix_PlayChannel(-1, game->damageSound, 0);

                    game->enemyValues[j].y = 1000;
                    game->enemyValues[j].visible = 0;
                    game->kills_score++;
                    game->tempScore++;
                    removeBullet(game, i);
                }
            }
    }

    for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i].active)
            {
                {
                    // Swept collision (Phase 14) -- see the enemy loop above.
                    float bx0 = fminf(game->bullets[i].prevX, game->bullets[i].x);
                    float bx1 = fmaxf(game->bullets[i].prevX, game->bullets[i].x);
                    if (bx1 > game->smartEnemies[j].x && bx0 < game->smartEnemies[j].x + 40 &&
                        game->bullets[i].y > game->smartEnemies[j].y && game->bullets[i].y < game->smartEnemies[j].y + 50)
                    {
                        game->smartEnemies[j].countShots++;
                        if (game->smartEnemies[j].countShots > 5)
                        {
                            Mix_PlayChannel(-1, game->kickSound, 0);
                        }
                        if (game->smartEnemies[j].countShots > 5)
                        {
                            Mix_PlayChannel(-1, game->damageSound, 0);
                            game->smartEnemies[j].y = 1000;
                            game->smartEnemies[j].visible = 0;
                            game->tempScore += 5;
                            game->kills_score += 5;
                        }
                        removeBullet(game, i);
                    }
                }
            }
    }

    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i].active)
            {
                {
                    // Swept collision (Phase 14) -- see the enemy loop above.
                    float bx0 = fminf(game->bullets[i].prevX, game->bullets[i].x);
                    float bx1 = fmaxf(game->bullets[i].prevX, game->bullets[i].x);
                    if (bx1 > game->boss[j].x && bx0 < game->boss[j].x + 40 &&
                        game->bullets[i].y > game->boss[j].y && game->bullets[i].y < game->boss[j].y + 50)
                    {
                        game->boss[j].countShots++;
                        if (game->boss[j].countShots > 30)
                        {
                            Mix_PlayChannel(-1, game->damageSound, 0);
                            game->boss[j].y = 1000;
                            game->boss[j].visible = 0;
                            game->tempScore += 10;
                            game->kills_score += 10;
                        }
                        removeBullet(game, i);
                    }
                }
            }
    }

    // SECOND BULLET___________________________________________________________________________________________

    if (game->multiPlayer)
    {
        for (int j = 0; j < NUM_ENEMIES; j++)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (game->secondBullets[i].active)
                {
                    // Swept collision (Phase 14) -- see the bullets[] loops above.
                    float bx0 = fminf(game->secondBullets[i].prevX, game->secondBullets[i].x);
                    float bx1 = fmaxf(game->secondBullets[i].prevX, game->secondBullets[i].x);
                    if (bx1 > game->enemyValues[j].x && bx0 < game->enemyValues[j].x + 40 &&
                        game->secondBullets[i].y > game->enemyValues[j].y && game->secondBullets[i].y < game->enemyValues[j].y + 50)
                    {
                        Mix_PlayChannel(-1, game->damageSound, 0);
                        game->enemyValues[j].y = 1000;
                        game->enemyValues[j].visible = 0;
                        game->kills_score_multi++;
                        game->tempScore++;
                        removeSecondBullet(game, i);
                    }
                }
            }
        }

        for (int j = 0; j < NUM_SMART_ENEMIES; j++)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
                if (game->secondBullets[i].active)
                {
                    {
                        // Swept collision (Phase 14) -- see the bullets[] loops above.
                        float bx0 = fminf(game->secondBullets[i].prevX, game->secondBullets[i].x);
                        float bx1 = fmaxf(game->secondBullets[i].prevX, game->secondBullets[i].x);
                        if (bx1 > game->smartEnemies[j].x && bx0 < game->smartEnemies[j].x + 40 &&
                            game->secondBullets[i].y > game->smartEnemies[j].y && game->secondBullets[i].y < game->smartEnemies[j].y + 50)
                        {
                            game->smartEnemies[j].countShots++;
                            if (game->smartEnemies[j].countShots > 5)
                            {
                                Mix_PlayChannel(-1, game->damageSound, 0);
                                game->smartEnemies[j].y = 1000;
                                game->smartEnemies[j].visible = 0;
                                game->kills_score_multi += 5;
                                game->tempScore += 5;
                            }
                            removeSecondBullet(game, i);
                        }
                    }
                }
        }
        for (int j = 0; j < 2; j++)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
                if (game->secondBullets[i].active)
                {
                    {
                        // Swept collision (Phase 14) -- see the bullets[] loops above.
                        float bx0 = fminf(game->secondBullets[i].prevX, game->secondBullets[i].x);
                        float bx1 = fmaxf(game->secondBullets[i].prevX, game->secondBullets[i].x);
                        if (bx1 > game->boss[j].x && bx0 < game->boss[j].x + 40 &&
                            game->secondBullets[i].y > game->boss[j].y && game->secondBullets[i].y < game->boss[j].y + 50)
                        {
                            game->boss[j].countShots++;
                            if (game->boss[j].countShots > 30)
                            {
                                Mix_PlayChannel(-1, game->damageSound, 0);
                                game->boss[j].y = 1000;
                                game->boss[j].visible = 0;
                                game->kills_score_multi += 10;
                                game->tempScore += 10;
                            }
                            removeSecondBullet(game, i);
                        }
                    }
                }
        }
    }

    // SECOND BULLET END ________________________________________________________________________________________________________________________

    if (game->time == 120)
    {
        //1st example of assimetric with random
        for (int i = 0; i < 10; i += 2)
        {
            enemy_spawn(game, i, 620, (i) * -200, 0, 0);         // - random() % 100;
            enemy_spawn(game, i + 1, 580, (i) * -200, 0, 0);     //- random() % 50;
        }
    }

    if (game->tempScore == 10)
    {
        //2nd example of assimetric with random
        for (int j = 0; j < 20; j += 2)
        {
            enemy_spawn(game, j, 620, (j) * -200, 0, 0);         //- random() % 80;
            enemy_spawn(game, j + 1, 580, (j) * -200, 0, 0);     //- random() % 40;
        }
        game->tempScore++;
    }

    if (game->tempScore == 31)
    {
        //3rd example of assimetric with random
        for (int j = 0; j < 30; j += 2)
        {
            enemy_spawn(game, j, 620, (j) * -200, 0, 0);         // - random() % 100; //- random() % 40;
            enemy_spawn(game, j + 1, 580, (j) * -200, 0, 0);     // - random() % 50; //- random() % 80;
        }

        smart_enemy_spawn(game, 0, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 1, 200, 200, 0, 0);

        game->tempScore++;
    }
    if (game->tempScore == 72)
    {
        for (int j = 0; j < 50; j += 2)
        {
            enemy_spawn(game, j, 620, (j) * -100, 0, 0);
            enemy_spawn(game, j + 1, 580, (j) * -100, 0, 0);
        }
        smart_enemy_spawn(game, 2, 1000, 200, 0, 0);
        smart_enemy_spawn(game, 3, 200, 200, 0, 0);

        game->tempScore++;
    }

    if (game->tempScore == 133)
    {
        for (int j = 0; j < 70; j += 2)
        {
            enemy_spawn(game, j, 620, (j) * -100, 0, 0);
            enemy_spawn(game, j + 1, 580, (j) * -100, 0, 0);
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
            enemy_spawn(game, j, 620, (j) * -100, 0, 0);
            enemy_spawn(game, j + 1, 580, (j) * -100, 0, 0);
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
            enemy_spawn(game, i, 620, i * (-100), 0, 0);
            enemy_spawn(game, i + 1, 580, i * (-100), 0, 0);
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
    // BOSS _________________________________________________________________________________________________________
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].y += game->boss[i].dy;
        game->boss[i].x += game->boss[i].dx;

        game->boss[i].dy += 0.1;
        if (game->boss[i].dy > 1.5)
        {
            game->boss[i].dy = 1.5;
        }
        if (game->boss[i].y > 300 && game->boss[i].y < 350 && game->boss[i].x < 640)
        {
            game->boss[i].dx += 0.1;
            if (game->boss[i].dx > 1)
            {
                game->boss[i].dx = 1;
            }
        }
        if (game->boss[i].y > 300 && game->boss[i].y < 350 && game->boss[i].x > 640)
        {
            game->boss[i].dx -= 0.1;
            if (game->boss[i].dx < -1)
            {
                game->boss[i].dx = -1;
            }
        }
        if (game->boss[i].x > 1280)
        {
            game->boss[i].x = 0;
        }
        if (game->boss[i].x < 0)
        {
            game->boss[i].x = 1280;
        }
    }
    // BOSS ----------------------------------------------------------------------------------------------------------------
    // STUPID BOTS
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        Enemies *enemy = &game->enemyValues[i];
        enemy->y += enemy->dy;
        enemy->x += enemy->dx;

        if (enemy->x > 610 && enemy->y > 150 && enemy->y < 170)
        {
            enemy->dx += 0.2;
            if (enemy->dx >= 2)
            {
                enemy->dx = 2;
            }
        }

        if (enemy->x > 1280)
        {
            enemy->x = 0;
        }

        if (enemy->x < 590 && enemy->y > 150 && enemy->y < 170)
        {
            enemy->dx -= 0.2;
            if (enemy->dx <= -2)
            {
                enemy->dx = -2;
            }
        }
        if (enemy->x < -1)
        {
            enemy->x = 1280;
        }
        enemy->dy += 0.4;
        if (enemy->dy >= 2.5)
        {
            enemy->dy = 2.5;
        }
    }

    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (game->smartEnemies[i].y > 720 && game->smartEnemies[i].y < 750)
        {
            game->smartEnemies[i].y = 200;
        }
        if (!game->multiPlayer)
        {
            game->smartEnemies[i].y += game->smartEnemies[i].dy;
            if (game->smartEnemies[i].y > game->man.y)
            {
                while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->man.y)
                {
                    if (game->smartEnemies[i].x > game->man.x)
                    {
                        game->smartEnemies[i].dx += 1;
                        game->smartEnemies[i].x += game->smartEnemies[i].dx;
                        if (game->smartEnemies[i].dx >= 4)
                        {
                            game->smartEnemies[i].dx = 4;
                        }
                    }
                    if (game->smartEnemies[i].x < game->man.x)
                    {
                        game->smartEnemies[i].dx -= 1;
                        game->smartEnemies[i].x += game->smartEnemies[i].dx;
                        if (game->smartEnemies[i].dx <= -4)
                        {
                            game->smartEnemies[i].dx = -4;
                        }
                    }
                    game->smartEnemies[i].dy -= 10;
                }
            }

            // if (game->smartEnemies[i].y < game->man.y && game->smartEnemies[i].x != 0)
            // {
            //     game->stopTime++;
            //     if (game->stopTime == 200)
            //     {
            //            game->smartEnemies[i].dx += 0.1;
            //            game->smartEnemies[i].x += game->smartEnemies[i].dx;
            //            if (game->smartEnemies[i].dx >= 5)
            //            {
            //                game->smartEnemies[i].dx = 5;
            //            }
            //            game->stopTime--;

            //     }
            // }

            if (game->smartEnemies[i].x + 6 < game->man.x)
            {
                if (game->smartEnemies[i].dx == 0)
                    game->smartEnemies[i].dy = -6;
                game->smartEnemies[i].x += game->smartEnemies[i].dx;
                game->smartEnemies[i].dx += 0.2;
                if (game->smartEnemies[i].dx >= 4)
                {
                    game->smartEnemies[i].dx = 4;
                }

                // game->man.facingLeft = 1;
                //game->enemy.slowingDown = 0;
            }
            else if (game->smartEnemies[i].x - 6 > game->man.x)
            {
                if (game->smartEnemies[i].dx == 0)
                    game->smartEnemies[i].dy = -6;

                game->smartEnemies[i].x += game->smartEnemies[i].dx;
                game->smartEnemies[i].dx -= 0.2;
                if (game->smartEnemies[i].dx <= -4)
                {
                    game->smartEnemies[i].dx = -4;
                }
                // game->man.facingLeft = 1;
                //  game->enemy.slowingDown = 0;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
            }
            game->smartEnemies[i].dy += 1;
        }

        // BOTS FOR MULTIPLAYER
        if (game->multiPlayer)
        {
            if (game->smartEnemies[i].y > 720 && game->smartEnemies[i].y < 750)
            {
                game->smartEnemies[i].y = 200;
            }
            if ((pow((game->smartEnemies[i].x - game->man.x), 2) + pow((game->smartEnemies[i].y - game->man.y), 2)) <
                (pow((game->smartEnemies[i].x - game->secondPlayer.x), 2) + pow(game->smartEnemies[i].y - game->secondPlayer.y, 2)))
            {
                game->smartEnemies[i].y += game->smartEnemies[i].dy;
                if (game->smartEnemies[i].y > game->man.y)
                {
                    while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->man.y)
                    {
                        if (game->smartEnemies[i].x > game->man.x)
                        {
                            game->smartEnemies[i].dx += 1;
                            if (game->smartEnemies[i].dx >= 4)
                            {
                                game->smartEnemies[i].dx = 4;
                            }
                        }
                        if (game->smartEnemies[i].x < game->man.x)
                        {
                            game->smartEnemies[i].dx -= 1;
                            if (game->smartEnemies[i].dx <= -4)
                            {
                                game->smartEnemies[i].dx = -4;
                            }
                        }
                        game->smartEnemies[i].dy -= 10;
                    }
                }
                if (game->smartEnemies[i].x + 6 < game->man.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;
                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx += 0.2;
                    if (game->smartEnemies[i].dx >= 4)
                    {
                        game->smartEnemies[i].dx = 4;
                    }

                    // game->man.facingLeft = 1;
                    //game->enemy.slowingDown = 0;
                }
                else if (game->smartEnemies[i].x - 6 > game->man.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;

                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx -= 0.2;
                    if (game->smartEnemies[i].dx <= -4)
                    {
                        game->smartEnemies[i].dx = -4;
                    }
                    // game->man.facingLeft = 1;
                    //  game->enemy.slowingDown = 0;
                }
                game->smartEnemies[i].dy += 1;
            }
            else
            {
                game->smartEnemies[i].y += game->smartEnemies[i].dy;
                if (game->smartEnemies[i].y > game->secondPlayer.y)
                {
                    while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->secondPlayer.y)
                    {
                        if (game->smartEnemies[i].x > game->secondPlayer.x)
                        {
                            game->smartEnemies[i].dx += 1;
                            if (game->smartEnemies[i].dx >= 4)
                            {
                                game->smartEnemies[i].dx = 4;
                            }
                        }
                        if (game->smartEnemies[i].x < game->secondPlayer.x)
                        {
                            game->smartEnemies[i].dx -= 1;
                            if (game->smartEnemies[i].dx <= -4)
                            {
                                game->smartEnemies[i].dx = -4;
                            }
                        }
                        game->smartEnemies[i].dy -= 10;
                    }
                }
                if (game->smartEnemies[i].x + 6 < game->secondPlayer.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;
                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx += 0.2;
                    if (game->smartEnemies[i].dx >= 4)
                    {
                        game->smartEnemies[i].dx = 4;
                    }

                    // game->man.facingLeft = 1;
                    //game->enemy.slowingDown = 0;
                }
                else if (game->smartEnemies[i].x - 6 > game->secondPlayer.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;

                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx -= 0.2;
                    if (game->smartEnemies[i].dx <= -4)
                    {
                        game->smartEnemies[i].dx = -4;
                    }
                    // game->man.facingLeft = 1;
                    //  game->enemy.slowingDown = 0;
                }
                game->smartEnemies[i].dy += 1;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
            }
        }
    }
    // END BOTS

    if (game->man.x > 1280)
    {
        game->man.x = -30;
    }
    if (game->man.x < -30)
    {
        game->man.x = 1270;
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
            // if (man->dx == 0 && man->onLedge)
            // {
            //     man->animFrame = 0;
            // }

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
                // init_game_over(game);
                // game->statusState = STATUS_STATE_GAMEOVER;
                init_status_lives(game);
            }
        }
    }
    // game->scrollX = -game->man.x + 320;
    // if (game->scrollX > 0)
    //     game->scrollX = 0;

    if (game->multiPlayer)
    {
        if (game->secondPlayer.x > 1280)
        {
            game->secondPlayer.x = -30;
        }
        if (game->secondPlayer.x < -30)
        {
            game->secondPlayer.x = 1270;
        }
        if (game->statusState == STATUS_STATE_GAME)
        {
            if (!game->secondPlayer.isDead)
            {
                // Man MOVEMENT
                Man *secondPlayer = &game->secondPlayer;
                secondPlayer->x += secondPlayer->dx * dt;
                secondPlayer->y += secondPlayer->dy * dt;

                // if (secondPlayer->dx != 0 && secondPlayer->onLedge && !secondPlayer->slowingDown)
                // {
                //     if (game->time % 3 == 0)
                //     {
                //         if (man->animFrame < 11)
                //         {
                //             man->animFrame++;
                //         }
                //         else
                //         {
                //             man->animFrame = 0;
                //         }
                //     }
                // }
                // if (man->dx == 0 && man->onLedge)
                // {
                //     man->animFrame = 0;
                // }

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
                    // init_game_over(game);
                    // game->statusState = STATUS_STATE_GAMEOVER;
                    init_status_lives(game);
                }
            }
        }
    }
    // game->scrollX = -game->man.x + 320;
    // if (game->scrollX > 0)
    //     game->scrollX = 0;
}

// Consumes the edge-triggered, buffered jump-request countdowns for Runner
// -- same design as consume_arcade_jump_requests() above (Phase 12, extended
// with coyote time + jump buffering in Phase 15, see docs/game-feel-map.md).
// Also fixes a regression found during Phase 12's audit: Runner's jump
// impulse had never been converted off the bare frame-tuned `-10` during
// Phase 11's timestep conversion, producing a jump 60x weaker than intended
// at the fixed-timestep integration now in use -- this function uses
// JUMP_SPEED_PER_SEC, matching Arcade and header.h's own documented intent.
void consume_runner_jump_requests(GameState *game)
{
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

    if (game->input.jumpBufferTicksPlayer1 > 0)
    {
        if (game->man.onLedge || game->man.coyoteTicksRemaining > 0)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;
            game->man.coyoteTicksRemaining = 0;
            game->input.jumpBufferTicksPlayer1 = 0;

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
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

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
        }
        else
        {
            game->input.jumpBufferTicksPlayer2--;
        }
    }
}

// Continuous held-key forces for Runner's `man`/`secondPlayer` (horizontal
// accel/clamp, friction/snap) plus variable-jump-height release-cut -- called
// by runner_simulate() (src/frame.c) before process2(), at the fixed physics
// tick rate. Kept separate from process2() itself, same reasoning as
// apply_arcade_player_forces() above (Phase 11, see
// docs/physics-timestep-map.md section 4): process2() must remain callable
// directly with a manually-set dx/dy/slowingDown, unaffected by real
// keyboard state, for docs/verification/*.c's existing direct-call tests.
void apply_runner_player_forces(GameState *game, float dt)
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    bool manJumpHeld = state[SDL_SCANCODE_W];
    if (!manJumpHeld && game->man.jumpKeyHeldLastTick && game->man.dy < -JUMP_CUT_SPEED_PER_SEC)
    {
        // Variable jump height -- see apply_arcade_player_forces()'s man
        // block, docs/game-feel-map.md.
        game->man.dy = -JUMP_CUT_SPEED_PER_SEC;
    }
    game->man.jumpKeyHeldLastTick = manJumpHeld;

    if (state[SDL_SCANCODE_A])
    {
        game->man.dx -= RUN_ACCEL_PER_SEC2 * dt;
        if (game->man.dx < -RUN_MAX_SPEED_PER_SEC)
        {
            game->man.dx = -RUN_MAX_SPEED_PER_SEC;
        }
        game->man.facingLeft = 1;
        game->man.slowingDown = 0;
    }
    else if (state[SDL_SCANCODE_D])
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
        bool secondPlayerJumpHeld = state[SDL_SCANCODE_UP];
        if (!secondPlayerJumpHeld && game->secondPlayer.jumpKeyHeldLastTick && game->secondPlayer.dy < -JUMP_CUT_SPEED_PER_SEC)
        {
            game->secondPlayer.dy = -JUMP_CUT_SPEED_PER_SEC;
        }
        game->secondPlayer.jumpKeyHeldLastTick = secondPlayerJumpHeld;

        if (state[SDL_SCANCODE_LEFT])
        {
            game->secondPlayer.dx -= RUN_ACCEL_PER_SEC2 * dt;
            if (game->secondPlayer.dx < -RUN_MAX_SPEED_PER_SEC)
            {
                game->secondPlayer.dx = -RUN_MAX_SPEED_PER_SEC;
            }
            game->secondPlayer.facingLeft = 1;
            game->secondPlayer.slowingDown = 0;
        }
        else if (state[SDL_SCANCODE_RIGHT])
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
        // shutdown_status_lives(game);
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

        // SECOND PLAYER _________________________________________________________________________________________________________________________
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
       // int i = 0;
        //random()%5;
        for(int i = 0; i < NUM_STARS; i++)
      {
        game->stars[i].x = game->stars[i].baseX;
        game->stars[i].y = game->stars[i].baseY;
        
        if(game->stars[i].mode == 0)
        {
          game->stars[i].x = game->stars[i].baseX+sinf(game->stars[i].phase+game->time*0.06f)*75;
        }
        else
        {
          game->stars[i].y = game->stars[i].baseY+cosf(game->stars[i].phase+game->time*0.06f)*75;
        }
      }
        // for (i = 0; i < 100; i++)
        // {

        //     //if(time == 2)
        //     game->stars[i].x += game->stars[i].dx;
        //     game->stars[i].y += game->stars[i].dy;

        //     if (game->stars[i].y > 720)
        //     {
        //             game->stars[i].dy -= 0.1;
        //              game->stars[i].y += game->stars[i].dy;
        //             if (game->stars[i].dy <= -3.5)
        //             {
        //                 game->stars[i].dy = -3.5;
        //             }
        //     }
        //     if (game->stars[i].y < 720 && game->stars[i].y > 0)
        //     {
        //     int rand = random() % 2;
        //     if (rand)
        //     {

        //         if (game->stars[i].tempStar == 0)
        //         {
        //             game->stars[i].dy += 0.1;
        //             if (game->stars[i].dy > 5)
        //             {
        //                 game->stars[i].dy = 0;
        //                 game->stars[i].tempStar = 1;
        //             }
        //         }
        //         if (game->stars[i].tempStar == 1)
        //         {
        //             game->stars[i].dy -= 0.1;
        //             if (game->stars[i].dy < -4.1)
        //             {
        //                 game->stars[i].dy = 0;
        //                 game->stars[i].tempStar = 0;
        //             }
        //         }
        //     }
        //     if (!rand)
        //     {
        //         if (game->stars[i].tempStar == 0)
        //         {
        //             game->stars[i].dx += 0.1;
        //             if (game->stars[i].dx > 5)
        //             {
        //                 game->stars[i].dx = 0;
        //                 game->stars[i].tempStar = 1;
        //             }
        //         }
        //         if (game->stars[i].tempStar == 1)
        //         {
        //             game->stars[i].dx -= 0.1;
        //             if (game->stars[i].dx < -4.1)
        //             {
        //                 game->stars[i].dx = 0;
        //                 game->stars[i].tempStar = 0;
        //             }
        //         }
        //     }
        //     }
        //     // if (game->stars[i].y > 720)
        //     // {
        //     //     game->stars[i].y = 400;
        //     // }
        // }

        //infinity field

        //_____________________________________________________________________________
        // Death-countdown progression itself is owned by runner_update_death()
        // (src/runner_death.c), called once per frame from runner_frame() --
        // see docs/runner-death-lifecycle.md. This used to also decrement
        // gameLives here, unboundedly (deathCountdown was never decremented),
        // independent of runner_resolve_death()'s own decrement; removed.
    }
    if (game->multiPlayer)
    {
        if (game->time >= 200)
            game->scrollX -= 3;
    }
    if (!game->multiPlayer)
    {
        game->scrollX = -game->man.x + 500;

        if (game->scrollX > 0)
            game->scrollX = 0;
    }
}
