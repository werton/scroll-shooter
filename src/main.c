// *****************************************************************************
// Scroll shooter example
//
// Written by werton playskin 05/2025
// GFX artwork created by Luis Zuno (@ansimuz)
// Moon surface artwork and GFX created by mattwalkden.itch.io
// SFX created by JDSherbert jdsherbert.itch.io
// *****************************************************************************

#include <genesis.h>
#include <maths.h>
#include "defs.h"
#include "game_object.h"
#include "resources.h"
#include "player.h"
#include "globals.h"
#include "enemy.h"
#include "game_types.h"
#include "enemy_type.h"
#include "explosion.h"



// =============================================
// Function Prototypes
// =============================================
void Projectile_UpdateEnemyCollision();
void Projectile_Update();

void Game_Init();
void Game_Render();
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool);
void ObjectsPools_Init();

void Score_Update(Player* player);
void RenderMessage();
void RenderScore(Player *player);

// =============================================
// Main Game Loop
// =============================================

int main(bool hardReset) {
    if (!hardReset)
        SYS_hardReset();

    Game_Init();

    while (TRUE) {
        Player_JoinUpdate();

        FOREACH_ACTIVE_PLAYER(player) {
            Player_UpdateInput(player);
            Player_Update(player);
            Player_UpdateEnemyCollision(player);
        }

        Projectile_Update();
        Enemies_Update();
        Explosions_Update();
        Projectile_UpdateEnemyCollision();
        EnemySpawner_Update();

        Game_Render();
        SYS_doVBlankProcess();
    }

    return 0;
}

// =============================================
// Function Implementations
// =============================================

// Release object with explosion effect
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool) {
    Explosion_Spawn(object->x - FIX16(EXPLOSION_X_OFFSET), object->y);
    SPR_setFrame(object->sprite, NORMAL_FRAME);
    GameObject_Release(object, pool);
}


void BackgroundScroll()
{
    for (u16 ind = 0; ind < SCROLL_PLANES; ind++) {
        PlaneScrollingRule *scrollRule = &game.scrollRules[ind];
        if (scrollRule->autoScrollSpeed == 0) continue;

        scrollRule->scrollOffset += scrollRule->autoScrollSpeed;
        memsetU16((u16 *)lineOffsetX[ind], -FF32_toInt(scrollRule->scrollOffset), scrollRule->numOfLines);

        VDP_setHorizontalScrollTile(scrollRule->plane, scrollRule->startLineIndex, lineOffsetX[ind],
                                   scrollRule->numOfLines, DMA_QUEUE);
    }
}


// Check collisions between bullets and enemies
void Projectile_UpdateEnemyCollision() {
    FOREACH_ALLOCATED_IN_POOL(Projectile, projectile, projectilePool) {
        if (!projectile) continue;

        FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
            if (!enemy) continue;

            if (GameObject_CollisionUpdate(projectile, (GameObject *) enemy)) {
                if (!enemy->hp) {
                    GameObject_ReleaseWithExplode((GameObject *)enemy, enemyPool);

                    game.players[projectile->ownerIndex].score += 10;
                    Score_Update(&game.players[projectile->ownerIndex]);
                }
                GameObject_Release(projectile, projectilePool);
                break;
            }
        }
    }
}

// Update all active bullets movement and boundaries
void Projectile_Update() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, projectile, projectilePool) {
//        kprintf("bullet");
        if (projectile) {
            projectile->x += BULLET_OFFSET_X;
            SPR_setPosition(projectile->sprite, F16_toInt(projectile->x), F16_toInt(projectile->y));

            if (projectile->x > FIX16(SCREEN_WIDTH))
                GameObject_Release(projectile, projectilePool);
        }
    }
}

// Initialize all game systems and resources
void Game_Init() {
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

#if PLAY_MUSIC
    XGM2_play(xgm2_music);
#endif

    JOY_init();
    SPR_init();

    VDP_drawImageEx(BG_A, &mapImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                   0, 0, TRUE, TRUE);
    VDP_drawImageEx(BG_B, &bgImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                   TILE_USER_INDEX + mapImage.tileset->numTile), 0, 0, TRUE, TRUE);

    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    ObjectsPools_Init();
    Players_Create();
    Player_Add(0);
    RenderScore(&game.players[0]);
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);
    Enemies_Init();
    EnemySpawner_Set(&sinSpawner);
}

void RenderFPS() {
    VDP_setTextPalette(PAL0);
    VDP_setWindowOnBottom(1);
    VDP_setTextPlane(WINDOW);
    VDP_showCPULoad(17, 27);
    VDP_showFPS(FALSE, 21, 27);
}


void Score_Update(Player *player) {
    if (player->index == 0) {
        static char str[5];
        uintToStr(player->score, &str, 5);
        VDP_drawTextBG(WINDOW, str, PLAYER1_JOIN_TEXT_POS_X+9, JOIN_TEXT_POS_Y);
    }
    else {
        static char str2[5];
        uintToStr(player->score, &str2, 5);
        VDP_drawTextBG(WINDOW, str2, PLAYER2_JOIN_TEXT_POS_X+9, JOIN_TEXT_POS_Y);
    }
}

void RenderScore(Player *player) {
    if (player->index == 0)
        VDP_drawTextBG(WINDOW, "1P SCORE:00000", PLAYER1_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
    else
        VDP_drawTextBG(WINDOW, "2P SCORE:00000", PLAYER2_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
}

// Render UI messages like join prompts
void RenderMessage() {
    blinkCounter++;

    FOREACH_PLAYER(player) {
        switch (blinkCounter) {

            case 1:
                if (player->state == PL_STATE_SUSPENDED) {
                    if (player->index == 0)
                        VDP_drawTextBG(WINDOW, "1P PRESS START", PLAYER1_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
                    else
                        VDP_drawTextBG(WINDOW, "2P PRESS START", PLAYER2_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
                }
                break;

            case JOIN_MESSAGE_VISIBLE_FRAMES:
                if (player->state == PL_STATE_SUSPENDED) {
                    if (player->index == 0)
                        VDP_clearTextAreaBG(WINDOW, PLAYER1_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y, 15, 1);
                    else
                        VDP_clearTextAreaBG(WINDOW, PLAYER2_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y, 15, 1);
                }
                break;

            case JOIN_MESSAGE_BLINK_INTERVAL:
                blinkCounter = 0;
                break;
    }

    }
}

// Render game frame including UI and backgrounds
void Game_Render() {
    BackgroundScroll();
    RenderMessage();
    RenderFPS();
    SPR_update();
}

// Initialize object pools for game entities
void ObjectsPools_Init() {
    enemyPool = POOL_create(MAX_ENEMIES, sizeof(Enemy));
    projectilePool = POOL_create(MAX_BULLETS, sizeof(Projectile));
    explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));
}

