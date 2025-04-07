// gfx artwork created by Luis Zuno (@ansimuz)
// moon surface artwork and gfx created by mattwalkden.itch.io
// sfx created by JDSherbert jdsherbert.itch.io/

#include <genesis.h>
#include <maths.h>
#include "resources.h"

// Game config
#define SHOW_FPS                        1
#define PLAY_MUSIC                      0

// Constants
#define PLAYER_SPEED                    2
#define BULLET_OFFSET_X                 8
#define ENEMY_SPEED                     2
#define MAX_BULLETS                     14
#define MAX_ENEMIES                     16
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
#define FIRE_RATE                       12
#define WAVE_INTERVAL                   300  // frames between waves
#define WAVE_DURATION                   180  // frames per wave

#define FOREACH_ALLOCATED_IN_POOL(ObjectType, object, pool) \
    u16 object##objectsNum = POOL_getNumAllocated(pool); \
    ObjectType **object##FirstPtr = (ObjectType **) POOL_getFirst(pool); \
    for (ObjectType *object = *object##FirstPtr; \
         object##objectsNum-- != 0; \
         object##FirstPtr++, object = *object##FirstPtr)
         
#define FOREACH_PLAYERS(plr) \
    for (Player* plr=game.player; plr<game.player+game.playersNum; plr++)
    
// Global frame counter
u32 frameCount = 0;

// Enemy spawn patterns
typedef enum {
    PATTERN_NONE,
    PATTERN_HOR,
    PATTERN_SIN
} EnemyPattern;

typedef struct {
    EnemyPattern pattern;
    u16 enemyCount;
    u16 delay;
    u16 enemyDelay;
} EnemySpawner;

typedef struct {
    EnemySpawner *spawner;
    u16 delay;
    u16 enemyDelay;
    u16 spawnedCount;
    bool active;
} EnemyWave;

// Data structures
typedef struct {
    Sprite *sprite;
    s16 x, y;
    u16 w, h;
    s16 hp;
    s16 damage;
} GameObject;

typedef struct {
    GameObject;
    u16 blinkCounter;
} Enemy;

typedef struct {
    GameObject;
    u16 blinkCounter;
    u16 coolDownTicks;
    u8 index;
} Player;

typedef struct {
    VDPPlane plane;
    u16 startLineIndex;
    u16 numOfLines;
    ff32 autoScrollSpeed;
    ff32 scrollOffset;
} PlaneScrollingRule;

typedef struct {
    Player player[2];
    u32 frameCounter;
    PlaneScrollingRule scrollRules[SCROLL_PLANES];
    bool gameOver;
    u8 playersNum;
    EnemyWave wave; 
} GameState;

// Pools for bullets and enemies
Pool *bulletPool;
Pool *enemyPool;
Pool *explosionPool;

static s16 lineOffsetX[SCROLL_PLANES][SCREEN_TILE_ROWS];

GameState game = {
    .scrollRules = {
        [0] = {.plane = BG_A, .startLineIndex = 0, .numOfLines = 9, .autoScrollSpeed = FF32(0.04)},
        [1] = {.plane = BG_A, .startLineIndex = 9, .numOfLines = 4, .autoScrollSpeed = FF32(0.4)},
        [2] = {.plane = BG_A, .startLineIndex = 13, .numOfLines = 4, .autoScrollSpeed = FF32(1.1)},
        [3] = {.plane = BG_A, .startLineIndex = 17, .numOfLines = 11, .autoScrollSpeed = FF32(2)},
        [4] = {.plane = BG_B, .startLineIndex = 0, .numOfLines = 28, .autoScrollSpeed = FF32(0.01)},
    },
};

const EnemySpawner lineSpawner = {
        .pattern = PATTERN_HOR,
        .enemyCount = 10,
        .enemyDelay = 30,
        .delay = 60,
    };
    
const EnemySpawner sinSpawner = {
        .pattern = PATTERN_SIN,
        .enemyCount = 10,
        .enemyDelay = 25,
        .delay = 60,
    };    
    
// Function prototypes
void InitGame();
void InitPlayer(Player* player, u8 index);
void InitEnemies();
void UpdateInput(Player* player);
void UpdatePlayer(Player* player);
void UpdateBullets();
void UpdateExplosions();
void UpdateEnemies();
void SpawnEnemy(s16 x, s16 y);
void SwitchEnemyWave();
void UpdateEnemySpawner();
void RenderGame();
void TryShootBullet(Player* player);
void InitObjectsPools();
void SpawnBullet(GameObject *bullet, s16 x, s16 y);
void SpawnExplosion(s16 x, s16 y);
void DestroyEnemy(GameObject *enemy);
void GameObject_Release(GameObject *gameObject, Pool *pool);
void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, s16 x, s16 y, u16 w, u16 h, s16 hp, s16 damage);
void UpdatePlayerEnemyCollision(Player* player);
void UpdateBulletEnemyCollision();

