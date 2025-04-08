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
#include "resources.h"

// =============================================
// Game Configuration
// =============================================
#define PLAY_MUSIC                      0
#define SHOW_FPS                        1

// =============================================
// Game Constants
// =============================================
// Collision and damage
#define BLINK_TICKS                     3
#define BULLET_DAMAGE                   10
#define BULLET_HP                       2
#define ENEMY_DAMAGE                    10
#define PLAYER_DAMAGE                   10
#define PLAYER_HP                       10

// Gameplay mechanics
#define FIRE_RATE                       14
#define WAVE_DURATION                   180  // frames per wave
#define WAVE_INTERVAL                   300  // frames between waves

// Object speeds
#define BULLET_OFFSET_X                 FIX16(8)
#define ENEMY_SPEED                     FIX16(2.5)
#define PLAYER_SPEED                    FIX16(2)

// Screen dimensions
#define SCREEN_HEIGHT                   224
#define SCREEN_WIDTH                    320
#define SCREEN_TILE_ROWS                28
#define SCROLL_PLANES                   5

// Sprite dimensions
#define BULLET_HEIGHT                   10
#define BULLET_WIDTH                    22
#define ENEMY_HEIGHT                    24
#define ENEMY_WIDTH                     24
#define ENEMY_HP                        10
#define OBJECT_SIZE                     16
#define PLAYER_HEIGHT                   24
#define PLAYER_WIDTH                    24

// Object pools limits
#define MAX_BULLETS                     20
#define MAX_ENEMIES                     16
#define MAX_EXPLOSION                   10

// New Constants for Magic Numbers Replacement
// =============================================
// Player related
#define PLAYER_INITIAL_X                16
#define PLAYER_INITIAL_Y_OFFSET         48
#define PLAYER_INVINCIBILITY_DURATION   150
#define PLAYER_RESPAWN_DELAY            60
#define PLAYER_BLINK_RATE               4
#define PLAYER_BLINK_VISIBLE_FRAMES     2
#define PLAYER_LIVES                    3

// UI and messages
#define JOIN_MESSAGE_BLINK_INTERVAL     60
#define JOIN_MESSAGE_VISIBLE_FRAMES     30
#define PLAYER1_JOIN_TEXT_POS_X         1
#define PLAYER2_JOIN_TEXT_POS_X         25
#define JOIN_TEXT_POS_Y                 27

// Explosion effects
#define EXPLOSION_X_OFFSET              8

// Animation frames
#define NORMAL_FRAME                    0
#define DAMAGE_FRAME                    1
#define PLAYER_NEUTRAL_ANIM             0
#define PLAYER_UP_ANIM                  1
#define PLAYER_DOWN_ANIM                2

// Sound channels
#define SHOOT_SOUND_CHANNEL             SOUND_PCM_CH2
#define EXPLOSION_SOUND_CHANNEL         SOUND_PCM_CH3

// =============================================
// Macros
// =============================================

