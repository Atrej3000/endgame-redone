#include "header.h"

// ---------------------------------------------------------------------------
// Shared assets: textures/font whose content is identical regardless of which
// mode loads them (see docs/game-session-lifecycle.md). Loaded once, by
// whichever mode is visited first; the other mode's load is a no-op skip via
// the game->FIELD == NULL guards below.
// ---------------------------------------------------------------------------
static bool shared_assets_load(GameState *game)
{
    if (game->sharedAssetsLoaded)
    {
        return true;
    }

    bool ok = true;
    if (game->mult == NULL)
    {
        ok = ok && load_texture(game->renderer, "./resource/images/backgrounds/mult.png", &game->mult);
    }
    if (game->leaders == NULL)
    {
        ok = ok && load_texture(game->renderer, "./resource/images/backgrounds/leaders.jpg", &game->leaders);
    }
    if (game->pause == NULL)
    {
        ok = ok && load_texture(game->renderer, "./resource/images/backgrounds/pause.jpg", &game->pause);
    }
    if (game->brick == NULL)
    {
        ok = ok && load_texture(game->renderer, "./resource/images/terrain/Clay_block.png", &game->brick);
    }
    if (game->death == NULL)
    {
        ok = ok && load_texture(game->renderer, "./resource/images/main_hero/death/fire.png", &game->death);
    }
    if (game->font == NULL)
    {
        ok = ok && load_font("./resource/text/Fonts/crazy-pixel.ttf", 48, &game->font);
    }

    if (!ok)
    {
        shared_assets_unload(game);
        return false;
    }

    game->sharedAssetsLoaded = true;
    return true;
}

void shared_assets_unload(GameState *game)
{
    destroy_texture(&game->mult);
    destroy_texture(&game->leaders);
    destroy_texture(&game->pause);
    destroy_texture(&game->brick);
    destroy_texture(&game->death);
    if (game->font)
    {
        TTF_CloseFont(game->font);
        game->font = NULL;
    }
    game->sharedAssetsLoaded = false;
}

// ---------------------------------------------------------------------------
// Arcade mode
// ---------------------------------------------------------------------------
bool arcade_assets_load(GameState *game)
{
    // Checked before the arcadeAssetsLoaded early-return (not after) so that
    // if the shared bucket is ever unloaded independently of the Arcade
    // flag, calling this again still re-establishes it -- arcadeAssetsLoaded
    // alone does not imply sharedAssetsLoaded is still true.
    if (!shared_assets_load(game))
    {
        return false;
    }

    if (game->arcadeAssetsLoaded)
    {
        return true;
    }

    bool ok = true;
    ok = ok && load_music("resource/sounds/battleMus.mp3", &game->battleMus);
    ok = ok && load_texture(game->renderer, "./resource/images/enemies/boss.png", &game->bossTexture);
    ok = ok && load_texture(game->renderer, "./resource/images/bullets/bullet_fireball.png", &game->bulletTexture);
    ok = ok && load_texture(game->renderer, "./resource/images/bullets/bullet_fireball2.png", &game->secondBulletTexture);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/Run_(32x32).png", &game->secondPlayer.sheetTextureRun2);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/jump/Double_Jump_(32x32).png", &game->secondPlayer.sheetTextureJump2);

    // manFrames[0] is written by both modes with different source images
    // (see docs/game-session-lifecycle.md) -- destroy whatever the other
    // mode may have loaded there instead of skip-if-loaded, so this mode's
    // own image always wins on its own (re)load, matching today's "whichever
    // mode loaded most recently wins" outcome without the leak.
    destroy_texture(&game->manFrames[0]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)0.png", &game->manFrames[0]);

    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/Run_(32x32).png", &game->man.sheetTextureRun);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/jump/Double_Jump_(32x32).png", &game->man.sheetTextureJump);
    ok = ok && load_texture(game->renderer, "./resource/images/enemies/shroom_Run(32x32).png", &game->enemy.sheetTextureRun);
    ok = ok && load_texture(game->renderer, "./resource/images/enemies/turtle_Run(44x26).png", &game->enemy.sheetTextureRun2);
    ok = ok && load_texture(game->renderer, "./resource/images/background/Sunset.png", &game->sheetTextureBack);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud01.png", &game->cloud1.sheetTextureCloud1);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud02.png", &game->cloud2.sheetTextureCloud2);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud03.png", &game->cloud3.sheetTextureCloud3);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud04.png", &game->cloud4.sheetTextureCloud4);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud05.png", &game->cloud5.sheetTextureCloud5);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud06.png", &game->cloud6.sheetTextureCloud6);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud07.png", &game->cloud7.sheetTextureCloud7);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_cloud08.png", &game->cloud8.sheetTextureCloud8);
    ok = ok && load_texture(game->renderer, "./resource/images/background/Sunset_sun.png", &game->sheetTextureSun);
    ok = ok && load_texture(game->renderer, "./resource/images/background/Sunset_front.png", &game->sheetTextureBack2);
    ok = ok && load_texture(game->renderer, "./resource/images/background/sunset_train.bmp", &game->train.textureTrain);
    ok = ok && load_texture(game->renderer, "./resource/images/terrain/brick_block.png", &game->brick_block);
    ok = ok && load_texture(game->renderer, "./resource/images/terrain/copper_block.png", &game->copper_block);

    if (!ok)
    {
        arcade_assets_unload(game);
        return false;
    }

    game->arcadeAssetsLoaded = true;
    return true;
}

