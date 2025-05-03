//
// Created by weerb on 02.05.2025.
//

#include "globals.h"
#include "enemy.h"
#include "player.h"
#include "resources.h"
#include "defs.h"
#include <maths.h>
#include <genesis.h>


// Spawns enemy at specified position
void Enemy_Spawn(fix16 x, fix16 y)
{
    // Allocate and initialize enemy
    GameObject *enemy = (GameObject *) POOL_allocate(game.enemyPool);
    if (enemy)
        GameObject_Init(enemy, &enemy_sprite, PAL3, x, y,
                        ENEMY_WIDTH, ENEMY_HEIGHT, ENEMY_HP, ENEMY_DAMAGE);
}

// Set current enemy spawner configuration
void EnemySpawner_Set(EnemySpawner *spawner)
{
    game.wave.spawner = spawner;
    game.wave.active = FALSE;
    game.wave.delay = spawner->delay;
    game.wave.enemyDelay = spawner->enemyDelay;
    game.wave.spawnedCount = 0;
}


// Updates enemy spawner logic
void EnemySpawner_Update()
{
    // Handle initial delay before wave starts
    if (game.wave.delay)
        game.wave.delay--;
    else
        game.wave.active = TRUE;
    
    if (!game.wave.active)
        return;
    
    // Handle delay between enemy spawns
    game.wave.enemyDelay--;
    
    if (!game.wave.enemyDelay)
        game.wave.enemyDelay = game.wave.spawner->enemyDelay;
    else
        return;
    
    // Spawn enemies according to current pattern
    switch (game.wave.spawner->pattern)
    {
        case PATTERN_HOR:
            // Spawn enemies on two horizontal lines
            Enemy_Spawn(FIX16(SCREEN_WIDTH), FIX16(50));  // Top line
            Enemy_Spawn(FIX16(SCREEN_WIDTH), FIX16(SCREEN_HEIGHT - ENEMY_HEIGHT / 2 - 50));  // Bottom line
            break;
        
        case PATTERN_SIN:
            // Spawn enemies in sinusoidal patterns (in opposite phase)
            fix16 y1 = FIX16(SCREEN_HEIGHT / 2) + F16_mul(F16_sin(F16(game.wave.spawnedCount * 20)), F16(80));
            fix16 y2 = FIX16(SCREEN_HEIGHT / 2) + F16_mul(F16_sin(F16(game.wave.spawnedCount * 20 + 180)), F16(80));
            Enemy_Spawn(FIX16(SCREEN_WIDTH), y1);  // First sine wave
            Enemy_Spawn(FIX16(SCREEN_WIDTH), y2);  // Opposite phase sine wave
            break;
        
        default:
            break;
    }
    
    // Check if wave is complete
    game.wave.spawnedCount++;
    
    if (game.wave.spawnedCount == game.wave.spawner->enemyCount)
        EnemyWave_Switch();
}

// Switches between enemy spawn patterns
void EnemyWave_Switch()
{
    // Alternate between patterns
    if (game.wave.spawner->pattern == PATTERN_NONE || game.wave.spawner->pattern == PATTERN_SIN)
        EnemySpawner_Set((EnemySpawner *) &game.lineSpawner);
    else
        EnemySpawner_Set((EnemySpawner *) &game.sinSpawner);
}

// Initialize enemies palette and resources
void Enemies_Init()
{
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
}

// Updates all active enemies
void Enemies_Update()
{
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, game.enemyPool)
    {
        if (!enemy)
            continue;
        
        // Handle damage blink effect
        if (enemy->blinkCounter)
        {
            enemy->blinkCounter--;
            if (enemy->blinkCounter == 0)
                SPR_setFrame(enemy->sprite, 0);  // Reset to normal frame
        }
        
        // Move enemy left
        enemy->x -= ENEMY_SPEED;
        SPR_setPosition(enemy->sprite, F16_toInt(enemy->x), F16_toInt(enemy->y));
        
        // Remove enemy if it goes off-screen
        if (F16_toInt(enemy->x) < -ENEMY_WIDTH)
            GameObject_Release((GameObject *) enemy, game.enemyPool);
    }
}