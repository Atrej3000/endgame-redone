#include "game_events.h"
#include "leaderboard.h"
#include "app.h"
#include "combat_feedback.h"
#include "scene.h"

static bool event_queue_count_is_valid(const GameEventQueue *queue)
{
    return queue != NULL && queue->count >= 0 && queue->count <= MAX_GAME_EVENTS;
}

static int saturating_add_score(int value, int amount)
{
    if (amount <= 0) return value;
    if (value > INT_MAX - amount) return INT_MAX;
    return value + amount;
}

void game_events_begin(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    game->events.count = 0;
}

static bool projectile_target_index_is_valid(GameEventTarget target, int targetIndex)
{
    if (target == GAME_EVENT_TARGET_REGULAR_ENEMY)
    {
        return targetIndex >= 0 && targetIndex < NUM_ENEMIES;
    }
    if (target == GAME_EVENT_TARGET_SMART_ENEMY)
    {
        return targetIndex >= 0 && targetIndex < NUM_SMART_ENEMIES;
    }
    if (target == GAME_EVENT_TARGET_BOSS)
    {
        return targetIndex >= 0 && targetIndex < 2;
    }
    return false;
}

static bool projectile_already_has_event(const GameEventQueue *queue, bool secondPlayerProjectile,
                                         int projectileIndex)
{
    if (!event_queue_count_is_valid(queue)) return true;
    for (int i = 0; i < queue->count; i++)
    {
        const GameEvent *event = &queue->events[i];
        if (event->type == GAME_EVENT_PROJECTILE_HIT && event->secondPlayerProjectile == secondPlayerProjectile &&
            event->projectileIndex == projectileIndex)
        {
            return true;
        }
    }
    return false;
}

bool game_events_push_projectile_hit(GameState *game, bool secondPlayerProjectile,
                                     int projectileIndex, GameEventTarget target, int targetIndex)
{
    if (game == NULL || !event_queue_count_is_valid(&game->events) ||
        projectileIndex < 0 || projectileIndex >= MAX_BULLETS ||
        !projectile_target_index_is_valid(target, targetIndex) ||
        (secondPlayerProjectile && !game->multiPlayer))
    {
        return false;
    }
    const Bullet *projectile = secondPlayerProjectile
        ? &game->secondBullets[projectileIndex]
        : &game->bullets[projectileIndex];
    if (!projectile->active || game->events.count >= MAX_GAME_EVENTS ||
        projectile_already_has_event(&game->events, secondPlayerProjectile, projectileIndex))
    {
        return false;
    }

    game->events.events[game->events.count++] = (GameEvent){GAME_EVENT_PROJECTILE_HIT, target,
        projectileIndex, targetIndex, secondPlayerProjectile};
    return true;
}

static bool event_already_queued(const GameEventQueue *queue, GameEventType type,
                                 int sourceIndex, int targetIndex)
{
    if (!event_queue_count_is_valid(queue)) return true;
    for (int i = 0; i < queue->count; i++)
    {
        const GameEvent *event = &queue->events[i];
        if (event->type == type && event->projectileIndex == sourceIndex &&
            event->targetIndex == targetIndex)
        {
            return true;
        }
    }
    return false;
}

static bool push_event(GameState *game, GameEventType type, int sourceIndex, int targetIndex)
{
    if (game == NULL || !event_queue_count_is_valid(&game->events) ||
        game->events.count >= MAX_GAME_EVENTS ||
        event_already_queued(&game->events, type, sourceIndex, targetIndex))
    {
        return false;
    }

    game->events.events[game->events.count++] =
        (GameEvent){type, GAME_EVENT_TARGET_REGULAR_ENEMY, sourceIndex, targetIndex, false};
    return true;
}

bool game_events_push_player_contact(GameState *game, GameEventType type, int playerIndex)
{
    const bool validType = type == GAME_EVENT_ARCADE_PLAYER_HIT ||
                           type == GAME_EVENT_ARCADE_PLAYER_FELL ||
                           type == GAME_EVENT_RUNNER_PLAYER_HIT ||
                           type == GAME_EVENT_RUNNER_PLAYER_FELL;
    if (game == NULL || !validType || playerIndex < 0 || playerIndex > 1 ||
        (playerIndex == 1 && !game->multiPlayer))
    {
        return false;
    }
    return push_event(game, type, playerIndex, -1);
}