// Iterate through all allocated objects in a pool
#define FOREACH_ALLOCATED_IN_POOL(ObjectType, object, pool) \
    u16 object##objectsNum = POOL_getNumAllocated(pool); \
    ObjectType **object##FirstPtr = (ObjectType **) POOL_getFirst(pool); \
    for (ObjectType *object = *object##FirstPtr; \
         object##objectsNum-- != 0; \
         object##FirstPtr++, object = *object##FirstPtr)

// Iterate through all active players in the linked list
#define FOREACH_ACTIVE_PLAYER(player) \
    for (Player *player = game.playerListHead; player != NULL; player = player->next)

// Iterate through all players
#define FOREACH_PLAYER(player) \
    for (Player* player = game.players; player <= game.players+1; player++)


typedef enum {
  PL_STATE_SUSPENDED,
  PL_STATE_NORMAL,
  PL_STATE_INVINCIBLE,
  PL_STATE_DIED,
} PlayerState;


// =============================================
// Type Definitions
// =============================================

// Enemy spawn patterns
typedef enum {
    PATTERN_NONE,
    PATTERN_HOR,  // Horizontal line pattern
    PATTERN_SIN   // Sinusoidal pattern
} EnemyPattern;

// Enemy spawner configuration
typedef struct {
    EnemyPattern pattern;  // Spawn pattern type
    u16 enemyCount;        // Total enemies to spawn
    u16 delay;             // Initial delay before spawning
    u16 enemyDelay;        // Delay between enemy spawns
} EnemySpawner;

// Current enemy wave state
typedef struct {
    EnemySpawner *spawner;  // Pointer to spawner configuration
    u16 delay;              // Current delay counter
    u16 enemyDelay;         // Current enemy delay counter
    u16 spawnedCount;       // Number of enemies spawned so far
    bool active;            // Whether wave is currently active
} EnemyWave;

// Basic game object properties
typedef struct {
    Sprite *sprite;         // Sprite reference
    fix16 x, y;             // Position (fixed point)
    u16 w, h;               // Dimensions
    s16 hp;                 // Hit points
    s16 damage;             // Damage dealt
    u16 blinkCounter;       // Counter for damage blink effect
} GameObject;

// Projectile object
typedef struct {
    GameObject;
    u8 ownerIndex;
} Projectile;

// Enemy object
typedef struct {
    GameObject;
} Enemy;

// Player object with cooldown and linked list pointers
typedef struct Player {
    GameObject;
    u16 coolDownTicks;      // Shooting cooldown counter
    struct Player* prev;    // Previous player in linked list
    struct Player* next;    // Next player in linked list
    u16 invincibleTimer;    // Timer for invincibility after respawn
    u16 respawnTimer;       // Timer for respawning
    u16 score;
    u8 index;               // Player index (0 or 1)
    u8 lives;
    PlayerState state;
} Player;

// Scrolling plane configuration
typedef struct {
    VDPPlane plane;         // Background plane (A or B)
    u16 startLineIndex;     // First line to apply scrolling
    u16 numOfLines;         // Number of lines to scroll
    ff32 autoScrollSpeed;   // Scrolling speed (fixed point)
    ff32 scrollOffset;      // Current scroll offset
} PlaneScrollingRule;

// Main game state structure
typedef struct {
    Player* players;
    Player* playerListHead;              // Linked list of active players
    PlaneScrollingRule scrollRules[SCROLL_PLANES]; // Background scrolling rules
    EnemyWave wave;                      // Current enemy wave state
} GameState;

// =============================================
// Global Variables
// =============================================
u32 blinkCounter = 0;
bool showSecondPlayerText = TRUE;

// Pools for game objects
Pool *projectilePool;
Pool *enemyPool;
Pool *explosionPool;

// Line offset buffers for scrolling
static s16 lineOffsetX[SCROLL_PLANES][SCREEN_TILE_ROWS];

// Global game state with default values
GameState game = {
    .scrollRules = {
        [0] = {.plane = BG_A, .startLineIndex = 0, .numOfLines = 9, .autoScrollSpeed = FF32(0.04)},
        [1] = {.plane = BG_A, .startLineIndex = 9, .numOfLines = 4, .autoScrollSpeed = FF32(0.4)},
        [2] = {.plane = BG_A, .startLineIndex = 13, .numOfLines = 4, .autoScrollSpeed = FF32(1.1)},
        [3] = {.plane = BG_A, .startLineIndex = 17, .numOfLines = 11, .autoScrollSpeed = FF32(2)},
        [4] = {.plane = BG_B, .startLineIndex = 0, .numOfLines = 28, .autoScrollSpeed = FF32(0.01)},
    },
    .playerListHead = NULL,  // Start with empty player list
};

// Enemy spawn patterns configurations
const EnemySpawner lineSpawner = {
    .pattern = PATTERN_HOR,
    .enemyCount = 8,
    .enemyDelay = 15,
    .delay = 60,
};

const EnemySpawner sinSpawner = {
    .pattern = PATTERN_SIN,
    .enemyCount = 8,
    .enemyDelay = 15,
    .delay = 60,
};

// =============================================
// Function Prototypes
// =============================================
void Projectile_Spawn(Projectile *bullet, fix16 x, fix16 y, u8 ownerIndex);
void Bullet_UpdateEnemyCollision();
void Bullets_Update();
void Enemy_Spawn(fix16 x, fix16 y);
void EnemySpawner_Set(EnemySpawner* spawner);
void EnemySpawner_Update();
void EnemyWave_Switch();
void Enemies_Init();
void Enemies_Update();
void Explosion_Spawn(fix16 x, fix16 y);
void Explosions_Update();
void Game_Init();
void Game_Render();
void GameObject_ApplyDamageBy(GameObject *object1, GameObject *object2);
bool GameObject_CollisionUpdate(GameObject *object1, GameObject *object2);
void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, fix16 x, fix16 y, u16 w, u16 h, s16 hp, s16 damage);
void GameObject_Release(GameObject *gameObject, Pool *pool);
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool);
bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2);
bool IsRectCollided(fix16 x1, fix16 y1, u16 w1, u16 h1, fix16 x2, fix16 y2, u16 w2, u16 h2);
void ObjectsPools_Init();
Player* Player_Add(u8 index);
void Player_Explode(Player *player);
void Player_JoinUpdate();
void Player_Remove(Player* player);
void Bullet(Player* player);
void Player_Update(Player* player);
void Player_UpdateEnemyCollision(Player* player);
void Player_UpdateInput(Player* player);
void Players_Create();
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

        Bullets_Update();
        Enemies_Update();
        Explosions_Update();
        Bullet_UpdateEnemyCollision();
        EnemySpawner_Update();

        Game_Render();
        SYS_doVBlankProcess();
    }

    return 0;
}