void arcade_assets_unload(GameState *game)
{
    free_music(&game->battleMus);
    destroy_texture(&game->bossTexture);
    destroy_texture(&game->bulletTexture);
    destroy_texture(&game->secondBulletTexture);
    destroy_texture(&game->secondPlayer.sheetTextureRun2);
    destroy_texture(&game->secondPlayer.sheetTextureJump2);
    destroy_texture(&game->man.sheetTextureRun);
    destroy_texture(&game->man.sheetTextureJump);
    destroy_texture(&game->enemy.sheetTextureRun);
    destroy_texture(&game->enemy.sheetTextureRun2);
    destroy_texture(&game->sheetTextureBack);
    destroy_texture(&game->cloud1.sheetTextureCloud1);
    destroy_texture(&game->cloud2.sheetTextureCloud2);
    destroy_texture(&game->cloud3.sheetTextureCloud3);
    destroy_texture(&game->cloud4.sheetTextureCloud4);
    destroy_texture(&game->cloud5.sheetTextureCloud5);
    destroy_texture(&game->cloud6.sheetTextureCloud6);
    destroy_texture(&game->cloud7.sheetTextureCloud7);
    destroy_texture(&game->cloud8.sheetTextureCloud8);
    destroy_texture(&game->sheetTextureSun);
    destroy_texture(&game->sheetTextureBack2);
    destroy_texture(&game->train.textureTrain);
    destroy_texture(&game->brick_block);
    destroy_texture(&game->copper_block);
    // manFrames[0] intentionally left alone -- contested field, destroyed
    // centrally in app_shutdown() (see docs/game-session-lifecycle.md).
    game->arcadeAssetsLoaded = false;
}

