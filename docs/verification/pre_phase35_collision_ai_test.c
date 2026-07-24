#include "ai_forces.h"
#include "collision_pipeline.h"
#include "frame.h"
#include "game_events.h"
#include "physics_body.h"

static int failures = 0;
static GameState snapshot;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            fprintf(stderr, "PRE-PHASE35 COLLISION/AI TEST: FAIL - %s\n", \
                    description);                                           \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void reset_game(GameState *game, GameMode mode)
{
    memset(game, 0, sizeof(*game));
    game->multiPlayer = mode;
    game->gameLives = 10;
    game->man.lives = 3;
    game->secondPlayer.lives = mode == GAME_MODE_MULTIPLAYER ? 3 : 0;
    game->man.visible0 = 1;
    game->secondPlayer.visible0 = mode == GAME_MODE_MULTIPLAYER;
    game->man.x = 100.0f;
    game->man.y = 100.0f;
    game->man.prevX = game->man.x;
    game->man.prevY = game->man.y;
    game->secondPlayer.x = 1000.0f;
    game->secondPlayer.y = 100.0f;
    game->secondPlayer.prevX = game->secondPlayer.x;
    game->secondPlayer.prevY = game->secondPlayer.y;
    game->statusState = STATUS_STATE_GAME;
    game->app.scene = APP_SCENE_ARCADE_GAME;
    game->simulationOnly = true;
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        game->enemyValues[i].visible = 0;
        game->enemyValues[i].y = 1000.0f;
        game->enemyValues[i].prevY = 1000.0f;
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        game->smartEnemies[i].visible = 0;
        game->smartEnemies[i].y = 1000.0f;
        game->smartEnemies[i].prevY = 1000.0f;
    }
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].visible = 0;
        game->boss[i].y = 1000.0f;
        game->boss[i].prevY = 1000.0f;
    }
    for (int i = 0; i < NUM_STARS; i++)
    {
        game->stars[i].x = 100000 + i * 100;
        game->stars[i].y = 100000;
        game->stars[i].baseX = game->stars[i].x;
        game->stars[i].baseY = game->stars[i].y;
    }
}

static int count_player_events(const GameState *game, GameEventType type,
                               int playerIndex)
{
    int count = 0;
    for (int i = 0; i < game->events.count; i++)
    {
        if (game->events.events[i].type == type &&
            game->events.events[i].projectileIndex == playerIndex)
        {
            count++;
        }
    }
    return count;
}

static void test_authoritative_colliders_and_edges(void)
{
    const Collider player = player_world_collider();
    const Collider regular =
        arcade_enemy_collider(GAME_EVENT_TARGET_REGULAR_ENEMY);
    const Collider smart =
        arcade_enemy_collider(GAME_EVENT_TARGET_SMART_ENEMY);
    const Collider boss = arcade_enemy_collider(GAME_EVENT_TARGET_BOSS);
    const Collider invalid = arcade_enemy_collider((GameEventTarget)INT_MAX);
    CHECK("all Arcade target types share one authoritative gameplay box",
          regular.width == 40.0f && regular.height == 50.0f &&
              smart.width == regular.width && smart.height == regular.height &&
              boss.width == regular.width && boss.height == regular.height);
    CHECK("invalid target type cannot create an interactive collider",
          invalid.layer == 0u && invalid.mask == 0u);

    KinematicBody first = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    KinematicBody touching = {player.width, 0.0f, player.width, 0.0f, 0.0f, 0.0f};
    CHECK("static AABBs touching at one edge do not overlap",
          !collider_bounds_overlap(&first, &player, &touching, &regular));
    touching.x -= 0.01f;
    touching.prevX = touching.x;
    CHECK("static AABBs overlap immediately inside the edge",
          collider_bounds_overlap(&first, &player, &touching, &regular));
    CHECK("legacy AABB helper follows the same half-open edge rule",
          !collide2d(0.0f, 0.0f, 48.0f, 0.0f,
                     48.0f, 48.0f, 40.0f, 50.0f) &&
              collide2d(0.0f, 0.0f, 47.99f, 0.0f,
                        48.0f, 48.0f, 40.0f, 50.0f));

    const Collider projectile = projectile_collider();
    KinematicBody target = {40.0f, 100.0f, 40.0f, 100.0f, 0.0f, 0.0f};
    KinematicBody endpoint = {40.0f, 100.0f, 0.0f, 100.0f, 0.0f, 0.0f};
    CHECK("swept projectile arriving exactly at the near face hits",
          collider_swept_horizontal_overlap(&endpoint, &projectile,
                                            &target, &regular));
    KinematicBody stationaryInside =
        {40.0f, 100.0f, 40.0f, 100.0f, 0.0f, 0.0f};
    CHECK("zero-movement projectile inside a target still hits",
          collider_swept_horizontal_overlap(&stationaryInside, &projectile,
                                            &target, &regular));
    stationaryInside.x = 80.0f;
    stationaryInside.prevX = 80.0f;
    CHECK("stationary projectile at the far half-open edge does not hit",
          !collider_swept_horizontal_overlap(&stationaryInside, &projectile,
                                             &target, &regular));
    endpoint.y = 150.0f;
    endpoint.prevY = 150.0f;
    CHECK("projectile at the target's bottom edge does not hit",
          !collider_swept_horizontal_overlap(&endpoint, &projectile,
                                             &target, &regular));
}

