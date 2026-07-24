#include "ai_forces.h"
#include "collision_pipeline.h"
#include "frame.h"
#include "game_events.h"
#include "physics_body.h"
#include "runner_segments.h"

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            fprintf(stderr, "PRE-PHASE35 SIMULATION TEST: FAIL - %s\n",   \
                    description);                                           \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void reset_game(GameState *game, GameMode mode)
{
    memset(game, 0, sizeof(*game));
    game->multiPlayer = mode;
    game->app.scene = APP_SCENE_ARCADE_GAME;
    game->simulationOnly = true;
    game->menu1 = (SDL_Texture *)game;
    game->menu2 = (SDL_Texture *)game;
    game->assetFlags.arcadeAssetsLoaded = true;
    game->assetFlags.runnerAssetsLoaded = true;
    game->gameLives = 10;
    game->man.lives = 3;
    game->secondPlayer.lives = 3;
    game->man.x = 100.0f;
    game->man.y = 100.0f;
    game->secondPlayer.x = 1000.0f;
    game->secondPlayer.y = 100.0f;
    game->statusState = STATUS_STATE_GAME;
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        game->enemyValues[i].y = 1000.0f;
        game->enemyValues[i].visible = 0;
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        game->smartEnemies[i].y = 1000.0f;
        game->smartEnemies[i].visible = 0;
    }
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].y = 1000.0f;
        game->boss[i].visible = 0;
    }
    for (int i = 0; i < NUM_STARS; i++)
    {
        game->stars[i].x = 100000 + i * 100;
        game->stars[i].y = 100000;
    }
}

static int active_bullet_count(const Bullet bullets[MAX_BULLETS])
{
    int count = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            count++;
        }
    }
    return count;
}

static void test_arcade_escape_and_multiplayer_boundaries(GameState *game)
{
    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100.0f;
    game->man.y = 690.0f;
    game->enemyValues[0] = (Enemies){.x = 100.0f, .y = (float)HEIGHT, .visible = 1};
    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("contact plus escape charges one life", game->gameLives == 9);
    CHECK("escape awards no kill or progression score",
          game->kills_score == 0 && game->kills_score_multi == 0 && game->tempScore == 0);
    CHECK("escape wins over contact without respawning the player",
          game->man.x == 100.0f && game->man.y == 690.0f);

    reset_game(game, GAME_MODE_MULTIPLAYER);
    game->app.scene = APP_SCENE_ARCADE_GAME;
    game->assetFlags.arcadeAssetsLoaded = true;
    game->boss[0] = (Enemies){.x = 500.0f, .y = (float)HEIGHT, .visible = 1};
    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("multiplayer boss escape ends the shared life pool",
          game->gameLives == 0 && game->man.lives == 0 && game->secondPlayer.lives == 0);
    CHECK("escaped boss is deactivated",
          !game->boss[0].visible && game->boss[0].y == 1000.0f);
    CHECK("headless boss game-over suppresses menu side effects",
          game->app.scene == APP_SCENE_ARCADE_GAME);

    reset_game(game, GAME_MODE_MULTIPLAYER);
    game->man.x = 100.0f;
    game->secondPlayer.x = 100.0f;
    game->boss[0] = (Enemies){.x = 145.0f, .y = 100.0f, .visible = 1};
    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("boss contact uses the same player collider for both players",
          game->gameLives == 8);

    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->secondPlayer.y = 900.0f;
    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("stale player two cannot damage single-player Arcade", game->gameLives == 10);
}

static void test_inactive_ai(GameState *game)
{
    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->enemyValues[0] =
        (Enemies){.x = 100.0f, .y = 100.0f, .dx = 120.0f, .dy = 120.0f, .visible = 0};
    game->smartEnemies[0] =
        (Enemies){.x = 200.0f, .y = 200.0f, .dx = 120.0f, .dy = 120.0f, .visible = 0};
    game->boss[0] =
        (Enemies){.x = 300.0f, .y = 300.0f, .dx = 120.0f, .dy = 120.0f, .visible = 0};
    move_regular_enemies(game, PHYSICS_DT);
    move_smart_enemies(game, PHYSICS_DT);
    move_boss_entities(game, PHYSICS_DT);
    CHECK("inactive regular enemy does not simulate",
          game->enemyValues[0].x == 100.0f && game->enemyValues[0].y == 100.0f);
    CHECK("inactive smart enemy does not simulate",
          game->smartEnemies[0].x == 200.0f && game->smartEnemies[0].y == 200.0f);
    CHECK("inactive boss does not simulate",
          game->boss[0].x == 300.0f && game->boss[0].y == 300.0f);
}

