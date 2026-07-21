#include "collision_pipeline.h"

void resolve_projectile_hits(GameState *game)
{
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
}

void resolve_arcade_hazards(GameState *game)
{
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        if (collide2d(game->man.x, game->man.y, game->enemyValues[i].x, game->enemyValues[i].y, 48, 48, 32, 32))
        {
            game->man.lives = 0;
            game->man.y = 1000;
        }
        if (game->enemyValues[i].y > 730 && game->enemyValues[i].y < 734)
        {
            game->gameLives--;
            if (!game->multiPlayer)
            {
                game->kills_score++;
                game->tempScore++;
                if (game->gameLives == 0)
                {
                    game->man.lives = 0;
                }
            }
            if (game->multiPlayer)
            {
                int rand = random() % 2;
                if (!rand)
                {
                    game->kills_score++;
                    game->tempScore++;
                }
                else
                {
                    game->kills_score_multi++;
                    game->tempScore++;
                }
                if (game->gameLives == 0)
                {
                    game->man.lives = 0;
                    game->secondPlayer.lives = 0;
                }
            }
        }
        for (int j = 0; j < 2; j++)
        {
            if (collide2d(game->man.x, game->man.y, game->boss[j].x, game->boss[j].y, 48, 48, 32, 32))
            {
                game->man.lives = 0;
                game->man.y = 1000;
            }
            if (game->multiPlayer)
            {
                if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->boss[j].x, game->boss[j].y, 30, 30, 30, 30))
                {
                    game->secondPlayer.lives = 0;
                    game->secondPlayer.y = 1000;
                }
            }
            if (game->boss[j].y > 730 && game->boss[j].y < 740)
            {
                game->gameLives = 0;
                if (game->gameLives == 0)
                {
                    game->man.lives = 0;
                }
            }
        }
        if (game->multiPlayer)
        {
            if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->enemyValues[i].x, game->enemyValues[i].y, 48, 48, 32, 32))
            {
                game->secondPlayer.lives = 0;
                game->secondPlayer.y = 1000;
            }
        }
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (collide2d(game->man.x, game->man.y, game->smartEnemies[i].x, game->smartEnemies[i].y, 30, 30, 30, 30))
        {
            game->man.lives = 0;
            game->man.y = 1000;
        }
        if (game->multiPlayer)
        {
            if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->smartEnemies[i].x, game->smartEnemies[i].y, 30, 30, 30, 30))
            {
                game->secondPlayer.lives = 0;
                game->secondPlayer.y = 1000;
            }
        }
    }
    //_________________________________________________________lives by falls

    if (game->man.y >= 719)
    {
        game->man.lives = 0;
    }

    if (game->secondPlayer.y >= 719)
    {
        game->secondPlayer.lives = 0;
    }
}

void resolve_arcade_game_over_transition(GameState *game)
{
    if (!game->multiPlayer)
    {
        if (game->man.lives == 0)
        {
            // Guard against overwriting a transition an earlier handler in
            // this same call (e.g. SDLK_q) already made -- see
            // docs/frame-pipeline-map.md's double-transition finding.
            if (game->app.scene == APP_SCENE_ARCADE_GAME)
            {
                app_change_scene(game, APP_SCENE_ARCADE_MENU);
            }
        }
    }
    if (game->multiPlayer)
    {
        if (game->man.lives == 0 && game->secondPlayer.lives == 0)
        {
            if (game->app.scene == APP_SCENE_ARCADE_GAME)
            {
                app_change_scene(game, APP_SCENE_ARCADE_MENU);
            }
        }
    }
}

void resolve_runner_hazard_contact(GameState *game)
{
    for (int i  = 0; i < 100; i++)
    {
        if (collide2d(game->man.x, game->man.y, game->stars[i].x, game->stars[i].y, 30, 30, 30, 30))
        {
            runner_trigger_death(game);
        }
        if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->stars[i].x, game->stars[i].y, 30, 30, 30, 30))
        {
            runner_trigger_death(game);
        }
    }
}

void check_runner_fall_hazard(GameState *game)
{
    if (game->man.y >= 719 || game->man.x < 0)
    {
        runner_trigger_death(game);
    }

    if (game->multiPlayer)
    {
        if (game->secondPlayer.y >= 719 || game->secondPlayer.x < 0)
        {
            runner_trigger_death(game);
        }
    }
}

void resolve_runner_game_over_transition(GameState *game)
{
    //_________________________________________generation array with score history
    if (game->gameLives == 0)
    {
        if (game->x_i < 25)
        {
            game->x_list[game->x_i] = game->x_score;
            game->x_i++;
        }

        // Score-persist above stays unconditional; only the transition
        // itself is guarded against overwriting one an earlier handler in
        // this same call (e.g. SDLK_q) already made -- see
        // docs/frame-pipeline-map.md's double-transition finding.
        if (game->app.scene == APP_SCENE_RUNNER_GAME)
        {
            app_change_scene(game, APP_SCENE_RUNNER_MENU);
        }
    }
}
