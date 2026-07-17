#include "header.h"

void doRender(SDL_Renderer *renderer, GameState *game)
{

    if (game->statusState == STATUS_STATE_GAME)
    {

        /*
        SDL_Rect srcRectBack = {2 * game->CurrentSpriteBack, 0, WIDTH, HEIGHT};
        SDL_Rect backRect = {0, 0, WIDTH, HEIGHT};
        SDL_RenderCopyEx(renderer, game->sheetTextureBack, &srcRectBack, &backRect, 0, NULL, SDL_FLIP_NONE);
*/

        //Andrew back
        SDL_Rect backRect = {0, 0, 1280, 720};
        SDL_RenderCopy(renderer, game->sheetTextureBack, NULL, &backRect);

        SDL_Rect backSunRect = {0, 0, 1280, 720};
        SDL_RenderCopy(renderer, game->sheetTextureSun, NULL, &backSunRect);

        //Clouds
        SDL_Rect cloudPos1 = {game->cloud1.x, game->cloud1.y, 312, 104}; //78x26
        SDL_RenderCopy(renderer, game->cloud1.sheetTextureCloud1, NULL, &cloudPos1);

        SDL_Rect cloudPos2 = {game->cloud2.x, game->cloud2.y, 416, 88}; //104x22
        SDL_RenderCopy(renderer, game->cloud2.sheetTextureCloud2, NULL, &cloudPos2);

        SDL_Rect cloudPos3 = {game->cloud3.x, game->cloud3.y, 280, 56}; //70x14
        SDL_RenderCopy(renderer, game->cloud3.sheetTextureCloud3, NULL, &cloudPos3);

        SDL_Rect cloudPos4 = {game->cloud4.x, game->cloud4.y, 292, 64}; //68x16
        SDL_RenderCopy(renderer, game->cloud4.sheetTextureCloud4, NULL, &cloudPos4);

        SDL_Rect cloudPos5 = {game->cloud5.x, game->cloud5.y, 216, 48}; //54x12
        SDL_RenderCopy(renderer, game->cloud5.sheetTextureCloud5, NULL, &cloudPos5);

        SDL_Rect cloudPos6 = {game->cloud6.x, game->cloud6.y, 162, 32}; //38x8
        SDL_RenderCopy(renderer, game->cloud6.sheetTextureCloud6, NULL, &cloudPos6);

        SDL_Rect cloudPos7 = {game->cloud7.x, game->cloud7.y, 264, 32}; //66x8
        SDL_RenderCopy(renderer, game->cloud7.sheetTextureCloud7, NULL, &cloudPos7);

        SDL_Rect cloudPos8 = {game->cloud8.x, game->cloud8.y, 352, 40}; //88x10
        SDL_RenderCopy(renderer, game->cloud8.sheetTextureCloud8, NULL, &cloudPos8);

        //Buildings
        SDL_Rect backRectfront = {0, 0, 1280, 720};
        SDL_RenderCopy(renderer, game->sheetTextureBack2, NULL, &backRectfront);

        //Train
        SDL_Rect trainPos = {game->train.x, game->train.y, 480, 16};
        SDL_RenderCopy(renderer, game->train.textureTrain, NULL, &trainPos);

        // //Set the drawing color to blue
        // SDL_SetRenderDrawColor(renderer, 128, 128, 255, 255);

        // //Clear the screen (to blue)
        // SDL_RenderClear(renderer);

        // //Set drawing color to white
        // SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int i = 0; i < 24; i++)
        {
            SDL_Rect ledgeRect = {game->ledges[i].x, game->ledges[i].y, game->ledges[i].w, game->ledges[i].h};
            SDL_RenderCopy(renderer, game->brick, NULL, &ledgeRect);
        }
        for (int i = 24; i < 40; i++)
        {
            SDL_Rect ledgeRect = {game->ledges[i].x, game->ledges[i].y, game->ledges[i].w, game->ledges[i].h};
            SDL_RenderCopy(renderer, game->brick_block, NULL, &ledgeRect);
        }

        for (int i = 40; i < 100; i++)
        {
            SDL_Rect ledgeRect = {game->ledges[i].x, game->ledges[i].y, game->ledges[i].w, game->ledges[i].h};
            SDL_RenderCopy(renderer, game->copper_block, NULL, &ledgeRect);
        }


        //SMART ENEMY RUN
        for (int i = 0; i < NUM_SMART_ENEMIES; i++)
            if (game->smartEnemies[i].visible)
            {
                SDL_Rect srcRect = {88 * game->enemy.currentSpriteRun2, 0, 44, 26};
                SDL_Rect rect = {game->smartEnemies[i].x, game->smartEnemies[i].y, 85, 50};
                SDL_RenderCopyEx(renderer, game->enemy.sheetTextureRun2, &srcRect, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }
        //ENEMY RUN
        for (int i = 0; i < NUM_ENEMIES; i++)
            if (game->enemyValues[i].visible)
            {
                SDL_Rect srcRect = {64 * game->enemy.currentSpriteRun, 0, 32, 32};
                SDL_Rect rect = {game->enemyValues[i].x, game->enemyValues[i].y, 55, 55};
                SDL_RenderCopyEx(renderer, game->enemy.sheetTextureRun, &srcRect, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }

        // BOSS RENDER ____________________________
        for (int j = 0; j < 2; j++)
        {
            if (game->boss[j].visible)
            {
                SDL_Rect srcRect = {88 * game->CurrentSheetBoss, 0, 44, 30};
                SDL_Rect rect = {game->boss[j].x, game->boss[j].y, 81, 55};
                SDL_RenderCopyEx(renderer, game->bossTexture, &srcRect, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }
        }

        // BUllets
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i] && game->bullets[i]->unvisible == 0)
            {
                SDL_Rect srcRect = {32 * game->CurrentSheetBullet, 0, 32, 32};
                SDL_Rect rect = {game->bullets[i]->x, game->bullets[i]->y, 32, 32};
                SDL_RenderCopyEx(renderer, game->bulletTexture, &srcRect, &rect, 0, NULL, (game->man.facingLeft == 1));
            }

        if (game->multiPlayer)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (game->secondBullets[i] && game->secondBullets[i]->unvisible == 0)
                {
                    SDL_Rect srcRect = {32 * game->CurrentSheetBullet2, 0, 32, 32};
                    SDL_Rect rect = {game->secondBullets[i]->x, game->secondBullets[i]->y, 32, 32};
                    SDL_RenderCopyEx(renderer, game->secondBulletTexture, &srcRect, &rect, 0, NULL, (game->secondPlayer.facingLeft == 1));
                }
            }
        }

        //HERO RUN
        if (game->man.visible0 && game->man.onLedge)
        {
            SDL_Rect srcRect = {64 * game->man.currentSpriteRun, 0, 32, 32};
            SDL_Rect rect = {game->man.x, game->man.y, 55, 55};
            SDL_RenderCopyEx(renderer, game->man.sheetTextureRun, &srcRect, &rect, 0, NULL, (game->man.facingLeft == 1));
        }
        //HERO JUMP
        if (game->man.visible0 && !game->man.onLedge)
        {
            SDL_Rect srcRect = {64 * game->man.currentSpriteJump, 0, 32, 32};
            SDL_Rect rect = {game->man.x, game->man.y, 55, 55};
            SDL_RenderCopyEx(renderer, game->man.sheetTextureJump, &srcRect, &rect, 0, NULL, (game->man.facingLeft == 1));
        }
        //SECOND HERO RUN
        if (game->multiPlayer)
        {
            if (game->man.visible0 && game->secondPlayer.onLedge)
            {
                SDL_Rect srcRect2 = {64 * game->secondPlayer.currentSpriteRun2, 0, 32, 32};
                SDL_Rect rect2 = {game->secondPlayer.x, game->secondPlayer.y, 55, 55};
                SDL_RenderCopyEx(renderer, game->secondPlayer.sheetTextureRun2, &srcRect2, &rect2, 0, NULL, (game->secondPlayer.facingLeft == 1));
            }
        }
        //SECOND HERO JUMP
        if (game->multiPlayer)
        {
            if (game->man.visible0 && !game->secondPlayer.onLedge)
            {
                SDL_Rect srcRect2 = {64 * game->secondPlayer.currentSpriteJump2, 0, 32, 32};
                SDL_Rect rect2 = {game->secondPlayer.x, game->secondPlayer.y, 55, 55};
                SDL_RenderCopyEx(renderer, game->secondPlayer.sheetTextureJump2, &srcRect2, &rect2, 0, NULL, (game->secondPlayer.facingLeft == 1));
            }
        }

        // if (game->man.visible0 && game->man.onLedge && game->man.attack)
        // {
        //     SDL_Rect srcRect = {128 * game->man.currentSpriteAttack1, 0, 128, 32};
        //     SDL_Rect rect = {game->man.x, game->man.y, 225, 55};
        //     SDL_RenderCopyEx(renderer, game->man.sheetTextureAttack1, &srcRect, &rect, 0, NULL, (game->man.facingLeft == 1));
        // }

        if (game->man.isDead)
        {
            SDL_Rect rect = {game->man.x, game->man.y - 10, 38, 83};
            SDL_RenderCopyEx(renderer, game->death, NULL, &rect, 0, NULL, (game->time % 20 < 10));
        }
    }
    //draw the trap image
    // for (int i = 0; i < 100; i++)
    // {
    //     SDL_Rect starRect = {game->stars[i].x, game->stars[i].y, 56, 56};
    //     SDL_RenderCopy(renderer, game->star, NULL, &starRect);
    // }

    if (game->statusState == STATUS_STATE_LIVES)
    {
        draw_status_lives(game);
    }

    //show us our Lives in game
    init_status_lives(game);
    draw_status_lives2(game);

    //score of kills
    init_status_kills(game);
    draw_status_kills(game);

    //We are done drawing, "present" or show to the screen what wer've drawn
    SDL_RenderPresent(renderer);
}