static void test_projectile_order_and_culling(GameState *game)
{
    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->enemyValues[0] = (Enemies){.x = 70.0f, .y = 100.0f, .visible = 1};
    game->enemyValues[1] = (Enemies){.x = 40.0f, .y = 100.0f, .visible = 1};
    game->bullets[0] = (Bullet){.prevX = 0.0f, .x = 100.0f, .y = 110.0f, .active = true};
    game_events_begin(game);
    detect_projectile_hits(game);
    game_events_apply(game);
    CHECK("nearest swept target wins over lower array index",
          game->enemyValues[0].visible && !game->enemyValues[1].visible);

    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->enemyValues[0] = (Enemies){.x = 0.0f, .y = 100.0f, .visible = 1};
    addBullet(game, 1.0f, 110.0f, -3.0f);
    move_arcade_bullets(game, PHYSICS_DT);
    process(game, PHYSICS_DT);
    CHECK("edge-crossing projectile hits before offscreen culling",
          !game->enemyValues[0].visible && !game->bullets[0].active);
}

static void test_fixed_tick_cadence(GameState *game)
{
    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->input.shootHeldPlayer1 = true;
    arcade_shooting_step(game);
    CHECK("first fixed shooting tick creates one projectile",
          active_bullet_count(game->bullets) == 1);
    for (int i = 0; i < 20; i++)
    {
        process_arcade_shooting(game);
    }
    CHECK("real-frame compatibility calls cannot create projectile bursts",
          active_bullet_count(game->bullets) == 1);
    for (int i = 0; i < ARCADE_SHOT_COOLDOWN_TICKS - 1; i++)
    {
        arcade_shooting_step(game);
    }
    CHECK("cooldown does not fire early", active_bullet_count(game->bullets) == 1);
    arcade_shooting_step(game);
    CHECK("cooldown fires on its fixed-tick boundary", active_bullet_count(game->bullets) == 2);

    game->time = 1;
    arcade_presentation_step(game);
    CHECK("presentation advances once per fixed tick",
          game->CurrentSheetBullet == 1 && game->train.x == 4);
    game->time = 2;
    arcade_presentation_step(game);
    CHECK("secondary cadence advances on its intended even tick",
          game->CurrentSheetBullet == 2 && game->CurrentSheetBullet2 == 1 &&
          game->train.x == 8);
}

static void test_runner_fixed_hazards_and_death(GameState *game)
{
    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->app.scene = APP_SCENE_RUNNER_GAME;
    game->secondPlayer.x = (float)game->stars[0].x;
    game->secondPlayer.y = (float)game->stars[0].y;
    game_events_begin(game);
    detect_runner_hazard_contacts(game);
    game_events_apply(game);
    CHECK("stale player two cannot kill single-player Runner", !game->man.isDead);

    reset_game(game, GAME_MODE_MULTIPLAYER);
    game->app.scene = APP_SCENE_RUNNER_GAME;
    game->gameLives = 2;
    game->man.x = -20.0f;
    game->man.y = 700.0f;
    game->man.dx = 50.0f;
    game->secondPlayer.x = -30.0f;
    game->secondPlayer.y = 500.0f;
    game->secondPlayer.dy = 50.0f;
    game->input.moveRightPlayer1 = true;
    game->input.jumpHeldPlayer2 = true;
    runner_trigger_death(game);
    CHECK("shared Runner death marks both active players dead",
          game->man.isDead && game->secondPlayer.isDead);
    game->deathCountdown = 1;
    runner_death_step(game);
    CHECK("non-final Runner death decrements exactly once and respawns safely",
          game->gameLives == 1 && !game->man.isDead && !game->secondPlayer.isDead &&
          game->man.x == 0.0f && game->man.y == 0.0f &&
          game->secondPlayer.x == 0.0f && game->secondPlayer.y == 0.0f);
    CHECK("Runner respawn clears velocity, interpolation, and input",
          game->man.dx == 0.0f && game->man.dy == 0.0f &&
          game->man.prevX == game->man.x && game->man.prevY == game->man.y &&
          !game->input.moveRightPlayer1 && !game->input.jumpHeldPlayer2);

    reset_game(game, GAME_MODE_MULTIPLAYER);
    game->app.scene = APP_SCENE_RUNNER_GAME;
    game->gameLives = 1;
    game->man.x = 500.0f;
    game->man.y = 600.0f;
    game->secondPlayer.x = 600.0f;
    game->secondPlayer.y = 600.0f;
    runner_trigger_death(game);
    game->deathCountdown = 1;
    runner_death_step(game);
    CHECK("last Runner life saturates at zero without respawning",
          game->gameLives == 0 && game->man.isDead && game->secondPlayer.isDead &&
          game->man.x == 500.0f && game->man.y == 600.0f);
}

