#include <genesis.h>
#include "resources.h"

// Constants
#define PLAYER_SPEED                    2
#define BULLET_OFFSET_X                 8
#define ENEMY_SPEED                     1
#define MAX_BULLETS                     6
#define MAX_ENEMIES                     10
#define MAX_EXPLOSION                   4
#define SCREEN_WIDTH                    320
#define SCREEN_HEIGHT                   224
#define OBJECT_SIZE                     16
#define SCROLL_LINES_BGA                3

// Gameplay constants
#define ENEMY_SPAWN_CHANCE              5 // Chance to spawn an enemy (out of 100)
#define RANDOM_MAX                      100       // Maximum value for random number generation
#define FIRE_RATE                       20         // Delay between shots (in frames)

#define FOREACH_ALLOCATED_IN_POOL(object, pool)                                                    \
    u16 object##_##objectsNum = POOL_getNumAllocated(pool);                                     \
    GameObject **object##_##FirstPtr = (GameObject **) POOL_getFirst(pool);                     \
    GameObject *object = *object##_##FirstPtr;                                                  \
    for (; object##_##objectsNum-- != 0; object##_##FirstPtr++, object = *object##_##FirstPtr)  \



// Global frame counter
u16 frameCount = 0;

// Data structures
typedef struct
{
    Sprite *sprite;
    s16 x, y;
} GameObject;

typedef struct
{
    VDPPlane plane;
    u16 startLineIndex;
    u16 numOfLines;
    s16 cameraSpeedDivider;
    ff32 autoScrollSpeed;
    ff32 scrollOffset;
    
} PlaneScrollingRule;

typedef struct
{
    GameObject player;
//    GameObject* bullets[MAX_BULLETS]; // Pointers to bullets in the pool
//    GameObject* enemies[MAX_ENEMIES]; // Pointers to enemies in the pool
    s16 bgScrollX;
    PlaneScrollingRule scrollRules[SCROLL_LINES_BGA];
    bool gameOver;
    u16 lastShotFrame;
} GameState;

// Pools for bullets and enemies
Pool *bulletPool;
Pool *enemyPool;
Pool *explosionPool;

GameState gameState = {
    .scrollRules =
        {
            [0] = {.startLineIndex = 9, .numOfLines = 4, .autoScrollSpeed = FF32(0.5)},
            [1] = {.startLineIndex = 13, .numOfLines = 4, .autoScrollSpeed = FF32(1)},
            [2] = {.startLineIndex = 17, .numOfLines = 11, .autoScrollSpeed = FF32(2)},
//            [3] = {.startLineIndex = 25, .numOfLines = 3, .autoScrollSpeed = FF32(2)},
//            [4] = {.startLineIndex = 24, .numOfLines = 4, .autoScrollSpeed = FF32(3.0)},
        },
};

// Function prototypes
void InitializeGame();

void InitializePlayer();

void InitializeBullets();

void InitializeEnemies();

void HandleInput();

void UpdatePlayer();

void UpdateBullets();

void UpdateExplosions();

void UpdateEnemies();

void SpawnEnemy();

void CheckCollisions();

void RenderGame();

u16 GetFrameCount();

void TryShootBullet();

void InitObjectsPools();

void ShootBullet(GameObject *bullet, s16 x, s16 y);

void CreateExplosion(s16 x, s16 y);

void DestroyEnemy(GameObject *enemy);

void ReleaseGameObject(GameObject *gameObject, Pool *pool);

int main()
{
    
    InitializeGame();
    
    while (TRUE)
    {
        if (!gameState.gameOver)
        {
            HandleInput();
            UpdatePlayer();
            UpdateBullets();
            UpdateExplosions();
            UpdateEnemies();
            CheckCollisions();
        }
        RenderGame();
        
        frameCount++; // Increment frame counter
        SYS_doVBlankProcess();
    }
    
    return 0;
}

// Get the current frame count
u16 GetFrameCount()
{
    return frameCount;
}

// Initialize the game
void InitializeGame()
{
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
    
    
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    
    JOY_init();
    SPR_init();
//    SND_startPlay_PCM(music, sizeof(music), SOUND_RATE_16000, SOUND_PAN_CENTER, 0);
    
    VDP_drawImageEx(BG_A, &mapImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, TRUE, TRUE);
    
    InitObjectsPools();
    InitializePlayer();
    
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);

    
    InitializeEnemies();
    
    
    gameState.bgScrollX = 0;
    gameState.gameOver = FALSE;
    gameState.lastShotFrame = 0; // Initialize last shot frame
}

