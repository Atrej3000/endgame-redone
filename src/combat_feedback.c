#include "combat_feedback.h"

#include <stdint.h>

#define FEEDBACK_PARTICLE_GRAVITY_PER_SEC2 900.0f
#define FEEDBACK_PARTICLE_TICKS 18
#define FEEDBACK_DEATH_PARTICLE_TICKS 26
#define FEEDBACK_SHAKE_PIXELS 4
#define FEEDBACK_TRANSITION_TICKS 7

static void arm_hit_stop(CombatFeedbackState *feedback, int ticks)
{
    if (feedback->hitStopTicks < ticks)
    {
        feedback->hitStopTicks = ticks;
    }
}

static void arm_screen_shake(GameState *game)
{
    if (!game->app.settings.screenShake)
    {
        return;
    }
    game->feedback.screenShakeTicks = FEEDBACK_SCREEN_SHAKE_TICKS;
}

static FeedbackParticle *claim_particle(GameState *game)
{
    for (int i = 0; i < MAX_FEEDBACK_PARTICLES; i++)
    {
        if (!game->feedback.particles[i].active)
        {
            return &game->feedback.particles[i];
        }
    }
    return &game->feedback.particles[0];
}

static void spawn_particle(GameState *game, float x, float y, float dx, float dy,
                           int lifetime, Uint8 red, Uint8 green, Uint8 blue)
{
    if (game == NULL || lifetime <= 0 ||
        !isfinite(x) || !isfinite(y) || !isfinite(dx) || !isfinite(dy))
    {
        return;
    }
    FeedbackParticle *particle = claim_particle(game);
    *particle = (FeedbackParticle){true, x, y, dx, dy, lifetime, lifetime, red, green, blue};
}

static void spawn_burst(GameState *game, float x, float y, bool defeated, bool boss)
{
    const int count = defeated ? 6 : 3;
    const int lifetime = defeated ? FEEDBACK_DEATH_PARTICLE_TICKS : FEEDBACK_PARTICLE_TICKS;
    const Uint8 red = 255U;
    const Uint8 green = boss ? 95U : (defeated ? 185U : 240U);
    const Uint8 blue = boss ? 45U : (defeated ? 45U : 110U);
    for (int i = 0; i < count; i++)
    {
        const float direction = (float)(i - count / 2);
        spawn_particle(game, x, y, direction * 42.0f, -90.0f - (float)(i % 3) * 28.0f,
                       lifetime, red, green, blue);
    }
}

void combat_feedback_reset(GameState *game)
{
    if (game == NULL) return;
    game->feedback = (CombatFeedbackState){0};
}

void combat_feedback_trigger_player_spawn(GameState *game, int playerIndex)
{
    if (game == NULL || playerIndex < 0 || playerIndex > 1)
    {
        return;
    }
    const Man *player = playerIndex == 0 ? &game->man : &game->secondPlayer;
    game->feedback.appearingTicks[playerIndex] = FEEDBACK_TRANSITION_TICKS;
    game->feedback.transitionX[playerIndex] = player->x;
    game->feedback.transitionY[playerIndex] = player->y;
}

void combat_feedback_trigger_shot(GameState *game, int playerIndex, float x, float y)
{
    if (game == NULL || playerIndex < 0 || playerIndex > 1)
    {
        return;
    }
    game->feedback.muzzleFlashTicks[playerIndex] = FEEDBACK_MUZZLE_FLASH_TICKS;
    const Man *player = playerIndex == 0 ? &game->man : &game->secondPlayer;
    const float direction = player->facingLeft ? -1.0f : 1.0f;
    spawn_particle(game, x, y, direction * 120.0f, -35.0f,
                   FEEDBACK_PARTICLE_TICKS / 2, 255U, 230U, 100U);
}

void combat_feedback_trigger_enemy_hit(GameState *game, Enemies *enemy, bool defeated, bool boss)
{
    if (game == NULL || enemy == NULL)
    {
        return;
    }
    enemy->hitFlashTicks = FEEDBACK_HIT_FLASH_TICKS;
    spawn_burst(game, enemy->x + enemy->w * 0.5f, enemy->y + enemy->h * 0.5f, defeated, boss);
    arm_hit_stop(&game->feedback, boss ? FEEDBACK_BOSS_HIT_STOP_TICKS : FEEDBACK_HIT_STOP_TICKS);
    arm_screen_shake(game);
}