static void test_multiplayer_symmetry_and_inactive_entities(void)
{
    GameState game;
    reset_game(&game, GAME_MODE_MULTIPLAYER);
    game.man.x = 40.0f;
    game.man.prevX = game.man.x;
    game.secondPlayer.x = 40.0f;
    game.secondPlayer.prevX = game.secondPlayer.x;
    game.enemyValues[0] = (Enemies){
        .x = 0.0f, .y = 100.0f, .prevX = 0.0f, .prevY = 100.0f,
        .visible = 1
    };
    game_events_begin(&game);
    detect_arcade_hazards(&game);
    CHECK("both players use the same exact-edge contact rule",
          count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 0) == 0 &&
              count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 1) == 0);

    game.man.x = 39.0f;
    game.man.prevX = game.man.x;
    game.secondPlayer.x = 39.0f;
    game.secondPlayer.prevX = game.secondPlayer.x;
    game_events_begin(&game);
    detect_arcade_hazards(&game);
    CHECK("both players receive symmetric contact just inside the edge",
          count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 0) == 1 &&
              count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 1) == 1);

    game.secondPlayer.isDead = 1;
    game_events_begin(&game);
    detect_arcade_hazards(&game);
    CHECK("dead player is not an Arcade collision target",
          count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 0) == 1 &&
              count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 1) == 0);

    game.enemyValues[0].visible = 0;
    game_events_begin(&game);
    detect_arcade_hazards(&game);
    CHECK("hidden enemy cannot emit contact or escape events",
          count_player_events(&game, GAME_EVENT_ARCADE_PLAYER_HIT, 0) == 0);
}