void InitObjectsPools()
{// Initialize object pools
    bulletPool = POOL_create(MAX_BULLETS, sizeof(GameObject));
    enemyPool = POOL_create(MAX_ENEMIES, sizeof(GameObject));
    explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));
}

// Initialize the player
void InitializePlayer()
{
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    
    gameState.player.sprite = SPR_addSprite(&player_sprite, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, TILE_ATTR(PAL1, 0, FALSE, FALSE));
    gameState.player.x = SCREEN_WIDTH / 2;
    gameState.player.y = SCREEN_HEIGHT / 2;
}


// Initialize enemies
void InitializeEnemies()
{
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
}

// Handle input
void HandleInput()
{
    u16 input = JOY_readJoypad(JOY_1);
    
    if (input & BUTTON_LEFT)
        gameState.player.x -= PLAYER_SPEED;
    else if (input & BUTTON_RIGHT)
        gameState.player.x += PLAYER_SPEED;
    
    if (input & BUTTON_UP)
        gameState.player.y -= PLAYER_SPEED;
    else if (input & BUTTON_DOWN)
        gameState.player.y += PLAYER_SPEED;
    
    if (input & BUTTON_A)
        TryShootBullet(); // Try shoot
    
}

void ReleaseGameObject(GameObject *gameObject, Pool *pool)
{
    SPR_releaseSprite(gameObject->sprite);
    gameObject->x = 0;
    POOL_release(pool, gameObject, TRUE);
    gameObject = NULL;
}


// Try to shoot a bullet
void TryShootBullet()
{
    // Check if enough time has passed since the last shot
    if (gameState.lastShotFrame + FIRE_RATE >= GetFrameCount())
        return; // Exit if the fire rate delay has not passed
    
    // Allocate a bullet from the pool
    GameObject *bullet = (GameObject *) POOL_allocate(bulletPool);
    
    if (bullet)
        ShootBullet(bullet, gameState.player.x + OBJECT_SIZE, gameState.player.y);
    
    // Allocate a bullet from the pool
    bullet = (GameObject *) POOL_allocate(bulletPool);
    
    if (bullet)
    {
        ShootBullet(bullet, gameState.player.x + OBJECT_SIZE, gameState.player.y + 16);
        
        XGM2_playPCM(xpcm_shoot, sizeof(xpcm_shoot), SOUND_PCM_CH2);
        // Update the last shot frame
        gameState.lastShotFrame = GetFrameCount();
    }
    
}

void ShootBullet(GameObject *bullet, s16 x, s16 y)
{
    bullet->sprite = SPR_addSprite(&bullet_sprite,
                                   gameState.player.x + OBJECT_SIZE,
                                   gameState.player.y,
                                   TILE_ATTR(PAL1, 0, FALSE, FALSE));
    bullet->x = x;
    bullet->y = y;
}

// Do Explosion
void CreateExplosion(s16 x, s16 y)
{
    // Allocate a bullet from the pool
    GameObject *explosion = (GameObject *) POOL_allocate(explosionPool);
    
    if (explosion)
    {
        explosion->sprite = SPR_addSprite(&explosion_sprite,
                                          x, y, TILE_ATTR(PAL2, 0, FALSE, FALSE));
        SPR_setAnimationLoop(explosion->sprite, FALSE);
        
        XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), SOUND_PCM_CH3);
    }
    
}

// Update player position
void UpdatePlayer()
{
    SPR_setPosition(gameState.player.sprite, gameState.player.x, gameState.player.y);
}

// Update bullets
void UpdateBullets()
{
    FOREACH_ALLOCATED_IN_POOL(bullet, bulletPool)
    {
        if (bullet)
        {
            bullet->x += BULLET_OFFSET_X;
            SPR_setPosition(bullet->sprite, bullet->x, bullet->y);
            
            if (bullet->x > SCREEN_WIDTH)
            {
                ReleaseGameObject(bullet, bulletPool);
            }
        }
    }
}