void arcade_session_reset(GameState *game, GameMode mode)
{
    game->multiPlayer = mode;

    game->kills_score = 0;
    game->kills_score_multi = 0;

    game->man.x = 320 - 40;
    game->man.y = 240 - 40;
    game->man.dy = 0;
    game->man.dx = 0;
    game->man.onLedge = 0;
    game->man.animFrame = 0;
    game->man.facingLeft = 0;
    game->man.slowingDown = 0;
    game->man.lives = 3;
    game->man.visible0 = 1;
    game->man.isDead = 0;
    game->statusState = STATUS_STATE_LIVES;
    game->man.shooting = 0;

    game->train.x = 0;
    game->train.y = 440;

    game->cloud1.x = -312;
    game->cloud1.y = 100;
    game->cloud2.x = -416;
    game->cloud2.y = 115;
    game->cloud3.x = -280;
    game->cloud3.y = 135;
    game->cloud4.x = -292;
    game->cloud4.y = 160;
    game->cloud5.x = -216;
    game->cloud5.y = 190;
    game->cloud6.x = -162;
    game->cloud6.y = 225;
    game->cloud7.x = -264;
    game->cloud7.y = 265;
    game->cloud8.x = -352;
    game->cloud8.y = 310;

    game->gameLives = 10;
    game->tempScore = 0;
    game->shotCount = 0;
    game->shotCountMultiplayer = 0;

    if (game->multiPlayer)
    {
        game->secondPlayer.x = 1000;
        game->secondPlayer.y = 240 - 40;
        game->secondPlayer.dy = 0;
        game->secondPlayer.dx = 0;
        game->secondPlayer.onLedge = 0;
        game->secondPlayer.animFrameSecond = 0;
        game->secondPlayer.facingLeft = 0;
        game->secondPlayer.slowingDown = 0;
        game->secondPlayer.lives = 3;
        game->secondPlayer.visible0 = 1;
        game->secondPlayer.isDead = 0;
        game->statusState = STATUS_STATE_LIVES;
        game->secondPlayer.shooting = 0;
    }

    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        game->enemyValues[i].x = 640;
        game->enemyValues[i].y = 1000;
        game->enemyValues[i].dy = 0;
        game->enemyValues[i].dx = 0;
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        game->smartEnemies[i].x = 640;
        game->smartEnemies[i].y = 1000;
        game->smartEnemies[i].dy = 0;
        game->smartEnemies[i].dx = 0;
        game->smartEnemies[i].visible = 1;
        game->smartEnemies[i].countShots = 0;
    }
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].x = 1100;
        game->boss[i].y = 1000;
        game->boss[i].dy = 0;
        game->boss[i].dx = 0;
        game->boss[i].countShots = 0;
        game->boss[i].visible = 1;
    }

    // removeBullet/removeSecondBullet free() any still-live bullet before
    // nulling the slot -- the previous direct "= NULL" here leaked whatever
    // was still in flight (see docs/game-session-lifecycle.md).
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        removeBullet(game, i);
    }
    if (game->multiPlayer)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            removeSecondBullet(game, i);
        }
    }

    game->time = 0;
    game->deathCountdown = -1;

    for (int i = 0; i < 12; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = i * 48;
        game->ledges[i].y = 672;
    }
    for (int i = 12; i < 24; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i + 3) * 48;
        game->ledges[i].y = 672;
    }
    for (int i = 24; i < 32; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 24) * 48;
        game->ledges[i].y = 624;
    }
    for (int i = 32; i < 40; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 13) * 48;
        game->ledges[i].y = 624;
    }
    for (int i = 40; i < 51; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 32) * 48;
        game->ledges[i].y = 490;
    }
    for (int i = 51; i < 59; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 51) * 48;
        game->ledges[i].y = 350;
    }
    for (int i = 59; i < 67; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 40) * 48;
        game->ledges[i].y = 350;
    }
    for (int i = 67; i < 78; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 59) * 48;
        game->ledges[i].y = 200;
    }
    for (int i = 78; i < 87; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 75) * 48;
        game->ledges[i].y = 0;
    }
    for (int i = 87; i < 99; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = (i - 73) * 48;
        game->ledges[i].y = 0;
    }
    game->ledges[99].x = 1000;
    game->ledges[99].y = 1000;
}

