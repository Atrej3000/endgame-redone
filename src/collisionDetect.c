#include "header.h"

// Captures each player's y at the start of this tick, before
// consume_*_jump_requests/apply_*_player_forces/process(2) move it -- see
// docs/collision-correctness-map.md (Phase 13). Called as the first line of
// arcade_simulate()/runner_simulate() (src/frame.c), every tick,
// unconditionally, so prevY is always correct relative to the tick that
// reads it, including the first tick after a session reset.
void capture_player_prev_y(GameState *game)
{
    game->man.prevY = game->man.y;
    game->secondPlayer.prevY = game->secondPlayer.y;
}

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
    // Reset grounded each step (Phase 13, see docs/collision-correctness-map.md):
    // onLedge defaults to not-grounded before this pass; the landing check
    // below re-affirms it to 1 only if the man is actually still resting on
    // a ledge this tick. Fixes a real bug where walking off a ledge's edge
    // (no ceiling hit, no jump) never used to clear onLedge.
    game->man.onLedge = 0;
    for (int i = 0; i < 100; i++)
    {
        float mw = PLAYER_LEDGE_HITBOX_W, mh = PLAYER_LEDGE_HITBOX_H;
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
            // Landing/resting: crossing-based using this tick's start-of-tick
            // y (man.prevY), not just the current-tick threshold -- a man
            // resting exactly on a ledge sits at my+mh == by (not > by), so
            // a strict current-position-only test would never re-fire while
            // at rest. See docs/collision-correctness-map.md section 3.
            if (game->man.prevY + mh <= by && my + mh >= by && my < by + bh && game->man.dy >= 0)
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

    // Reset grounded each step -- see the man loop above and
    // docs/collision-correctness-map.md. Only this (first) secondPlayer
    // block needs the reset; the multiplayer-duplicate block below re-uses
    // it since secondPlayer's position doesn't change in between.
    game->secondPlayer.onLedge = 0;
    for (int i = 0; i < 100; i++)
    {
        float mw = PLAYER_LEDGE_HITBOX_W, mh = PLAYER_LEDGE_HITBOX_H;
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
            // Landing/resting: crossing-based, see the man loop above.
            if (game->secondPlayer.prevY + mh <= by && my + mh >= by && my < by + bh && game->secondPlayer.dy >= 0)
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
                float mw = PLAYER_LEDGE_HITBOX_W, mh = PLAYER_LEDGE_HITBOX_H;
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
                    // Landing/resting: crossing-based, see the man loop above.
                    // No onLedge=0 reset needed in this (duplicate) block --
                    // the first secondPlayer loop already reset it this tick.
                    if (game->secondPlayer.prevY + mh <= by && my + mh >= by && my < by + bh && game->secondPlayer.dy >= 0)
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
            runner_trigger_death(game);
        }
        if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->stars[i].x, game->stars[i].y, 30, 30, 30, 30))
        {
            runner_trigger_death(game);
        }
    }
    //Check for collision wit any ledges (brick blocks)
    // Reset grounded each step -- see docs/collision-correctness-map.md.
    game->man.onLedge = 0;
    for (int i = 0; i < 100; i++)
    {
        float mw = PLAYER_LEDGE_HITBOX_W, mh = PLAYER_LEDGE_HITBOX_H;
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
                // Fixed (Phase 13): was incorrectly `1` -- a ceiling bump
                // must leave the man airborne, matching Arcade's equivalent
                // branch. See docs/collision-correctness-map.md section 2.
                game->man.onLedge = 0;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            // Landing/resting: crossing-based, see collisionDetect()'s man
            // loop above.
            if (game->man.prevY + mh <= by && my + mh >= by && my < by + bh && game->man.dy >= 0)
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

    // Reset grounded each step -- see docs/collision-correctness-map.md.
    game->secondPlayer.onLedge = 0;
    for (int i = 0; i < 100; i++)
    {
        float mw = PLAYER_LEDGE_HITBOX_W, mh = PLAYER_LEDGE_HITBOX_H;
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
                // Fixed (Phase 13): was incorrectly `1` -- see the man loop
                // above and docs/collision-correctness-map.md section 2.
                game->secondPlayer.onLedge = 0;
            }
        }
        if (mx + mw > bx && mx < bx + bw)
        {
            // Landing/resting: crossing-based, see the man loop above.
            if (game->secondPlayer.prevY + mh <= by && my + mh >= by && my < by + bh && game->secondPlayer.dy >= 0)
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

