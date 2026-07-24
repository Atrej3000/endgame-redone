#include "header.h"
#include "arcade_waves.h"
#include "combat_feedback.h"

static int render_pixel(float coordinate)
{
    if (isnan(coordinate)) return 0;
    if (coordinate <= (float)INT_MIN) return INT_MIN;
    if (coordinate >= (float)INT_MAX) return INT_MAX;
    return (int)lroundf(coordinate);
}

static SDL_Texture *animation_player_texture(const Man *player)
{
    if (player == NULL)
    {
        return NULL;
    }
    switch (player->animation.state)
    {
        case ANIMATION_STATE_IDLE: return player->sheetTextureIdle;
        case ANIMATION_STATE_RUN: return player->sheetTextureRun;
        case ANIMATION_STATE_JUMP: return player->sheetTextureJump;
        case ANIMATION_STATE_DOUBLE_JUMP: return player->sheetTextureDoubleJump;
        case ANIMATION_STATE_WALL_JUMP: return player->sheetTextureWallJump;
        case ANIMATION_STATE_FALL: return player->sheetTextureFall;
        case ANIMATION_STATE_HIT: return player->sheetTextureHit;
        case ANIMATION_STATE_DEAD: return player->sheetTextureIdle;
    }
    return player->sheetTextureIdle;
}

static void draw_animation_player(SDL_Renderer *renderer, const GameState *game, const Man *player,
                                  float worldOffsetX)
{
    if (!renderer || !game || !player)
    {
        return;
    }
    SDL_Texture *texture = animation_player_texture(player);
    if (texture == NULL)
    {
        return;
    }
    int textureWidth = 0;
    int textureHeight = 0;
    if (SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight) != 0 ||
        textureWidth < 32 || textureHeight < 32)
    {
        return;
    }
    int frame = player->animation.frame;
    const int availableFrames = textureWidth / 32;
    if (frame < 0 || frame >= availableFrames)
    {
        frame = 0;
    }
    SDL_Rect source = {32 * frame, 0, 32, 32};
    SDL_Rect destination = {render_pixel(worldOffsetX + render_lerp(player->prevX, player->x, game->renderAlpha)),
                           render_pixel(render_lerp(player->prevY, player->y, game->renderAlpha)), 54, 54};
    (void)SDL_RenderCopyEx(renderer, texture, &source, &destination, 0, NULL,
                           player->facingLeft == 1
                               ? SDL_FLIP_HORIZONTAL
                               : SDL_FLIP_NONE);
}

static void draw_death_player(SDL_Renderer *renderer, const GameState *game,
                              const Man *player, float worldOffsetX)
{
    if (!renderer || !game || !player || !player->visible0 ||
        !player->isDead || !game->death)
    {
        return;
    }
    SDL_Rect destination = {
        render_pixel(worldOffsetX +
                     render_lerp(player->prevX, player->x, game->renderAlpha)),
        render_pixel(render_lerp(player->prevY, player->y, game->renderAlpha) -
                     10.0f),
        38, 83};
    (void)SDL_RenderCopyEx(renderer, game->death, NULL, &destination, 0, NULL,
                           game->time % 20 < 10
                               ? SDL_FLIP_HORIZONTAL
                               : SDL_FLIP_NONE);
}

static void draw_runner_second_player(SDL_Renderer *renderer,
                                      const GameState *game,
                                      float worldOffsetX)
{
    const Man *player = &game->secondPlayer;
    if (!player->visible0 || player->isDead)
    {
        return;
    }
    int frame = player->animation.frame;
    if (frame < 0 || frame >= 12)
    {
        frame = 0;
    }
    SDL_Texture *texture = game->secondPlayerFrames[frame];
    if (!texture)
    {
        texture = game->secondPlayerFrames[0];
    }
    if (!texture)
    {
        return;
    }
    SDL_Rect destination = {
        render_pixel(worldOffsetX +
                     render_lerp(player->prevX, player->x, game->renderAlpha)),
        render_pixel(render_lerp(player->prevY, player->y, game->renderAlpha)),
        54, 54};
    (void)SDL_RenderCopyEx(renderer, texture, NULL, &destination, 0, NULL,
                           player->facingLeft == 1
                               ? SDL_FLIP_HORIZONTAL
                               : SDL_FLIP_NONE);
}

