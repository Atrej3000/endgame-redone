#include "header.h"

void loadGame(GameState *game)
{
    if (!game->arcadeAssetsLoaded)
    {
    SDL_Surface *surface = NULL;

    //Load images and create rendering textures from them
    // surface = IMG_Load("./resource/images/traps/spike_head.png");
    // if (surface == NULL)
    // {
    //     printf("Cannot find star.png!\n\n");
    //     SDL_Quit();
    //     exit(1);
    // }

    // game->star = SDL_CreateTextureFromSurface(game->renderer, surface);
    // SDL_FreeSurface(surface);

    //music
    game->battleMus = Mix_LoadMUS("resource/sounds/battleMus.mp3");
    //Mix_PlayMusic(game->battleMus, -1);

    //multiplayer
    surface = IMG_Load("./resource/images/enemies/boss.png");
    if (surface == NULL)
    {
        printf("Cannot find boss.png");
        SDL_Quit();
        exit(1);
    }
    game->bossTexture = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/backgrounds/mult.png");
    game->mult = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find menu1.png");
        SDL_Quit();
        exit(1);
    }

    //BULLET
    surface = IMG_Load("./resource/images/bullets/bullet_fireball.png");
    if (surface == NULL)
    {
        printf("Cannot found a bullet \n\n");
        SDL_Quit();
        exit(1);
    }
    game->bulletTexture = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    // SECOND BULLET
    surface = IMG_Load("./resource/images/bullets/bullet_fireball2.png");
    game->secondBulletTexture = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find second bullet \n");
        SDL_Quit();
        exit(1);
    }

    //SECOND HERO RUN
    surface = IMG_Load("./resource/images/secondplayer/run/Run_(32x32).png");
    if (surface == NULL)
    {
        printf("Cannot find secondplayer");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayer.sheetTextureRun2 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //SECOND HERO JUMP
    surface = IMG_Load("./resource/images/secondplayer/jump/Double_Jump_(32x32).png");
    if (surface == NULL)
    {
        printf("Cannot find secondplayer");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayer.sheetTextureJump2 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //leaderbord
    surface = IMG_Load("./resource/images/backgrounds/leaders.jpg");
    game->leaders = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find menu1.png");
        SDL_Quit();
        exit(1);
    }

    //pause_________________________________________________________________________
    surface = IMG_Load("./resource/images/backgrounds/pause.jpg");
    game->pause = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find pause\n.png");
        SDL_Quit();
        exit(1);
    }

    //Load images and create rendering textures from them
    surface = IMG_Load("./resource/images/traps/spike_head.png");
    if (surface == NULL)
    {
        printf("Cannot find star.png!\n\n");
        SDL_Quit();
        exit(1);
    }

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)0.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)0.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[0] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //HERO RUN
    surface = IMG_Load("./resource/images/main_hero/run/Run_(32x32).png");
    if (surface == NULL)
    {
        printf("Cannot find Run_(32x32).png");
        SDL_Quit();
        exit(1);
    }
    game->man.sheetTextureRun = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //HERO JUMP
    surface = IMG_Load("./resource/images/main_hero/jump/Double_Jump_(32x32).png");
    if (surface == NULL)
    {
        printf("Cannot find Double_Jump_(32x32).png");
        SDL_Quit();
        exit(1);
    }
    game->man.sheetTextureJump = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    // //HERO ATTACK
    // surface = IMG_Load("./resource/images/main_hero/attack1/Attack1_(128x32).png");
    // if (surface == NULL)
    // {
    //     printf("Cannot find Attack1_(128x32).png");
    //     SDL_Quit();
    //     exit(1);
    // }
    // game->man.sheetTextureAttack1 = SDL_CreateTextureFromSurface(game->renderer, surface);
    // SDL_FreeSurface(surface);

    //HERO DEATH
    surface = IMG_Load("./resource/images/main_hero/death/fire.png");
    game->death = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //ENEMIES
    surface = IMG_Load("./resource/images/enemies/shroom_Run(32x32).png");
    if (surface == NULL)
    {
        printf("Cannot found enemy \n\n");
        SDL_Quit();
        exit(1);
    }
    game->enemy.sheetTextureRun = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/enemies/turtle_Run(44x26).png");
    if (surface == NULL)
    {
        printf("Cannot found enemy \n\n");
        SDL_Quit();
        exit(1);
    }
    game->enemy.sheetTextureRun2 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //BACKGROUND
    surface = IMG_Load("./resource/images/background/Sunset.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->sheetTextureBack = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //CLOUDS
    surface = IMG_Load("./resource/images/background/sunset_cloud01.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud1.sheetTextureCloud1 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud02.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud2.sheetTextureCloud2 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud03.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud3.sheetTextureCloud3 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud04.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud4.sheetTextureCloud4 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud05.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud5.sheetTextureCloud5 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud06.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud6.sheetTextureCloud6 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud07.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud7.sheetTextureCloud7 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/background/sunset_cloud08.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset \n\n");
        SDL_Quit();
        exit(1);
    }
    game->cloud8.sheetTextureCloud8 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //SUN
    surface = IMG_Load("./resource/images/background/Sunset_sun.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset sun \n\n");
        SDL_Quit();
        exit(1);
    }
    game->sheetTextureSun = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //BUILDINGS
    surface = IMG_Load("./resource/images/background/Sunset_front.png");
    if (surface == NULL)
    {
        printf("Cannot found sunset_front \n\n");
        SDL_Quit();
        exit(1);
    }
    game->sheetTextureBack2 = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //TRAIN
    surface = IMG_Load("./resource/images/background/sunset_train.bmp");
    if (surface == NULL)
    {
        printf("Cannot found train \n\n");
        SDL_Quit();
        exit(1);
    }
    game->train.textureTrain = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //TERRAIN
    surface = IMG_Load("./resource/images/terrain/clay_block.png");
    game->brick = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/terrain/brick_block.png");
    game->brick_block = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/terrain/copper_block.png");
    game->copper_block = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    game->font = TTF_OpenFont("./resource/text/Fonts/crazy-pixel.ttf", 48);
    if (!game->font)
    {
        printf("Cannot find font file");
        SDL_Quit();
        exit(1);
    }

    //    surface = IMG_Load("./resource/images/bullets/bullet.png");
    //    if (surface == NULL)
    //    {
    //        printf("Cannot found a bullet \n\n");
    //        SDL_Quit();
    //        exit(1);
    //    }
    //    game->bulletTexture = SDL_CreateTextureFromSurface(game->renderer, surface);
    //    SDL_FreeSurface(surface);
    game->arcadeAssetsLoaded = true;
    }

    if (game->label) {
        SDL_DestroyTexture(game->label);
    }
    game->label = NULL;

    // man init
    game->kills_score = 0;
    game->kills_score_multi = 0;
    //game->kills_list = {0};

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

    //TRAIN init
    game->train.x = 0;
    game->train.y = 440;

    //CLOUDS INIT
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

    //Game lives
    game->gameLives = 10;
    game->tempScore = 0;
    game->shotCount = 0;
    game->shotCountMultiplayer = 0;

    game->multiPlayer = 1;

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

    // enemy init
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        game->enemyValues[i].x = 640;
        game->enemyValues[i].y = 1000;
        game->enemyValues[i].dy = 0;
        game->enemyValues[i].dx = 0;
    }
    // INIT SMART ENEMIES
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        game->smartEnemies[i].x = 640;
        game->smartEnemies[i].y = 1000;
        game->smartEnemies[i].dy = 0;
        game->smartEnemies[i].dx = 0;
        game->smartEnemies[i].visible = 1;
        game->smartEnemies[i].countShots = 0;
    }

    // INIT BOSS
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].x = 1100;
        game->boss[i].y = 1000;
        game->boss[i].dy = 0;
        game->boss[i].dx = 0;
        game->boss[i].countShots = 0;
        game->boss[i].visible = 1;
    }

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        game->bullets[i] = NULL;
    }

    if (game->multiPlayer)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            game->secondBullets[i] = NULL;
        }
    }

    init_status_lives(game);

    game->time = 0;

    //game->scrollX = 0;
    game->deathCountdown = -1;

    // Init enemies

    //init stars
    // for (int i = 0; i < 100; i++)
    // {
    //     game->stars[i].x = 320 +random()%38400;
    //     game->stars[i].y = random()%HEIGHT;
    // }

    //init ledges
    for (int i = 0; i < 12; i++)
    {
        game->ledges[i].w = 48;
        game->ledges[i].h = 48;
        game->ledges[i].x = i * 48;
        game->ledges[i].y = 672;
    }
    // for (int i = 7; i < 14; i++)
    // {
    //     game->ledges[i].w = 64;
    //     game->ledges[i].h = 64;
    //     game->ledges[i].x = (i+4)*64;
    //     game->ledges[i].y = 656;
    // }

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
        {
            game->ledges[i].w = 48;
            game->ledges[i].h = 48;
            game->ledges[i].x = (i - 73) * 48;
            game->ledges[i].y = 0;
        }
    }
    game->ledges[99].x = 1000;
    game->ledges[99].y = 1000;
}

