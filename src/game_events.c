#include "game_events.h"
#include "app.h"
#include "scene.h"

void game_events_begin(GameState *game)
{
    game->events.count = 0;
}

static bool projectile_already_has_event(const GameEventQueue *queue, bool secondPlayerProjectile,
                                         int projectileIndex)
{
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
    if (game->events.count >= MAX_GAME_EVENTS ||
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
    if (game->events.count >= MAX_GAME_EVENTS ||
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
    return push_event(game, type, playerIndex, -1);
}

bool game_events_push_target_contact(GameState *game, GameEventType type, int targetIndex)
{
    return push_event(game, type, -1, targetIndex);
}

bool game_events_push_transition_check(GameState *game, GameEventType type)
{
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
    if (enemy->y >= 1000.0f)
    {
        return false;
    }
    play_damage_sound(game);
    enemy->y = 1000.0f;
    enemy->visible = 0;
    if (event->secondPlayerProjectile)
    {
        game->kills_score_multi++;
    }
    else
    {
        game->kills_score++;
    }
    game->tempScore++;
    return true;
}

static bool apply_smart_enemy_hit(GameState *game, const GameEvent *event)
{
    Enemies *enemy = &game->smartEnemies[event->targetIndex];
    if (enemy->y >= 1000.0f)
    {
        return false;
    }
    enemy->countShots++;
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
            game->kills_score_multi += 5;
        }
        else
        {
            game->kills_score += 5;
        }
        game->tempScore += 5;
    }
    return true;
}

static bool apply_boss_hit(GameState *game, const GameEvent *event)
{
    Enemies *boss = &game->boss[event->targetIndex];
    if (boss->y >= 1000.0f)
    {
        return false;
    }
    boss->countShots++;
    if (boss->countShots > 30)
    {
        play_damage_sound(game);
        boss->y = 1000.0f;
        boss->visible = 0;
        if (event->secondPlayerProjectile)
        {
            game->kills_score_multi += 10;
        }
        else
        {
            game->kills_score += 10;
        }
        game->tempScore += 10;
    }
    return true;
}

static void defeat_arcade_player(GameState *game, int playerIndex, bool moveOffscreen)
{
    Man *player = (playerIndex == 0) ? &game->man : &game->secondPlayer;
    player->lives = 0;
    if (moveOffscreen)
    {
        player->y = 1000.0f;
    }
}

static void apply_arcade_enemy_escaped(GameState *game)
{
    game->gameLives--;
    if (!game->multiPlayer)
    {
        game->kills_score++;
        game->tempScore++;
        if (game->gameLives == 0)
        {
            game->man.lives = 0;
        }
        return;
    }

    if (random() % 2 == 0)
    {
        game->kills_score++;
    }
    else
    {
        game->kills_score_multi++;
    }
    game->tempScore++;
    if (game->gameLives == 0)
    {
        game->man.lives = 0;
        game->secondPlayer.lives = 0;
    }
}

static void apply_arcade_game_over_transition(GameState *game)
{
    bool gameOver = !game->multiPlayer ? game->man.lives == 0
                                       : game->man.lives == 0 && game->secondPlayer.lives == 0;
    if (gameOver && game->app.scene == APP_SCENE_ARCADE_GAME)
    {
        app_change_scene(game, APP_SCENE_ARCADE_MENU);
    }
}

static void apply_runner_game_over_transition(GameState *game)
{
    if (game->gameLives != 0)
    {
        return;
    }
    if (game->x_i < 25)
    {
        game->x_list[game->x_i] = game->x_score;
        game->x_i++;
    }
    if (game->app.scene == APP_SCENE_RUNNER_GAME)
    {
        app_change_scene(game, APP_SCENE_RUNNER_MENU);
    }
}

void game_events_apply(GameState *game)
{
    for (int i = 0; i < game->events.count; i++)
    {
        const GameEvent *event = &game->events.events[i];
        if (event->type == GAME_EVENT_ARCADE_PLAYER_HIT)
        {
            defeat_arcade_player(game, event->projectileIndex, true);
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_PLAYER_FELL)
        {
            defeat_arcade_player(game, event->projectileIndex, false);
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_ENEMY_ESCAPED)
        {
            apply_arcade_enemy_escaped(game);
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_BOSS_ESCAPED)
        {
            game->gameLives = 0;
            game->man.lives = 0;
            continue;
        }
        if (event->type == GAME_EVENT_ARCADE_GAME_OVER_CHECK)
        {
            apply_arcade_game_over_transition(game);
            continue;
        }
        if (event->type == GAME_EVENT_RUNNER_PLAYER_HIT ||
            event->type == GAME_EVENT_RUNNER_PLAYER_FELL)
        {
            runner_trigger_death(game);
            continue;
        }
        if (event->type == GAME_EVENT_RUNNER_GAME_OVER_CHECK)
        {
            apply_runner_game_over_transition(game);
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
        if (applied)
        {
            remove_event_projectile(game, event);
        }
    }
    game->events.count = 0;
}
