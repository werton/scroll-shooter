//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_GLOBALS
#define HEADER_GLOBALS

#include "game_object.h"
#include "enemy_type.h"
#include "player.h"
#include "game_types.h"


// Line offset buffers for scrolling
extern s16 lineOffsetX[SCROLL_PLANES][SCREEN_TILE_ROWS];

// Enemy spawn patterns configurations
extern const EnemySpawner lineSpawner;
extern const EnemySpawner sinSpawner;
extern u32 blinkCounter;
// Pools for game objects
extern Pool *projectilePool;
extern Pool *enemyPool;
extern Pool *explosionPool;

// Global game state with default values
extern GameState game;

#endif //HEADER_GLOBALS
