//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_ENEMY
#define HEADER_ENEMY

#include <types.h>
#include "enemy_type.h"


void Enemy_Spawn(s16 x, s16 y);

void EnemySpawner_Set(EnemySpawner * spawner);

void EnemySpawner_Update();

void EnemyWave_Switch();

void Enemies_Init();

void Enemies_Update();

#endif //HEADER_ENEMY
