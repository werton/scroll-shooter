

// gfx artwork created by Luis Zuno (@ansimuz)
// moon surface artwork and gfx created by mattwalkden.itch.io
// sfx created by JDSherbert jdsherbert.itch.io/


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
#define ENEMY_SIZE                      24
#define BULLET_WIDTH                    22
#define BULLET_HEIGHT                   10
#define SCROLL_LINES_BGA                3
#define PLAYER_HP 100
#define PLAYER_DAMAGE 2
#define ENEMY_HP 4
#define ENEMY_DAMAGE 2
#define BULLET_HP 2
#define BULLET_DAMAGE 2
#define BLINK_TICKS                      3


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
    s16 hp;
    s16 damage;
    u16 blinkCounter;
} GameObject;

typedef struct
{
    VDPPlane plane;
    u16 startLineIndex;
    u16 numOfLines;
    ff32 autoScrollSpeed;
    ff32 scrollOffset;
} PlaneScrollingRule;

typedef struct
{
    GameObject player;
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
        },
};

// Function prototypes
void InitGame();

void InitPlayer();

void InitEnemies();

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
    
    InitGame();
    
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
void InitGame()
{
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
    
    
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    XGM2_play(xgm2_music);
    
    JOY_init();
    SPR_init();

    
    VDP_drawImageEx(BG_A, &mapImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, TRUE, TRUE);
    
    InitObjectsPools();
    InitPlayer();
    
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);

    
    InitEnemies();
    
    
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
void InitPlayer()
{
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    
    gameState.player.sprite = SPR_addSprite(&player_sprite, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, TILE_ATTR(PAL1, 0, FALSE, FALSE));
    SPR_setAnimationLoop(gameState.player.sprite, FALSE);
    gameState.player.x = SCREEN_WIDTH / 2;
    gameState.player.y = SCREEN_HEIGHT / 2;
    gameState.player.hp = PLAYER_HP;
    gameState.player.damage = PLAYER_DAMAGE;
}


// Initialize enemies
void InitEnemies()
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
    {
        gameState.player.y -= PLAYER_SPEED;
        SPR_setAnim(gameState.player.sprite, 1);
    }
    else if (input & BUTTON_DOWN)
    {
        gameState.player.y += PLAYER_SPEED;
        SPR_setAnim(gameState.player.sprite, 2);
    }
    else
        SPR_setAnim(gameState.player.sprite, 0);
    
    if (input & BUTTON_A)
        TryShootBullet(); // Try shoot
    
}

void DamageGameObject(GameObject *gameObject, u16 damage)
{
    if (gameObject->hp - damage > 0)
        gameObject->hp -= damage;
    else
        gameObject->hp = 0;
}

void ReleaseGameObject(GameObject *gameObject, Pool *pool)
{
    SPR_releaseSprite(gameObject->sprite);
    gameObject->x = 0;
    POOL_release(pool, gameObject, TRUE);
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
    SPR_setAlwaysOnTop(bullet->sprite);
    
    bullet->x = x;
    bullet->y = y;
    bullet->hp = BULLET_HP;
    bullet->damage = BULLET_DAMAGE;
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
        SPR_setAlwaysOnTop(explosion->sprite);
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
            if (enemy->blinkCounter)
            {
                enemy->blinkCounter--;
                if (enemy->blinkCounter == 0)
                    SPR_setFrame(enemy->sprite, 0);
            }
            
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
        
        enemy->hp = ENEMY_HP;
        enemy->damage = ENEMY_DAMAGE;
        
    }
}

// Spawn an enemy
void DestroyEnemy(GameObject *enemy)
{
    CreateExplosion(enemy->x-8, enemy->y);
    SPR_setFrame(enemy->sprite, 0);
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
                    
                    bullet->x < enemy->x + ENEMY_SIZE &&
                    bullet->x + BULLET_WIDTH > enemy->x &&
                    bullet->y < enemy->y + ENEMY_SIZE &&
                    bullet->y + BULLET_HEIGHT > enemy->y)
                {
                    DamageGameObject(enemy, bullet->damage);
                    
                    if (enemy->hp == 0)
                        DestroyEnemy(enemy);
                    else
                    {
                        SPR_setFrame(enemy->sprite, 1);
                        enemy->blinkCounter = BLINK_TICKS;
                    }
                    
                    // Destroy bullet
                    ReleaseGameObject(bullet, bulletPool);
                    break;
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
            gameState.gameOver = TRUE;
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

