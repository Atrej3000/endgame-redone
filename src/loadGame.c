#include "header.h"
#include "audio_assets.h"
#include "arcade_waves.h"
#include "combat_feedback.h"
#include "entity_spawn.h"
#include "input_snapshot.h"
#include "runner_segments.h"

// ---------------------------------------------------------------------------
// Shared assets: textures whose content is identical regardless of which
// mode loads them (see docs/game-session-lifecycle.md). Loaded once, by
// whichever mode is visited first; the other mode's load is a no-op skip via
// the game->FIELD == NULL guards below.
// ---------------------------------------------------------------------------
static bool texture_group_complete(SDL_Texture *const *textures, size_t count)
{
    if (!textures) return false;
    for (size_t index = 0U; index < count; index++)
    {
        if (textures[index] == NULL) return false;
    }
    return true;
}

static bool shared_assets_complete(const GameState *game)
{
    if (!game) return false;
    SDL_Texture *const textures[] = {
        game->mult, game->leaders, game->pause, game->brick, game->death,
        game->manFrames[0], game->man.sheetTextureIdle,
        game->man.sheetTextureRun, game->man.sheetTextureJump,
        game->man.sheetTextureFall, game->man.sheetTextureHit,
        game->man.sheetTextureDoubleJump, game->man.sheetTextureWallJump,
        game->appearanceEffect, game->disappearanceEffect,
        game->shadowEffect, game->dustParticle,
    };
    return texture_group_complete(textures,
                                  sizeof(textures) / sizeof(textures[0]));
}

static bool shared_assets_load(GameState *game)
{
    if (!game || !game->app.renderer) return false;
    if (shared_assets_complete(game))
    {
        game->assetFlags.sharedAssetsLoaded = true;
        return true;
    }
    if (game->assetFlags.sharedAssetsLoaded)
    {
        shared_assets_unload(game);
    }

    bool ok = true;
    if (game->mult == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/backgrounds/mult.png", &game->mult);
    }
    if (game->leaders == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/backgrounds/leaders.jpg", &game->leaders);
    }
    if (game->pause == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/backgrounds/pause.jpg", &game->pause);
    }
    if (game->brick == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/terrain/Clay_block.png", &game->brick);
    }
    if (game->death == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/death/fire.png", &game->death);
    }
    if (game->manFrames[0] == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)0.png", &game->manFrames[0]);
    }
    if (game->man.sheetTextureIdle == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/idle.png", &game->man.sheetTextureIdle);
    }
    if (game->man.sheetTextureRun == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/run.png", &game->man.sheetTextureRun);
    }
    if (game->man.sheetTextureJump == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/jump.png", &game->man.sheetTextureJump);
    }
    if (game->man.sheetTextureFall == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/fall.png", &game->man.sheetTextureFall);
    }
    if (game->man.sheetTextureHit == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/hit.png", &game->man.sheetTextureHit);
    }
    if (game->man.sheetTextureDoubleJump == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/double_jump.png", &game->man.sheetTextureDoubleJump);
    }
    if (game->man.sheetTextureWallJump == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/mask_dude/wall_jump.png", &game->man.sheetTextureWallJump);
    }
    if (game->appearanceEffect == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/effects/appearing.png", &game->appearanceEffect);
    }
    if (game->disappearanceEffect == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/effects/disappearing.png", &game->disappearanceEffect);
    }
    if (game->shadowEffect == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/effects/shadow.png", &game->shadowEffect);
    }
    if (game->dustParticle == NULL)
    {
        ok = ok && load_texture(game->app.renderer, "./resource/images/effects/dust_particle.png", &game->dustParticle);
    }

    if (!ok)
    {
        shared_assets_unload(game);
        return false;
    }

    game->assetFlags.sharedAssetsLoaded = true;
    return true;
}

void shared_assets_unload(GameState *game)
{
    if (!game) return;
    destroy_texture(&game->mult);
    destroy_texture(&game->leaders);
    destroy_texture(&game->pause);
    destroy_texture(&game->brick);
    destroy_texture(&game->death);
    destroy_texture(&game->manFrames[0]);
    destroy_texture(&game->man.sheetTextureIdle);
    destroy_texture(&game->man.sheetTextureRun);
    destroy_texture(&game->man.sheetTextureJump);
    destroy_texture(&game->man.sheetTextureFall);
    destroy_texture(&game->man.sheetTextureHit);
    destroy_texture(&game->man.sheetTextureDoubleJump);
    destroy_texture(&game->man.sheetTextureWallJump);
    destroy_texture(&game->appearanceEffect);
    destroy_texture(&game->disappearanceEffect);
    destroy_texture(&game->shadowEffect);
    destroy_texture(&game->dustParticle);
    game->assetFlags.sharedAssetsLoaded = false;
}

