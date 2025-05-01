//
// Created by weerb on 02.05.2025.
//
#include <genesis.h>
#include "globals.h"
#include "defs.h"
#include "game_object.h"
#include "enemy.h"
#include "player.h"



// Enemy spawn patterns configurations
const EnemySpawner lineSpawner = {
    .pattern = PATTERN_HOR,
    .enemyCount = 8,
    .enemyDelay = 15,
    .delay = 60,
};

const EnemySpawner sinSpawner = {
    .pattern = PATTERN_SIN,
    .enemyCount = 8,
    .enemyDelay = 15,
    .delay = 60,
};

// =============================================
// Global Variables
// =============================================
u32 blinkCounter = 0;

// Pools for game objects
Pool *projectilePool;
Pool *enemyPool;
Pool *explosionPool;

// Line offset buffers for scrolling
s16 lineOffsetX[SCROLL_PLANES][SCREEN_TILE_ROWS];

// Global game state with default values
GameState game = {
    .scrollRules = {
        [0] = {.plane = BG_A, .startLineIndex = 0, .numOfLines = 9, .autoScrollSpeed = FF32(0.04)},
        [1] = {.plane = BG_A, .startLineIndex = 9, .numOfLines = 4, .autoScrollSpeed = FF32(0.4)},
        [2] = {.plane = BG_A, .startLineIndex = 13, .numOfLines = 4, .autoScrollSpeed = FF32(1.1)},
        [3] = {.plane = BG_A, .startLineIndex = 17, .numOfLines = 11, .autoScrollSpeed = FF32(2)},
        [4] = {.plane = BG_B, .startLineIndex = 0, .numOfLines = 28, .autoScrollSpeed = FF32(0.01)},
    },
    .playerListHead = NULL,  // Start with empty player list
};