// =============================================
// Function Implementations
// =============================================

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

// Initialize a bullet object at specified position
void Projectile_Spawn(Projectile *bullet, fix16 x, fix16 y, u8 ownerIndex) {
    GameObject_Init(bullet, &bullet_sprite, PAL1, x, y,
                   BULLET_WIDTH, BULLET_HEIGHT, BULLET_HP, BULLET_DAMAGE);
    SPR_setAlwaysOnTop(bullet->sprite);
    bullet->ownerIndex = ownerIndex;
}

// Check collisions between bullets and enemies
void Bullet_UpdateEnemyCollision() {
    FOREACH_ALLOCATED_IN_POOL(Projectile, bullet, projectilePool) {
        if (!bullet) continue;

        FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
            if (!enemy) continue;

            if (GameObject_CollisionUpdate(bullet, (GameObject *) enemy)) {
                if (!enemy->hp) {
                    GameObject_ReleaseWithExplode((GameObject *)enemy, enemyPool);

                    game.players[bullet->ownerIndex].score += 10;
                    Score_Update(&game.players[bullet->ownerIndex]);
                }
                GameObject_Release(bullet, projectilePool);
                break;
            }
        }
    }
}

// Update all active bullets movement and boundaries
void Bullets_Update() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, projectilePool) {
//        kprintf("bullet");
        if (bullet) {
            bullet->x += BULLET_OFFSET_X;
            SPR_setPosition(bullet->sprite, F16_toInt(bullet->x), F16_toInt(bullet->y));

            if (bullet->x > FIX16(SCREEN_WIDTH))
                GameObject_Release(bullet, projectilePool);
        }
    }
}

// Spawn enemy at specified position
void Enemy_Spawn(fix16 x, fix16 y) {
    GameObject *enemy = (GameObject *) POOL_allocate(enemyPool);
    if (enemy)
        GameObject_Init(enemy, &enemy_sprite, PAL3, x, y,
                       ENEMY_WIDTH, ENEMY_HEIGHT, ENEMY_HP, ENEMY_DAMAGE);
}

// Set current enemy spawner configuration
void EnemySpawner_Set(EnemySpawner* spawner) {
    game.wave.spawner = spawner;
    game.wave.active = FALSE;
    game.wave.delay = spawner->delay;
    game.wave.enemyDelay = spawner->enemyDelay;
    game.wave.spawnedCount = 0;
}

