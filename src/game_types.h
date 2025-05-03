//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_GAME_TYPES
#define HEADER_GAME_TYPES

#include "enemy.h"
#include "globals.h"
#include "player.h"
#include "resources.h"
#include "game_object.h"
#include "defs.h"
#include "enemy_type.h"
#include <maths.h>
#include <genesis.h>


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
} GameState;


#endif //HEADER_GAME_TYPES
