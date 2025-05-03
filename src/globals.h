//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_GLOBALS
#define HEADER_GLOBALS

#include "game_object.h"
#include "enemy_type.h"
#include "player.h"
#include "game_types.h"

// Scrolling plane configuration
typedef struct
{
    VDPPlane plane;         // Background plane (A or B)
    u16 startLineIndex;     // First line to apply scrolling
    u16 numOfLines;         // Number of lines to scroll
    ff32 autoScrollSpeed;   // Scrolling speed (fixed point)
    ff32 scrollOffset;      // Current scroll offset
} PlaneScrollingRule;


// Main game state structure
typedef struct
{
    Player *players;
    Player *playerListHead;              // Linked list of active players
    PlaneScrollingRule scrollRules[SCROLL_PLANES]; // Background scrolling rules
    EnemyWave wave;                      // Current enemy wave state
    
    Pool *projectilePool;
    Pool *enemyPool;
    Pool *explosionPool;
    
    s16 lineOffsetX[SCROLL_PLANES][SCREEN_TILE_ROWS]; // Line offset buffers for scrolling
    const EnemySpawner lineSpawner; // Enemy spawn patterns configurations
    const EnemySpawner sinSpawner;
} GameState;


// Global game state with default values
extern GameState game;

#endif //HEADER_GLOBALS