int main(int hardReset) {
    if (!hardReset)
        SYS_hardReset();
        
    InitGame();
    
    while (TRUE) {
        if (!game.gameOver) {
            frameCount++;
            
            FOREACH_PLAYERS(player) {
                UpdateInput(player);
                UpdatePlayer(player);
                UpdatePlayerEnemyCollision(player);
            }
            
            UpdateBullets();
            UpdateEnemies();
            UpdateExplosions();
            UpdateBulletEnemyCollision();
            UpdateEnemySpawner();
            
            if (frameCount % 60 == 0) {
                // Spawn enemies in waves instead of randomly
                //SpawnEnemyWave();
            }
        }
        RenderGame();
        
        game.frameCounter++;
        SYS_doVBlankProcess();
    }
    
    return 0;
}

void InitGame() {
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
    
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    
#if PLAY_MUSIC    
    XGM2_play(xgm2_music);
#endif    
    JOY_init();
    SPR_init();
    
    VDP_drawImageEx(BG_A, &mapImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, TRUE, TRUE);
    VDP_drawImageEx(BG_B, &bgImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX + mapImage.tileset->numTile), 0, 0, TRUE, TRUE);
    
    game.gameOver = FALSE;
    game.playersNum = 2; 
    
    InitObjectsPools();
    
    InitPlayer(game.player, 0);
    InitPlayer(game.player+1, 1);
    
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);
    InitEnemies();
    SetEnemySpawner(&sinSpawner);
}

void InitObjectsPools() {
    enemyPool = POOL_create(MAX_ENEMIES, sizeof(Enemy));
    bulletPool = POOL_create(MAX_BULLETS, sizeof(GameObject));
    explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));
}

void InitPlayer(Player* player, u8 index) {
    player->index = index;
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    
    GameObject_Init((GameObject *)player, &player_sprite, PAL1, 16, SCREEN_HEIGHT/2 + index*48, 
                   PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_HP, PLAYER_DAMAGE);
    
    SPR_setAnimationLoop(player->sprite, FALSE);
    SPR_setVisibility(player->sprite, AUTO_FAST);
}

void InitEnemies() {
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
}

void UpdateInput(Player* player) {
    u16 input = JOY_readJoypad(player->index);
    
    if (input & BUTTON_LEFT)
        player->x -= PLAYER_SPEED;
    else if (input & BUTTON_RIGHT)
        player->x += PLAYER_SPEED;
    
    if (input & BUTTON_UP) {
        player->y -= PLAYER_SPEED;
        SPR_setAnim(player->sprite, 1);
    }
    else if (input & BUTTON_DOWN) {
        player->y += PLAYER_SPEED;
        SPR_setAnim(player->sprite, 2);
    }
    else {
        SPR_setAnim(player->sprite, 0);
    }
    
    if (input & BUTTON_A)
        TryShootBullet(player);
}

void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, s16 x, s16 y, u16 w, u16 h, s16 hp, s16 damage) {
    if (!object->sprite) {
        object->sprite = SPR_addSprite(spriteDef, x, y, TILE_ATTR(pal, FALSE, FALSE, FALSE));
    }
    else {
        SPR_setPosition(object->sprite, x, y);
    }
    
    SPR_setVisibility(object->sprite, AUTO_FAST);
    SPR_setAnimAndFrame(object->sprite, 0, 0);
    
    object->x = x;
    object->y = y;
    object->w = w;
    object->h = h;
    object->hp = hp;
    object->damage = damage;
}

void GameObject_Release(GameObject *gameObject, Pool *pool) {
    SPR_setVisibility(gameObject->sprite, HIDDEN);
    POOL_release(pool, gameObject, TRUE);
}