// Update enemy spawner logic
void EnemySpawner_Update() {
    if (game.wave.delay) {
        game.wave.delay--;
    } else {
        game.wave.active = TRUE;
    }

    if (!game.wave.active) return;

    game.wave.enemyDelay--;
    if (!game.wave.enemyDelay) {
        game.wave.enemyDelay = game.wave.spawner->enemyDelay;
    } else {
        return;
    }

    switch (game.wave.spawner->pattern) {
        case PATTERN_HOR:
            Enemy_Spawn(FIX16(SCREEN_WIDTH), FIX16(80));
            Enemy_Spawn(FIX16(SCREEN_WIDTH), FIX16(SCREEN_HEIGHT - ENEMY_HEIGHT - 80));
            break;

        case PATTERN_SIN:
            fix16 y1 = FIX16(SCREEN_HEIGHT/2 - ENEMY_HEIGHT) +
                      F16_mul(F16_sin(F16(game.wave.spawnedCount*20)), F16(50));
            fix16 y2 = FIX16(SCREEN_HEIGHT/2 - ENEMY_HEIGHT) +
                      F16_mul(F16_sin(F16(game.wave.spawnedCount*20 + 180)), F16(50));
            Enemy_Spawn(FIX16(SCREEN_WIDTH), y1);
            Enemy_Spawn(FIX16(SCREEN_WIDTH), y2);
            break;

        default: break;
    }

    game.wave.spawnedCount++;
    if (game.wave.spawnedCount == game.wave.spawner->enemyCount) {
        EnemyWave_Switch();
    }
}

// Switch to next enemy wave pattern
void EnemyWave_Switch() {
    if (game.wave.spawner->pattern == PATTERN_NONE ||
        game.wave.spawner->pattern == PATTERN_SIN) {
        EnemySpawner_Set(&lineSpawner);
    } else {
        EnemySpawner_Set(&sinSpawner);
    }
}

// Initialize enemies palette and resources
void Enemies_Init() {
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
}

// Update all active enemies movement and state
void Enemies_Update() {
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
        if (!enemy) continue;

        if (enemy->blinkCounter) {
            enemy->blinkCounter--;
            if (enemy->blinkCounter == 0)
                SPR_setFrame(enemy->sprite, NORMAL_FRAME);
        }

        enemy->x -= ENEMY_SPEED;
        SPR_setPosition(enemy->sprite, F16_toInt(enemy->x), F16_toInt(enemy->y));

        if (F16_toInt(enemy->x) < -ENEMY_WIDTH)
            GameObject_Release((GameObject *)enemy, enemyPool);
    }
}

// Create explosion effect at specified position
void Explosion_Spawn(fix16 x, fix16 y) {
    GameObject *explosion = (GameObject *)POOL_allocate(explosionPool);
    if (explosion) {
        GameObject_Init(explosion, &explosion_sprite, PAL2, x, y,
                       OBJECT_SIZE, OBJECT_SIZE, 0, 0);
        SPR_setAlwaysOnTop(explosion->sprite);
        SPR_setAnimationLoop(explosion->sprite, FALSE);
        XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), EXPLOSION_SOUND_CHANNEL);
    }
}

// Update all active explosions animation state
void Explosions_Update() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, explosionPool) {
        if (explosion && SPR_isAnimationDone(explosion->sprite)) {
            GameObject_Release(explosion, explosionPool);
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

// Apply damage from one object to another
void GameObject_ApplyDamageBy(GameObject *object1, GameObject *object2) {
     if (object1->hp > object2->damage) {
         object1->hp -= object2->damage;
         SPR_setFrame(object1->sprite, DAMAGE_FRAME);
         object1->blinkCounter = BLINK_TICKS;
     }
     else
        object1->hp = 0;
}

// Check and handle collision between two objects
bool GameObject_CollisionUpdate(GameObject *object1, GameObject *object2) {
     if (GameObject_IsCollided(object1, object2)) {
         GameObject_ApplyDamageBy(object1, object2);
         GameObject_ApplyDamageBy(object2, object1);
         return TRUE;
     }
     return FALSE;
}

// Initialize game object with specified parameters
void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal,
                    fix16 x, fix16 y, u16 w, u16 h, s16 hp, s16 damage) {
    if (!object->sprite) {
        object->sprite = SPR_addSprite(spriteDef, F16_toInt(x), F16_toInt(y),
                                     TILE_ATTR(pal, FALSE, FALSE, FALSE));
    }
    else {
        SPR_setPosition(object->sprite, F16_toInt(x), F16_toInt(y));
    }

    SPR_setVisibility(object->sprite, VISIBLE);
    SPR_setAnimAndFrame(object->sprite, 0, 0);

    object->x = x;
    object->y = y;
    object->w = w;
    object->h = h;
    object->hp = hp;
    object->damage = damage;
}

// Release game object back to pool
void GameObject_Release(GameObject *gameObject, Pool *pool) {
    SPR_setVisibility(gameObject->sprite, HIDDEN);
    POOL_release(pool, gameObject, TRUE);
}

// Release object with explosion effect
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool) {
    Explosion_Spawn(object->x - FIX16(EXPLOSION_X_OFFSET), object->y);
    SPR_setFrame(object->sprite, NORMAL_FRAME);
    GameObject_Release(object, pool);
}

