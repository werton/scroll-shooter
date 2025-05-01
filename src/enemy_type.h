//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_ENEMY_TYPE
#define HEADER_ENEMY_TYPE

#include <types.h>
#include "game_object.h"


// Enemy object
typedef struct {
    GameObject;
} Enemy;

// Enemy spawn patterns
typedef enum {
    PATTERN_NONE,
    PATTERN_HOR,  // Horizontal line pattern
    PATTERN_SIN   // Sinusoidal pattern
} EnemyPattern;

// Enemy spawner configuration
typedef struct {
    EnemyPattern pattern;  // Spawn pattern type
    u16 enemyCount;        // Total enemies to spawn
    u16 delay;             // Initial delay before spawning
    u16 enemyDelay;        // Delay between enemy spawns
} EnemySpawner;

// Current enemy wave state
typedef struct {
    EnemySpawner *spawner;  // Pointer to spawner configuration
    u16 delay;              // Current delay counter
    u16 enemyDelay;         // Current enemy delay counter
    u16 spawnedCount;       // Number of enemies spawned so far
    bool active;            // Whether wave is currently active
} EnemyWave;
#endif //HEADER_ENEMY_TYPE
