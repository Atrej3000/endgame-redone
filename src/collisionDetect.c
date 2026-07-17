#include "header.h"

void collisionDetect(GameState *game)
{

    // // //DEATH TOUCH
    // // for (int i  = 0; i < 100; i++)
    // // {
    // //     if (collide2d(game->man.x, game->man.y, game->stars[i].x, game->stars[i].y, 50, 50, 50, 50))
    // //     {
    // //         game->man.isDead = 1;
    // //     }
    // // }

    // ENEMY COLISION DETECT
/*
    for (int j = 0; j < NUM_ENEMIES; j++)
    {
        game->man.w = 48;
        game->man.h = 48;
        float mw = 48, mh = 48;
        float mx = game->enemyValues[j].x, my = game->enemyValues[j].y;
        float bx = game->man.x, by = game->man.y, bw = game->man.w, bh = game->man.h;

        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        {
            //are we bumping our head?
            if (my < by + bh && my > by && game->enemyValues[j].dy < 0)
            {
                //correct y
                game->enemyValues[j].y = by + bh;
                my = by + bh;

                //bumped our head, stop any jump velocity
                game->enemyValues[j].dy = 0;
                //game->enemy.onLedge = 1;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->enemyValues[j].dy > 0)
            {
                //correct y
                game->enemyValues[j].y = by - mh;
                my = by - mh;

                //landed on this ledge, stop any jump velocity
                game->enemyValues[j].dy = 0;
                //game->enemy.onLedge = 1;
            }
        }

        if (my + mh > by && my < by + bh)
        {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->enemyValues[j].dx < 0)
            {
                //correct x
                game->enemyValues[j].x = bx + bw;
                mx = bx + bw;

                game->enemyValues[j].dx = 0;
            }
                //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->enemyValues[j].dx > 0)
            {
                //correct x
                game->enemyValues[j].x = bx - mw;
                mx = bx - mw;

                game->enemyValues[j].dx = 0;
            }
        }
    }*/

    for (int j = 0; j < NUM_ENEMIES; j++)
    {
        for (int i = 0; i < 100; i++)
        {

            float mw = 48, mh = 48;
            float mx = game->enemyValues[j].x, my = game->enemyValues[j].y;
            float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

            if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
            {
                //are we bumping our head?
                if (my < by + bh && my > by && game->enemyValues[j].dy < 0)
                {
                    //correct y
                    game->enemyValues[j].y = by + bh;
                    my = by + bh;

                    //bumped our head, stop any jump velocity
                    game->enemyValues[j].dy = 0;
                    //game->enemy.onLedge = 1;
                }
            }
            if (mx + mw > bx && mx < bx + bw)
            {
                //are we landing on the ledge
                if (my + mh > by && my < by && game->enemyValues[j].dy > 0)
                {
                    //correct y
                    game->enemyValues[j].y = by - mh;
                    my = by - mh;

                    //landed on this ledge, stop any jump velocity
                    game->enemyValues[j].dy = 0;
                    //game->enemy.onLedge = 1;
                }
            }

            if (my + mh > by && my < by + bh)
            {
                //rubbing against right edge
                if (mx < bx + bw && mx + mw > bx + bw && game->enemyValues[j].dx < 0)
                {
                    //correct x
                    game->enemyValues[j].x = bx + bw;
                    mx = bx + bw;

                    game->enemyValues[j].dx = 0;
                }
                    //rubbing against left edge
                else if (mx + mw > bx && mx < bx && game->enemyValues[j].dx > 0)
                {
                    //correct x
                    game->enemyValues[j].x = bx - mw;
                    mx = bx - mw;

                    game->enemyValues[j].dx = 0;
                }
            }
        }
    }

    for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    {
        for (int i = 0; i < 100; i++)
        {

            float mw = 48, mh = 48;
            float mx = game->smartEnemies[j].x, my = game->smartEnemies[j].y;
            float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

            if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
            {
                //are we bumping our head?
                if (my < by + bh && my > by && game->smartEnemies[j].dy < 0)
                {
                    //correct y
                    game->smartEnemies[j].y = by + bh;
                    my = by + bh;

                    //bumped our head, stop any jump velocity
                    game->smartEnemies[j].dy = 0;
                    //game->enemy.onLedge = 1;
                }
            }
            if (mx + mw > bx && mx < bx + bw)
            {
                //are we landing on the ledge
                if (my + mh > by && my < by && game->smartEnemies[j].dy > 0)
                {
                    //correct y
                    game->smartEnemies[j].y = by - mh;
                    my = by - mh;

                    //landed on this ledge, stop any jump velocity
                    game->smartEnemies[j].dy = 0;
                    //game->enemy.onLedge = 1;
                }
            }

            if (my + mh > by && my < by + bh)
            {
                //rubbing against right edge
                if (mx < bx + bw && mx + mw > bx + bw && game->smartEnemies[j].dx < 0)
                {
                    //correct x
                    game->smartEnemies[j].x = bx + bw;
                    mx = bx + bw;

                    game->smartEnemies[j].dx = 0;
                }
                    //rubbing against left edge
                else if (mx + mw > bx && mx < bx && game->smartEnemies[j].dx > 0)
                {
                    //correct x
                    game->smartEnemies[j].x = bx - mw;
                    mx = bx - mw;

                    game->smartEnemies[j].dx = 0;
                }
            }
        }
    }

    // for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    // {
    //     game->man.w = 48;
    //     game->man.h = 48;
    //     float mw = 48, mh = 48;
    //     float mx = game->smartEnemies[j].x, my = game->smartEnemies[j].y;
    //     float bx = game->man.x, by = game->man.y, bw = game->man.w, bh = game->man.h;

    //     if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
    //     {
    //         //are we bumping our head?
    //         if (my < by + bh && my > by && game->smartEnemies[j].dy < 0)
    //         {
    //             //correct y
    //             game->smartEnemies[j].y = by + bh;
    //             my = by + bh;

    //             //bumped our head, stop any jump velocity
    //             game->smartEnemies[j].dy = 0;
    //             //game->enemy.onLedge = 1;
    //         }
    //     }
    //     if (mx + mw > bx && mx < bx + bw)
    //     {
    //         //are we landing on the ledge
    //         if (my + mh > by && my < by && game->smartEnemies[j].dy > 0)
    //         {
    //             //correct y
    //             game->smartEnemies[j].y = by - mh;
    //             my = by - mh;

    //             //landed on this ledge, stop any jump velocity
    //             game->smartEnemies[j].dy = 0;
    //             //game->enemy.onLedge = 1;
    //         }
    //     }

    //     if (my + mh > by && my < by + bh)
    //     {
    //         //rubbing against right edge
    //         if (mx < bx + bw && mx + mw > bx + bw && game->smartEnemies[j].dx < 0)
    //         {
    //             //correct x
    //             game->smartEnemies[j].x = bx + bw;
    //             mx = bx + bw;

    //             game->smartEnemies[j].dx = 0;
    //         }
    //             //rubbing against left edge
    //         else if (mx + mw > bx && mx < bx && game->smartEnemies[j].dx > 0)
    //         {
    //             //correct x
    //             game->smartEnemies[j].x = bx - mw;
    //             mx = bx - mw;

    //             game->smartEnemies[j].dx = 0;
    //         }
    //     }
    // }

    //Check for collision with any ledges (brick blocks)
    for (int i = 0; i < 100; i++)
    {
        float mw = 48, mh = 48;
        float mx = game->man.x, my = game->man.y;
        float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        {
            //are we bumping our head?
            if (my < by + bh && my > by && game->man.dy < 0)
            {
                //correct y
                game->man.y = by + bh;
                my = by + bh;

                //bumped our head, stop any jump velocity
                game->man.dy = 0;
                game->man.onLedge = 0;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->man.dy > 0)
            {
                //correct y
                game->man.y = by - mh;
                my = by - mh;

                //landed on this ledge, stop any jump velocity
                game->man.dy = 0;
                game->man.onLedge = 1;
            }
        }

        if (my + mh > by && my < by + bh)
        {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->man.dx < 0)
            {
                //correct x
                game->man.x = bx + bw;
                mx = bx + bw;

                game->man.dx = 0;
            }
                //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->man.dx > 0)
            {
                //correct x
                game->man.x = bx - mw;
                mx = bx - mw;

                game->man.dx = 0;
            }
        }
    }

    //Check for collision with any ledges (brick blocks)
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 100; i++)
        {
            float mw = 48, mh = 48;
            float mx = game->boss[j].x, my = game->boss[j].y;
            float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

            if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
            {
                //are we bumping our head?
                if (my < by + bh && my > by && game->boss[j].dy < 0)
                {
                    //correct y
                    game->boss[j].y = by + bh;
                    my = by + bh;

                    //bumped our head, stop any jump velocity
                    game->boss[j].dy = 0;
                    game->boss[j].onLedge = 0;
                }
            }
            if (mx + mw > bx && mx < bx + bw)
            {
                //are we landing on the ledge
                if (my + mh > by && my < by && game->boss[j].dy > 0)
                {
                    //correct y
                    game->boss[j].y = by - mh;
                    my = by - mh;

                    //landed on this ledge, stop any jump velocity
                    game->boss[j].dy = 0;
                    game->boss[j].onLedge = 1;
                }
            }

            if (my + mh > by && my < by + bh)
            {
                //rubbing against right edge
                if (mx < bx + bw && mx + mw > bx + bw && game->boss[j].dx < 0)
                {
                    //correct x
                    game->boss[j].x = bx + bw;
                    mx = bx + bw;

                    game->boss[j].dx = 0;
                }
                    //rubbing against left edge
                else if (mx + mw > bx && mx < bx && game->boss[j].dx > 0)
                {
                    //correct x
                    game->boss[j].x = bx - mw;
                    mx = bx - mw;

                    game->boss[j].dx = 0;
                }
            }
        }
    }

    for (int i = 0; i < 100; i++)
    {
        float mw = 48, mh = 48;
        float mx = game->secondPlayer.x, my = game->secondPlayer.y;
        float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        {
            //are we bumping our head?
            if (my < by + bh && my > by && game->secondPlayer.dy < 0)
            {
                //correct y
                game->secondPlayer.y = by + bh;
                my = by + bh;

                //bumped our head, stop any jump velocity
                game->secondPlayer.dy = 0;
                game->secondPlayer.onLedge = 0;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->secondPlayer.dy > 0)
            {
                //correct y
                game->secondPlayer.y = by - mh;
                my = by - mh;

                //landed on this ledge, stop any jump velocity
                game->secondPlayer.dy = 0;
                game->secondPlayer.onLedge = 1;
            }
        }

        if (my + mh > by && my < by + bh)
        {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->secondPlayer.dx < 0)
            {
                //correct x
                game->secondPlayer.x = bx + bw;
                mx = bx + bw;

                game->secondPlayer.dx = 0;
            }
                //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->secondPlayer.dx > 0)
            {
                //correct x
                game->secondPlayer.x = bx - mw;
                mx = bx - mw;

                game->secondPlayer.dx = 0;
            }
        }
    }

    //Multiplayer
    if (game->multiPlayer)
    {
        //  for (int j = 0; j < NUM_SMART_ENEMIES; j++)
        //  {
        //     game->man.w = 48;
        //     game->man.h = 48;
        //     float mw = 48, mh = 48;
        //     float mx = game->smartEnemies[j].x, my = game->smartEnemies[j].y;
        //     float bx = game->secondPlayer.x, by = game->secondPlayer.y, bw = game->secondPlayer.w, bh = game->secondPlayer.h;

        //     if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        //     {
        //         //are we bumping our head?
        //         if (my < by + bh && my > by && game->smartEnemies[j].dy < 0)
        //         {
        //             //correct y
        //             game->smartEnemies[j].y = by + bh;
        //             my = by + bh;

        //             //bumped our head, stop any jump velocity
        //             game->smartEnemies[j].dy = 0;
        //             //game->enemy.onLedge = 1;
        //         }
        //     }
        //     if (mx + mw > bx && mx < bx + bw)
        //     {
        //         //are we landing on the ledge
        //         if (my + mh > by && my < by && game->smartEnemies[j].dy > 0)
        //         {
        //             //correct y
        //             game->smartEnemies[j].y = by - mh;
        //             my = by - mh;

        //             //landed on this ledge, stop any jump velocity
        //             game->smartEnemies[j].dy = 0;
        //             //game->enemy.onLedge = 1;
        //         }
        //     }

        //     if (my + mh > by && my < by + bh)
        //     {
        //         //rubbing against right edge
        //         if (mx < bx + bw && mx + mw > bx + bw && game->smartEnemies[j].dx < 0)
        //         {
        //             //correct x
        //             game->smartEnemies[j].x = bx + bw;
        //             mx = bx + bw;

        //             game->smartEnemies[j].dx = 0;
        //         }
        //             //rubbing against left edge
        //         else if (mx + mw > bx && mx < bx && game->smartEnemies[j].dx > 0)
        //         {
        //             //correct x
        //             game->smartEnemies[j].x = bx - mw;
        //             mx = bx - mw;

        //             game->smartEnemies[j].dx = 0;
        //         }
        //     }
        // }
            for (int i = 0; i < 100; i++)
            {
                float mw = 48, mh = 48;
                float mx = game->secondPlayer.x, my = game->secondPlayer.y;
                float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

                if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
                {
                    //are we bumping our head?
                    if (my < by + bh && my > by && game->secondPlayer.dy < 0)
                    {
                        //correct y
                        game->secondPlayer.y = by + bh;
                        my = by + bh;

                        //bumped our head, stop any jump velocity
                        game->secondPlayer.dy = 0;
                        game->secondPlayer.onLedge = 0;
                    }
                }
                if (mx + mw > bx && mx < bx + bw)
                {
                    //are we landing on the ledge
                    if (my + mh > by && my < by && game->secondPlayer.dy > 0)
                    {
                        //correct y
                        game->secondPlayer.y = by - mh;
                        my = by - mh;

                        //landed on this ledge, stop any jump velocity
                        game->secondPlayer.dy = 0;
                        game->secondPlayer.onLedge = 1;
                    }
                }

                if (my + mh > by && my < by + bh)
                {
                    //rubbing against right edge
                    if (mx < bx + bw && mx + mw > bx + bw && game->secondPlayer.dx < 0)
                    {
                        //correct x
                        game->secondPlayer.x = bx + bw;
                        mx = bx + bw;

                        game->secondPlayer.dx = 0;
                    }
                        //rubbing against left edge
                    else if (mx + mw > bx && mx < bx && game->secondPlayer.dx > 0)
                    {
                        //correct x
                        game->secondPlayer.x = bx - mw;
                        mx = bx - mw;

                        game->secondPlayer.dx = 0;
                    }
                }
            }
        }
    // COLLISION ENEMY TO ENEMY _________________________________________________________________________________________________________________________
     for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    {
        for (int i = 0; i < NUM_SMART_ENEMIES; i++) 
        {
        game->smartEnemies[i].w = 48;
        game->smartEnemies[i].h = 48;
        float mw = 48, mh = 48;
        float mx = game->smartEnemies[j].x, my = game->smartEnemies[j].y;
        float bx = game->smartEnemies[i].x, by = game->smartEnemies[i].y, bw = game->smartEnemies[i].w, bh = game->smartEnemies[i].h;

        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        {
            //are we bumping our head?
            if (my < by + bh && my > by && game->smartEnemies[j].dy < 0)
            {
                //correct y
                game->smartEnemies[j].y = by + bh;
                my = by + bh;

                //bumped our head, stop any jump velocity
                game->smartEnemies[j].dy = 0;
               // game->enemy.onLedge = 1;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->smartEnemies[j].dy > 0)
            {
                //correct y
                game->smartEnemies[j].y = by - mh;
                my = by - mh;

                //landed on this ledge, stop any jump velocity
                game->smartEnemies[j].dy = 0;
               // game->enemy.onLedge = 1;
            }
        }

        if (my + mh > by && my < by + bh)
        {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->smartEnemies[j].dx < 0)
            {
                //correct x
                game->smartEnemies[j].x = bx + bw;
                mx = bx + bw;

                game->smartEnemies[j].dx = 0;
            }
            //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->smartEnemies[j].dx > 0)
            {
                //correct x
                game->smartEnemies[j].x = bx - mw;
                mx = bx - mw;

                game->smartEnemies[j].dx = 0;
            }
        }
        }
    }
}