// Check if two objects are colliding
bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2) {
    return IsRectCollided(obj1->x, obj1->y, obj1->w, obj1->h,
                         obj2->x, obj2->y, obj2->w, obj2->h);
}

// Check rectangle collision between two objects
bool IsRectCollided(fix16 x1, fix16 y1, u16 w1, u16 h1,
                   fix16 x2, fix16 y2, u16 w2, u16 h2) {
    return (x1 < x2 + FIX16(w2) && x1 + FIX16(w1) > x2 &&
           y1 < y2 + FIX16(h2) && y1 + FIX16(h1) > y2);
}

// Initialize object pools for game entities
void ObjectsPools_Init() {
    enemyPool = POOL_create(MAX_ENEMIES, sizeof(Enemy));
    projectilePool = POOL_create(MAX_BULLETS, sizeof(Projectile));
    explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));
}

// Create player objects array
void Players_Create() {
    if (!game.players) {
        game.players = (Player*) MEM_alloc(sizeof(Player)*2);
        if (!game.players) return NULL;
        memset(game.players, 0, sizeof(Player)*2);
        game.players[0].index = 0;
        game.players[1].index = 1;
        game.players[0].lives = PLAYER_LIVES;
        game.players[1].lives = PLAYER_LIVES;
        game.players[0].state = PL_STATE_SUSPENDED;
        game.players[1].state = PL_STATE_SUSPENDED;

    }
}

// Add new player to the game
Player* Player_Add(u8 index) {
    Player* player = &game.players[index];

    GameObject_Init((GameObject *)player, &player_sprite, PAL1,
                   FIX16(PLAYER_INITIAL_X),
                   FIX16(SCREEN_HEIGHT/2 + index*PLAYER_INITIAL_Y_OFFSET),
                   PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_HP, PLAYER_DAMAGE);
    SPR_setAnimationLoop(player->sprite, FALSE);
    player->index = index;
    player->invincibleTimer = PLAYER_INVINCIBILITY_DURATION;
    player->state = PL_STATE_INVINCIBLE;

    SPR_setVisibility(player->sprite, VISIBLE);
    SPR_setAnimAndFrame(player->sprite, 0, 0);

    player->prev = NULL;
    player->next = game.playerListHead;
    if (game.playerListHead)
        game.playerListHead->prev = player;
    game.playerListHead = player;

    return player;
}

// Handle player explosion and removal
void Player_Explode(Player *player) {
    Explosion_Spawn(player->x - FIX16(EXPLOSION_X_OFFSET), player->y);
    SPR_setVisibility(player->sprite, HIDDEN);
    player->hp = 0;
    player->state = PL_STATE_DIED;
    player->coolDownTicks = 0;
    player->respawnTimer = PLAYER_RESPAWN_DELAY;
    Player_Remove(player);

    if (player->lives > 0)
        player->lives--;

    if (player->lives == 0)
        player->state = PL_STATE_SUSPENDED;

}