bool game_events_push_target_contact(GameState *game, GameEventType type, int targetIndex)
{
    if (game == NULL ||
        (type == GAME_EVENT_ARCADE_ENEMY_ESCAPED &&
         (targetIndex < 0 || targetIndex >= NUM_ENEMIES)) ||
        (type == GAME_EVENT_ARCADE_BOSS_ESCAPED &&
         (targetIndex < 0 || targetIndex >= 2)) ||
        (type != GAME_EVENT_ARCADE_ENEMY_ESCAPED &&
         type != GAME_EVENT_ARCADE_BOSS_ESCAPED))
    {
        return false;
    }
    return push_event(game, type, -1, targetIndex);
}

bool game_events_push_transition_check(GameState *game, GameEventType type)
{
    if (game == NULL ||
        (type != GAME_EVENT_ARCADE_GAME_OVER_CHECK &&
         type != GAME_EVENT_RUNNER_GAME_OVER_CHECK))
    {
        return false;
    }
    return push_event(game, type, -1, -1);
}

static void play_damage_sound(GameState *game)
{
    if (game->audio.damage != NULL)
    {
        Mix_PlayChannel(-1, game->audio.damage, 0);
    }
}

static void play_kick_sound(GameState *game)
{
    if (game->audio.kick != NULL)
    {
        Mix_PlayChannel(-1, game->audio.kick, 0);
    }
}

static void remove_event_projectile(GameState *game, const GameEvent *event)
{
    if (event->secondPlayerProjectile)
    {
        removeSecondBullet(game, event->projectileIndex);
    }
    else
    {
        removeBullet(game, event->projectileIndex);
    }
}

static bool apply_regular_enemy_hit(GameState *game, const GameEvent *event)
{
    Enemies *enemy = &game->enemyValues[event->targetIndex];
    if (enemy->y >= 1000.0f || !enemy->visible)
    {
        return false;
    }
    play_damage_sound(game);
    combat_feedback_trigger_enemy_hit(game, enemy, true, false);
    enemy->y = 1000.0f;
    enemy->visible = 0;
    if (event->secondPlayerProjectile)
    {
        game->kills_score_multi = saturating_add_score(game->kills_score_multi, 1);
    }
    else
    {
        game->kills_score = saturating_add_score(game->kills_score, 1);
    }
    game->tempScore = saturating_add_score(game->tempScore, 1);
    return true;
}

static bool apply_smart_enemy_hit(GameState *game, const GameEvent *event)
{
    Enemies *enemy = &game->smartEnemies[event->targetIndex];
    if (enemy->y >= 1000.0f || !enemy->visible)
    {
        return false;
    }
    enemy->countShots = saturating_add_score(enemy->countShots, 1);
    const bool defeated = enemy->countShots > 5;
    combat_feedback_trigger_enemy_hit(game, enemy, defeated, false);
    if (!event->secondPlayerProjectile && enemy->countShots > 5)
    {
        play_kick_sound(game);
    }
    if (enemy->countShots > 5)
    {
        play_damage_sound(game);
        enemy->y = 1000.0f;
        enemy->visible = 0;
        if (event->secondPlayerProjectile)
        {
            game->kills_score_multi = saturating_add_score(game->kills_score_multi, 5);
        }
        else
        {
            game->kills_score = saturating_add_score(game->kills_score, 5);
        }
        game->tempScore = saturating_add_score(game->tempScore, 5);
    }
    return true;
}

static bool apply_boss_hit(GameState *game, const GameEvent *event)
{
    Enemies *boss = &game->boss[event->targetIndex];
    if (boss->y >= 1000.0f || !boss->visible)
    {
        return false;
    }
    boss->countShots = saturating_add_score(boss->countShots, 1);
    const bool defeated = boss->countShots > 30;
    combat_feedback_trigger_enemy_hit(game, boss, defeated, true);
    if (boss->countShots > 30)
    {
        play_damage_sound(game);
        boss->y = 1000.0f;
        boss->visible = 0;
        if (event->secondPlayerProjectile)
        {
            game->kills_score_multi = saturating_add_score(game->kills_score_multi, 10);
        }
        else
        {
            game->kills_score = saturating_add_score(game->kills_score, 10);
        }
        game->tempScore = saturating_add_score(game->tempScore, 10);
    }
    return true;
}

static void set_arcade_game_over_lives(GameState *game)
{
    game->gameLives = 0;
    game->man.lives = 0;
    if (game->multiPlayer)
    {
        game->secondPlayer.lives = 0;
    }
}

