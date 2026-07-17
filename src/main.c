#include "header.h"

int main()
{
    GameState *gameState =  (GameState*)calloc(1, sizeof(GameState));
    SDL_Window *window;     //Declare a window
    SDL_Renderer *renderer; //Declare a renderer

    SDL_Init(SDL_INIT_VIDEO); //Initialize SDL2

    srandom((int)time(NULL));

    //Create an application window  with the following settings:
    window = SDL_CreateWindow("Game Window",           //window title
                              SDL_WINDOWPOS_UNDEFINED, //initial x position
                              SDL_WINDOWPOS_UNDEFINED, //initial y position
                              WIDTH,                    //width, in pixels
                              HEIGHT,                     //height, in pixels
                              0                        //flags
    );
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gameState->renderer = renderer;

    TTF_Init(); // init font system

    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096); //initialize sound

    int done0 = 0;
    int done = 0;

    gameState->x_score = 0;
    for(gameState->x_i = 0; gameState->x_i < 25; gameState->x_i++) {
        gameState->x_list[gameState->x_i] = 0;
        //gameState.x_names[gameState.x_i] = "";
    }

    load_menu0(gameState);

    while(!done0) {
        switch (gameState->menu0_status) {
            //___________________________________________________________________________menu0
            case 0:
                menu0_events(gameState);
                doRender_menu0(renderer, gameState);
                break;

                //___________________________________________________________________________battle
            case 1:
                done = 0;
                load_menu1(gameState);
                while (!done) {
                    switch (gameState->menu_status) {
                        //menu
                        case 0:
                            loadGame(gameState);
                            menu_events(gameState);
                            doRender_menu1(renderer, gameState);
                            break;
                            //battle game
                        case 1:
                            //Check for events
                            process(gameState);
                            collisionDetect(gameState);
                            doRender(renderer, gameState);
                            processEvents(window, gameState);
                            break;
                            //multiplayer
                        case 2:
                            process(gameState);
                            collisionDetect(gameState);
                            doRender(renderer, gameState);
                            processEvents(window, gameState);
                            break;
                            //leaderboard
                        case 3:
                            processEvents(window, gameState);
                            doRender_leaderboard(renderer, gameState);
                            //leader_events(&gameState);
                            break;
                            //exit
                        case 4:
                            done = 1;
                            break;
                            //Pause
                        case 5:
                            doRender_pause(renderer, gameState);
                            pause_events(gameState);
                            break;
                        default:
                            gameState->menu_status = 0;
                            break;
                    }
                }
                break;
                //___________________________________________________________________________runner
            case 2:
                done = 0;
                load_menu2(gameState);
                while (!done) {
                    switch (gameState->menu_status) {
                        //menu
                        case 0:
                            loadGame2(gameState);
                            menu_events(gameState);//___________________no changes
                            doRender_menu2(renderer, gameState);//_______no changes
                            break;
                            //runner
                        case 1:
                            //Check for events
                            process2(gameState);
                            collisionDetect2(gameState);
                            doRender2(renderer, gameState);
                            processEvents2(window, gameState);
                            break;
                            //multiplayer
                        case 2:
                            process2(gameState);
                            collisionDetect2(gameState);
                            doRender2(renderer, gameState);
                            processEvents2(window, gameState);
                            break;
                            //leaderboard
                        case 3:
                            //score
//                            for(int i = 0; i < 10; i++) {
//                                printf("%d ", gameState.x_list[i]);
//                            }
                            processEvents2(window, gameState);
                            doRender_leaderboard2(renderer, gameState);
                           /* for(int i = 0; i < 25; i++) {
                                init_status_x_list(&gameState);
                                draw_status_x_list(&gameState);
                            }*/
                            //leader_events(&gameState);
                            break;
                            //exit
                        case 4:
                            done = 1;
                            break;
                            //Pause
                        case 5:
                            doRender_pause(renderer, gameState);
                            pause_events(gameState);
                            break;
                        default:
                            gameState->menu_status = 0;
                            break;
                    }
                }
                break;
                //___________________________________________________________________________exit
            case 3:
                done0 = 1;
                //done0 = 1;
                break;
            default:
                done0 = 0;
                break;
        }
        if (done == 1) {
            //done0 = 1;
            gameState->menu0_status = 0;
            done = 0;
        }
    }

    //Shutdown game and umload all memory
    //SDL_DestroyTexture(gameState.star);
    //printf("EXIT\n");
    /*SDL_DestroyTexture(gameState.manFrames[0]);
    SDL_DestroyTexture(gameState.man.sheetTextureIdle);
    SDL_DestroyTexture(gameState.man.sheetTextureRun);
    SDL_DestroyTexture(gameState.man.sheetTextureJump);
    SDL_DestroyTexture(gameState.enemy.sheetTextureRun);
    SDL_DestroyTexture(gameState.bulletTexture);
    // SDL_DestroyTexture(gameState.man.sheetTextureAttack1);
    SDL_DestroyTexture(gameState.brick);
    if (gameState.label != NULL)
        SDL_DestroyTexture(gameState.label);
    TTF_CloseFont(gameState.font);
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        removeBullet(&gameState, i);
    }*/
    // SDL_DestroyTexture(gameState->background);
    // SDL_DestroyTexture(gameState->bossTexture);
    // SDL_DestroyTexture (gameState->star);
    // for (int i = 0; i < 11; i++)
    // {
    // SDL_DestroyTexture (gameState->manFrames[i]);
    // SDL_DestroyTexture (gameState->secondPlayerFrames[i]);
    // }
    // SDL_DestroyTexture (gameState->secondPlayerImage);
    // SDL_DestroyTexture (gameState->brick);
    // SDL_DestroyTexture (gameState->menu0);
    // SDL_DestroyTexture (gameState->menu1);
    // SDL_DestroyTexture (gameState->menu2);
    // SDL_DestroyTexture (gameState->mult);
    // SDL_DestroyTexture (gameState->leaders);
    // SDL_DestroyTexture (gameState->fon);
    // SDL_DestroyTexture (gameState->pause);
    // SDL_DestroyTexture (gameState->label);
    // SDL_DestroyTexture (gameState->labelMultiplayer);
    // SDL_DestroyTexture (gameState->death);
    // SDL_DestroyTexture (gameState->enemyFrame);
    // SDL_DestroyTexture (gameState->brick_block);
    // SDL_DestroyTexture (gameState->copper_block);
    // SDL_DestroyTexture (gameState->background);
    // SDL_DestroyTexture (gameState->bulletTexture);
    // SDL_DestroyTexture (gameState->secondBulletTexture);
    // SDL_DestroyTexture (gameState->enemyTexture2);
    // SDL_DestroyTexture (gameState->bossTexture);

    //Close and destroy the window
    SDL_DestroyWindow(window);
    free(gameState);
    SDL_DestroyRenderer(renderer);

    //Clean up
    TTF_Quit();
    SDL_Quit();
    //printf("END\n");
    return 0;
}