void TryShootBullet(Player* player) {
    if (player->coolDownTicks != 0)
        return;
    
    GameObject *bullet1 = (GameObject *)POOL_allocate(bulletPool);
    GameObject *bullet2 = (GameObject *)POOL_allocate(bulletPool);
    
    if (bullet1)
        SpawnBullet(bullet1, player->x + OBJECT_SIZE, player->y);
    if (bullet2)
        SpawnBullet(bullet2, player->x + OBJECT_SIZE, player->y + 16);
    
    if (bullet1 || bullet2) {
        XGM2_playPCM(xpcm_shoot, sizeof(xpcm_shoot), SOUND_PCM_CH2);
        player->coolDownTicks = FIRE_RATE;
    }
}

void SpawnBullet(GameObject *bullet, s16 x, s16 y) {
    GameObject_Init(bullet, &bullet_sprite, PAL1, x, y, BULLET_WIDTH, BULLET_HEIGHT, BULLET_HP, BULLET_DAMAGE);
    SPR_setAlwaysOnTop(bullet->sprite);
}

void SpawnExplosion(s16 x, s16 y) {
    GameObject *explosion = (GameObject *)POOL_allocate(explosionPool);
    
    if (explosion) {
        GameObject_Init(explosion, &explosion_sprite, PAL2, x, y, OBJECT_SIZE, OBJECT_SIZE, 0, 0);
        SPR_setAlwaysOnTop(explosion->sprite);
        SPR_setAnimationLoop(explosion->sprite, FALSE);
        XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), SOUND_PCM_CH3);
    }
}

void UpdatePlayer(Player* player) {
    player->x = clamp(player->x, 0, SCREEN_WIDTH - player->w);
    player->y = clamp(player->y, 0, SCREEN_HEIGHT - player->h);
    
    SPR_setPosition(player->sprite, player->x, player->y);
    
    if (player->coolDownTicks)
        player->coolDownTicks--;
}

void UpdateBullets() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, bulletPool) {
        if (bullet) {
            bullet->x += BULLET_OFFSET_X;
            SPR_setPosition(bullet->sprite, bullet->x, bullet->y);
            
            if (bullet->x > SCREEN_WIDTH * 2)
                GameObject_Release(bullet, bulletPool);
        }
    }
}

void UpdateExplosions() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, explosionPool) {
        if (explosion && SPR_isAnimationDone(explosion->sprite)) {
            GameObject_Release(explosion, explosionPool);
        }
    }
}

void UpdateEnemies() {
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
        if (!enemy)
            continue;
        
        if (enemy->blinkCounter) {
            enemy->blinkCounter--;
            if (enemy->blinkCounter == 0)
                SPR_setFrame(enemy->sprite, 0);
        }
        
        enemy->x -= ENEMY_SPEED;
        SPR_setPosition(enemy->sprite, enemy->x, enemy->y);
        
        if (SPR_getPositionX(enemy->sprite) < -ENEMY_WIDTH)
            GameObject_Release((GameObject *)enemy, enemyPool);
    }
}

void SpawnEnemy(s16 x, s16 y) {
    GameObject *enemy = (GameObject *) POOL_allocate(enemyPool);
    
    if (enemy)
        GameObject_Init(enemy, &enemy_sprite, PAL3, x, y, 
                       ENEMY_WIDTH, ENEMY_HEIGHT, ENEMY_HP, ENEMY_DAMAGE);
}

void SetEnemySpawner(EnemySpawner* spawner) {
	game.wave.spawner = spawner;
        
	game.wave.active = FALSE;
    game.wave.delay = spawner->delay;
    game.wave.enemyDelay = spawner->enemyDelay;
    game.wave.spawnedCount = 0;
}

void SwitchEnemyWave() {
    // Alternate between patterns
    if (game.wave.spawner->pattern == PATTERN_NONE || 
        game.wave.spawner->pattern == PATTERN_SIN) {
        // Start horizontal lines pattern
        SetEnemySpawner(&lineSpawner);
        
    } else {
        // Start sinusoidal pattern
        SetEnemySpawner(&sinSpawner);
    }   
}

