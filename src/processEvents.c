#include "header.h"
#include "scene.h"
#include "input_command.h"
#include "input_snapshot.h"
#include "collision_pipeline.h"
#include "game_events.h"
#include "display.h"

int processEvents(SDL_Window *window, GameState *game)
{
    (void)window;
    SDL_Event event;
    int done = 0;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&game->app, &event);
        display_handle_event(game, &event);
        switch (event.type)
        {
        case SDL_WINDOWEVENT_CLOSE:
        {
            done = 1;
        }
        break;
        case SDL_KEYDOWN:
        {
            if (event.key.repeat != 0) break;
            GameCommand command = translate_arcade_command(event.key.keysym.sym);
            if (event.key.keysym.scancode == game->app.settings.player1.pause) command = GAME_COMMAND_PAUSE;
            else if (command == GAME_COMMAND_PAUSE) command = GAME_COMMAND_NONE;
            switch (command)
            {
            //return to menu
            case GAME_COMMAND_QUIT_TO_MODE_MENU:
                app_change_scene(game, APP_SCENE_ARCADE_MENU);

                Mix_VolumeChunk(game->audio.select, 32);
                Mix_PlayChannel(-1, game->audio.select, 0);
                return done;
                break;
                //pause
            case GAME_COMMAND_PAUSE:
                app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
                done = 0;

                Mix_VolumeChunk(game->audio.select, 32);
                Mix_PlayChannel(-1, game->audio.select, 0);
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
            case GAME_COMMAND_QUIT_TO_MAIN_MENU:
                app_change_scene(game, APP_SCENE_MAIN_MENU);

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

    // SDL event polling remains here; the snapshot-driven shooting action is
    // shared with deterministic replay in process_arcade_shooting().
    process_arcade_shooting(game);

    return done;
}

int processEvents2(SDL_Window *window, GameState *game)
{
    (void)window;
    SDL_Event event;
    int done = 0;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&game->app, &event);
        display_handle_event(game, &event);
        switch (event.type)
        {
        case SDL_WINDOWEVENT_CLOSE:
        {
            done = 1;
        }
        break;
        case SDL_KEYDOWN:
        {
            if (event.key.repeat != 0) break;
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

            GameCommand command = translate_runner_command(event.key.keysym.sym);
            if (event.key.keysym.scancode == game->app.settings.player1.pause) command = GAME_COMMAND_PAUSE;
            else if (command == GAME_COMMAND_PAUSE) command = GAME_COMMAND_NONE;
            switch (command)
            {
            //return to menu
            case GAME_COMMAND_QUIT_TO_MODE_MENU:
                app_change_scene(game, APP_SCENE_RUNNER_MENU);

                Mix_VolumeChunk(game->audio.select, 32);
                Mix_PlayChannel(-1, game->audio.select, 0);
                return done;
                break;
                //pause
            case GAME_COMMAND_PAUSE:
                app_change_scene(game, APP_SCENE_RUNNER_PAUSE);
                done = 0;

                Mix_VolumeChunk(game->audio.select, 32);
                Mix_PlayChannel(-1, game->audio.select, 0);
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

    // Fall-off-screen hazard, and the resulting game-over transition
    // (Phase 19, see docs/collision-ordering-map.md) -- extracted verbatim
    // into src/collision_pipeline.c, same position.
    game_events_begin(game);
    detect_runner_fall_hazards(game);
    game_events_apply(game);

    return done;
}
