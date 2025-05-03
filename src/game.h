#ifndef HEADER_GAME
#define HEADER_GAME

#include <types.h>
#include "player.h"


void Game_MainLoop();
void Projectile_UpdateEnemyCollision();
void Projectile_Update();
void Game_Init();
void Game_Render();
void ObjectsPools_Init();
void Score_Update(Player* player);
void Game_RenderMessage();
void Game_RenderScore(Player *player);
void Player_JoinUpdate();
void Player_JoinUpdate();

#endif //HEADER_GAME