static void test_runner_transition_and_recycling(GameState *game)
{
    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->gameLives = 0;
    game->x_score = 42;
    game->app.scene = APP_SCENE_RUNNER_GAME;
    game->assetFlags.runnerAssetsLoaded = true;
    game_events_begin(game);
    (void)game_events_push_transition_check(game, GAME_EVENT_RUNNER_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("headless Runner game-over suppresses persistence and menu side effects",
          game->x_i == 0 && game->app.scene == APP_SCENE_RUNNER_GAME);
    game_events_begin(game);
    (void)game_events_push_transition_check(game, GAME_EVENT_RUNNER_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("repeated headless transition checks remain side-effect free", game->x_i == 0);

    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    runner_segments_reset(game);
    const int oldX = game->stars[0].x;
    game->man.x = 55.0f * 293.0f;
    runner_segments_update(game);
    CHECK("recycled star slot moves to a new segment", game->stars[0].x != oldX);
    CHECK("recycled star interpolation starts at its new position",
          game->stars[0].prevX == (float)game->stars[0].x &&
          game->stars[0].prevY == (float)game->stars[0].y);
}

static void test_world_sweep_and_api_boundaries(GameState *game)
{
    const Collider collider = player_world_collider();
    Ledge descending[2] = {{0, 300, 200, 20}, {0, 200, 200, 20}};
    Ledge reversed[2] = {descending[1], descending[0]};
    KinematicBody first = {50.0f, 400.0f, 50.0f, 0.0f, 0.0f, 100.0f};
    KinematicBody second = first;
    const WorldCollisionResult firstResult =
        resolve_kinematic_world(&first, &collider, descending, 2);
    const WorldCollisionResult secondResult =
        resolve_kinematic_world(&second, &collider, reversed, 2);
    CHECK("high-speed downward sweep lands on the first surface",
          firstResult.grounded && first.y == 152.0f);
    CHECK("world Y resolution is independent of ledge order",
          secondResult.grounded && second.y == first.y);

    reset_game(game, GAME_MODE_SINGLE_PLAYER);
    game->bullets[0].active = true;
    CHECK("projectile event rejects invalid projectile and target indices",
          !game_events_push_projectile_hit(game, false, -1,
                                           GAME_EVENT_TARGET_REGULAR_ENEMY, 0) &&
          !game_events_push_projectile_hit(game, false, 0,
                                           GAME_EVENT_TARGET_REGULAR_ENEMY,
                                           NUM_ENEMIES));
    CHECK("player-two event is rejected in single-player",
          !game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1));
    removeBullet(game, -1);
    removeBullet(game, MAX_BULLETS);
    addBullet(NULL, 0.0f, 0.0f, 1.0f);
    game_events_begin(NULL);
    CHECK("null and invalid public API boundaries return safely",
          !game_events_push_transition_check(NULL, GAME_EVENT_ARCADE_GAME_OVER_CHECK));
}

int main(void)
{
    static GameState game;
    test_arcade_escape_and_multiplayer_boundaries(&game);
    test_projectile_order_and_culling(&game);
    test_inactive_ai(&game);
    test_fixed_tick_cadence(&game);
    test_runner_fixed_hazards_and_death(&game);
    test_runner_transition_and_recycling(&game);
    test_world_sweep_and_api_boundaries(&game);

    if (failures == 0)
    {
        printf("PRE-PHASE35 SIMULATION TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
