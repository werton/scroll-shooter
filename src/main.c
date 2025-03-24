

// gfx artwork created by Luis Zuno (@ansimuz)
// moon surface artwork and gfx created by mattwalkden.itch.io
// sfx created by JDSherbert jdsherbert.itch.io/


#include <genesis.h>
#include "resources.h"



// Game config
#define SHOW_FPS                        1

// Constants
#define PLAYER_SPEED                    2
#define BULLET_OFFSET_X                 8
#define ENEMY_SPEED                     1
#define MAX_BULLETS                     10
#define MAX_ENEMIES                     10
#define MAX_EXPLOSION                   4
#define SCREEN_WIDTH                    320
#define SCREEN_HEIGHT                   224
#define OBJECT_SIZE                     16
#define ENEMY_WIDTH                     24
#define ENEMY_HEIGHT                    24
#define BULLET_WIDTH                    22
#define BULLET_HEIGHT                   10
#define PLAYER_WIDTH                    24
#define PLAYER_HEIGHT                   24
#define SCROLL_PLANES                   5
#define PLAYER_HP                       100
#define PLAYER_DAMAGE                   2
#define ENEMY_HP                        4
#define ENEMY_DAMAGE                    2
#define BULLET_HP                       2
#define BULLET_DAMAGE                   2
#define BLINK_TICKS                     3
#define SCREEN_TILE_ROWS                28


// Gameplay constants
#define ENEMY_SPAWN_CHANCE              5 // Chance to spawn an enemy (out of 100)
#define RANDOM_MAX                      100       // Maximum value for random number generation
#define FIRE_RATE                       20         // Delay between shots (in frames)

#define FOREACH_ALLOCATED_IN_POOL(ObjectType, object, pool)                                              \
    u16 object##objectsNum = POOL_getNumAllocated(pool);                                     \
    ObjectType **object##FirstPtr = (ObjectType **) POOL_getFirst(pool);                     \
    for (ObjectType *object = *object##FirstPtr;                                             \
    object##objectsNum-- != 0;                                                               \
    object##FirstPtr++, object = *object##FirstPtr)                                          \


// Global frame counter
u32 frameCount = 0;

// Data structures
typedef struct
{
    Sprite *sprite;
    s16 x, y;
    u16 w, h;
    s16 hp;
    s16 damage;
} GameObject;

typedef struct
{
    GameObject obj;
    u16 blinkCounter;
} Enemy;

typedef struct
{
    GameObject obj;
    u16 blinkCounter;
    u16 coolDownTicks;
} Player;

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
    Player player;
    u32 frameCounter;
    s16 bgScrollX;
    PlaneScrollingRule scrollRules[SCROLL_PLANES];
    bool gameOver;
} GameState;

// Pools for bullets and enemies
Pool *bulletPool;
Pool *enemyPool;
Pool *explosionPool;
static s16 lineOffsetX[SCROLL_PLANES][SCREEN_TILE_ROWS];

GameState game = {
    .scrollRules =
        {
            [0] = {.plane = BG_A, .startLineIndex = 0, .numOfLines = 9, .autoScrollSpeed = FF32(0.04)},
            [1] = {.plane = BG_A, .startLineIndex = 9, .numOfLines = 4, .autoScrollSpeed = FF32(0.4)},
            [2] = {.plane = BG_A, .startLineIndex = 13, .numOfLines = 4, .autoScrollSpeed = FF32(1.1)},
            [3] = {.plane = BG_A, .startLineIndex = 17, .numOfLines = 11, .autoScrollSpeed = FF32(2)},
            [4] = {.plane = BG_B, .startLineIndex = 0, .numOfLines = 28, .autoScrollSpeed = FF32(0.01)},
        },
};

// Function prototypes
void InitGame();

void InitPlayer();

void InitEnemies();

void UpdateInput();

void UpdatePlayer();

void UpdateBullets();

void UpdateExplosions();

void UpdateEnemies();

void SpawnEnemy();

void UpdateCollisions();

void RenderGame();

u16 GetFrameCount();

void TryShootBullet();

void InitObjectsPools();

void SpawnBullet(GameObject *bullet, s16 x, s16 y);

void SpawnExplosion(s16 x, s16 y);

void DestroyEnemy(GameObject *enemy);

void GameObject_Release(GameObject *gameObject, Pool *pool);

void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, s16 x, s16 y, u16 w, u16 h, s16 hp, s16 damage);


void UpdatePlayerEnemyCollision();

void UpdateBulletEnemyCollision();

