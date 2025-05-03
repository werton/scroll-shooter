#ifndef HEADER_GAME
#define HEADER_GAME

#include <types.h>
#include "player.h"


void Game_MainLoop();

void Projectile_UpdateEnemyCollision();

void Projectile_Update();

void Game_Init();

void Game_Render();

void Game_ObjectsPoolsInit();

void Game_RenderMessage();

void Game_RenderScore(Player *player);

void Game_PlayerJoinUpdate();

void Game_PlayerJoinUpdate();

#endif //HEADER_GAME