static void test_projectile_snapshot_and_saturation(void)
{
    GameState game;
    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.enemyValues[0] = (Enemies){
        .x = 40.0f, .y = 100.0f, .prevX = 40.0f, .prevY = 100.0f,
        .visible = 1
    };
    for (int i = 0; i < 2; i++)
    {
        game.bullets[i] = (Bullet){
            .x = 100.0f, .y = 110.0f, .prevX = 0.0f, .prevY = 110.0f,
            .dx = BULLET_SPEED_PER_SEC, .active = true
        };
    }
    game_events_begin(&game);
    detect_projectile_hits(&game);
    CHECK("two simultaneous projectiles both queue snapshot contacts",
          game.events.count == 2);
    game_events_apply(&game);
    CHECK("simultaneous hits score one defeated target but consume both bullets",
          game.kills_score == 1 && game.tempScore == 1 &&
              !game.bullets[0].active && !game.bullets[1].active);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.kills_score = INT_MAX;
    game.tempScore = INT_MAX;
    game.enemyValues[0] = (Enemies){
        .x = 40.0f, .y = 100.0f, .prevX = 40.0f, .prevY = 100.0f,
        .visible = 1
    };
    game.bullets[0] = (Bullet){
        .x = 50.0f, .y = 110.0f, .prevX = 30.0f, .prevY = 110.0f,
        .dx = BULLET_SPEED_PER_SEC, .active = true
    };
    game_events_begin(&game);
    detect_projectile_hits(&game);
    game_events_apply(&game);
    CHECK("kill and progression counters saturate at INT_MAX",
          game.kills_score == INT_MAX && game.tempScore == INT_MAX);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.bullets[0] = (Bullet){
        .x = 50.0f, .y = 110.0f, .prevX = 50.0f, .prevY = 110.0f,
        .dx = 0.0f, .active = true
    };
    move_arcade_bullets(&game, PHYSICS_DT);
    CHECK("zero-speed projectile remains stationary instead of choosing right",
          game.bullets[0].active && game.bullets[0].x == 50.0f &&
              game.bullets[0].dx == 0.0f);

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        game.bullets[i].active = true;
    }
    game.bullets[MAX_BULLETS - 1].active = false;
    addBullet(&game, 12.0f, 34.0f, -1.0f);
    CHECK("pool reuse claims the only free boundary slot",
          game.bullets[MAX_BULLETS - 1].active &&
              game.bullets[MAX_BULLETS - 1].x == 12.0f);
    removeBullet(&game, -1);
    removeBullet(&game, MAX_BULLETS);
    addBullet(&game, NAN, 0.0f, 1.0f);
    CHECK("invalid pool operations cannot disturb a valid boundary slot",
          game.bullets[MAX_BULLETS - 1].active &&
              game.bullets[MAX_BULLETS - 1].x == 12.0f);
}