int main(int hardReset)
{
    if (!hardReset)
        SYS_hardReset();
        
    InitGame();
    
    while (TRUE)
    {
        if (!game.gameOver)
        {
            UpdateInput();
            UpdatePlayer();
            UpdateBullets();
            UpdateEnemies();
            UpdateExplosions();
            UpdateBulletEnemyCollision();
            UpdatePlayerEnemyCollision();
        }
        RenderGame();
        
        game.frameCounter++; // Increment frame counter
        SYS_doVBlankProcess();
    }
    
    return 0;
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
    VDP_drawImageEx(BG_B, &bgImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX + mapImage.tileset->numTile), 0, 0, TRUE,
                    TRUE);
    
    InitObjectsPools();
    InitPlayer();
    
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);
    
    InitEnemies();
    
    
    game.bgScrollX = 0;
    game.gameOver = FALSE;
}

void InitObjectsPools()
{// Initialize object pools
    enemyPool = POOL_create(MAX_ENEMIES, sizeof(Enemy));
    bulletPool = POOL_create(MAX_BULLETS, sizeof(GameObject));
    explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));
}

// Initialize the player
void InitPlayer()
{
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    
    GameObject_Init(&game.player.obj, &player_sprite, PAL1, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_HP, PLAYER_DAMAGE);
    
    SPR_setAnimationLoop(game.player.obj.sprite, FALSE);
    SPR_setVisibility(game.player.obj.sprite, AUTO_FAST);
    
}


// Initialize enemies
void InitEnemies()
{
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
}

// Handle input
void UpdateInput()
{
    u16 input = JOY_readJoypad(JOY_1);
    
    if (input & BUTTON_LEFT)
        game.player.obj.x -= PLAYER_SPEED;
    else if (input & BUTTON_RIGHT)
        game.player.obj.x += PLAYER_SPEED;
    
    if (input & BUTTON_UP)
    {
        game.player.obj.y -= PLAYER_SPEED;
        SPR_setAnim(game.player.obj.sprite, 1);
    }
    else if (input & BUTTON_DOWN)
    {
        game.player.obj.y += PLAYER_SPEED;
        SPR_setAnim(game.player.obj.sprite, 2);
    }
    else
        SPR_setAnim(game.player.obj.sprite, 0);
    
    if (input & BUTTON_A)
        TryShootBullet(); // Try shoot
}

void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, s16 x, s16 y, u16 w, u16 h, s16 hp, s16 damage)
{
    if (!object->sprite)
    {
        object->sprite = SPR_addSprite(spriteDef, x, y, TILE_ATTR(pal, FALSE, FALSE, FALSE));
//        kprintf("sprite created: %p", spriteDef);
    }
    else
        SPR_setPosition(object->sprite, x, y);
    
    SPR_setVisibility(object->sprite, AUTO_FAST);
    SPR_setAnimAndFrame(object->sprite, 0, 0);
    
    object->x = x;
    object->y = y;
    object->w = w;
    object->h = h;
    object->hp = hp;
    object->damage = damage;

}

void GameObject_Release(GameObject *gameObject, Pool *pool)
{
    SPR_setVisibility(gameObject->sprite, HIDDEN);
    POOL_release(pool, gameObject, TRUE);
}

void GameObject_DeliverDamage(GameObject *gameObject, u16 damage)
{
    if (gameObject->hp - damage > 0)
        gameObject->hp -= damage;
    else
        gameObject->hp = 0;
}

// Try to shoot a bullet
void TryShootBullet()
{
    // Check if enough time has passed since the last shot
    if (game.player.coolDownTicks != 0)
        return; // Exit if the fire rate delay has not passed
    
    // Allocate a bullet from the pool
    GameObject *bullet1 = (GameObject *) POOL_allocate(bulletPool);
    GameObject *bullet2 = (GameObject *) POOL_allocate(bulletPool);
    
    if (bullet1)
        SpawnBullet(bullet1, game.player.obj.x + OBJECT_SIZE, game.player.obj.y);
    if (bullet2)
        SpawnBullet(bullet2, game.player.obj.x + OBJECT_SIZE, game.player.obj.y + 16);
    
    if (bullet1 || bullet2)
    {
        XGM2_playPCM(xpcm_shoot, sizeof(xpcm_shoot), SOUND_PCM_CH2);
        // Update the last shot frame
        game.player.coolDownTicks = FIRE_RATE;
    }
    
}

void SpawnBullet(GameObject *bullet, s16 x, s16 y)
{
    GameObject_Init(bullet, &bullet_sprite, PAL1, x, y, BULLET_WIDTH, BULLET_HEIGHT, BULLET_HP, BULLET_DAMAGE);
    SPR_setAlwaysOnTop(bullet->sprite);
}

// Do Explosion
void SpawnExplosion(s16 x, s16 y)
{
    // Allocate a bullet from the pool
    GameObject *explosion = (GameObject *) POOL_allocate(explosionPool);
    
    if (explosion)
    {
        GameObject_Init(explosion, &explosion_sprite, PAL2, x, y, OBJECT_SIZE, OBJECT_SIZE, 0, 0);
        
        SPR_setAlwaysOnTop(explosion->sprite);
        SPR_setAnimationLoop(explosion->sprite, FALSE);
        
        XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), SOUND_PCM_CH3);
    }
    
}