void UpdateEnemySpawner() {
 
    // End wave if duration exceeded
    if (game.wave.delay)
    	game.wave.delay--;
    else
        game.wave.active = TRUE;
           
    if (!game.wave.active)
    	return;
    	
    game.wave.enemyDelay--;
    
    // End wave if duration exceeded
    if (!game.wave.enemyDelay)
    	game.wave.enemyDelay = game.wave.spawner->enemyDelay;
    else    
    	return;  	
    	
    switch (game.wave.spawner->pattern) {
        case PATTERN_HOR:
            // Spawn enemies on two horizontal lines
            SpawnEnemy(SCREEN_WIDTH, 50);  // Top line
            SpawnEnemy(SCREEN_WIDTH, SCREEN_HEIGHT - ENEMY_HEIGHT/2 - 50);  // Bottom line
            break;
            
        case PATTERN_SIN:
            // Spawn enemies in sinusoidal patterns (in opposite phase)
            // Calculate sinusoidal y positions
            s16 y1 = SCREEN_HEIGHT/2 + F16_toInt(F16_mul(F16_sin(F16(game.wave.spawnedCount*20)), F16(80)));
            s16 y2 = SCREEN_HEIGHT/2 + F16_toInt(F16_mul(F16_sin(F16(game.wave.spawnedCount*20 + 180)), F16(80)));   
            SpawnEnemy(SCREEN_WIDTH, y1);  // First sine wave
            SpawnEnemy(SCREEN_WIDTH, y2);  // Opposite phase sine wave
            break;
            
        default:
            break;
    }
    
    game.wave.spawnedCount++;
    
    if (game.wave.spawnedCount == game.wave.spawner->enemyCount) {
    	SwitchEnemyWave();
    }	
}

void DestroyEnemy(GameObject *enemy) {
    SpawnExplosion(enemy->x - 8, enemy->y);
    SPR_setFrame(enemy->sprite, 0);
    GameObject_Release(enemy, enemyPool);
}

FORCE_INLINE bool IsRectCollided(s16 x1, s16 y1, u16 w1, u16 h1, 
                               s16 x2, s16 y2, u16 w2, u16 h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && 
            y1 < y2 + h2 && y1 + h1 > y2);
}

FORCE_INLINE bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2) {
    return IsRectCollided(obj1->x, obj1->y, obj1->w, obj1->h,
                         obj2->x, obj2->y, obj2->w, obj2->h);
}

void UpdateBulletEnemyCollision() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, bulletPool) {
        if (!bullet)
            continue;
        
        FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
            if (!enemy)
                continue;
            
            if (GameObject_IsCollided(bullet, (GameObject *)enemy)) {
                if (enemy->hp - bullet->damage > 0) {
                    enemy->hp -= bullet->damage;
                    SPR_setFrame(enemy->sprite, 1);
                    enemy->blinkCounter = BLINK_TICKS;
                }
                else {
                    DestroyEnemy((GameObject *)enemy);
                }
                
                GameObject_Release(bullet, bulletPool);
                break;
            }
        }
    }
}

void UpdatePlayerEnemyCollision(Player* player) {
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
        if (!enemy)
            continue;
        
        if (GameObject_IsCollided((GameObject *)player, (GameObject *)enemy)) {
            DestroyEnemy((GameObject *)enemy);
            game.gameOver = TRUE;
        }
    }
}

void RenderGame() {
#if SHOW_FPS
    static char str[8];
    uintToStr(game.wave.spawner->pattern, str, 2);
    
    VDP_setTextPalette(PAL0);
    VDP_setWindowOnBottom(1);
    VDP_setTextPlane(WINDOW);
    VDP_showCPULoad(0, 27);
    VDP_showFPS(FALSE, 6, 27);
    VDP_drawTextBG(WINDOW, str, 10, 27);
#endif
    
    for (u16 ind = 0; ind < SCROLL_PLANES; ind++) {
        PlaneScrollingRule *scrollRule = &game.scrollRules[ind];
        
        if (scrollRule->autoScrollSpeed == 0)
            continue;
        
        scrollRule->scrollOffset += scrollRule->autoScrollSpeed;
        memsetU16((u16 *)lineOffsetX[ind], -FF32_toInt(scrollRule->scrollOffset), scrollRule->numOfLines);
        VDP_setHorizontalScrollTile(scrollRule->plane, scrollRule->startLineIndex, lineOffsetX[ind],
                                   scrollRule->numOfLines, DMA_QUEUE);
    }
    
    SPR_update();
}