//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_PLAYER
#define HEADER_PLAYER

#include <types.h>
#include "game_object.h"
#include "defs.h"

typedef enum
{
    PL_STATE_SUSPENDED,
    PL_STATE_NORMAL,
    PL_STATE_INVINCIBLE,
    PL_STATE_DIED,
} PlayerState;

// Player object with cooldown and linked list pointers
typedef struct Player
{
    GameObject;
    u16 coolDownTicks;      // Shooting cooldown counter
    struct Player *prev;    // Previous player in linked list
    struct Player *next;    // Next player in linked list
    u16 invincibleTimer;    // Timer for invincibility after respawn
    u16 respawnTimer;       // Timer for respawning
    u16 score;
    u8 index;               // Player index (0 or 1)
    u8 lives;
    bool isDamageable;
    PlayerState state;
} Player;

// Projectile object
typedef struct
{
    GameObject;
    u8 ownerIndex;
} Projectile;


void Players_Create();

struct Player *Player_Add(unsigned char index);

void Player_Explode(Player *player);

void Player_Remove(Player *player);

void Player_TryShoot(Player *player);

void Player_Update(Player *player);

void Player_UpdateEnemyCollision(Player *player);

void Player_UpdateInput(Player *player);

void Projectile_Spawn(Projectile *bullet, fix16 x, fix16 y, u8 ownerIndex);

void Player_ScoreUpdate(Player *player);

#endif //HEADER_PLAYER
