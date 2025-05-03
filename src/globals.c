//
// Created by weerb on 02.05.2025.
//
#include <genesis.h>
#include "globals.h"
#include "defs.h"
#include "enemy.h"

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
    
    // Enemy spawn patterns configurations
    .sinSpawner = {
        .pattern = PATTERN_SIN,
        .enemyCount = 8,
        .enemyDelay = 15,
        .delay = 60,
    },
    
    // Enemy spawn patterns configurations
    .lineSpawner = {
        .pattern = PATTERN_HOR,
        .enemyCount = 8,
        .enemyDelay = 15,
        .delay = 60,
    },
    
};
