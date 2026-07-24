#include "arcade_waves.h"

#include "entity_spawn.h"

#define ARCADE_WAVE_REST_TICKS 120

// The table repeats after its final entry. This preserves a readable opening
// progression while avoiding the prior score-threshold progression dead end.
static const WaveDefinition waveDefinitions[] = {
    {10, 0, 0, 18}, {16, 2, 0, 16}, {24, 4, 0, 14},
    {32, 6, 0, 12}, {40, 8, 1, 10}, {50, 10, 2, 8},
};
static const int waveDefinitionCount = (int)(sizeof(waveDefinitions) / sizeof(waveDefinitions[0]));

const WaveDefinition *arcade_waves_get_definition(int waveNumber)
{
    if (waveNumber < 1) return NULL;
    return &waveDefinitions[(waveNumber - 1) % waveDefinitionCount];
}

void arcade_waves_reset(GameState *game)
{
    if (game == NULL) return;
    game->arcadeWaves = (ArcadeWaveState){1, 0, 0, 0, 0, 0, false};
}

static bool inactive(const Enemies *enemy)
{
    return enemy->y >= 1000.0f || !enemy->visible;
}

static bool active_entities(const GameState *game)
{
    for (int i = 0; i < NUM_ENEMIES; i++) if (!inactive(&game->enemyValues[i])) return true;
    for (int i = 0; i < NUM_SMART_ENEMIES; i++) if (!inactive(&game->smartEnemies[i])) return true;
    for (int i = 0; i < 2; i++) if (!inactive(&game->boss[i])) return true;
    return false;
}

static bool spawn_regular(GameState *game, int spawned)
{
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        if (inactive(&game->enemyValues[i]))
        {
            return enemy_spawn(game, i, spawned % 2 == 0 ? 620.0f : 580.0f,
                               -(float)(spawned % 5) * 120.0f, 0.0f, 0.0f);
        }
    }
    return false;
}

static bool spawn_smart(GameState *game, int spawned)
{
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (inactive(&game->smartEnemies[i]))
        {
            return smart_enemy_spawn(game, i, spawned % 2 == 0 ? 1000.0f : 200.0f,
                                     150.0f + (float)(spawned % 3) * 120.0f, 0.0f, 0.0f);
        }
    }
    return false;
}

static bool spawn_boss(GameState *game, int spawned)
{
    for (int i = 0; i < 2; i++)
    {
        if (inactive(&game->boss[i]))
        {
            return boss_spawn(game, i, spawned % 2 == 0 ? 1100.0f : 100.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    return false;
}

static bool fully_spawned(const ArcadeWaveState *state, const WaveDefinition *definition)
{
    return state->regularSpawned == definition->regularEnemies &&
           state->smartSpawned == definition->smartEnemies && state->bossesSpawned == definition->bosses;
}

static bool wave_has_spawned_nothing(const ArcadeWaveState *state)
{
    return state->regularSpawned == 0 && state->smartSpawned == 0 && state->bossesSpawned == 0;
}

static bool spawn_next(GameState *game, const WaveDefinition *definition)
{
    ArcadeWaveState *state = &game->arcadeWaves;
    if (state->regularSpawned < definition->regularEnemies)
    {
        if (!spawn_regular(game, state->regularSpawned)) return false;
        state->regularSpawned++;
        return true;
    }
    if (state->smartSpawned < definition->smartEnemies)
    {
        if (!spawn_smart(game, state->smartSpawned)) return false;
        state->smartSpawned++;
        return true;
    }
    if (state->bossesSpawned < definition->bosses)
    {
        if (!spawn_boss(game, state->bossesSpawned)) return false;
        state->bossesSpawned++;
        return true;
    }
    return false;
}

void arcade_waves_update(GameState *game)
{
    if (game == NULL) return;
    ArcadeWaveState *state = &game->arcadeWaves;
    const WaveDefinition *definition = arcade_waves_get_definition(state->waveNumber);
    if (definition == NULL ||
        state->regularSpawned < 0 || state->regularSpawned > definition->regularEnemies ||
        state->smartSpawned < 0 || state->smartSpawned > definition->smartEnemies ||
        state->bossesSpawned < 0 || state->bossesSpawned > definition->bosses ||
        state->spawnCooldownTicks < 0 || state->restTicksRemaining < 0)
    {
        arcade_waves_reset(game);
        return;
    }
    if (state->restTicksRemaining > 0)
    {
        state->restTicksRemaining--;
        return;
    }
    if (state->spawnCooldownTicks > 0)
    {
        state->spawnCooldownTicks--;
        return;
    }
    if (!fully_spawned(state, definition))
    {
        // A fresh session normally begins with no active enemies. If callers
        // intentionally stage an entity before the scheduler's first tick
        // (focused collision tests and future scripted intros), do not claim
        // its slot as a Wave 1 spawn.
        if (wave_has_spawned_nothing(state) && active_entities(game)) return;
        if (spawn_next(game, definition)) state->spawnCooldownTicks = definition->spawnIntervalTicks;
        return;
    }
    if (active_entities(game)) return;

    if (state->waveNumber < INT_MAX)
    {
        state->waveNumber++;
    }
    state->regularSpawned = 0;
    state->smartSpawned = 0;
    state->bossesSpawned = 0;
    state->restTicksRemaining = ARCADE_WAVE_REST_TICKS;
    const WaveDefinition *next = arcade_waves_get_definition(state->waveNumber);
    state->bossTelegraphActive = next != NULL && next->bosses > 0;
}

void arcade_waves_draw_hud(SDL_Renderer *renderer, const GameState *game)
{
    if (renderer == NULL || game == NULL || game->font == NULL) return;
    const ArcadeWaveState *state = &game->arcadeWaves;
    char text[64];
    if (state->bossTelegraphActive)
        (void)snprintf(text, sizeof(text), "BOSS WAVE %d IN %d", state->waveNumber,
                       state->restTicksRemaining / (int)PHYSICS_HZ + 1);
    else if (state->restTicksRemaining > 0)
        (void)snprintf(text, sizeof(text), "WAVE %d IN %d", state->waveNumber,
                       state->restTicksRemaining / (int)PHYSICS_HZ + 1);
    else
        (void)snprintf(text, sizeof(text), "WAVE %d", state->waveNumber);

    const SDL_Color color = state->bossTelegraphActive ? (SDL_Color){255, 96, 64, 255}
                                                        : (SDL_Color){255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(game->font, text, color);
    if (surface == NULL) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destination = {WIDTH / 2 - surface->w / 2, 12, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (texture != NULL)
    {
        SDL_RenderCopy(renderer, texture, NULL, &destination);
        SDL_DestroyTexture(texture);
    }
}