// Update player position
void UpdatePlayer()
{
    game.player.obj.x = clamp(game.player.obj.x, 0, SCREEN_WIDTH - game.player.obj.w);
    game.player.obj.y = clamp(game.player.obj.y, 0, SCREEN_HEIGHT - game.player.obj.h);
    
    SPR_setPosition(game.player.obj.sprite, game.player.obj.x, game.player.obj.y);
    
    if (game.player.coolDownTicks)
        game.player.coolDownTicks--;
}

// Update bullets
void UpdateBullets()
{
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, bulletPool)
    {
        if (bullet)
        {
            bullet->x += BULLET_OFFSET_X;
            SPR_setPosition(bullet->sprite, bullet->x, bullet->y);
            
            if (bullet->x > SCREEN_WIDTH * 2)
                GameObject_Release(bullet, bulletPool);
        }
    }
}

// Update explosion
void UpdateExplosions()
{
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, explosionPool)
    {
        if (explosion && SPR_isAnimationDone(explosion->sprite))
        {
            GameObject_Release(explosion, explosionPool);
        }
    }
}

// Update enemies
void UpdateEnemies()
{
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool)
    {
        if (!enemy)
            continue;
        
        if (enemy->blinkCounter)
        {
            enemy->blinkCounter--;
            if (enemy->blinkCounter == 0)
                SPR_setFrame(enemy->obj.sprite, 0);
        }
        
        enemy->obj.x -= ENEMY_SPEED;
        SPR_setPosition(enemy->obj.sprite, enemy->obj.x, enemy->obj.y);
        
        if (SPR_getPositionX(enemy->obj.sprite) < -ENEMY_WIDTH)
            GameObject_Release(&enemy->obj, enemyPool);
        
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
        GameObject_Init(enemy, &enemy_sprite, PAL3, SCREEN_WIDTH, random() % SCREEN_HEIGHT, ENEMY_WIDTH, ENEMY_HEIGHT, ENEMY_HP,
                        ENEMY_DAMAGE);
}

// Spawn an enemy
void DestroyEnemy(GameObject *enemy)
{
    SpawnExplosion(enemy->x - 8, enemy->y);
    SPR_setFrame(enemy->sprite, 0);
    GameObject_Release(enemy, enemyPool);
}

FORCE_INLINE bool IsRectCollided(s16 x1, s16 y1, u16 w1, u16 h1, s16 x2, s16 y2, u16 w2, u16 h2)
{
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

FORCE_INLINE bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2)
{
    return IsRectCollided(obj1->x, obj1->y, obj1->w, obj1->h,
                          obj2->x, obj2->y, obj2->w, obj2->h);
}


void UpdateBulletEnemyCollision()
{// Check bullet-enemy collisions
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, bulletPool)
    {
        if (!bullet)
            continue;
        
        FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool)
        {
            if (!enemy)
                continue;
            
            if (GameObject_IsCollided(bullet, &enemy->obj))
            {
                
                GameObject_DeliverDamage(&enemy->obj, bullet->damage);
                
                if (enemy->obj.hp == 0)
                    DestroyEnemy(&enemy->obj);
                else
                {
                    SPR_setFrame(enemy->obj.sprite, 1);
                    enemy->blinkCounter = BLINK_TICKS;
                }
                
                // Destroy bullet
                GameObject_Release(bullet, bulletPool);
                break;
            }
        }
    }
}

void UpdatePlayerEnemyCollision()
{
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool)
    {
        if (!enemy)
            continue;
        
        // Check player-enemy collisions
        if (GameObject_IsCollided(&game.player.obj, &enemy->obj))
        {
            // Destroy player and enemy
            DestroyEnemy(&enemy->obj);
            game.gameOver = TRUE;
        }
    }
}

// Render the game
void RenderGame()
{

#if SHOW_FPS
    VDP_setTextPalette(PAL0);
    VDP_setWindowOnBottom(1);
    VDP_setTextPlane(WINDOW);
    VDP_showCPULoad(0, 27);
    VDP_showFPS(FALSE, 4, 27);
#endif
    
    
    for (u16 ind = 0; ind < SCROLL_PLANES; ind++)
    {
        PlaneScrollingRule *scrollRule = &game.scrollRules[ind];
        
        if (scrollRule->autoScrollSpeed == 0)
            continue;
        
        scrollRule->scrollOffset += scrollRule->autoScrollSpeed;
        memsetU16((u16 *) lineOffsetX[ind], -FF32_toInt(scrollRule->scrollOffset), scrollRule->numOfLines);
        VDP_setHorizontalScrollTile(scrollRule->plane, scrollRule->startLineIndex, lineOffsetX[ind],
                                    scrollRule->numOfLines, DMA_QUEUE);
        
    }
    
    SPR_update();
}