void collisionDetect2(GameState *game)
{
    for (int i  = 0; i < 100; i++)
    {
        if (collide2d(game->man.x, game->man.y, game->stars[i].x, game->stars[i].y, 30, 30, 30, 30))
        {
           // game->man.isDead = 1;
            game->gameLives = 0;
//            game->man.isDead = 0;
        }
        if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->stars[i].x, game->stars[i].y, 30, 30, 30, 30))
        {
            game->gameLives = 0;
        }
    }
    //Check for collision wit any ledges (brick blocks)
    for (int i = 0; i < 100; i++)
    {
        float mw = 48, mh = 48;
        float mx = game->man.x, my = game->man.y;
        float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        {
            //are we bumping our head?
            if (my < by + bh && my > by && game->man.dy < 0)
            {
                //correct y
                game->man.y = by + bh;
                my = by + bh;

                //bumped our head, stop any jump velocity
                game->man.dy = 0;
                game->man.onLedge = 1;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->man.dy > 0)
            {
                //correct y
                game->man.y = by - mh;
                my = by - mh;

                //landed on this ledge, stop any jump velocity
                game->man.dy = 0;
                game->man.onLedge = 1;
            }
        }

        if (my + mh > by && my < by + bh)
        {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->man.dx < 0)
            {
                //correct x
                game->man.x = bx + bw;
                mx = bx + bw;

                game->man.dx = 0;
            }
                //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->man.dx > 0)
            {
                //correct x
                game->man.x = bx - mw;
                mx = bx - mw;

                game->man.dx = 0;
            }
        }
    }

    for (int i = 0; i < 100; i++)
    {
        float mw = 48, mh = 48;
        float mx = game->secondPlayer.x, my = game->secondPlayer.y;
        float bx = game->ledges[i].x, by = game->ledges[i].y, bw = game->ledges[i].w, bh = game->ledges[i].h;

        if (mx + mw / 2 > bx && mx + mw / 2 < bx + bw)
        {
            //are we bumping our head?
            if (my < by + bh && my > by && game->secondPlayer.dy < 0)
            {
                //correct y
                game->secondPlayer.y = by + bh;
                my = by + bh;

                //bumped our head, stop any jump velocity
                game->secondPlayer.dy = 0;
                game->secondPlayer.onLedge = 1;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            //are we landing on the ledge
            if (my + mh > by && my < by && game->secondPlayer.dy > 0)
            {
                //correct y
                game->secondPlayer.y = by - mh;
                my = by - mh;

                //landed on this ledge, stop any jump velocity
                game->secondPlayer.dy = 0;
                game->secondPlayer.onLedge = 1;
            }
        }

        if (my + mh > by && my < by + bh)
        {
            //rubbing against right edge
            if (mx < bx + bw && mx + mw > bx + bw && game->secondPlayer.dx < 0)
            {
                //correct x
                game->secondPlayer.x = bx + bw;
                mx = bx + bw;

                game->secondPlayer.dx = 0;
            }
                //rubbing against left edge
            else if (mx + mw > bx && mx < bx && game->secondPlayer.dx > 0)
            {
                //correct x
                game->secondPlayer.x = bx - mw;
                mx = bx - mw;

                game->secondPlayer.dx = 0;
            }
        }
    }
}

