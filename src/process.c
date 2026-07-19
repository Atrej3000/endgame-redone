#include "header.h"
#include "entity_spawn.h"

void addBullet(GameState *game, float x, float y, float dx)
{

    if (game->shootSound == NULL) {
        game->shootSound = Mix_LoadWAV("resource/sounds/shoot.wav");
    }

    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (game->bullets[i] == NULL)
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
        game->bullets[i] = malloc(sizeof(Bullet));
        game->bullets[i]->x = x;
        game->bullets[i]->y = y;
        game->bullets[i]->dx = dx;
    }
}

void removeBullet(GameState *game, int i)
{
    if (game->bullets[i])
    {
        free(game->bullets[i]);
        game->bullets[i] = NULL;
    }
}

void addSecondBullet(GameState *game, float x, float y, float dx)
{
    if (game->shootSound == NULL) {
        game->shootSound = Mix_LoadWAV("resource/sounds/shoot.wav");
    }
    int found = -1;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (game->secondBullets[i] == NULL)
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
        game->secondBullets[i] = malloc(sizeof(Bullet));
        game->secondBullets[i]->x = x;
        game->secondBullets[i]->y = y;
        game->secondBullets[i]->dx = dx;
    }
}

void removeSecondBullet(GameState *game, int i)
{
    if (game->secondBullets[i])
    {
        free(game->secondBullets[i]);
        game->secondBullets[i] = NULL;
    }
}

// Consumes the edge-triggered jump-request flags (Phase 12, see
// docs/input-simulation-separation-map.md) at the fixed physics tick rate --
// called by arcade_simulate() (src/frame.c) before apply_arcade_player_forces,
// matching the original relative ordering (processEvents applied the keydown
// jump impulse before its own later held-key jump-hold-thrust check in the
// same real frame, so both could stack in one frame; preserved here). Each
// request is consumed (flag cleared) exactly once regardless of how many
// physics ticks a real frame produces, and regardless of whether the player
// was actually grounded -- giving one input edge exactly one jump, never
// dropped across a zero-tick frame, never double-applied across a
// multi-tick one.
void consume_arcade_jump_requests(GameState *game)
{
    if (game->input.jumpRequestedPlayer1)
    {
        if (game->man.onLedge)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
        }
        game->input.jumpRequestedPlayer1 = false;
    }

    if (game->input.jumpRequestedPlayer2)
    {
        if (game->secondPlayer.onLedge)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.onLedge = 0;

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
        }
        game->input.jumpRequestedPlayer2 = false;
    }
}