void loadGame2(GameState *game)
{
    if (!game->runnerAssetsLoaded)
    {
    SDL_Surface *surface = NULL;
    //game->menu_status = 0;

    //    //меню_________________________________________________________________________
    //    surface = IMG_Load("./resource/images/backgrounds/menu1.png");
    //    game->menu = SDL_CreateTextureFromSurface(game->renderer, surface);
    //    SDL_FreeSurface(surface);
    //    if (surface == NULL)
    //    {
    //        printf("Cannot find menu1.png");
    //        SDL_Quit();
    //        exit(1);
    //    }

    //music
    game->runnerMus = Mix_LoadMUS("resource/sounds/runnerMus.mp3");
    //Mix_PlayMusic(game->runnerMus, -1);

    //multiplayer
    surface = IMG_Load("./resource/images/backgrounds/mult.png");
    if (surface == NULL)
    {
        printf("Cannot find menu1.png");
        SDL_Quit();
        exit(1);
    }
    game->mult = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //leaderbord
    surface = IMG_Load("./resource/images/backgrounds/leaders.jpg");
    game->leaders = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find menu1.png");
        SDL_Quit();
        exit(1);
    }

    //fon
    surface = IMG_Load("./resource/images/backgrounds/fon.png");
    game->fon = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find menu1.png");
        SDL_Quit();
        exit(1);
    }
    //pause_________________________________________________________________________
    surface = IMG_Load("./resource/images/backgrounds/pause.jpg");
    game->pause = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
    if (surface == NULL)
    {
        printf("Cannot find pause\n.png");
        SDL_Quit();
        exit(1);
    }

    //Load images and create rendering textures from them
    surface = IMG_Load("./resource/images/traps/spike_head.png");
    if (surface == NULL)
    {
        printf("Cannot find star.png!\n\n");
        SDL_Quit();
        exit(1);
    }
    game->star = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //HERO RUN
    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)0.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)0.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[0] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)1.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)1.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[1] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)2.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)2.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[2] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)3.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)3.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[3] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)4.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)4.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[4] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)5.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)5.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[5] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)6.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)6.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[6] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)7.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)7.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[7] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)8.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)8.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[8] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)9.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)9.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[9] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)10.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)10.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[10] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/run/V_g(rn)11.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)11.png");
        SDL_Quit();
        exit(1);
    }
    game->manFrames[11] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    //SECOND HERO RUN

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)0.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)0.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[0] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)1.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)1.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[1] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)2.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)2.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[2] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)3.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)3.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[3] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)4.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)4.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[4] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)5.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)5.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[5] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)6.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)6.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[6] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)7.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)7.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[7] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)8.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)8.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[8] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)9.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)9.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[9] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)10.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)10.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[10] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/secondplayer/run/V_g(rn)11.png");
    if (surface == NULL)
    {
        printf("Cannot find V_g(rn)11.png");
        SDL_Quit();
        exit(1);
    }
    game->secondPlayerFrames[11] = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/terrain/Clay_block.png");
    game->brick = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./resource/images/main_hero/death/fire.png");
    game->death = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    game->font = TTF_OpenFont("./resource/text/Fonts/crazy-pixel.ttf", 48);
    if (!game->font)
    {
        printf("Cannot find font file");
        SDL_Quit();
        exit(1);
    }
    game->runnerAssetsLoaded = true;
    }

    if (game->label) {
        SDL_DestroyTexture(game->label);
    }
    game->label = NULL;

    game->x_score = 0;
    //    for(game->x_i = 0; game->x_i < 10; game->x_i++) {
    //        game->x_list[game->x_i] = 0;
    //    }
    //    game->x_i = 0;

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

    init_status_lives(game);

    game->time = 0;
    game->scrollX = 0;
    game->deathCountdown = -1;

    //init ledges for runner
    int bulb = 700;
    int i;
    game->iter = 1;
    for (i = 0; i < 100; i++)
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

        //init stars
        game->stars[i].baseX = game->ledges[i].x - random_sign(1, 1) * random() % 120;
        game->stars[i].baseY = game->ledges[i].y - random() % 120;
        game->stars[i].mode = random() % 2;
        game->stars[i].phase = 2 * 3.14 * (random() % 360) / 360.0f;

        //  game->stars[i + 100].x = 320 +random()%38400;
        //  game->stars[i + 100].y = random()%720;
    }
    game->ledges[99].x = 00;
    game->ledges[99].y = 100;

    //init ledges for battle

    //walls and floor
    /*   int i = 0;
       for (i = 0; i < 20; i++) {
           game->ledges[i].w = 64;
           game->ledges[i].h = 64;
           game->ledges[i].x = i * 64;
           game->ledges[i].y = 656;
       }
       for (i = 20; i < 32; i++) {
           game->ledges[i].w = 64;
           game->ledges[i].h = 64;
           game->ledges[i].x = 0;
           game->ledges[i].y = (i - 20) * 64;
       }
       for (i = 32; i < 44; i++) {
           game->ledges[i].w = 64;
           game->ledges[i].h = 64;
           game->ledges[i].x = 1216;
           game->ledges[i].y = (i - 32) * 64;
       }


       for (i = 44; i < 47; i++) { //1st (highest) layer
           game->ledges[i].w = 128;
           game->ledges[i].h = 32;
           game->ledges[i].x = (i - 43) * 352 - 128;
           game->ledges[i].y = 40;
       }
       for (i = 47; i < 51; i++) { //2nd layer
           game->ledges[i].w = 128;
           game->ledges[i].h = 32;
           game->ledges[i].x = (i - 46) * 340 - 275;
           game->ledges[i].y = 280;
       }
       for (i = 51; i < 53; i++) { //3rd layer
           game->ledges[i].w = 350;
           game->ledges[i].h = 32;
           game->ledges[i].x = (i - 50) * 540 - 350;
           game->ledges[i].y = 440;
       }
       game->ledges[i].w = 128; //lowest block
       game->ledges[i].h = 64;
       game->ledges[i].x = 580;
       game->ledges[i].y = 590;
   */

    /*for (i  = 44; i < 50; i++) {
        game->ledges[i].w = 80 ;
        game->ledges[i].h = 40;
        game->ledges[i].x = 80 + 100 * (i - 44); //+ random_sign(1, 50) * (i - 44);
        game->ledges[i].y = 100 + 100 * (i - 42); //+ random_sign(1, 50) * (i - 42);
        while (game->ledges[i].y >= 700)
            game->ledges[i].y -= 50;
        while (game->ledges[i].y <= 000)
            game->ledges[i].y += 50;
    }*/

    //init stars
    //    for (i = 0; i < 20; i++) {
    //        game->stars[i].x = random() % 1216;
    //        game->stars[i].y = random() % 720;
    //    }
}
