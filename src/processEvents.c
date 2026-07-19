#include "header.h"
#include "scene.h"
#include "input_command.h"

int processEvents(SDL_Window *window, GameState *game)
{
    SDL_Event event;
    int done = 0;

    if (game->jumpSound == NULL) {
        game->jumpSound = Mix_LoadWAV("resource/sounds/jump.wav");
    }
    if (game->select == NULL) {
        game->select = Mix_LoadWAV("resource/sounds/select.wav");
    }

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_WINDOWEVENT_CLOSE:
        {
            if (window)
            {
                SDL_DestroyWindow(window);
                window = NULL;
                done = 0;
            }
        }
        break;
        case SDL_KEYDOWN:
        {
            switch (translate_arcade_command(event.key.keysym.sym))
            {
            //return to menu
            case GAME_COMMAND_QUIT_TO_MODE_MENU:
                app_change_scene(game, APP_SCENE_ARCADE_MENU);
//                game->kills_score = 0;
//                game->kills_score_multi = 0;

                Mix_VolumeChunk(game->select, 32);
                Mix_PlayChannel(-1, game->select, 0);
                Mix_FreeChunk(game->shootSound);
                game->shootSound = NULL;
                Mix_FreeChunk(game->damageSound);
                game->damageSound = NULL;
                Mix_FreeChunk(game->kickSound);
                game->kickSound = NULL;
                Mix_FreeMusic(game->battleMus);
                game->battleMus = NULL;
                return done;
                break;
                //pause
            case GAME_COMMAND_PAUSE:
                app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
                done = 0;

                Mix_VolumeChunk(game->select, 32);
                Mix_PlayChannel(-1, game->select, 0);
                Mix_PauseMusic();
                break;

            case GAME_COMMAND_JUMP_PLAYER1:
                if (game->man.onLedge)
                {
                    game->man.dy = -10;
                    game->man.onLedge = 0;

                    Mix_VolumeChunk(game->jumpSound, 32);
                    Mix_PlayChannel(-1, game->jumpSound, 0);
                }
                break;
            case GAME_COMMAND_JUMP_PLAYER2:
                if (game->secondPlayer.onLedge)
                {
                    game->secondPlayer.dy = -10;
                    game->secondPlayer.onLedge = 0;

                    Mix_VolumeChunk(game->jumpSound, 32);
                    Mix_PlayChannel(-1, game->jumpSound, 0);
                }
                break;
            // case SDLK_SPACE:
            //     if (!game->man.facingLeft)
            //     {
            //         addBullet(game, game->man.x + 40, game->man.y + 15, 3);
            //     }
            //     else
            //     {
            //         addBullet(game, game->man.x, game->man.y + 15, -3);
            //     }
            //     goto etc;
            //     break;
            case GAME_COMMAND_QUIT_TO_MAIN_MENU:
                app_change_scene(game, APP_SCENE_MAIN_MENU);

                Mix_FreeChunk(game->select);
                game->select = NULL;
                Mix_FreeChunk(game->jumpSound);
                game->jumpSound = NULL;
                Mix_FreeChunk(game->shootSound);
                game->shootSound = NULL;
                Mix_FreeChunk(game->damageSound);
                game->damageSound = NULL;
                Mix_FreeChunk(game->kickSound);
                game->kickSound = NULL;
                Mix_FreeMusic(game->battleMus);
                game->battleMus = NULL;
                break;
            default:
                break;
            }
        }
        break;
        case SDL_QUIT:
            //quit out of the game
            done = 1;
            break;
        }
    }
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    //BULLETS
    if (game->time % 1 == 0)
    {
        game->CurrentSheetBullet++;
        game->CurrentSheetBullet %= 60;
    }

    if (game->time % 2 == 0)
    {
        game->CurrentSheetBullet2++;
        game->CurrentSheetBullet2 %= 30;
    }

    //TRAIN
    if (game->time % 1 == 0)
    {
        game->train.x += 4;

        if (game->train.x > 1280)
            game->train.x = -480;
    }

    //CLOUDS
    if (game->time % 2 == 0)
    {
        game->cloud1.x -= 2;

        if (game->cloud1.x < -312)
            game->cloud1.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud2.x -= 3;

        if (game->cloud2.x < -416)
            game->cloud2.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud3.x -= 2;

        if (game->cloud3.x < -280)
            game->cloud3.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud4.x -= 3;

        if (game->cloud4.x < -292)
            game->cloud4.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud5.x -= 2;

        if (game->cloud5.x < -216)
            game->cloud5.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud6.x -= 3;

        if (game->cloud6.x < -152)
            game->cloud6.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud7.x -= 2;

        if (game->cloud7.x < -264)
            game->cloud7.x = 1280;
    }

    if (game->time % 2 == 0)
    {
        game->cloud8.x -= 3;

        if (game->cloud8.x < -352)
            game->cloud8.x = 1280;
    }

    if (game->time % 1 == 0)
    {
        game->train.x += 2;

        if (game->train.x > 1280)
            game->train.x = -480;
    }

    //Background animation
    if (game->time % 4 == 0)
    {
        game->CurrentSpriteBack++;
        game->CurrentSpriteBack %= 2;

        game->enemy.currentSpriteRun++;
        game->enemy.currentSpriteRun %= 4;

        game->enemy.currentSpriteRun2++;
        game->enemy.currentSpriteRun2 %= 6;
    }

    //Boss
    if (game->time % 5 == 0)
    {
        game->CurrentSheetBoss++;
        game->CurrentSheetBoss %= 4;
    }

    if (state[SDL_SCANCODE_W])
    {
        game->man.dy -= 0.2f;

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
        game->man.dx -= 0.5;
        if (game->man.dx < -6)
        {
            game->man.dx = -6;
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
        game->man.dx += 0.5;
        if (game->man.dx > 6)
        {
            game->man.dx = 6;
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
        game->man.dx *= 0.8f;
        game->man.slowingDown = 1;
        game->man.currentSpriteRun = 1;
        //game->enemy.currentSpriteRun = 1;
        if (fabsf(game->man.dx) < 0.1f)
        {
            game->man.dx = 0;
        }
    }
    if (state[SDL_SCANCODE_SPACE])
    {
        //game->shotCount++;
        if (game->shotCount == 0)
        {

            if (!game->man.facingLeft)
            {
                addBullet(game, game->man.x + 40, game->man.y + 15, 3);
                game->shotCount++;
            }
            else
            {
                addBullet(game, game->man.x, game->man.y + 15, -3);
                game->shotCount++;
            }
        }
    }
    if (game->time % 23 == 0)
    {
        game->shotCount = 0;
    }
    // SECOND PLAYER
    if (game->multiPlayer)
    {
        if (state[SDL_SCANCODE_UP])
        {
            game->secondPlayer.dy -= 0.2f;

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
            game->secondPlayer.dx -= 0.5;
            if (game->secondPlayer.dx < -6)
            {
                game->secondPlayer.dx = -6;
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
            game->secondPlayer.dx += 0.5;
            if (game->secondPlayer.dx > 6)
            {
                game->secondPlayer.dx = 6;
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
            game->secondPlayer.dx *= 0.8f;
            game->secondPlayer.slowingDown = 1;
            game->secondPlayer.currentSpriteRun2 = 1;
            //game->enemy.currentSpriteRun = 1;
            if (fabsf(game->secondPlayer.dx) < 0.1f)
            {
                game->secondPlayer.dx = 0;
            }
        }
        if (state[SDL_SCANCODE_KP_0])
        {
            if (game->shotCountMultiplayer == 0)
            {

                if (!game->secondPlayer.facingLeft)
                {
                    addSecondBullet(game, game->secondPlayer.x + 40, game->secondPlayer.y + 15, 3);
                    game->shotCountMultiplayer++;
                }
                else
                {
                    addSecondBullet(game, game->secondPlayer.x, game->secondPlayer.y + 15, -3);
                    game->shotCountMultiplayer++;
                }
            }
        }
        if (game->time % 23 == 0)
        {
            game->shotCountMultiplayer = 0;
        }
    }

    // if (state[SDL_SCANCODE_UP])
    // {
    //     game->man.y -= 10;
    // }
    // if (state[SDL_SCANCODE_DOWN])
    // {
    //     game->man.y += 10;
    // }

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

    if (!game->multiPlayer)
    {
        if (game->man.lives == 0)
        {
            // Guard against overwriting a transition an earlier handler in
            // this same call (e.g. SDLK_q) already made -- see
            // docs/frame-pipeline-map.md's double-transition finding.
            if (game->scene == APP_SCENE_ARCADE_GAME)
            {
                app_change_scene(game, APP_SCENE_ARCADE_MENU);
            }
        }
    }
    if (game->multiPlayer)
    {
        if (game->man.lives == 0 && game->secondPlayer.lives == 0)
        {
            if (game->scene == APP_SCENE_ARCADE_GAME)
            {
                app_change_scene(game, APP_SCENE_ARCADE_MENU);
            }
        }
    }
    return done;
}

int processEvents2(SDL_Window *window, GameState *game)
{
    SDL_Event event;
    int done = 0;

    if (game->jumpSound == NULL) {
        game->jumpSound = Mix_LoadWAV("resource/sounds/jump.wav");
    }
    if (game->select == NULL) {
        game->select = Mix_LoadWAV("resource/sounds/select.wav");
    }
    if (game->shootSound == NULL) {
        game->shootSound = Mix_LoadWAV("resource/sounds/shoot.wav");
    }

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_WINDOWEVENT_CLOSE:
        {
            if (window)
            {
                SDL_DestroyWindow(window);
                window = NULL;
                done = 0;
            }
        }
        break;
        case SDL_KEYDOWN:
        {
            // Cheat code -- SDLK_0 is not represented in GameCommand (not
            // duplicated anywhere else, so routing it through the
            // translation boundary would add indirection with no benefit).
            // Mutually exclusive with the mapped commands below, same as the
            // original single raw-keycode switch.
            if (event.key.keysym.sym == SDLK_0)
            {
                game->man.x += 14400;
                game->gameLives += 30;
            }

            switch (translate_runner_command(event.key.keysym.sym))
            {
            //return to menu
            case GAME_COMMAND_QUIT_TO_MODE_MENU:
                app_change_scene(game, APP_SCENE_RUNNER_MENU);

                Mix_VolumeChunk(game->select, 32);
                Mix_PlayChannel(-1, game->select, 0);
                Mix_FreeMusic(game->runnerMus);
                game->runnerMus = NULL;
                Mix_FreeChunk(game->kickSound);
                game->kickSound = NULL;
                return done;
                break;
                //pause
            case GAME_COMMAND_PAUSE:
                app_change_scene(game, APP_SCENE_RUNNER_PAUSE);
                done = 0;

                Mix_VolumeChunk(game->select, 32);
                Mix_PlayChannel(-1, game->select, 0);
                Mix_PauseMusic();
                break;

            case GAME_COMMAND_JUMP_PLAYER1:
                if (game->man.onLedge)
                {
                    game->man.dy = -10;
                    game->man.onLedge = 0;

                    Mix_VolumeChunk(game->jumpSound, 32);
                    Mix_PlayChannel(-1, game->jumpSound, 0);
                }
                break;
            case GAME_COMMAND_QUIT_TO_MAIN_MENU:
                app_change_scene(game, APP_SCENE_MAIN_MENU);

                Mix_FreeChunk(game->select);
                game->select = NULL;
                Mix_FreeChunk(game->jumpSound);
                game->jumpSound = NULL;
                Mix_FreeChunk(game->shootSound);
                game->shootSound = NULL;
                Mix_FreeChunk(game->kickSound);
                game->kickSound = NULL;
                Mix_FreeMusic(game->runnerMus);
                game->runnerMus = NULL;
                break;
            case GAME_COMMAND_JUMP_PLAYER2:
                if (game->secondPlayer.onLedge)
                {
                    game->secondPlayer.dy = -10;
                    game->secondPlayer.onLedge = 0;

                    Mix_VolumeChunk(game->jumpSound, 32);
                    Mix_PlayChannel(-1, game->jumpSound, 0);
                }
                break;
            default:
                break;
            }
        }
        break;
        case SDL_QUIT:
            //quit out of the game
            done = 1;
            break;
        }
    }
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_W])
    {
        game->man.dy -= 0.15f;
    }

    //    if (state[SDL_SCANCODE_ESCAPE]) {
    //        game->menu_status = 1;
    //    }
    if (state[SDL_SCANCODE_A])
    {
        game->man.dx -= 0.5;
        if (game->man.dx < -6)
        {
            game->man.dx = -6;
        }
        game->man.facingLeft = 1;
        game->man.slowingDown = 0;
    }
    else if (state[SDL_SCANCODE_D])
    {
        game->man.dx += 0.5;
        if (game->man.dx > 6)
        {
            game->man.dx = 6;
        }
        game->man.facingLeft = 0;
        game->man.slowingDown = 0;
    }
    else
    {
        game->man.animFrame = 0;
        game->man.dx *= 0.8f;
        game->man.slowingDown = 1;
        if (fabsf(game->man.dx) < 0.1f)
        {
            game->man.dx = 0;
        }
    }

    if (state[SDL_SCANCODE_UP])
    {
        game->secondPlayer.dy -= 0.15f;
    }

    //    if (state[SDL_SCANCODE_ESCAPE]) {
    //        game->menu_status = 1;
    //    }
    if (state[SDL_SCANCODE_LEFT])
    {
        game->secondPlayer.dx -= 0.5;
        if (game->secondPlayer.dx < -6)
        {
            game->secondPlayer.dx = -6;
        }
        game->secondPlayer.facingLeft = 1;
        game->secondPlayer.slowingDown = 0;
    }
    else if (state[SDL_SCANCODE_RIGHT])
    {
        game->secondPlayer.dx += 0.5;
        if (game->secondPlayer.dx > 6)
        {
            game->secondPlayer.dx = 6;
        }
        game->secondPlayer.facingLeft = 0;
        game->secondPlayer.slowingDown = 0;
    }
    else
    {
        game->secondPlayer.animFrameSecond = 0;
        game->secondPlayer.dx *= 0.8f;
        game->secondPlayer.slowingDown = 1;
        if (fabsf(game->secondPlayer.dx) < 0.1f)
        {
            game->secondPlayer.dx = 0;
        }
    }

    // if (state[SDL_SCANCODE_UP])
    // {
    //     game->man.y -= 10;
    // }
    // if (state[SDL_SCANCODE_DOWN])
    // {
    //     game->man.y += 10;
    // }

    //infinity fields
    //printf("%d", game->iter);
    if (game->x_score == (50 * game->iter + 5))
    {
        int bulb = 700;
        //int k;

        for (int i = 0; i < 50; i++)
        {

            game->ledges[i].w = 180;
            game->ledges[i].h = 60;
            game->ledges[i].x = i * 300 + 29400;
            game->ledges[i].y = bulb - random_sign(3, 40);
            while (game->ledges[i].y >= 700)
                game->ledges[i].y -= 50;
            while (game->ledges[i].y <= 100)
                game->ledges[i].y += 50;
            bulb = game->ledges[i].y;

            game->stars[i].x = game->ledges[i].x - random_sign(1, 1) * random() % 120;
            game->stars[i].y = game->ledges[i].y - random() % 120;
            //SDL_Delay(2000);
        }
        game->iter++;
        //printf("'%d'", game->iter);
    }
    if ((game->man.x / 293) > game->x_score)
        game->x_score = game->man.x / 293;

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
        if (game->scene == APP_SCENE_RUNNER_GAME)
        {
            app_change_scene(game, APP_SCENE_RUNNER_MENU);
        }
    }

    return done;
}