// ---------------------------------------------------------------------------
// Arcade mode
// ---------------------------------------------------------------------------
static bool arcade_assets_complete(const GameState *game)
{
    if (!game) return false;
    SDL_Texture *const textures[] = {
        game->bossTexture, game->bulletTexture, game->secondBulletTexture,
        game->secondPlayer.sheetTextureIdle,
        game->secondPlayer.sheetTextureRun,
        game->secondPlayer.sheetTextureJump,
        game->secondPlayer.sheetTextureFall,
        game->secondPlayer.sheetTextureHit,
        game->secondPlayer.sheetTextureDoubleJump,
        game->secondPlayer.sheetTextureWallJump,
        game->enemy.sheetTextureRun, game->enemy.sheetTextureRun2,
        game->sheetTextureBack, game->cloud1.sheetTextureCloud1,
        game->cloud2.sheetTextureCloud2, game->cloud3.sheetTextureCloud3,
        game->cloud4.sheetTextureCloud4, game->cloud5.sheetTextureCloud5,
        game->cloud6.sheetTextureCloud6, game->cloud7.sheetTextureCloud7,
        game->cloud8.sheetTextureCloud8, game->sheetTextureSun,
        game->sheetTextureBack2, game->train.textureTrain,
        game->brick_block, game->copper_block,
    };
    const bool texturesComplete =
        texture_group_complete(textures,
                               sizeof(textures) / sizeof(textures[0]));
    const bool audioComplete =
        !audio_assets_output_available() ||
        (game->assetFlags.sharedAudioAssetsLoaded &&
         game->audio.menuMusic != NULL && game->audio.select != NULL &&
         game->audio.jump != NULL && game->audio.arcadeMusic != NULL &&
         game->audio.shoot != NULL && game->audio.damage != NULL &&
         game->audio.kick != NULL);
    return texturesComplete && audioComplete;
}

bool arcade_assets_load(GameState *game)
{
    if (!game || !game->app.renderer) return false;
    // Checked before the arcadeAssetsLoaded early-return (not after) so that
    // if the shared bucket is ever unloaded independently of the Arcade
    // flag, calling this again still re-establishes it -- arcadeAssetsLoaded
    // alone does not imply sharedAssetsLoaded is still true.
    if (!shared_assets_load(game))
    {
        return false;
    }

    if (arcade_assets_complete(game))
    {
        game->assetFlags.arcadeAssetsLoaded = true;
        return true;
    }
    arcade_assets_unload(game);

    bool ok = true;
    ok = ok && audio_assets_load_arcade(game);
    ok = ok && load_texture(game->app.renderer, "./resource/images/enemies/boss.png", &game->bossTexture);
    ok = ok && load_texture(game->app.renderer, "./resource/images/bullets/bullet_fireball.png", &game->bulletTexture);
    ok = ok && load_texture(game->app.renderer, "./resource/images/bullets/bullet_fireball2.png", &game->secondBulletTexture);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/idle.png", &game->secondPlayer.sheetTextureIdle);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/run.png", &game->secondPlayer.sheetTextureRun);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/jump.png", &game->secondPlayer.sheetTextureJump);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/fall.png", &game->secondPlayer.sheetTextureFall);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/hit.png", &game->secondPlayer.sheetTextureHit);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/double_jump.png", &game->secondPlayer.sheetTextureDoubleJump);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/pink_man/wall_jump.png", &game->secondPlayer.sheetTextureWallJump);

    // manFrames[0] is loaded by shared_assets_load() above -- both modes
    // load the byte-for-byte identical path (see docs/game-session-lifecycle.md),
    // so it belongs in the shared bucket, not here.
    ok = ok && load_texture(game->app.renderer, "./resource/images/enemies/shroom_Run(32x32).png", &game->enemy.sheetTextureRun);
    ok = ok && load_texture(game->app.renderer, "./resource/images/enemies/turtle_Run(44x26).png", &game->enemy.sheetTextureRun2);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/Sunset.png", &game->sheetTextureBack);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud01.png", &game->cloud1.sheetTextureCloud1);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud02.png", &game->cloud2.sheetTextureCloud2);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud03.png", &game->cloud3.sheetTextureCloud3);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud04.png", &game->cloud4.sheetTextureCloud4);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud05.png", &game->cloud5.sheetTextureCloud5);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud06.png", &game->cloud6.sheetTextureCloud6);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud07.png", &game->cloud7.sheetTextureCloud7);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_cloud08.png", &game->cloud8.sheetTextureCloud8);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/Sunset_sun.png", &game->sheetTextureSun);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_front.png", &game->sheetTextureBack2);
    ok = ok && load_texture(game->app.renderer, "./resource/images/background/sunset_train.bmp", &game->train.textureTrain);
    ok = ok && load_texture(game->app.renderer, "./resource/images/terrain/Brick_block.png", &game->brick_block);
    ok = ok && load_texture(game->app.renderer, "./resource/images/terrain/Copper_block.png", &game->copper_block);

    if (!ok)
    {
        arcade_assets_unload(game);
        return false;
    }

    game->assetFlags.arcadeAssetsLoaded = true;
    return true;
}

