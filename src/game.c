// *****************************************************************************
// Scroll shooter example
//
// Written by werton playskin 05/2025
// GFX artwork created by Luis Zuno (@ansimuz)
// Moon surface artwork and GFX created by mattwalkden.itch.io
// SFX created by JDSherbert jdsherbert.itch.io
// *****************************************************************************

// --- Standard and Library Includes ---
#include <genesis.h>
#include <maths.h>

// --- Project Includes ---
#include "defs.h"
#include "game.h"
#include "game_object.h"
#include "resources.h"
#include "player.h"
#include "globals.h"
#include "enemy.h"
#include "game_types.h"
#include "enemy_type.h"
#include "explosion.h"

// =============================================
// Function Implementations
// =============================================

// Initialize all game systems and resources
void Game_Init()
{
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
    Game_ObjectsPoolsInit();
    Players_Create();
    Player_Add(0);
    Game_RenderScore(&game.players[0]);
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);
    Enemies_Init();
    EnemySpawner_Set(&game.sinSpawner);
}

// Add frame timing variables
static u32 lastFrameTime = 0;
static fix16 deltaTime = 0;

// Optimized main game loop with frame timing
void Game_MainLoop()
{
    while (TRUE)
    {
        u32 currentTime = SYS_getTime();
        deltaTime = FIX16(currentTime - lastFrameTime) / FIX16(60); // 60 FPS target
        lastFrameTime = currentTime;

        // Cap delta time to prevent large jumps
        if (deltaTime > FIX16(2)) deltaTime = FIX16(2);

        Game_PlayerJoinUpdate();

        FOREACH_ACTIVE_PLAYER(player)
        {
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
        
        // Frame rate limiting
        while (SYS_getTime() - currentTime < 16) // ~60 FPS
        {
            SYS_doVBlankProcess();
        }
    }
}

// Optimized object pool initialization
void Game_ObjectsPoolsInit()
{
    // Pre-allocate memory for pools
    game.enemyPool = POOL_create(MAX_ENEMIES, sizeof(Enemy));
    game.projectilePool = POOL_create(MAX_BULLETS, sizeof(Projectile));
    game.explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));

    // Pre-allocate objects for better performance
    for (u16 i = 0; i < MAX_ENEMIES; i++) {
        Enemy *enemy = POOL_allocate(game.enemyPool);
        if (enemy) {
            memset(enemy, 0, sizeof(Enemy));
            POOL_release(game.enemyPool, enemy, TRUE);
        }
    }
    
    for (u16 i = 0; i < MAX_BULLETS; i++) {
        Projectile *projectile = POOL_allocate(game.projectilePool);
        if (projectile) {
            memset(projectile, 0, sizeof(Projectile));
            POOL_release(game.projectilePool, projectile, TRUE);
        }
    }
    
    for (u16 i = 0; i < MAX_EXPLOSION; i++) {
        GameObject *explosion = POOL_allocate(game.explosionPool);
        if (explosion) {
            memset(explosion, 0, sizeof(GameObject));
            POOL_release(game.explosionPool, explosion, TRUE);
        }
    }
}

// Optimized object release
void GameObject_Release(GameObject *gameObject, Pool *pool)
{
    if (!gameObject) return;
    
    // Clear object memory for reuse
    memset(gameObject, 0, sizeof(GameObject));
    
    SPR_setVisibility(gameObject->sprite, HIDDEN);
    POOL_release(pool, gameObject, TRUE);
}

// Release object with explosion effect
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool)
{
    Explosion_Spawn(object->x - FIX16(EXPLOSION_X_OFFSET), object->y);
    SPR_setFrame(object->sprite, NORMAL_FRAME);
    GameObject_Release(object, pool);
}

// Scroll background planes according to their rules
void BackgroundScroll()
{
    for (u16 ind = 0; ind < SCROLL_PLANES; ind++)
    {
        PlaneScrollingRule *scrollRule = &game.scrollRules[ind];
        if (scrollRule->autoScrollSpeed == 0) continue;

        scrollRule->scrollOffset += scrollRule->autoScrollSpeed;
        memsetU16((u16 *) game.lineOffsetX[ind], -FF32_toInt(scrollRule->scrollOffset), scrollRule->numOfLines);

        VDP_setHorizontalScrollTile(scrollRule->plane, scrollRule->startLineIndex, game.lineOffsetX[ind],
                                    scrollRule->numOfLines, DMA_QUEUE);
    }
}

// Spatial grid settings
#define GRID_CELL_SIZE 32
#define GRID_WIDTH (SCREEN_WIDTH / GRID_CELL_SIZE + 1)
#define GRID_HEIGHT (SCREEN_HEIGHT / GRID_CELL_SIZE + 1)

// Grid cell structure
typedef struct {
    GameObject *objects[MAX_ENEMIES + MAX_BULLETS];
    u16 count;
} GridCell;

// Grid system
static GridCell grid[GRID_WIDTH][GRID_HEIGHT];

// Clear grid
static void Grid_Clear()
{
    for (u16 x = 0; x < GRID_WIDTH; x++) {
        for (u16 y = 0; y < GRID_HEIGHT; y++) {
            grid[x][y].count = 0;
        }
    }
}