// Update explosion
void UpdateExplosions()
{
    FOREACH_ALLOCATED_IN_POOL(explosion, explosionPool)
    {
        if (explosion && SPR_isAnimationDone(explosion->sprite))
        {
            ReleaseGameObject(explosion, explosionPool);
        }
    }
}

// Update enemies
void UpdateEnemies()
{
    FOREACH_ALLOCATED_IN_POOL(enemy, enemyPool)
    {
        if (enemy)
        {
            enemy->x -= ENEMY_SPEED;
            SPR_setPosition(enemy->sprite, enemy->x, enemy->y);
            
            if (enemy->x < -OBJECT_SIZE)
            {
                SPR_releaseSprite(enemy->sprite);
                POOL_release(enemyPool, enemy, TRUE); // Free the enemy
                enemy = NULL; // Clear the pointer
            }
        }
    }
    
    // Spawn new enemies
    if (random() % RANDOM_MAX < ENEMY_SPAWN_CHANCE)
    {
        SpawnEnemy();
    }
}

// Spawn an enemy
void SpawnEnemy()
{
    // Allocate an enemy from the pool
    GameObject *enemy = (GameObject *) POOL_allocate(enemyPool);
    
    if (enemy)
    {
        enemy->sprite = SPR_addSprite(&enemy_sprite, SCREEN_WIDTH, random() % SCREEN_HEIGHT, TILE_ATTR(PAL3, 0, FALSE, FALSE));
        enemy->x = SCREEN_WIDTH;
        enemy->y = random() % SCREEN_HEIGHT;
    }
}

// Spawn an enemy
void DestroyEnemy(GameObject *enemy)
{
    CreateExplosion(enemy->x-8, enemy->y);
    ReleaseGameObject(enemy, enemyPool);
}

// Check collisions
void CheckCollisions()
{
    // Check bullet-enemy collisions
    
    
    FOREACH_ALLOCATED_IN_POOL(bullet, bulletPool)
    {
        if (bullet)
        {
            FOREACH_ALLOCATED_IN_POOL(enemy, enemyPool)
            {
                if (enemy &&
                    
                    bullet->x < enemy->x + OBJECT_SIZE &&
                    bullet->x + OBJECT_SIZE > enemy->x &&
                    bullet->y < enemy->y + OBJECT_SIZE &&
                    bullet->y + OBJECT_SIZE > enemy->y)
                {
                    DestroyEnemy(enemy);
                    
                    // Destroy bullet
                    SPR_releaseSprite(bullet->sprite);
                    POOL_release(bulletPool, bullet, TRUE); // Free the bullet
                    bullet = NULL;

//                    SND_startPlay_PCM(explosion_sound, sizeof(explosion_sound), SOUND_RATE_16000, SOUND_PAN_CENTER, 0);
                }
            }
        }
    }
    
    // Check player-enemy collisions
    FOREACH_ALLOCATED_IN_POOL(enemy, enemyPool)
    {
        if (enemy &&
            gameState.player.x < enemy->x + OBJECT_SIZE &&
            gameState.player.x + OBJECT_SIZE > enemy->x &&
            gameState.player.y < enemy->y + OBJECT_SIZE &&
            gameState.player.y + OBJECT_SIZE > enemy->y)
        {
            // Destroy player and enemy
            DestroyEnemy(enemy);
            
            ReleaseGameObject(enemy, enemyPool);
            
            gameState.gameOver = TRUE;
//            SND_startPlay_PCM(explosion_sound, sizeof(explosion_sound), SOUND_RATE_16000, SOUND_PAN_CENTER, 0);
            return; // Exit after collision
        }
    }
}

// Render the game
void RenderGame()
{
    SPR_update();
    
    static s16 lineOffsetX[10][30];
    
    for (u16 ind = 0; ind < SCROLL_LINES_BGA; ind++)
    {
        PlaneScrollingRule *scrollRule = &gameState.scrollRules[ind];
        
        if (scrollRule->autoScrollSpeed == 0)
            continue;
        
        scrollRule->scrollOffset += scrollRule->autoScrollSpeed;
        memsetU16((u16 *) lineOffsetX[ind], -FF32_toInt(scrollRule->scrollOffset), scrollRule->numOfLines);
        VDP_setHorizontalScrollTile(BG_A, scrollRule->startLineIndex, lineOffsetX[ind],
                                    scrollRule->numOfLines, DMA_QUEUE);
        
    }
}

