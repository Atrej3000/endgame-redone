#include "header.h"
#include "app.h"

int main()
{
    GameState *gameState = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&gameState, &window, &renderer))
    {
        return 1;
    }

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

    app_shutdown(&gameState, &window, &renderer);
    return 0;
}