// Add object to grid
static void Grid_AddObject(GameObject *obj)
{
    u16 gridX = F16_toInt(obj->x) / GRID_CELL_SIZE;
    u16 gridY = F16_toInt(obj->y) / GRID_CELL_SIZE;
    
    if (gridX < GRID_WIDTH && gridY < GRID_HEIGHT) {
        GridCell *cell = &grid[gridX][gridY];
        if (cell->count < MAX_ENEMIES + MAX_BULLETS) {
            cell->objects[cell->count++] = obj;
        }
    }
}

// Optimized collision check using grid
void Projectile_UpdateEnemyCollision()
{
    Grid_Clear();
    
    // Add all enemies to grid
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, game.enemyPool)
    {
        if (enemy) Grid_AddObject((GameObject *)enemy);
    }
    
    // Check collisions for each projectile
    FOREACH_ALLOCATED_IN_POOL(Projectile, projectile, game.projectilePool)
    {
        if (!projectile) continue;
        
        u16 gridX = F16_toInt(projectile->x) / GRID_CELL_SIZE;
        u16 gridY = F16_toInt(projectile->y) / GRID_CELL_SIZE;
        
        // Check current cell and adjacent cells
        for (s16 x = -1; x <= 1; x++) {
            for (s16 y = -1; y <= 1; y++) {
                u16 checkX = gridX + x;
                u16 checkY = gridY + y;
                
                if (checkX < GRID_WIDTH && checkY < GRID_HEIGHT) {
                    GridCell *cell = &grid[checkX][checkY];
                    
                    for (u16 i = 0; i < cell->count; i++) {
                        Enemy *enemy = (Enemy *)cell->objects[i];
                        if (GameObject_CollisionUpdate(projectile, (GameObject *)enemy)) {
                            if (!enemy->hp) {
                                GameObject_ReleaseWithExplode((GameObject *)enemy, game.enemyPool);
                                game.players[projectile->ownerIndex].score += ENEMY_SCORE_VALUE;
                                Player_ScoreUpdate(&game.players[projectile->ownerIndex]);
                            }
                            GameObject_Release(projectile, game.projectilePool);
                            goto next_projectile;
                        }
                    }
                }
            }
        }
        next_projectile:;
    }
}

// Update all active bullets movement and boundaries
void Projectile_Update()
{
    FOREACH_ALLOCATED_IN_POOL(GameObject, projectile, game.projectilePool)
    {
        if (projectile)
        {
            projectile->x += BULLET_OFFSET_X;
            SPR_setPosition(projectile->sprite, F16_toInt(projectile->x), F16_toInt(projectile->y));

            if (projectile->x > FIX16(SCREEN_WIDTH))
                GameObject_Release(projectile, game.projectilePool);
        }
    }
}

// Render FPS and CPU load
void RenderFPS()
{
    VDP_setTextPalette(PAL0);
    VDP_setWindowOnBottom(1);
    VDP_setTextPlane(WINDOW);
    VDP_showCPULoad(FPS_CPU_LOAD_POS_X, FPS_CPU_LOAD_POS_Y);
    VDP_showFPS(FALSE, FPS_POS_X, FPS_POS_Y);
}

// Render player score
void Game_RenderScore(Player *player)
{
    if (player->index == 0)
        VDP_drawTextBG(WINDOW, "1P SCORE:00000", PLAYER1_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
    else
        VDP_drawTextBG(WINDOW, "2P SCORE:00000", PLAYER2_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
}

// Render UI messages like join prompts
void Game_RenderMessage()
{
    static u16 blinkCounter;
    blinkCounter++;

    FOREACH_PLAYER(player)
    {
        switch (blinkCounter)
        {
            case 1:
                if (player->state == PL_STATE_SUSPENDED)
                {
                    if (player->index == 0)
                        VDP_drawTextBG(WINDOW, "1P PRESS START", PLAYER1_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
                    else
                        VDP_drawTextBG(WINDOW, "2P PRESS START", PLAYER2_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y);
                }
                break;
            case JOIN_MESSAGE_VISIBLE_FRAMES:
                if (player->state == PL_STATE_SUSPENDED)
                {
                    if (player->index == 0)
                        VDP_clearTextAreaBG(WINDOW, PLAYER1_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y, JOIN_TEXT_WIDTH, 1);
                    else
                        VDP_clearTextAreaBG(WINDOW, PLAYER2_JOIN_TEXT_POS_X, JOIN_TEXT_POS_Y, JOIN_TEXT_WIDTH, 1);
                }
                break;
            case JOIN_MESSAGE_BLINK_INTERVAL:
                blinkCounter = 0;
                break;
        }
    }
}

// Render game frame including UI and backgrounds
void Game_Render()
{
    BackgroundScroll();
    Game_RenderMessage();
    RenderFPS();
    SPR_update();
}

// Check for new players joining the game
void Game_PlayerJoinUpdate()
{
    FOREACH_PLAYER(player)
    {
        switch (player->state)
        {
            case PL_STATE_SUSPENDED:
                if (JOY_readJoypad(player->index) & BUTTON_START)
                {
                    player->lives = PLAYER_LIVES;
                    player->state = PL_STATE_DIED;
                    player->respawnTimer = 0;
                    player->score = 0;
                    Game_RenderScore(player);
                }
                break;
            case PL_STATE_DIED:
                if (player->respawnTimer > 0)
                    player->respawnTimer--;
                if (player->respawnTimer == 0)
                {
                    Player_Add(player->index);
                }
                break;
        }
    }
}