void combat_feedback_trigger_player_hit(GameState *game, int playerIndex)
{
    if (game == NULL || playerIndex < 0 || playerIndex > 1)
    {
        return;
    }
    Man *player = playerIndex == 0 ? &game->man : &game->secondPlayer;
    player->hitFlashTicks = FEEDBACK_HIT_FLASH_TICKS;
    game->feedback.disappearingTicks[playerIndex] = FEEDBACK_TRANSITION_TICKS;
    game->feedback.transitionX[playerIndex] = player->x;
    game->feedback.transitionY[playerIndex] = player->y;
    spawn_burst(game, player->x + 24.0f, player->y + 24.0f, false, false);
    arm_hit_stop(&game->feedback, FEEDBACK_HIT_STOP_TICKS);
    arm_screen_shake(game);
}

static void update_particles(CombatFeedbackState *feedback)
{
    for (int i = 0; i < MAX_FEEDBACK_PARTICLES; i++)
    {
        FeedbackParticle *particle = &feedback->particles[i];
        if (!particle->active)
        {
            continue;
        }
        if (particle->ticksRemaining <= 0 || particle->totalTicks <= 0 ||
            !isfinite(particle->x) || !isfinite(particle->y) ||
            !isfinite(particle->dx) || !isfinite(particle->dy))
        {
            particle->active = false;
            continue;
        }
        particle->x += particle->dx * PHYSICS_DT;
        particle->y += particle->dy * PHYSICS_DT;
        particle->dy += FEEDBACK_PARTICLE_GRAVITY_PER_SEC2 * PHYSICS_DT;
        particle->ticksRemaining--;
        if (particle->ticksRemaining <= 0)
        {
            particle->active = false;
        }
    }
}

static void decrement_hit_flash(Enemies *enemy)
{
    if (enemy->hitFlashTicks > 0)
    {
        enemy->hitFlashTicks--;
    }
}

static void decrement_feedback_timers(GameState *game)
{
    for (int player = 0; player < 2; player++)
    {
        if (game->feedback.muzzleFlashTicks[player] > 0)
        {
            game->feedback.muzzleFlashTicks[player]--;
        }
        if (game->feedback.appearingTicks[player] > 0)
        {
            game->feedback.appearingTicks[player]--;
        }
        if (game->feedback.disappearingTicks[player] > 0)
        {
            game->feedback.disappearingTicks[player]--;
        }
    }
    if (game->man.hitFlashTicks > 0) game->man.hitFlashTicks--;
    if (game->secondPlayer.hitFlashTicks > 0) game->secondPlayer.hitFlashTicks--;
    for (int i = 0; i < NUM_ENEMIES; i++) decrement_hit_flash(&game->enemyValues[i]);
    for (int i = 0; i < NUM_SMART_ENEMIES; i++) decrement_hit_flash(&game->smartEnemies[i]);
    for (int i = 0; i < 2; i++) decrement_hit_flash(&game->boss[i]);
}

bool combat_feedback_step(GameState *game)
{
    if (game == NULL) return false;
    decrement_feedback_timers(game);
    update_particles(&game->feedback);

    if (game->feedback.screenShakeTicks > 0)
    {
        const int sign = (game->feedback.screenShakeTicks % 2 == 0) ? 1 : -1;
        game->feedback.screenShakeOffsetX = sign * FEEDBACK_SHAKE_PIXELS;
        game->feedback.screenShakeOffsetY = -sign * (FEEDBACK_SHAKE_PIXELS / 2);
        game->feedback.screenShakeTicks--;
    }
    else
    {
        game->feedback.screenShakeOffsetX = 0;
        game->feedback.screenShakeOffsetY = 0;
    }

    if (game->feedback.hitStopTicks <= 0)
    {
        return false;
    }
    game->feedback.hitStopTicks--;
    return true;
}

static void update_animation(AnimationPlayer *animation, AnimationState state, int frameCount)
{
    if (animation == NULL || frameCount < 1) return;
    if (animation->state != state || animation->frameCount != frameCount)
    {
        *animation = (AnimationPlayer){state, 0, FEEDBACK_ANIMATION_TICKS, frameCount};
        return;
    }
    if (frameCount <= 1)
    {
        animation->frame = 0;
        animation->ticksUntilNextFrame = FEEDBACK_ANIMATION_TICKS;
        return;
    }
    if (animation->frame < 0 || animation->frame >= frameCount)
    {
        animation->frame = 0;
    }
    if (animation->ticksUntilNextFrame <= 1)
    {
        animation->frame = (animation->frame + 1) % frameCount;
        animation->ticksUntilNextFrame = FEEDBACK_ANIMATION_TICKS;
    }
    else
    {
        animation->ticksUntilNextFrame--;
    }
}

