#include "arcade_waves.h"

static int failures = 0;

static void expect_true(bool condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "ARCADE WAVES TEST: FAIL - %s\n", message);
        failures++;
    }
}

static void clear_entities(GameState *game)
{
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
}

static void mark_wave_spawned(GameState *game, const WaveDefinition *definition)
{
    game->arcadeWaves.regularSpawned = definition->regularEnemies;
    game->arcadeWaves.smartSpawned = definition->smartEnemies;
    game->arcadeWaves.bossesSpawned = definition->bosses;
}

int main(void)
{
    GameState game = {0};
    clear_entities(&game);
    arcade_waves_reset(&game);

    const WaveDefinition *waveOne = arcade_waves_get_definition(1);
    const WaveDefinition *waveFive = arcade_waves_get_definition(5);
    expect_true(waveOne != NULL && waveOne->regularEnemies == 10 && waveOne->smartEnemies == 0 &&
                    waveOne->bosses == 0 && waveOne->spawnIntervalTicks == 18,
                "wave one is defined by data, not score thresholds");
    expect_true(waveFive != NULL && waveFive->bosses == 1,
                "the boss wave is represented in the wave table");
    expect_true(arcade_waves_get_definition(0) == NULL, "invalid wave numbers are rejected");
    expect_true(game.arcadeWaves.waveNumber == 1 && game.arcadeWaves.spawnCooldownTicks == 0 &&
                    game.arcadeWaves.restTicksRemaining == 0 && !game.arcadeWaves.bossTelegraphActive,
                "reset starts wave one with no pending rest or boss warning");

    arcade_waves_update(&game);
    expect_true(game.arcadeWaves.regularSpawned == 1 && game.enemyValues[0].visible == 1 &&
                    game.enemyValues[0].y == 0.0f,
                "the first update spawns the first regular enemy");
    expect_true(game.arcadeWaves.spawnCooldownTicks == waveOne->spawnIntervalTicks,
                "spawning uses the wave's declared cadence");

    clear_entities(&game);
    mark_wave_spawned(&game, waveOne);
    game.arcadeWaves.spawnCooldownTicks = 0;
    arcade_waves_update(&game);
    expect_true(game.arcadeWaves.waveNumber == 2 && game.arcadeWaves.restTicksRemaining == 120 &&
                    !game.arcadeWaves.bossTelegraphActive,
                "a cleared normal wave advances through a rest period");
    arcade_waves_update(&game);
    expect_true(game.arcadeWaves.restTicksRemaining == 119 && game.arcadeWaves.regularSpawned == 0,
                "the rest period defers the next wave's spawn");

    clear_entities(&game);
    game.arcadeWaves.waveNumber = 4;
    const WaveDefinition *waveFour = arcade_waves_get_definition(4);
    mark_wave_spawned(&game, waveFour);
    game.arcadeWaves.restTicksRemaining = 0;
    game.arcadeWaves.spawnCooldownTicks = 0;
    arcade_waves_update(&game);
    expect_true(game.arcadeWaves.waveNumber == 5 && game.arcadeWaves.restTicksRemaining == 120 &&
                    game.arcadeWaves.bossTelegraphActive,
                "the rest before a boss wave enables its telegraph");

    clear_entities(&game);
    game.arcadeWaves.waveNumber = 1;
    mark_wave_spawned(&game, waveOne);
    game.enemyValues[0].y = 100.0f;
    game.enemyValues[0].visible = 1;
    game.arcadeWaves.restTicksRemaining = 0;
    arcade_waves_update(&game);
    expect_true(game.arcadeWaves.waveNumber == 1,
                "a wave cannot advance while one of its spawned enemies is active");

    if (failures == 0) printf("ARCADE WAVES TEST: ALL PASS\n");
    return failures == 0 ? 0 : 1;
}