static void test_event_queue_corruption_and_ordering(void)
{
    GameState game;
    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.bullets[0].active = true;
    game.events.count = -1;
    CHECK("negative queue count rejects pushes",
          !game_events_push_projectile_hit(
              &game, false, 0, GAME_EVENT_TARGET_REGULAR_ENEMY, 0));
    game_events_apply(&game);
    CHECK("negative queue count is recovered without indexing", game.events.count == 0);

    game.events.count = MAX_GAME_EVENTS + 1;
    CHECK("oversized queue count rejects pushes",
          !game_events_push_transition_check(
              &game, GAME_EVENT_ARCADE_GAME_OVER_CHECK));
    game_events_apply(&game);
    CHECK("oversized queue count is recovered without indexing", game.events.count == 0);

    game.events.count = MAX_GAME_EVENTS;
    CHECK("full queue rejects another event without count overflow",
          !game_events_push_transition_check(
              &game, GAME_EVENT_ARCADE_GAME_OVER_CHECK) &&
              game.events.count == MAX_GAME_EVENTS);
    game_events_begin(&game);
    game.events.count = 1;
    game.events.events[0].type = (GameEventType)INT_MAX;
    game_events_apply(&game);
    CHECK("invalid queued event type is ignored safely", game.events.count == 0);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.app.scene = APP_SCENE_ARCADE_GAME;
    game_events_begin(&game);
    (void)game_events_push_player_contact(
        &game, GAME_EVENT_RUNNER_PLAYER_HIT, 0);
    game_events_apply(&game);
    CHECK("Runner event cannot mutate an active Arcade scene",
          !game.man.isDead && game.gameLives == 10);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.app.scene = APP_SCENE_RUNNER_GAME;
    game_events_begin(&game);
    (void)game_events_push_player_contact(
        &game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
    game_events_apply(&game);
    CHECK("Arcade event cannot mutate an active Runner scene",
          !game.man.isDead && game.gameLives == 10);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.app.scene = APP_SCENE_ARCADE_GAME;
    game.gameLives = 0;
    game.enemyValues[0] = (Enemies){
        .x = 40.0f, .y = 100.0f, .prevX = 40.0f, .prevY = 100.0f,
        .visible = 1
    };
    game.bullets[0] = (Bullet){
        .x = 50.0f, .y = 110.0f, .prevX = 30.0f, .prevY = 110.0f,
        .dx = BULLET_SPEED_PER_SEC, .active = true
    };
    game_events_begin(&game);
    (void)game_events_push_transition_check(
        &game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    (void)game_events_push_projectile_hit(
        &game, false, 0, GAME_EVENT_TARGET_REGULAR_ENEMY, 0);
    game_events_apply(&game);
    CHECK("terminal transition stops later consequences without UI side effects",
          game.app.scene == APP_SCENE_ARCADE_GAME &&
              game.enemyValues[0].visible && game.bullets[0].active);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.gameLives = 1;
    game.enemyValues[0].visible = 1;
    game.enemyValues[0].y = (float)HEIGHT;
    game.enemyValues[1].visible = 1;
    game.enemyValues[1].y = (float)HEIGHT;
    game_events_begin(&game);
    detect_arcade_hazards(&game);
    game_events_apply(&game);
    CHECK("simultaneous escapes saturate the shared life pool at zero",
          game.gameLives == 0 && game.man.lives == 0);
}

static void test_runner_per_player_hazard_gating(void)
{
    GameState game;
    reset_game(&game, GAME_MODE_MULTIPLAYER);
    game.app.scene = APP_SCENE_RUNNER_GAME;
    game.man.isDead = 1;
    game.secondPlayer.isDead = 0;
    game.secondPlayer.x = 50.0f;
    game.secondPlayer.y = 50.0f;
    game.secondPlayer.prevX = 50.0f;
    game.secondPlayer.prevY = 50.0f;
    game.stars[0].x = 50;
    game.stars[0].y = 50;
    game_events_begin(&game);
    detect_runner_hazard_contacts(&game);
    CHECK("live P2 still detects star contact while P1 is dead",
          count_player_events(&game, GAME_EVENT_RUNNER_PLAYER_HIT, 1) == 1);

    game.secondPlayer.y = (float)HEIGHT;
    game_events_begin(&game);
    detect_runner_fixed_hazards(&game);
    CHECK("live P2 still detects a fall while P1 is dead",
          count_player_events(&game, GAME_EVENT_RUNNER_PLAYER_FELL, 1) == 1);
}

static void test_simulation_and_interpolation_api_guards(void)
{
    GameState game;
    reset_game(&game, GAME_MODE_MULTIPLAYER);
    game.time = 17;
    game.man.x = 321.0f;
    game.man.prevX = 123.0f;
    game.feedback.hitStopTicks = 3;
    snapshot = game;

    arcade_simulate(NULL, PHYSICS_DT);
    runner_simulate(NULL, PHYSICS_DT);
    capture_player_prev_y(NULL);
    capture_render_transforms(NULL);
    sync_render_transforms(NULL);

    const float invalidDt[] = {
        0.0f, -PHYSICS_DT, NAN, INFINITY, -INFINITY,
        (float)MAX_FRAME_TIME + 1.0f
    };
    const size_t invalidDtCount = sizeof(invalidDt) / sizeof(invalidDt[0]);
    for (size_t i = 0U; i < invalidDtCount; i++)
    {
        arcade_simulate(&game, invalidDt[i]);
        runner_simulate(&game, invalidDt[i]);
    }
    CHECK("simulation entry points reject null and invalid dt without mutation",
          memcmp(&game, &snapshot, sizeof(game)) == 0);
    CHECK("render interpolation maps nonfinite alpha to the current endpoint",
          render_lerp(10.0f, 20.0f, NAN) == 20.0f &&
              render_lerp(10.0f, 20.0f, INFINITY) == 20.0f &&
              render_lerp(10.0f, 20.0f, -INFINITY) == 20.0f);
}

static void test_ai_and_dt_boundaries(void)
{
    GameState game;
    reset_game(&game, GAME_MODE_MULTIPLAYER);
    game.boss[0] = (Enemies){
        .x = 10.0f, .y = 10.0f, .prevX = 10.0f, .prevY = 10.0f,
        .dx = 5.0f, .dy = 5.0f, .visible = 1
    };
    const Enemies originalBoss = game.boss[0];
    move_boss_entities(&game, NAN);
    move_boss_entities(&game, INFINITY);
    move_boss_entities(&game, -PHYSICS_DT);
    move_boss_entities(&game, 0.0f);
    CHECK("AI rejects NaN, infinite, negative, and zero dt",
          memcmp(&game.boss[0], &originalBoss, sizeof(originalBoss)) == 0);

    game.enemyValues[0] = (Enemies){
        .x = NAN, .y = 10.0f, .prevX = 0.0f, .prevY = 10.0f,
        .visible = 1
    };
    move_regular_enemies(&game, PHYSICS_DT);
    CHECK("corrupted active AI is safely deactivated",
          !game.enemyValues[0].visible &&
              game.enemyValues[0].y == 1000.0f);

    game.enemyValues[0] = (Enemies){
        .x = 0.0f, .y = INFINITY, .prevX = 0.0f, .prevY = 0.0f,
        .visible = 1
    };
    game.boss[0] = game.enemyValues[0];
    game.smartEnemies[0] = game.enemyValues[0];
    move_regular_enemies(&game, PHYSICS_DT);
    move_boss_entities(&game, PHYSICS_DT);
    move_smart_enemies(&game, PHYSICS_DT);
    CHECK("positive-infinite AI coordinates cannot bypass deactivation",
          !game.enemyValues[0].visible && !game.boss[0].visible &&
              !game.smartEnemies[0].visible);

    game.smartEnemies[0] = (Enemies){
        .x = 0.0f, .y = 0.0f, .prevX = 0.0f, .prevY = 0.0f,
        .visible = 1
    };
    game.man.x = 1.0f;
    game.man.y = 0.0f;
    game.secondPlayer.x = 100.0f;
    game.secondPlayer.y = 0.0f;
    game.man.isDead = 1;
    CHECK("smart AI ignores a nearer dead player",
          smart_enemy_select_target(&game, &game.smartEnemies[0]) ==
              &game.secondPlayer);
    game.secondPlayer.isDead = 1;
    CHECK("smart AI has no target when both players are dead",
          smart_enemy_select_target(&game, &game.smartEnemies[0]) == NULL);

    const Enemies frozenSmart = game.smartEnemies[0];
    move_smart_enemies(&game, PHYSICS_DT);
    CHECK("smart AI remains still when no live target exists",
          memcmp(&game.smartEnemies[0], &frozenSmart,
                 sizeof(frozenSmart)) == 0);
}

static void test_world_and_process_boundaries(void)
{
    const Collider collider = player_world_collider();
    const Ledge walls[2] = {{200, 0, 20, 200}, {100, 0, 20, 200}};
    const Ledge reversedWalls[2] = {walls[1], walls[0]};
    KinematicBody first = {300.0f, 40.0f, 0.0f, 40.0f, 18000.0f, 0.0f};
    KinematicBody second = first;
    const WorldCollisionResult firstResult =
        resolve_kinematic_world(&first, &collider, walls, 2);
    const WorldCollisionResult secondResult =
        resolve_kinematic_world(&second, &collider, reversedWalls, 2);
    CHECK("high-speed X sweep hits the nearest wall independent of order",
          firstResult.hitRight && secondResult.hitRight &&
              first.x == 100.0f - collider.width && second.x == first.x);

    const Ledge ceilings[2] = {{0, 100, 500, 20}, {0, 200, 500, 20}};
    const Ledge reversedCeilings[2] = {ceilings[1], ceilings[0]};
    KinematicBody rising = {50.0f, 0.0f, 50.0f, 300.0f, 0.0f, -18000.0f};
    KinematicBody risingReversed = rising;
    const WorldCollisionResult ceilingResult =
        resolve_kinematic_world(&rising, &collider, ceilings, 2);
    const WorldCollisionResult ceilingReversedResult =
        resolve_kinematic_world(&risingReversed, &collider,
                                reversedCeilings, 2);
    CHECK("high-speed Y sweep hits the first ceiling independent of order",
          ceilingResult.hitCeiling && ceilingReversedResult.hitCeiling &&
              rising.y == 220.0f && risingReversed.y == rising.y);

    const Ledge wall = {100, 0, 20, 200};
    KinematicBody embeddedWall =
        {90.0f, 40.0f, 90.0f, 40.0f, 0.0f, 0.0f};
    const WorldCollisionResult embeddedWallResult =
        resolve_kinematic_world(&embeddedWall, &collider, &wall, 1);
    CHECK("zero-movement wall penetration resolves along the shallow axis",
          embeddedWallResult.hitLeft && embeddedWall.x == 120.0f);

    const Ledge platform = {0, 100, 500, 20};
    KinematicBody embeddedPlatform =
        {50.0f, 90.0f, 50.0f, 90.0f, 0.0f, 0.0f};
    const WorldCollisionResult embeddedPlatformResult =
        resolve_kinematic_world(&embeddedPlatform, &collider, &platform, 1);
    CHECK("zero-movement platform penetration resolves vertically",
          embeddedPlatformResult.hitCeiling && embeddedPlatform.y == 120.0f);

    GameState game;
    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.input.jumpBufferTicksPlayer2 = 5;
    game.secondPlayer.onLedge = 1;
    game.secondPlayer.coyoteTicksRemaining = 4;
    game.secondPlayer.airJumpsRemaining = 7;
    game.secondPlayer.damageInvulnerabilityTicks = 6;
    game.secondPlayer.doubleJumpAnimationTicks = 3;
    const Man hiddenBefore = game.secondPlayer;
    consume_arcade_jump_requests(&game);
    consume_runner_jump_requests(&game);
    CHECK("single-player jump consumption does not mutate hidden P2 state",
          memcmp(&game.secondPlayer, &hiddenBefore, sizeof(hiddenBefore)) == 0 &&
              game.input.jumpBufferTicksPlayer2 == 5);

    game.man.dx = 100.0f;
    game.input.moveLeftPlayer1 = true;
    game.input.moveRightPlayer1 = true;
    apply_arcade_player_forces(&game, PHYSICS_DT);
    CHECK("simultaneous Arcade left+right input is neutral",
          game.man.dx > 0.0f && game.man.dx < 100.0f &&
              game.man.slowingDown);
    game.man.dx = 100.0f;
    apply_runner_player_forces(&game, PHYSICS_DT);
    CHECK("simultaneous Runner left+right input is neutral",
          game.man.dx > 0.0f && game.man.dx < 100.0f &&
              game.man.slowingDown);

    const int originalTime = game.time;
    const float originalX = game.man.x;
    process(&game, NAN);
    process2(&game, -PHYSICS_DT);
    CHECK("process entry points reject invalid dt without advancing state",
          game.time == originalTime && game.man.x == originalX);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.time = INT_MAX;
    process2(&game, PHYSICS_DT);
    CHECK("simulation time wraps safely instead of signed overflow",
          game.time == 0);

    reset_game(&game, GAME_MODE_SINGLE_PLAYER);
    game.time = 2;
    game.cloud6.x = -150;
    arcade_presentation_step(&game);
    CHECK("cloud 6 uses its full 162-pixel wrap boundary",
          game.cloud6.x == -153);

    game.time = 20;
    game.CurrentSheetBullet = INT_MAX;
    game.CurrentSheetBullet2 = INT_MIN;
    game.CurrentSpriteBack = INT_MAX;
    game.enemy.currentSpriteRun = INT_MIN;
    game.enemy.currentSpriteRun2 = INT_MAX;
    game.CurrentSheetBoss = INT_MIN;
    arcade_presentation_step(&game);
    CHECK("corrupt presentation counters normalize without signed overflow",
          game.CurrentSheetBullet == 0 &&
              game.CurrentSheetBullet2 == 0 &&
              game.CurrentSpriteBack == 0 &&
              game.enemy.currentSpriteRun == 0 &&
              game.enemy.currentSpriteRun2 == 0 &&
              game.CurrentSheetBoss == 0);
}

int main(void)
{
    if (SDL_Init(0) != 0)
    {
        fprintf(stderr, "PRE-PHASE35 COLLISION/AI TEST: SDL_Init failed: %s\n",
                SDL_GetError());
        return 1;
    }
    test_authoritative_colliders_and_edges();
    test_multiplayer_symmetry_and_inactive_entities();
    test_projectile_snapshot_and_saturation();
    test_event_queue_corruption_and_ordering();
    test_runner_per_player_hazard_gating();
    test_simulation_and_interpolation_api_guards();
    test_ai_and_dt_boundaries();
    test_world_and_process_boundaries();
    SDL_Quit();

    if (failures == 0)
    {
        printf("PRE-PHASE35 COLLISION/AI TEST: ALL PASS\n");
    }
    return failures == 0 ? 0 : 1;
}