static void decrement_arcade_life(GameState *game)
{
    if (game->gameLives > 0)
    {
        game->gameLives--;
    }
    if (game->gameLives <= 0)
    {
        set_arcade_game_over_lives(game);
    }
}

static void reset_arcade_player_after_damage(GameState *game, int playerIndex)
{
    Man *player = playerIndex == 0 ? &game->man : &game->secondPlayer;
    player->x = playerIndex == 0 ? 280.0f : 1000.0f;
    player->y = 200.0f;
    player->dx = 0.0f;
    player->dy = 0.0f;
    player->prevX = player->x;
    player->prevY = player->y;
    player->onLedge = 0;
    player->isDead = 0;
    player->coyoteTicksRemaining = 0;
    player->airJumpsRemaining = 0;
    player->doubleJumpAnimationTicks = 0;
    player->jumpKeyHeldLastTick = false;
    player->slowingDown = 0;
    player->damageInvulnerabilityTicks = PLAYER_DAMAGE_INVULNERABILITY_TICKS;
    if (playerIndex == 0)
    {
        game->input.jumpBufferTicksPlayer1 = 0;
    }
    else
    {
        game->input.jumpBufferTicksPlayer2 = 0;
    }
}

static bool damage_arcade_player(GameState *game, int playerIndex, bool fell)
{
    if (playerIndex < 0 || playerIndex > 1 ||
        (playerIndex == 1 && !game->multiPlayer))
    {
        return false;
    }
    Man *player = (playerIndex == 0) ? &game->man : &game->secondPlayer;
    if (player->isDead || player->damageInvulnerabilityTicks > 0 ||
        game->gameLives <= 0)
    {
        return false;
    }
    (void)fell;
    decrement_arcade_life(game);
    reset_arcade_player_after_damage(game, playerIndex);
    return true;
}

static void apply_arcade_enemy_escaped(GameState *game, int enemyIndex)
{
    if (enemyIndex < 0 || enemyIndex >= NUM_ENEMIES ||
        !game->enemyValues[enemyIndex].visible ||
        game->enemyValues[enemyIndex].y >= 1000.0f)
    {
        return;
    }
    decrement_arcade_life(game);
    game->enemyValues[enemyIndex].y = 1000.0f;
    game->enemyValues[enemyIndex].visible = 0;
}

static void apply_arcade_game_over_transition(GameState *game)
{
    if (game->gameLives <= 0 && game->app.scene == APP_SCENE_ARCADE_GAME)
    {
        set_arcade_game_over_lives(game);
        if (game->simulationOnly)
        {
            return;
        }
        app_change_scene(game, APP_SCENE_ARCADE_MENU);
    }
}

static void apply_runner_game_over_transition(GameState *game)
{
    if (game->gameLives > 0 || game->app.scene != APP_SCENE_RUNNER_GAME)
    {
        return;
    }
    if (game->simulationOnly)
    {
        return;
    }
    leaderboard_record_score(game, game->x_score);
    app_change_scene(game, APP_SCENE_RUNNER_MENU);
}

static bool game_event_is_valid(const GameState *game, const GameEvent *event)
{
    if (game == NULL || event == NULL) return false;
    switch (event->type)
    {
        case GAME_EVENT_PROJECTILE_HIT:
            return event->projectileIndex >= 0 &&
                   event->projectileIndex < MAX_BULLETS &&
                   projectile_target_index_is_valid(event->target,
                                                    event->targetIndex) &&
                   (!event->secondPlayerProjectile || game->multiPlayer);
        case GAME_EVENT_ARCADE_PLAYER_HIT:
        case GAME_EVENT_ARCADE_PLAYER_FELL:
        case GAME_EVENT_RUNNER_PLAYER_HIT:
        case GAME_EVENT_RUNNER_PLAYER_FELL:
            return event->projectileIndex >= 0 &&
                   event->projectileIndex <= 1 &&
                   (event->projectileIndex == 0 || game->multiPlayer);
        case GAME_EVENT_ARCADE_ENEMY_ESCAPED:
            return event->targetIndex >= 0 &&
                   event->targetIndex < NUM_ENEMIES;
        case GAME_EVENT_ARCADE_BOSS_ESCAPED:
            return event->targetIndex >= 0 && event->targetIndex < 2;
        case GAME_EVENT_ARCADE_GAME_OVER_CHECK:
        case GAME_EVENT_RUNNER_GAME_OVER_CHECK:
            return true;
    }
    return false;
}