// Continuous held-key forces for Arcade's `man`/`secondPlayer` (horizontal
// accel/clamp, jump-hold thrust, friction/snap) -- called by
// arcade_simulate() (src/frame.c) before process(), at the fixed physics
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

    if (state[SDL_SCANCODE_W])
    {
        game->man.dy -= ARCADE_JUMP_HOLD_ACCEL_PER_SEC2 * dt;

        //game->man.facingLeft = 1;
        game->man.slowingDown = 0;

        if (game->time % 6 == 0)
        {
            game->man.currentSpriteJump++;
            game->man.currentSpriteJump %= 3;
        }
    }
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
        if (state[SDL_SCANCODE_UP])
        {
            game->secondPlayer.dy -= ARCADE_JUMP_HOLD_ACCEL_PER_SEC2 * dt;

            //game->man.facingLeft = 1;
            game->secondPlayer.slowingDown = 0;

            if (game->time % 6 == 0)
            {
                game->secondPlayer.currentSpriteJump2++;
                game->secondPlayer.currentSpriteJump2 %= 3;
            }
        }
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
            if (game->bullets[i])
            {
                game->bullets[i]->unvisible = 0;
                game->bullets[i]->x += game->bullets[i]->dx;
                if (game->bullets[i]->dx > 0.1)
                {
                    game->bullets[i]->dx = 0.1;
                }
                if (game->bullets[i]->dx < -0.1)
                {
                    game->bullets[i]->dx = -0.1;
                }
                if (game->bullets[i]->x > game->enemyValues[j].x && game->bullets[i]->x < game->enemyValues[j].x + 40 &&
                    game->bullets[i]->y > game->enemyValues[j].y && game->bullets[i]->y < game->enemyValues[j].y + 50)
                {
                    Mix_PlayChannel(-1, game->damageSound, 0);

                    game->enemyValues[j].y = 1000;
                    game->enemyValues[j].visible = 0;
                    game->kills_score++;
                    game->tempScore++;
                    game->bullets[i]->unvisible = 1;
                }
                if ((game->bullets[i]->x < 0) || (game->bullets[i]->x > 1280) || game->bullets[i]->unvisible)
                    removeBullet(game, i);
            }
    }

    for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i])
            {
                {
                    game->bullets[i]->unvisible = 0;
                    game->bullets[i]->x += game->bullets[i]->dx;
                    if (game->bullets[i]->dx > 0.1)
                    {
                        game->bullets[i]->dx = 0.1;
                    }
                    if (game->bullets[i]->dx < -0.1)
                    {
                        game->bullets[i]->dx = -0.1;
                    }
                    if (game->bullets[i]->x > game->smartEnemies[j].x && game->bullets[i]->x < game->smartEnemies[j].x + 40 &&
                        game->bullets[i]->y > game->smartEnemies[j].y && game->bullets[i]->y < game->smartEnemies[j].y + 50)
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
                        game->bullets[i]->unvisible = 1;
                    }
                    if ((game->bullets[i]->x < 0) || (game->bullets[i]->x > 1280) || game->bullets[i]->unvisible)
                        removeBullet(game, i);
                }
            }
    }

    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i])
            {
                {
                    game->bullets[i]->unvisible = 0;
                    game->bullets[i]->x += game->bullets[i]->dx;
                    if (game->bullets[i]->dx > 0.1)
                    {
                        game->bullets[i]->dx = 0.1;
                    }
                    if (game->bullets[i]->dx < -0.1)
                    {
                        game->bullets[i]->dx = -0.1;
                    }
                    if (game->bullets[i]->x > game->boss[j].x && game->bullets[i]->x < game->boss[j].x + 40 &&
                        game->bullets[i]->y > game->boss[j].y && game->bullets[i]->y < game->boss[j].y + 50)
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
                        game->bullets[i]->unvisible = 1;
                    }
                    if ((game->bullets[i]->x < 0) || (game->bullets[i]->x > 1280) || game->bullets[i]->unvisible)
                        removeBullet(game, i);
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
                if (game->secondBullets[i])
                {
                    game->secondBullets[i]->unvisible = 0;
                    game->secondBullets[i]->x += game->secondBullets[i]->dx;
                    if (game->secondBullets[i]->dx > 0.1)
                    {
                        game->secondBullets[i]->dx = 0.1;
                    }
                    if (game->secondBullets[i]->dx < -0.1)
                    {
                        game->secondBullets[i]->dx = -0.1;
                    }
                    if (game->secondBullets[i]->x > game->enemyValues[j].x && game->secondBullets[i]->x < game->enemyValues[j].x + 40 &&
                        game->secondBullets[i]->y > game->enemyValues[j].y && game->secondBullets[i]->y < game->enemyValues[j].y + 50)
                    {
                        Mix_PlayChannel(-1, game->damageSound, 0);
                        game->enemyValues[j].y = 1000;
                        game->enemyValues[j].visible = 0;
                        game->kills_score_multi++;
                        game->tempScore++;
                        game->secondBullets[i]->unvisible = 1;
                    }
                    if ((game->secondBullets[i]->x < 0) || (game->secondBullets[i]->x > 1280) || game->secondBullets[i]->unvisible)
                        removeSecondBullet(game, i);
                }
            }
        }

        for (int j = 0; j < NUM_SMART_ENEMIES; j++)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
                if (game->secondBullets[i])
                {
                    {
                        game->secondBullets[i]->unvisible = 0;
                        game->secondBullets[i]->x += game->secondBullets[i]->dx;
                        if (game->secondBullets[i]->dx > 0.1)
                        {
                            game->secondBullets[i]->dx = 0.1;
                        }
                        if (game->secondBullets[i]->dx < -0.1)
                        {
                            game->secondBullets[i]->dx = -0.1;
                        }
                        if (game->secondBullets[i]->x > game->smartEnemies[j].x && game->secondBullets[i]->x < game->smartEnemies[j].x + 40 &&
                            game->secondBullets[i]->y > game->smartEnemies[j].y && game->secondBullets[i]->y < game->smartEnemies[j].y + 50)
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
                            game->secondBullets[i]->unvisible = 1;
                        }
                        if ((game->secondBullets[i]->x < 0) || (game->secondBullets[i]->x > 1280) || game->secondBullets[i]->unvisible)
                            removeSecondBullet(game, i);
                    }
                }
        }
        for (int j = 0; j < 2; j++)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
                if (game->secondBullets[i])
                {
                    {
                        game->secondBullets[i]->unvisible = 0;
                        game->secondBullets[i]->x += game->secondBullets[i]->dx;
                        if (game->secondBullets[i]->dx > 0.1)
                        {
                            game->secondBullets[i]->dx = 0.1;
                        }
                        if (game->secondBullets[i]->dx < -0.1)
                        {
                            game->secondBullets[i]->dx = -0.1;
                        }
                        if (game->secondBullets[i]->x > game->boss[j].x && game->secondBullets[i]->x < game->boss[j].x + 40 &&
                            game->secondBullets[i]->y > game->boss[j].y && game->secondBullets[i]->y < game->boss[j].y + 50)
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
                            game->secondBullets[i]->unvisible = 1;
                        }
                        if ((game->secondBullets[i]->x < 0) || (game->secondBullets[i]->x > 1280) || game->secondBullets[i]->unvisible)
                            removeSecondBullet(game, i);
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

// Consumes the edge-triggered jump-request flags for Runner -- same design
// as consume_arcade_jump_requests() above (Phase 12, see
// docs/input-simulation-separation-map.md). Also fixes a regression found
// during that phase's audit: Runner's jump impulse had never been converted
// off the bare frame-tuned `-10` during Phase 11's timestep conversion,
// producing a jump 60x weaker than intended at the fixed-timestep
// integration now in use -- this function uses JUMP_SPEED_PER_SEC, matching
// Arcade and header.h's own documented intent.
void consume_runner_jump_requests(GameState *game)
{
    if (game->input.jumpRequestedPlayer1)
    {
        if (game->man.onLedge)
        {
            game->man.dy = -JUMP_SPEED_PER_SEC;
            game->man.onLedge = 0;

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
        }
        game->input.jumpRequestedPlayer1 = false;
    }

    if (game->input.jumpRequestedPlayer2)
    {
        if (game->secondPlayer.onLedge)
        {
            game->secondPlayer.dy = -JUMP_SPEED_PER_SEC;
            game->secondPlayer.onLedge = 0;

            Mix_VolumeChunk(game->jumpSound, 32);
            Mix_PlayChannel(-1, game->jumpSound, 0);
        }
        game->input.jumpRequestedPlayer2 = false;
    }
}

// Continuous held-key forces for Runner's `man`/`secondPlayer` -- called by
// runner_simulate() (src/frame.c) before process2(), at the fixed physics
// tick rate. Kept separate from process2() itself, same reasoning as
// apply_arcade_player_forces() above (Phase 11, see
// docs/physics-timestep-map.md section 4): process2() must remain callable
// directly with a manually-set dx/dy/slowingDown, unaffected by real
// keyboard state, for docs/verification/*.c's existing direct-call tests.
void apply_runner_player_forces(GameState *game, float dt)
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (state[SDL_SCANCODE_W])
    {
        game->man.dy -= RUNNER_JUMP_HOLD_ACCEL_PER_SEC2 * dt;
    }
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
        if (state[SDL_SCANCODE_UP])
        {
            game->secondPlayer.dy -= RUNNER_JUMP_HOLD_ACCEL_PER_SEC2 * dt;
        }
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