// ---------------------------------------------------------------------------
// Runner mode
// ---------------------------------------------------------------------------
bool runner_assets_load(GameState *game)
{
    // See arcade_assets_load()'s comment -- checked before the
    // runnerAssetsLoaded early-return for the same reason.
    if (!shared_assets_load(game))
    {
        return false;
    }

    if (game->runnerAssetsLoaded)
    {
        return true;
    }

    bool ok = true;
    ok = ok && load_music("resource/sounds/runnerMus.mp3", &game->runnerMus);
    ok = ok && load_texture(game->renderer, "./resource/images/backgrounds/fon.png", &game->fon);
    ok = ok && load_texture(game->renderer, "./resource/images/traps/spike_head.png", &game->star);

    // manFrames[0] -- see arcade_assets_load()'s comment; same contested-field handling.
    destroy_texture(&game->manFrames[0]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)0.png", &game->manFrames[0]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)1.png", &game->manFrames[1]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)2.png", &game->manFrames[2]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)3.png", &game->manFrames[3]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)4.png", &game->manFrames[4]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)5.png", &game->manFrames[5]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)6.png", &game->manFrames[6]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)7.png", &game->manFrames[7]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)8.png", &game->manFrames[8]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)9.png", &game->manFrames[9]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)10.png", &game->manFrames[10]);
    ok = ok && load_texture(game->renderer, "./resource/images/main_hero/run/V_g(rn)11.png", &game->manFrames[11]);

    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)0.png", &game->secondPlayerFrames[0]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)1.png", &game->secondPlayerFrames[1]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)2.png", &game->secondPlayerFrames[2]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)3.png", &game->secondPlayerFrames[3]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)4.png", &game->secondPlayerFrames[4]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)5.png", &game->secondPlayerFrames[5]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)6.png", &game->secondPlayerFrames[6]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)7.png", &game->secondPlayerFrames[7]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)8.png", &game->secondPlayerFrames[8]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)9.png", &game->secondPlayerFrames[9]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)10.png", &game->secondPlayerFrames[10]);
    ok = ok && load_texture(game->renderer, "./resource/images/secondplayer/run/V_g(rn)11.png", &game->secondPlayerFrames[11]);

    if (!ok)
    {
        runner_assets_unload(game);
        return false;
    }

    game->runnerAssetsLoaded = true;
    return true;
}

void runner_assets_unload(GameState *game)
{
    free_music(&game->runnerMus);
    destroy_texture(&game->fon);
    destroy_texture(&game->star);
    for (int i = 1; i < 12; i++)
    {
        destroy_texture(&game->manFrames[i]);
    }
    for (int i = 0; i < 12; i++)
    {
        destroy_texture(&game->secondPlayerFrames[i]);
    }
    // manFrames[0] intentionally left alone -- see arcade_assets_unload().
    game->runnerAssetsLoaded = false;
}

void runner_session_reset(GameState *game, GameMode mode)
{
    game->multiPlayer = mode;

    game->x_score = 0;

    game->man.x = 320 - 240;
    game->man.y = 240 - 240;
    game->man.dy = 0;
    game->man.dx = 0;
    game->man.onLedge = 0;
    game->man.animFrame = 0;
    game->man.facingLeft = 0;
    game->man.slowingDown = 0;
    game->gameLives = 3;
    game->man.isDead = 0;
    game->statusState = STATUS_STATE_LIVES;

    if (game->multiPlayer)
    {
        game->secondPlayer.x = 100;
        game->secondPlayer.y = 80;
        game->secondPlayer.dy = 0;
        game->secondPlayer.dx = 0;
        game->secondPlayer.onLedge = 0;
        game->secondPlayer.animFrameSecond = 0;
        game->secondPlayer.facingLeft = 0;
        game->secondPlayer.slowingDown = 0;
        game->gameLives = 3;
        game->secondPlayer.isDead = 0;
        game->statusState = STATUS_STATE_LIVES;
    }

    game->time = 0;
    game->scrollX = 0;
    game->deathCountdown = -1;

    int bulb = 700;
    game->iter = 1;
    for (int i = 0; i < 100; i++)
    {
        game->ledges[i].w = 180;
        game->ledges[i].h = 60;
        game->ledges[i].x = i * 300;
        game->ledges[i].y = bulb - random_sign(3, 40);
        while (game->ledges[i].y >= 700)
            game->ledges[i].y -= 50;
        while (game->ledges[i].y <= 100)
            game->ledges[i].y += 50;
        bulb = game->ledges[i].y;

        game->stars[i].baseX = game->ledges[i].x - random_sign(1, 1) * random() % 120;
        game->stars[i].baseY = game->ledges[i].y - random() % 120;
        game->stars[i].mode = random() % 2;
        game->stars[i].phase = 2.0f * 3.14f * (random() % 360) / 360.0f;
    }
    game->ledges[99].x = 00;
    game->ledges[99].y = 100;
}