static bool event_matches_active_gameplay_scene(const GameState *game,
                                                const GameEvent *event)
{
    const bool arcadeEvent =
        event->type == GAME_EVENT_PROJECTILE_HIT ||
        event->type == GAME_EVENT_ARCADE_PLAYER_HIT ||
        event->type == GAME_EVENT_ARCADE_PLAYER_FELL ||
        event->type == GAME_EVENT_ARCADE_ENEMY_ESCAPED ||
        event->type == GAME_EVENT_ARCADE_BOSS_ESCAPED ||
        event->type == GAME_EVENT_ARCADE_GAME_OVER_CHECK;
    const bool runnerEvent =
        event->type == GAME_EVENT_RUNNER_PLAYER_HIT ||
        event->type == GAME_EVENT_RUNNER_PLAYER_FELL ||
        event->type == GAME_EVENT_RUNNER_GAME_OVER_CHECK;
    if (game->app.scene == APP_SCENE_ARCADE_GAME) return arcadeEvent;
    if (game->app.scene == APP_SCENE_RUNNER_GAME) return runnerEvent;
    return false;
}

void game_events_apply(GameState *game)
{
    if (game == NULL)
    {
        return;
    }
    if (!event_queue_count_is_valid(&game->events))
    {
        game->events.count = 0;
        return;
    }
    const int eventCount = game->events.count;
    const AppScene sceneBeforeApply = game->app.scene;
    for (int i = 0; i < eventCount; i++)
    {
        const GameEvent *event = &game->events.events[i];
        if (!game_event_is_valid(game, event) ||
            !event_matches_active_gameplay_scene(game, event))
        {
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_PLAYER_HIT)
        {
            if (damage_arcade_player(game, event->projectileIndex, false))
            {
                combat_feedback_trigger_player_hit(game, event->projectileIndex);
            }
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_PLAYER_FELL)
        {
            if (damage_arcade_player(game, event->projectileIndex, true))
            {
                combat_feedback_trigger_player_hit(game, event->projectileIndex);
            }
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_ENEMY_ESCAPED)
        {
            apply_arcade_enemy_escaped(game, event->targetIndex);
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_BOSS_ESCAPED)
        {
            if (event->targetIndex >= 0 && event->targetIndex < 2)
            {
                game->boss[event->targetIndex].y = 1000.0f;
                game->boss[event->targetIndex].visible = 0;
            }
            set_arcade_game_over_lives(game);
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_GAME_OVER_CHECK)
        {
            const bool terminal =
                game->gameLives <= 0 &&
                game->app.scene == APP_SCENE_ARCADE_GAME;
            apply_arcade_game_over_transition(game);
            if (terminal || game->app.scene != sceneBeforeApply)
            {
                break;
            }
            continue;
        }
        if (event->type == GAME_EVENT_RUNNER_PLAYER_HIT ||
            event->type == GAME_EVENT_RUNNER_PLAYER_FELL)
        {
            combat_feedback_trigger_player_hit(game, event->projectileIndex);
            runner_trigger_death(game);
            continue;
        }
        if (event->type == GAME_EVENT_RUNNER_GAME_OVER_CHECK)
        {
            const bool terminal =
                game->gameLives <= 0 &&
                game->app.scene == APP_SCENE_RUNNER_GAME;
            apply_runner_game_over_transition(game);
            if (terminal || game->app.scene != sceneBeforeApply)
            {
                break;
            }
            continue;
        }
        if (event->type != GAME_EVENT_PROJECTILE_HIT)
        {
            continue;
        }

        bool applied = false;
        if (event->target == GAME_EVENT_TARGET_REGULAR_ENEMY)
        {
            applied = apply_regular_enemy_hit(game, event);
        }
        else if (event->target == GAME_EVENT_TARGET_SMART_ENEMY)
        {
            applied = apply_smart_enemy_hit(game, event);
        }
        else if (event->target == GAME_EVENT_TARGET_BOSS)
        {
            applied = apply_boss_hit(game, event);
        }
        // Detection is snapshot-based. If two projectiles hit the same target
        // in one tick, the first may defeat it, but the second still made
        // contact and must not pass through merely because consequences are
        // applied sequentially. Score/damage remains guarded by `applied`.
        (void)applied;
        remove_event_projectile(game, event);
        if (game->app.scene != sceneBeforeApply)
        {
            break;
        }
    }
    game->events.count = 0;
}