void arcade_assets_unload(GameState *game)
{
    if (!game) return;
    audio_assets_unload_arcade(game);
    destroy_texture(&game->bossTexture);
    destroy_texture(&game->bulletTexture);
    destroy_texture(&game->secondBulletTexture);
    destroy_texture(&game->secondPlayer.sheetTextureIdle);
    destroy_texture(&game->secondPlayer.sheetTextureRun);
    destroy_texture(&game->secondPlayer.sheetTextureJump);
    destroy_texture(&game->secondPlayer.sheetTextureFall);
    destroy_texture(&game->secondPlayer.sheetTextureHit);
    destroy_texture(&game->secondPlayer.sheetTextureDoubleJump);
    destroy_texture(&game->secondPlayer.sheetTextureWallJump);
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
    // manFrames[0] intentionally left alone -- owned by shared_assets_unload()
    // (see docs/game-session-lifecycle.md).
    game->assetFlags.arcadeAssetsLoaded = false;
}

static void reset_player_runtime(Man *player, float x, float y, bool active)
{
    if (!player) return;

    player->x = x;
    player->y = y;
    player->prevX = x;
    player->prevY = y;
    player->dx = 0.0f;
    player->dy = 0.0f;
    player->onLedge = 0;
    player->isDead = 0;
    player->lives = active ? 3 : 0;
    player->visible0 = active ? 1 : 0;
    player->coyoteTicksRemaining = 0;
    player->jumpKeyHeldLastTick = false;
    player->airJumpsRemaining = 0;
    player->doubleJumpAnimationTicks = 0;
    player->damageInvulnerabilityTicks = 0;
    player->animFrame = 0;
    player->animFrameSecond = 0;
    player->currentSpriteIdle = 0;
    player->currentSpriteRun = 0;
    player->currentSpriteRun2 = 0;
    player->currentSpriteJump = 0;
    player->currentSpriteJump2 = 0;
    player->facingLeft = 0;
    player->slowingDown = 0;
    player->animation = (AnimationPlayer){.state = ANIMATION_STATE_IDLE};
    player->hitFlashTicks = 0;
}

static void deactivate_enemy_runtime(Enemies *enemy, float x, float y)
{
    if (!enemy) return;

    enemy->x = x;
    enemy->y = y;
    enemy->prevX = x;
    enemy->prevY = y;
    enemy->dx = 0.0f;
    enemy->dy = 0.0f;
    enemy->onLedge = 0;
    enemy->isdead = 0;
    enemy->visible = 0;
    enemy->countShots = 0;
    enemy->currentSpriteRun = 0;
    enemy->currentSpriteRun2 = 0;
    enemy->animation = (AnimationPlayer){.state = ANIMATION_STATE_IDLE};
    enemy->hitFlashTicks = 0;
}

static void reset_session_transients(GameState *game)
{
    game->input = (InputState){0};
    game->events = (GameEventQueue){0};
    input_controller_sync_jump_edge(&game->app);
    game->shotCount = 0;
    game->shotCountMultiplayer = 0;
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        removeBullet(game, index);
        removeSecondBullet(game, index);
    }
    game->CurrentSheetBullet = 0;
    game->CurrentSheetBullet2 = 0;
    game->CurrentSheetBoss = 0;
    game->CurrentSpriteBack = 0;
    game->CurrentSpriteBack2 = 0;
    game->CurrentSpriteBack3 = 0;
    game->renderAlpha = 0.0f;
    game->perfProjectileActiveSamples = 0;
    game->perfProjectileTargetChecks = 0;
    game->simulationOnly = false;
}

void arcade_session_reset(GameState *game, GameMode mode)
{
    if (!game) return;
    game->multiPlayer = mode;
    combat_feedback_reset(game);
    reset_session_transients(game);

    game->kills_score = 0;
    game->kills_score_multi = 0;
    arcade_waves_reset(game);

    reset_player_runtime(&game->man, 320.0f - 40.0f, 240.0f - 40.0f, true);
    reset_player_runtime(&game->secondPlayer, 1000.0f, 240.0f - 40.0f,
                         mode == GAME_MODE_MULTIPLAYER);
    game->statusState = STATUS_STATE_LIVES;

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
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        deactivate_enemy_runtime(&game->enemyValues[i], 640.0f, 1000.0f);
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        deactivate_enemy_runtime(&game->smartEnemies[i], 640.0f, 1000.0f);
    }
    for (int i = 0; i < 2; i++)
    {
        deactivate_enemy_runtime(&game->boss[i], 1100.0f, 1000.0f);
    }

    game->time = 0;
    game->deathCountdown = -1;
    combat_feedback_trigger_player_spawn(game, 0);
    if (game->multiPlayer) combat_feedback_trigger_player_spawn(game, 1);

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
    sync_render_transforms(game);
}