static void update_player_animation(const GameState *game, Man *player)
{
    AnimationState state = ANIMATION_STATE_IDLE;
    if (player->hitFlashTicks > 0)
    {
        state = ANIMATION_STATE_HIT;
    }
    else if (player->isDead)
    {
        state = ANIMATION_STATE_DEAD;
    }
    else if (player->doubleJumpAnimationTicks > 0)
    {
        state = ANIMATION_STATE_DOUBLE_JUMP;
    }
    else if (!player->onLedge)
    {
        state = player->dy < 0.0f ? ANIMATION_STATE_JUMP : ANIMATION_STATE_FALL;
    }
    else if (fabsf(player->dx) >= RUN_SNAP_ZERO_SPEED_PER_SEC)
    {
        state = ANIMATION_STATE_RUN;
    }
    const bool primaryPlayer = player == &game->man;
    int frameCount = 1;
    if (primaryPlayer)
    {
        switch (state)
        {
            case ANIMATION_STATE_IDLE: frameCount = 11; break;
            case ANIMATION_STATE_RUN: frameCount = 12; break;
            case ANIMATION_STATE_JUMP: frameCount = 1; break;
            case ANIMATION_STATE_DOUBLE_JUMP: frameCount = 6; break;
            case ANIMATION_STATE_WALL_JUMP: frameCount = 5; break;
            case ANIMATION_STATE_FALL: frameCount = 1; break;
            case ANIMATION_STATE_HIT: frameCount = 7; break;
            case ANIMATION_STATE_DEAD: frameCount = 1; break;
        }
    }
    else if (game->app.scene == APP_SCENE_ARCADE_GAME)
    {
        switch (state)
        {
            case ANIMATION_STATE_IDLE: frameCount = 11; break;
            case ANIMATION_STATE_RUN: frameCount = 12; break;
            case ANIMATION_STATE_JUMP: frameCount = 1; break;
            case ANIMATION_STATE_DOUBLE_JUMP: frameCount = 6; break;
            case ANIMATION_STATE_WALL_JUMP: frameCount = 5; break;
            case ANIMATION_STATE_FALL: frameCount = 1; break;
            case ANIMATION_STATE_HIT: frameCount = 7; break;
            case ANIMATION_STATE_DEAD: frameCount = 1; break;
        }
    }
    else if (state == ANIMATION_STATE_RUN)
    {
        frameCount = 12;
    }
    else if (state == ANIMATION_STATE_JUMP || state == ANIMATION_STATE_FALL || state == ANIMATION_STATE_HIT)
    {
        frameCount = 12;
    }
    update_animation(&player->animation, state, frameCount);
}

static void update_enemy_animation(Enemies *enemy, int frameCount)
{
    const AnimationState state = enemy->visible ? ANIMATION_STATE_RUN : ANIMATION_STATE_IDLE;
    update_animation(&enemy->animation, state, state == ANIMATION_STATE_RUN ? frameCount : 1);
}

void combat_feedback_update_animations(GameState *game)
{
    if (game == NULL) return;
    update_player_animation(game, &game->man);
    if (game->multiPlayer)
    {
        update_player_animation(game, &game->secondPlayer);
    }
    for (int i = 0; i < NUM_ENEMIES; i++) update_enemy_animation(&game->enemyValues[i], 4);
    for (int i = 0; i < NUM_SMART_ENEMIES; i++) update_enemy_animation(&game->smartEnemies[i], 6);
    for (int i = 0; i < 2; i++) update_enemy_animation(&game->boss[i], 4);
}

void combat_feedback_begin_render(SDL_Renderer *renderer, const GameState *game)
{
    if (renderer == NULL || game == NULL) return;
    const SDL_Rect viewport = {game->feedback.screenShakeOffsetX,
                               game->feedback.screenShakeOffsetY, WIDTH, HEIGHT};
    SDL_RenderSetViewport(renderer, &viewport);
}

static void draw_flash_rect(SDL_Renderer *renderer, int x, int y, int width, int height)
{
    SDL_Rect rect = {x, y, width, height};
    SDL_SetRenderDrawColor(renderer, 255U, 255U, 255U, 105U);
    SDL_RenderFillRect(renderer, &rect);
}

static int feedback_render_pixel(float value)
{
    if (isnan(value)) return 0;
    if (value <= (float)INT_MIN) return INT_MIN;
    if (value >= (float)INT_MAX) return INT_MAX;
    return (int)lroundf(value);
}