// Check for new players joining the game
void Player_JoinUpdate() {
  FOREACH_PLAYER(player) {
    switch (player->state) {

        case PL_STATE_SUSPENDED:
            if (JOY_readJoypad(player->index) & BUTTON_START) {
                player->lives = PLAYER_LIVES;
                player->state = PL_STATE_DIED;
                player->respawnTimer = 0;
                player->score = 0;
                RenderScore(player);
            }
            break;

        case PL_STATE_DIED:
            if (player->respawnTimer > 0)
                player->respawnTimer--;

            if (player->respawnTimer == 0) {
                Player_Add(player->index);
            }
            break;
    }
  }
}

// Remove player from active players list
void Player_Remove(Player* player) {
    if (!player) return;

    if (player->prev)
        player->prev->next = player->next;
    else
        game.playerListHead = player->next;

    if (player->next)
        player->next->prev = player->prev;
}

// Attempt to shoot bullets if cooldown allows
void Bullet(Player* player) {
    if (player->state == PL_STATE_DIED)
      return;

    if (player->coolDownTicks != 0) return;

    GameObject *bullet1 = (Projectile *)POOL_allocate(projectilePool);
    GameObject *bullet2 = (Projectile *)POOL_allocate(projectilePool);

    if (bullet1)
        Projectile_Spawn(bullet1, player->x + FIX16(OBJECT_SIZE), player->y, player->index);
    if (bullet2)
        Projectile_Spawn(bullet2, player->x + FIX16(OBJECT_SIZE), player->y + FIX16(16), player->index);

    if (bullet1 || bullet2) {
        XGM2_playPCM(xpcm_shoot, sizeof(xpcm_shoot), SHOOT_SOUND_CHANNEL);
        player->coolDownTicks = FIRE_RATE;
    }
}

// Update player state and position
void Player_Update(Player* player) {
    if (player->state == PL_STATE_DIED)
      return;

    if (player->state == PL_STATE_INVINCIBLE) {
        player->invincibleTimer--;

        if ((player->invincibleTimer % PLAYER_BLINK_RATE) <= PLAYER_BLINK_VISIBLE_FRAMES) {
            SPR_setVisibility(player->sprite, VISIBLE);
        } else {
            SPR_setVisibility(player->sprite, HIDDEN);
        }

        if (player->invincibleTimer == 0) {
            player->state = PL_STATE_NORMAL;
            SPR_setVisibility(player->sprite, VISIBLE);
        }
    }

    player->x = clamp(player->x, FIX16(0), FIX16(SCREEN_WIDTH - player->w));
    player->y = clamp(player->y, FIX16(0), FIX16(SCREEN_HEIGHT - player->h));

    SPR_setPosition(player->sprite, F16_toInt(player->x), F16_toInt(player->y));

    if (player->coolDownTicks)
        player->coolDownTicks--;
}

// Check collisions between player and enemies
void Player_UpdateEnemyCollision(Player* player) {
    if (player->state != PL_STATE_NORMAL)
      return;

    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
        if (!enemy) continue;

        if (GameObject_CollisionUpdate((GameObject *) player, (GameObject *) enemy)) {
            if (!enemy->hp)
                GameObject_ReleaseWithExplode((GameObject *)enemy, enemyPool);

            if (!player->hp) {
                Player_Explode(player);
                return;
            }
        }
    }
}

// Handle player input and movement
void Player_UpdateInput(Player* player) {
    if (player->state == PL_STATE_DIED)
      return;

    u16 input = JOY_readJoypad(player->index);

    if (input & BUTTON_LEFT)
        player->x -= PLAYER_SPEED;
    else if (input & BUTTON_RIGHT)
        player->x += PLAYER_SPEED;

    if (input & BUTTON_UP) {
        player->y -= PLAYER_SPEED;
        SPR_setAnim(player->sprite, PLAYER_UP_ANIM);
    }
    else if (input & BUTTON_DOWN) {
        player->y += PLAYER_SPEED;
        SPR_setAnim(player->sprite, PLAYER_DOWN_ANIM);
    }
    else {
        SPR_setAnim(player->sprite, PLAYER_NEUTRAL_ANIM);
    }

    if (input & BUTTON_A)
        Bullet(player);

    if (input & BUTTON_C) {
        SPR_setVisibility(player->sprite, HIDDEN);
        Player_Remove(player);
        player->state = PL_STATE_SUSPENDED;
    }

}