// ---------------------------------------------------------------------------
// Runner mode
// ---------------------------------------------------------------------------
static bool runner_assets_complete(const GameState *game)
{
    if (!game || game->fon == NULL || game->star == NULL) return false;
    for (int index = 1; index < 12; index++)
    {
        if (game->manFrames[index] == NULL) return false;
    }
    for (int index = 0; index < 12; index++)
    {
        if (game->secondPlayerFrames[index] == NULL) return false;
    }
    return !audio_assets_output_available() ||
           (game->assetFlags.sharedAudioAssetsLoaded &&
            game->audio.menuMusic != NULL && game->audio.select != NULL &&
            game->audio.jump != NULL && game->audio.runnerMusic != NULL);
}

bool runner_assets_load(GameState *game)
{
    if (!game || !game->app.renderer) return false;
    // See arcade_assets_load()'s comment -- checked before the
    // runnerAssetsLoaded early-return for the same reason.
    if (!shared_assets_load(game))
    {
        return false;
    }

    if (runner_assets_complete(game))
    {
        game->assetFlags.runnerAssetsLoaded = true;
        return true;
    }
    runner_assets_unload(game);

    bool ok = true;
    ok = ok && audio_assets_load_runner(game);
    ok = ok && load_texture(game->app.renderer, "./resource/images/backgrounds/fon.png", &game->fon);
    ok = ok && load_texture(game->app.renderer, "./resource/images/traps/spike_head.png", &game->star);

    // manFrames[0] is loaded by shared_assets_load() above -- see
    // arcade_assets_load()'s comment.
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)1.png", &game->manFrames[1]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)2.png", &game->manFrames[2]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)3.png", &game->manFrames[3]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)4.png", &game->manFrames[4]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)5.png", &game->manFrames[5]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)6.png", &game->manFrames[6]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)7.png", &game->manFrames[7]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)8.png", &game->manFrames[8]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)9.png", &game->manFrames[9]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)10.png", &game->manFrames[10]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/main_hero/run/V_g(rn)11.png", &game->manFrames[11]);

    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)0.png", &game->secondPlayerFrames[0]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)1.png", &game->secondPlayerFrames[1]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)2.png", &game->secondPlayerFrames[2]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)3.png", &game->secondPlayerFrames[3]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)4.png", &game->secondPlayerFrames[4]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)5.png", &game->secondPlayerFrames[5]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)6.png", &game->secondPlayerFrames[6]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)7.png", &game->secondPlayerFrames[7]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)8.png", &game->secondPlayerFrames[8]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)9.png", &game->secondPlayerFrames[9]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)10.png", &game->secondPlayerFrames[10]);
    ok = ok && load_texture(game->app.renderer, "./resource/images/secondplayer/run/V_g(rn)11.png", &game->secondPlayerFrames[11]);

    if (!ok)
    {
        runner_assets_unload(game);
        return false;
    }

    game->assetFlags.runnerAssetsLoaded = true;
    return true;
}

void runner_assets_unload(GameState *game)
{
    if (!game) return;
    audio_assets_unload_runner(game);
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
    // manFrames[0] intentionally left alone -- owned by shared_assets_unload()
    // (see docs/game-session-lifecycle.md).
    game->assetFlags.runnerAssetsLoaded = false;
}

// Death-state fields (isDead, deathCountdown, and the players' position/velocity these two
// fields gate) are reset below to their idle values -- see docs/runner-death-lifecycle.md for
// the full lifecycle these fields drive.
void runner_session_reset(GameState *game, GameMode mode)
{
    if (!game) return;
    game->multiPlayer = mode;
    combat_feedback_reset(game);
    reset_session_transients(game);

    game->x_score = 0;

    reset_player_runtime(&game->man, 320.0f - 240.0f, 240.0f - 240.0f, true);
    reset_player_runtime(&game->secondPlayer, 100.0f, 80.0f,
                         mode == GAME_MODE_MULTIPLAYER);
    game->gameLives = 3;
    game->statusState = STATUS_STATE_LIVES;

    game->time = 0;
    game->scrollX = 0;
    game->deathCountdown = -1;

    game->iter = 0; // deprecated; segment streaming owns Runner extension.
    runner_segments_reset(game);
    combat_feedback_trigger_player_spawn(game, 0);
    if (game->multiPlayer) combat_feedback_trigger_player_spawn(game, 1);
    sync_render_transforms(game);
}