void doRender(SDL_Renderer *renderer, GameState *game)
{
    if (!renderer || !game)
    {
        return;
    }
    if (game->statusState == STATUS_STATE_LIVES)
    {
        init_status_lives(game);
        draw_status_lives(game);
        SDL_RenderPresent(renderer);
        return;
    }
    if (game->statusState != STATUS_STATE_GAME)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        return;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (game->statusState == STATUS_STATE_GAME)
    {
        combat_feedback_begin_render(renderer, game);

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
                SDL_Rect srcRect = {88 * game->smartEnemies[i].animation.frame, 0, 44, 26};
                SDL_Rect rect = {render_pixel(render_lerp(game->smartEnemies[i].prevX, game->smartEnemies[i].x, game->renderAlpha)),
                                 render_pixel(render_lerp(game->smartEnemies[i].prevY, game->smartEnemies[i].y, game->renderAlpha)), 85, 50};
                SDL_RenderCopyEx(renderer, game->enemy.sheetTextureRun2, &srcRect, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }
        //ENEMY RUN
        for (int i = 0; i < NUM_ENEMIES; i++)
            if (game->enemyValues[i].visible)
            {
                SDL_Rect srcRect = {64 * game->enemyValues[i].animation.frame, 0, 32, 32};
                SDL_Rect rect = {render_pixel(render_lerp(game->enemyValues[i].prevX, game->enemyValues[i].x, game->renderAlpha)),
                                 render_pixel(render_lerp(game->enemyValues[i].prevY, game->enemyValues[i].y, game->renderAlpha)), 55, 55};
                SDL_RenderCopyEx(renderer, game->enemy.sheetTextureRun, &srcRect, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }

        // Bosses
        for (int j = 0; j < 2; j++)
        {
            if (game->boss[j].visible)
            {
                SDL_Rect srcRect = {88 * game->boss[j].animation.frame, 0, 44, 30};
                SDL_Rect rect = {render_pixel(render_lerp(game->boss[j].prevX, game->boss[j].x, game->renderAlpha)),
                                 render_pixel(render_lerp(game->boss[j].prevY, game->boss[j].y, game->renderAlpha)), 81, 55};
                SDL_RenderCopyEx(renderer, game->bossTexture, &srcRect, &rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }
        }

        // BUllets
        for (int i = 0; i < MAX_BULLETS; i++)
            if (game->bullets[i].active)
            {
                SDL_Rect srcRect = {32 * game->CurrentSheetBullet, 0, 32, 32};
                SDL_Rect rect = {render_pixel(render_lerp(game->bullets[i].prevX, game->bullets[i].x, game->renderAlpha)),
                                 render_pixel(render_lerp(game->bullets[i].prevY, game->bullets[i].y, game->renderAlpha)), 32, 32};
                (void)SDL_RenderCopyEx(renderer, game->bulletTexture, &srcRect,
                                       &rect, 0, NULL,
                                       game->bullets[i].dx < 0.0f
                                           ? SDL_FLIP_HORIZONTAL
                                           : SDL_FLIP_NONE);
            }

        if (game->multiPlayer)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (game->secondBullets[i].active)
                {
                    SDL_Rect srcRect = {32 * game->CurrentSheetBullet2, 0, 32, 32};
                    SDL_Rect rect = {render_pixel(render_lerp(game->secondBullets[i].prevX, game->secondBullets[i].x, game->renderAlpha)),
                                     render_pixel(render_lerp(game->secondBullets[i].prevY, game->secondBullets[i].y, game->renderAlpha)), 32, 32};
                    (void)SDL_RenderCopyEx(renderer, game->secondBulletTexture,
                                           &srcRect, &rect, 0, NULL,
                                           game->secondBullets[i].dx < 0.0f
                                               ? SDL_FLIP_HORIZONTAL
                                               : SDL_FLIP_NONE);
                }
            }
        }

        if (game->man.visible0 && !game->man.isDead)
        {
            draw_animation_player(renderer, game, &game->man, 0.0f);
        }
        if (game->multiPlayer && game->secondPlayer.visible0 &&
            !game->secondPlayer.isDead)
        {
            draw_animation_player(renderer, game, &game->secondPlayer, 0.0f);
        }

        draw_death_player(renderer, game, &game->man, 0.0f);
        if (game->multiPlayer)
        {
            draw_death_player(renderer, game, &game->secondPlayer, 0.0f);
        }
        combat_feedback_draw(renderer, game, 0.0f);
        combat_feedback_end_render(renderer);
    }

    //show us our Lives in game
    init_status_lives(game);
    draw_status_lives2(game);

    //score of kills
    init_status_kills(game);
    draw_status_kills(game);
    arcade_waves_draw_hud(renderer, game);

    //We are done drawing, "present" or show to the screen what wer've drawn
    SDL_RenderPresent(renderer);
}

void doRender2(SDL_Renderer *renderer, GameState *game)
{
    if (!renderer || !game)
    {
        return;
    }
    if (game->statusState == STATUS_STATE_LIVES)
    {
        init_status_lives(game);
        draw_status_lives(game);
        SDL_RenderPresent(renderer);
        return;
    }
    if (game->statusState != STATUS_STATE_GAME)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        return;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    float renderScrollX = render_lerp(game->prevScrollX, game->scrollX, game->renderAlpha);
    if (game->statusState == STATUS_STATE_GAME)
    {
        combat_feedback_begin_render(renderer, game);
        SDL_Rect fon = {0, 0, 1280, 720};
        SDL_RenderCopy(renderer, game->fon, NULL, &fon);
    if (game->multiPlayer)
    {
        draw_runner_second_player(renderer, game, renderScrollX);
    }


        for (int i = 0; i < 100; i++)
        {
            SDL_Rect ledgeRect = {render_pixel(renderScrollX + (float)game->ledges[i].x), game->ledges[i].y, game->ledges[i].w, game->ledges[i].h};
            SDL_RenderCopy(renderer, game->brick, NULL, &ledgeRect);
        }
        if (game->man.visible0 && !game->man.isDead)
        {
            draw_animation_player(renderer, game, &game->man, renderScrollX);
        }
        draw_death_player(renderer, game, &game->man, renderScrollX);
        if (game->multiPlayer)
        {
            draw_death_player(renderer, game, &game->secondPlayer,
                              renderScrollX);
        }
        combat_feedback_draw(renderer, game, renderScrollX);
        combat_feedback_end_render(renderer);
    }
    //draw the trap image
    for (int i = 0; i < 100; i++)
    {
        SDL_Rect starRect = {render_pixel(renderScrollX + render_lerp(game->stars[i].prevX, (float)game->stars[i].x, game->renderAlpha)),
                             render_pixel(render_lerp(game->stars[i].prevY, (float)game->stars[i].y, game->renderAlpha)), 56, 56};
        SDL_RenderCopy(renderer, game->star, NULL, &starRect);
    }

    //lifes
    init_status_lives(game);
    draw_status_lives2(game);

    //score of kills
    init_status_x(game);
    draw_status_x(game);

    //We are done drawing, "present" or show to the screen what wer've drawn
    SDL_RenderPresent(renderer);
}