void doRender2(SDL_Renderer *renderer, GameState *game)
{
    if (game->statusState == STATUS_STATE_GAME)
    {
        SDL_Rect fon = {0, 0, 1280, 720};
        SDL_RenderCopy(renderer, game->fon, NULL, &fon);
        /*
       //Set the drawing color to blue
       SDL_SetRenderDrawColor(renderer, 128, 128, 255, 255);

       //Clear the screen (to blue)
       SDL_RenderClear(renderer);

       //Set drawing color to white
       SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
   */
    if (game->multiPlayer)
    {
        SDL_Rect scdPlr = {game->scrollX + game->secondPlayer.x, game->secondPlayer.y, 54, 54};
        SDL_RenderCopyEx(renderer, game->secondPlayerFrames[game->secondPlayer.animFrameSecond], NULL, &scdPlr, 0, NULL, (game->secondPlayer.facingLeft == 1));

        // SDL_Rect rect = {game->scrollX + game->man.x, game->man.y, 54, 54};
        // SDL_RenderCopyEx(renderer, game->manFrames[game->man.animFrame], NULL, &rect, 0, NULL, (game->man.facingLeft == 1));
        // if (game->man.isDead)
        // {
        //     SDL_Rect rect = {game->scrollX + game->man.x, game->man.y - 10, 38, 83};
        //     SDL_RenderCopyEx(renderer, game->death, NULL, &rect, 0, NULL, (game->time % 20 < 10));

        //     //_______________________________________________________________________death from stars
        //     game->gameLives--;
        //     game->man.isDead = 0;
        //     game->man.y = 0;
        // }
    }
    

        for (int i = 0; i < 100; i++)
        {
            SDL_Rect ledgeRect = {game->scrollX + game->ledges[i].x, game->ledges[i].y, game->ledges[i].w, game->ledges[i].h};
            SDL_RenderCopy(renderer, game->brick, NULL, &ledgeRect);
        }
        //HERO ANIMATION
        SDL_Rect rect = {game->scrollX + game->man.x, game->man.y, 54, 54};
        SDL_RenderCopyEx(renderer, game->manFrames[game->man.animFrame], NULL, &rect, 0, NULL, (game->man.facingLeft == 1));
        if (game->man.isDead)
        {
            SDL_Rect rect = {game->scrollX + game->man.x, game->man.y - 10, 38, 83};
            SDL_RenderCopyEx(renderer, game->death, NULL, &rect, 0, NULL, (game->time % 20 < 10));

            //_______________________________________________________________________death from stars
            game->gameLives--;
            game->man.isDead = 0;
            game->man.y = 0;
        }
    }
    //draw the trap image
    for (int i = 0; i < 100; i++)
    {
        SDL_Rect starRect = {game->scrollX + game->stars[i].x, game->stars[i].y, 56, 56};
        SDL_RenderCopy(renderer, game->star, NULL, &starRect);
    }

    if (game->statusState == STATUS_STATE_LIVES)
    {
        draw_status_lives(game);
    }
    //lifes
    init_status_lives(game);
    draw_status_lives2(game);

    //score of kills
    init_status_x(game);
    draw_status_x(game);

    //We are done drawing, "present" or show to the screen what wer've drawn
    SDL_RenderPresent(renderer);
    //    system("leaks -q endgame");
}
