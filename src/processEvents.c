#include "header.h"
#include "scene.h"
#include "input_command.h"
#include "collision_pipeline.h"

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

            // Jump is edge-triggered here but consumed at the fixed physics
            // tick rate -- see consume_arcade_jump_requests() (src/process.c)
            // and docs/input-simulation-separation-map.md (Phase 12).
            case GAME_COMMAND_JUMP_PLAYER1:
                game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
                break;
            case GAME_COMMAND_JUMP_PLAYER2:
                game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;
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

    // Continuous held-key forces for `man` (horizontal accel/clamp, jump-hold
    // thrust, friction/snap) moved to src/process.c (Phase 11, see
    // docs/physics-timestep-map.md section 4) -- they now run at the fixed
    // physics tick rate instead of the render rate. Discrete actions (jump
    // trigger, shooting below) stay here, unchanged.
    //
    // Reads game->input.shootHeldPlayer1 (Phase 17, see
    // docs/input-snapshot-architecture-map.md) instead of calling
    // SDL_GetKeyboardState() itself -- main.c captures one snapshot per real
    // frame; the shotCount cooldown logic below is unchanged.
    if (game->input.shootHeldPlayer1)
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
        // Continuous held-key forces for `secondPlayer` moved to
        // src/process.c (Phase 11) -- same reasoning as `man` above.
        if (game->input.shootHeldPlayer2)
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

    // Body-contact hazards + fall-off-screen, and the resulting game-over
    // transition (Phase 19, see docs/collision-ordering-map.md) --
    // extracted verbatim into src/collision_pipeline.c, same position.
    resolve_arcade_hazards(game);
    resolve_arcade_game_over_transition(game);
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

            // Jump is edge-triggered here but consumed at the fixed physics
            // tick rate -- see consume_runner_jump_requests() (src/process.c)
            // and docs/input-simulation-separation-map.md (Phase 12). Also
            // fixes a regression found during that phase's audit: this site
            // (and the JUMP_PLAYER2 site below) had never been converted off
            // the bare frame-tuned `-10` during Phase 11 -- consumed jumps
            // now use JUMP_SPEED_PER_SEC, matching Arcade.
            case GAME_COMMAND_JUMP_PLAYER1:
                game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
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
                game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;
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
    // Continuous held-key forces for `man`/`secondPlayer` (jump-hold thrust,
    // horizontal accel/clamp, friction/snap) moved to src/process.c's
    // process2() (Phase 11, see docs/physics-timestep-map.md section 4) --
    // they now run at the fixed physics tick rate instead of the render
    // rate.

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

    // Fall-off-screen hazard, and the resulting game-over transition
    // (Phase 19, see docs/collision-ordering-map.md) -- extracted verbatim
    // into src/collision_pipeline.c, same position.
    check_runner_fall_hazard(game);
    resolve_runner_game_over_transition(game);

    return done;
}