void combat_feedback_draw(SDL_Renderer *renderer, const GameState *game, float worldOffsetX)
{
    if (renderer == NULL || game == NULL) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    const Man *players[2] = {&game->man, &game->secondPlayer};
    if (game->shadowEffect != NULL)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 1 && !game->multiPlayer) continue;
            SDL_Rect shadow = {feedback_render_pixel(worldOffsetX + players[i]->x + 3.0f),
                               feedback_render_pixel(players[i]->y + 47.0f), 48, 12};
            SDL_RenderCopy(renderer, game->shadowEffect, NULL, &shadow);
        }
    }
    for (int i = 0; i < MAX_FEEDBACK_PARTICLES; i++)
    {
        const FeedbackParticle *particle = &game->feedback.particles[i];
        if (!particle->active)
        {
            continue;
        }
        int alpha = 0;
        if (particle->totalTicks > 0 && particle->ticksRemaining > 0)
        {
            const int64_t scaled = (int64_t)particle->ticksRemaining * 255;
            const int64_t computed = scaled / particle->totalTicks;
            alpha = computed > 255 ? 255 : (int)computed;
        }
        SDL_Rect rect = {feedback_render_pixel(worldOffsetX + particle->x),
                         feedback_render_pixel(particle->y), 10, 10};
        if (game->dustParticle != NULL)
        {
            SDL_SetTextureColorMod(game->dustParticle, particle->red, particle->green, particle->blue);
            SDL_SetTextureAlphaMod(game->dustParticle, (Uint8)alpha);
            SDL_RenderCopy(renderer, game->dustParticle, NULL, &rect);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, particle->red, particle->green, particle->blue, (Uint8)alpha);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (i == 1 && !game->multiPlayer) continue;
        if (game->feedback.muzzleFlashTicks[i] > 0)
        {
            const int direction = i == 0 && players[i]->facingLeft ? -1 : 1;
            SDL_Rect muzzle = {feedback_render_pixel(worldOffsetX + players[i]->x +
                                                     25.0f * (float)direction),
                               feedback_render_pixel(players[i]->y + 20.0f), 16, 10};
            SDL_SetRenderDrawColor(renderer, 255U, 222U, 80U, 220U);
            SDL_RenderFillRect(renderer, &muzzle);
        }
        if (players[i]->hitFlashTicks > 0)
        {
            draw_flash_rect(renderer, feedback_render_pixel(worldOffsetX + players[i]->x),
                            feedback_render_pixel(players[i]->y), 54, 54);
        }
        if (game->feedback.appearingTicks[i] > 0 && game->appearanceEffect != NULL)
        {
            const int ticks = game->feedback.appearingTicks[i] > FEEDBACK_TRANSITION_TICKS
                ? FEEDBACK_TRANSITION_TICKS : game->feedback.appearingTicks[i];
            const int appearingFrame = FEEDBACK_TRANSITION_TICKS - ticks;
            SDL_Rect source = {96 * appearingFrame, 0, 96, 96};
            SDL_Rect destination = {
                feedback_render_pixel(worldOffsetX + game->feedback.transitionX[i] - 21.0f),
                feedback_render_pixel(game->feedback.transitionY[i] - 21.0f), 96, 96};
            SDL_RenderCopy(renderer, game->appearanceEffect, &source, &destination);
        }
        if (game->feedback.disappearingTicks[i] > 0 && game->disappearanceEffect != NULL)
        {
            const int ticks = game->feedback.disappearingTicks[i] > FEEDBACK_TRANSITION_TICKS
                ? FEEDBACK_TRANSITION_TICKS : game->feedback.disappearingTicks[i];
            const int disappearingFrame = FEEDBACK_TRANSITION_TICKS - ticks;
            SDL_Rect source = {96 * disappearingFrame, 0, 96, 96};
            SDL_Rect destination = {
                feedback_render_pixel(worldOffsetX + game->feedback.transitionX[i] - 21.0f),
                feedback_render_pixel(game->feedback.transitionY[i] - 21.0f), 96, 96};
            SDL_RenderCopy(renderer, game->disappearanceEffect, &source, &destination);
        }
    }
    if (game->dustParticle != NULL)
    {
        SDL_SetTextureColorMod(game->dustParticle, 255U, 255U, 255U);
        SDL_SetTextureAlphaMod(game->dustParticle, 255U);
    }
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        if (game->enemyValues[i].hitFlashTicks > 0)
            draw_flash_rect(renderer, feedback_render_pixel(worldOffsetX + game->enemyValues[i].x),
                            feedback_render_pixel(game->enemyValues[i].y), 55, 55);
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (game->smartEnemies[i].hitFlashTicks > 0)
            draw_flash_rect(renderer, feedback_render_pixel(worldOffsetX + game->smartEnemies[i].x),
                            feedback_render_pixel(game->smartEnemies[i].y), 85, 50);
    }
    for (int i = 0; i < 2; i++)
    {
        if (game->boss[i].hitFlashTicks > 0)
            draw_flash_rect(renderer, feedback_render_pixel(worldOffsetX + game->boss[i].x),
                            feedback_render_pixel(game->boss[i].y), 81, 55);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void combat_feedback_end_render(SDL_Renderer *renderer)
{
    if (renderer == NULL) return;
    SDL_RenderSetViewport(renderer, NULL);
}
