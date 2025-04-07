// Scroll shooter example
//
// Written by werton playskin 05/2025
// GFX artwork created by Luis Zuno (@ansimuz)
// Moon surface artwork and GFX created by mattwalkden.itch.io
// SFX created by JDSherbert jdsherbert.itch.io/

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
#define BULLET_DAMAGE                   5
#define BULLET_HP                       2
#define ENEMY_DAMAGE                    10
#define PLAYER_DAMAGE                   10
#define PLAYER_HP                       10

// Gameplay mechanics
#define FIRE_RATE                       12
#define WAVE_DURATION                   180  // frames per wave
#define WAVE_INTERVAL                   300  // frames between waves

// Object speeds
#define BULLET_OFFSET_X                 FIX16(8)
#define ENEMY_SPEED                     FIX16(1.6)
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
#define MAX_BULLETS                     14
#define MAX_ENEMIES                     16
#define MAX_EXPLOSION                   4

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
    for (Player* player = game.playerListHead; player != NULL; player = player->next)
    
    // Iterate through all players
#define FOREACH_PLAYER(player) \							
    for (Player* player = game.players[0]; player <= game.players[1]; player++)

    
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
    u16 delay;            // Initial delay before spawning
    u16 enemyDelay;       // Delay between enemy spawns
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
    Sprite *sprite;  // Sprite reference
    fix16 x, y;      // Position (fixed point)
    u16 w, h;        // Dimensions
    s16 hp;          // Hit points
    s16 damage;      // Damage dealt
    u16 blinkCounter;  // Counter for damage blink effect
} GameObject;

// Enemy object with blinking effect
typedef struct {
    GameObject;

} Enemy;

// Player object with cooldown and linked list pointers
typedef struct Player {
    GameObject;
    u16 coolDownTicks;  // Shooting cooldown counter
    u8 index;           // Player index (0 or 1)
    struct Player* next;// Next player in linked list
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
	Player* players[2];
    Player* playerListHead;              // Linked list of active players
    u32 frameCounter;                // Game frame counter
    PlaneScrollingRule scrollRules[SCROLL_PLANES]; // Background scrolling rules 
    EnemyWave wave;                  // Current enemy wave state
} GameState;

// =============================================
// Game State Initialization
// =============================================

// =============================================
// Global Variables
// =============================================
u32 frameCount = 0;  // Global frame counter
u32 blinkCounter = 0;
bool showSecondPlayerText = TRUE;

// Pools for game objects
Pool *bulletPool;
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
    .playerListHead = NULL  // Start with empty player list
};

// Enemy spawn patterns configurations
const EnemySpawner lineSpawner = {
    .pattern = PATTERN_HOR,
    .enemyCount = 10,
    .enemyDelay = 20,
    .delay = 60,
};

const EnemySpawner sinSpawner = {
    .pattern = PATTERN_SIN,
    .enemyCount = 10,
    .enemyDelay = 20,
    .delay = 60,
};

// Function prototypes
void InitGame();
Player* CreatePlayer(u8 index);
void RemovePlayer(Player* player);
void InitEnemies();
void UpdateInput(Player* player);
void UpdatePlayer(Player* player);
void UpdateBullets();
void UpdateExplosions();
void UpdateEnemies();
void SpawnEnemy(fix16 x, fix16 y);
void SwitchEnemyWave();
void UpdateEnemySpawner();
void RenderGame();
void TryShootBullet(Player* player);
void InitObjectsPools();
void SpawnBullet(GameObject *bullet, fix16 x, fix16 y);
void SpawnExplosion(fix16 x, fix16 y);
void GameObject_ReleaseWithExplode(GameObject *enemy, Pool *pool);
void GameObject_Release(GameObject *gameObject, Pool *pool);
void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, fix16 x, fix16 y, u16 w, u16 h, s16 hp, s16 damage);
void UpdatePlayerEnemyCollision(Player* player);
void UpdateBulletEnemyCollision();
void PlayerJoinUpdate();
void Player_Explode(Player *player);
void SetEnemySpawner(EnemySpawner* spawner);

// =============================================
// Main Game Loop
// =============================================

int main(int hardReset) {
    // Perform hard reset if needed
    if (!hardReset)
        SYS_hardReset();
        
    // Initialize game systems
    InitGame();
    
    // Main game loop
    while (TRUE) {
        
            frameCount++;
            
            // Check if second player wants to join
            PlayerJoinUpdate();
            
            // Update all active players
            FOREACH_ACTIVE_PLAYER(player) {
                UpdateInput(player);
                UpdatePlayer(player);
                UpdatePlayerEnemyCollision(player);
            }
            
            // Update game objects
            UpdateBullets();
            UpdateEnemies();
            UpdateExplosions();
            UpdateBulletEnemyCollision();
            UpdateEnemySpawner();
        
        
        // Render frame
        RenderGame();
        
        // Increment frame counter and wait for VBlank
        game.frameCounter++;
        SYS_doVBlankProcess();
    }
    
    return 0;
}

// =============================================
// Player Management Functions
// =============================================

// Checks if second player pressed START to join the game
void PlayerJoinUpdate() {
	
    // If second player doesn't exist and START is pressed on joypad 1
    if (game.players[0]->hp == 0 && (JOY_readJoypad(JOY_1) & BUTTON_START)) {
        CreatePlayer(0);  // Create second player
    }
    
    // If second player doesn't exist and START is pressed on joypad 2
    if ((!game.players[1] || game.players[1]->hp == 0) && (JOY_readJoypad(JOY_2) & BUTTON_START)) {
        CreatePlayer(1);  // Create second player
    }
}

// Creates a new player and adds it to the player list
// @param index Player index (0 for first player, 1 for second)
// @return Pointer to created player or NULL if failed
Player* CreatePlayer(u8 index) {
	
	if (!game.players[index]) {
    	// Allocate memory for new player
    	game.players[index] = MEM_alloc(sizeof(Player));
    	if (!game.players[index])
    		return NULL;
    }
    
    Player* player = game.players[index];
    // Initialize player structure
    memset(player, 0, sizeof(Player));
    
    // Add to beginning of linked list 
    player->next = game.playerListHead;
    game.playerListHead = player;
    
    // Set player properties
    player->index = index;
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    
    // Initialize game object properties
    GameObject_Init((GameObject *)player, &player_sprite, PAL1, 
                   FIX16(16), FIX16(SCREEN_HEIGHT/2 + index*48), 
                   PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_HP, PLAYER_DAMAGE);
    
    // Configure sprite
    SPR_setAnimationLoop(player->sprite, FALSE);
    SPR_setVisibility(player->sprite, AUTO_FAST);
    
    return player;
}

// Removes a player from the game and frees its resources
// @param player Pointer to player to remove
void RemovePlayer(Player* player) {
    if (!player) return;
        
    if (player->next)
        game.playerListHead = player->next;
    else
    {
    	if (game.playerListHead->next)
    		game.playerListHead->next = NULL;
    	else
    		game.playerListHead = NULL;
    }
    
    
    // Release sprite and memory
//    SPR_releaseSprite(player->sprite);
//    MEM_free(player);
}

// Handles player destruction with explosion effect
// @param player Pointer to player being destroyed
void Player_Explode(Player *player) {
    // Create explosion effect
    SpawnExplosion(player->x - FIX16(8), player->y);
    SPR_setFrame(player->sprite, 0);  // Reset sprite frame
    SPR_setVisibility(player->sprite, HIDDEN);
    
    // Remove player from game
    RemovePlayer(player);
    
}

// =============================================
// Game Initialization Functions
// =============================================

// Initializes all game systems and resources
void InitGame() {
    // Set up video mode and scrolling
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
    
    // Initialize sound system
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    
#if PLAY_MUSIC    
    XGM2_play(xgm2_music);
#endif
    
    // Initialize controllers and sprites
    JOY_init();
    SPR_init();
    
    // Load background images
    VDP_drawImageEx(BG_A, &mapImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 
                   0, 0, TRUE, TRUE);
    VDP_drawImageEx(BG_B, &bgImage, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 
                   TILE_USER_INDEX + mapImage.tileset->numTile), 0, 0, TRUE, TRUE);
                   
    
    // Create object pools
    InitObjectsPools();
    
    // Create first player
    CreatePlayer(0);
    
    // Load explosion palette and initialize enemies
    PAL_setPalette(PAL2, explosion_sprite.palette->data, DMA);
    InitEnemies();
    SetEnemySpawner(&sinSpawner);
}

// Initializes object pools for bullets, enemies and explosions
void InitObjectsPools() {
    enemyPool = POOL_create(MAX_ENEMIES, sizeof(Enemy));
    bulletPool = POOL_create(MAX_BULLETS, sizeof(GameObject));
    explosionPool = POOL_create(MAX_EXPLOSION, sizeof(GameObject));
}

// Initializes enemy resources (palette)
void InitEnemies() {
    PAL_setPalette(PAL3, enemy_sprite.palette->data, DMA);
}

// =============================================
// Input Handling
// =============================================

// Processes input for a player
// @param player Pointer to player to update
void UpdateInput(Player* player) {
    // Read controller state (JOY_1 is 0, JOY_2 is 1)
    u16 input = JOY_readJoypad(player->index);
    
    // Handle movement
    if (input & BUTTON_LEFT)
        player->x -= PLAYER_SPEED;
    else if (input & BUTTON_RIGHT)
        player->x += PLAYER_SPEED;
    
    // Handle vertical movement and animation
    if (input & BUTTON_UP) {
        player->y -= PLAYER_SPEED;
        SPR_setAnim(player->sprite, 1);  // Up animation
    }
    else if (input & BUTTON_DOWN) {
        player->y += PLAYER_SPEED;
        SPR_setAnim(player->sprite, 2);  // Down animation
    }
    else {
        SPR_setAnim(player->sprite, 0);  // Neutral animation
    }
    
    // Handle shooting
    if (input & BUTTON_A)
        TryShootBullet(player);
}

// =============================================
// Object Management
// =============================================

// Initializes a game object with given parameters
// @param object Pointer to object to initialize
// @param spriteDef Sprite definition to use
// @param pal Palette to use
// @param x Initial X position (fixed point)
// @param y Initial Y position (fixed point)
// @param w Width in pixels
// @param h Height in pixels
// @param hp Initial hit points
// @param damage Damage dealt by this object
void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, 
                    fix16 x, fix16 y, u16 w, u16 h, s16 hp, s16 damage) {
    // Create sprite if it doesn't exist, otherwise update position
    if (!object->sprite) {
        object->sprite = SPR_addSprite(spriteDef, F16_toInt(x), F16_toInt(y), 
                                     TILE_ATTR(pal, FALSE, FALSE, FALSE));
    }
    else {
        SPR_setPosition(object->sprite, F16_toInt(x), F16_toInt(y));
    }
    
    // Configure sprite properties
    SPR_setVisibility(object->sprite, AUTO_FAST);
    SPR_setAnimAndFrame(object->sprite, 0, 0);
    
    // Set object properties
    object->x = x;
    object->y = y;
    object->w = w;
    object->h = h;
    object->hp = hp;
    object->damage = damage;
}

// Releases a game object back to its pool
// @param gameObject Pointer to object to release
// @param pool Pool to release object to
void GameObject_Release(GameObject *gameObject, Pool *pool) {
    // Hide sprite and release back to pool
    SPR_setVisibility(gameObject->sprite, HIDDEN);
    POOL_release(pool, gameObject, TRUE);
}

// Attempts to shoot bullets from player's position
// @param player Pointer to shooting player
void TryShootBullet(Player* player) {
    // Check if shooting is on cooldown
    if (player->coolDownTicks != 0)
        return;
    
    // Allocate two bullets (for spread pattern)
    GameObject *bullet1 = (GameObject *)POOL_allocate(bulletPool);
    GameObject *bullet2 = (GameObject *)POOL_allocate(bulletPool);
    
    // Spawn bullets if allocation succeeded
    if (bullet1)
        SpawnBullet(bullet1, player->x + FIX16(OBJECT_SIZE), player->y);
    if (bullet2)
        SpawnBullet(bullet2, player->x + FIX16(OBJECT_SIZE), player->y + FIX16(16));
    
    // Play sound and set cooldown if any bullet was fired
    if (bullet1 || bullet2) {
        XGM2_playPCM(xpcm_shoot, sizeof(xpcm_shoot), SOUND_PCM_CH2);
        player->coolDownTicks = FIRE_RATE;
    }
}

// Spawns a bullet at specified position
// @param bullet Pointer to bullet object to initialize
// @param x X position (fixed point)
// @param y Y position (fixed point)
void SpawnBullet(GameObject *bullet, fix16 x, fix16 y) {
    GameObject_Init(bullet, &bullet_sprite, PAL1, x, y, 
                   BULLET_WIDTH, BULLET_HEIGHT, BULLET_HP, BULLET_DAMAGE);
    SPR_setAlwaysOnTop(bullet->sprite);  // Bullets should appear above other objects
}

// Spawns explosion effect at specified position
// @param x X position (fixed point)
// @param y Y position (fixed point)
void SpawnExplosion(fix16 x, fix16 y) {
    // Try to allocate explosion object
    GameObject *explosion = (GameObject *)POOL_allocate(explosionPool);
    
    if (explosion) {
        // Initialize explosion (no HP or damage as it's just visual)
        GameObject_Init(explosion, &explosion_sprite, PAL2, x, y, 
                       OBJECT_SIZE, OBJECT_SIZE, 0, 0);
        SPR_setAlwaysOnTop(explosion->sprite);
        SPR_setAnimationLoop(explosion->sprite, FALSE);  // Play once
        XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), SOUND_PCM_CH3);
    }
}

// Releases a game object after spawning explosion effect
// @param object Pointer to object being destroyed
// @param pool Pool to release object to
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool) {
    // Create explosion effect before releasing object
    SpawnExplosion(object->x - FIX16(8), object->y);
    SPR_setFrame(object->sprite, 0);  // Reset sprite frame
    GameObject_Release(object, pool);
}


// =============================================
// Update Functions
// =============================================

// Updates player state (position, cooldowns)
// @param player Pointer to player to update
void UpdatePlayer(Player* player) {
    // Clamp player position to screen boundaries
    player->x = clamp(player->x, FIX16(0), FIX16(SCREEN_WIDTH - player->w));
    player->y = clamp(player->y, FIX16(0), FIX16(SCREEN_HEIGHT - player->h));
    
    // Update sprite position
    SPR_setPosition(player->sprite, F16_toInt(player->x), F16_toInt(player->y));
    
    // Decrement shooting cooldown if active
    if (player->coolDownTicks)
        player->coolDownTicks--;
}

// Updates all active bullets
void UpdateBullets() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, bulletPool) {
        if (bullet) {
            // Move bullet right
            bullet->x += BULLET_OFFSET_X;
            SPR_setPosition(bullet->sprite, F16_toInt(bullet->x), F16_toInt(bullet->y));
            
            // Remove bullet if it goes off-screen
            if (bullet->x > FIX16(SCREEN_WIDTH))
                GameObject_Release(bullet, bulletPool);
        }
    }
}

// Updates all active explosions
void UpdateExplosions() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, explosionPool) {
        // Remove explosion when its animation finishes
        if (explosion && SPR_isAnimationDone(explosion->sprite)) {
            GameObject_Release(explosion, explosionPool);
        }
    }
}

// Updates all active enemies
void UpdateEnemies() {
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
        if (!enemy)
            continue;
        
        // Handle damage blink effect
        if (enemy->blinkCounter) {
            enemy->blinkCounter--;
            if (enemy->blinkCounter == 0)
                SPR_setFrame(enemy->sprite, 0);  // Reset to normal frame
        }
        
        // Move enemy left
        enemy->x -= ENEMY_SPEED;
        SPR_setPosition(enemy->sprite, F16_toInt(enemy->x), F16_toInt(enemy->y));
        
        // Remove enemy if it goes off-screen
        if (F16_toInt(enemy->x) < -ENEMY_WIDTH)
            GameObject_Release((GameObject *)enemy, enemyPool);
    }
}

// Spawns enemy at specified position
// @param x X position (fixed point)
// @param y Y position (fixed point)
void SpawnEnemy(fix16 x, fix16 y) {
    // Allocate and initialize enemy
    GameObject *enemy = (GameObject *) POOL_allocate(enemyPool);
    if (enemy)
        GameObject_Init(enemy, &enemy_sprite, PAL3, x, y, 
                       ENEMY_WIDTH, ENEMY_HEIGHT, ENEMY_HP, ENEMY_DAMAGE);
}

// Sets current enemy spawner configuration
// @param spawner Pointer to spawner configuration to use
void SetEnemySpawner(EnemySpawner* spawner) {
    game.wave.spawner = spawner;
        
    // Reset wave state
    game.wave.active = FALSE;
    game.wave.delay = spawner->delay;
    game.wave.enemyDelay = spawner->enemyDelay;
    game.wave.spawnedCount = 0;
}

// Switches between enemy spawn patterns
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

// Updates enemy spawner logic
void UpdateEnemySpawner() {
    // Handle initial delay before wave starts
    if (game.wave.delay) {
        game.wave.delay--;
    } else {
        game.wave.active = TRUE;
    }
           
    if (!game.wave.active)
        return;
        
    // Handle delay between enemy spawns
    game.wave.enemyDelay--;
    if (!game.wave.enemyDelay) {
        game.wave.enemyDelay = game.wave.spawner->enemyDelay;
    } else {    
        return;      
    }
        
    // Spawn enemies according to current pattern
    switch (game.wave.spawner->pattern) {
        case PATTERN_HOR:
            // Spawn enemies on two horizontal lines
            SpawnEnemy(FIX16(SCREEN_WIDTH), FIX16(50));  // Top line
            SpawnEnemy(FIX16(SCREEN_WIDTH), FIX16(SCREEN_HEIGHT - ENEMY_HEIGHT/2 - 50));  // Bottom line
            break;
            
        case PATTERN_SIN:
            // Spawn enemies in sinusoidal patterns (in opposite phase)
            fix16 y1 = FIX16(SCREEN_HEIGHT/2) + F16_mul(F16_sin(F16(game.wave.spawnedCount*20)), F16(80));
            fix16 y2 = FIX16(SCREEN_HEIGHT/2) + F16_mul(F16_sin(F16(game.wave.spawnedCount*20 + 180)), F16(80));   
            SpawnEnemy(FIX16(SCREEN_WIDTH), y1);  // First sine wave
            SpawnEnemy(FIX16(SCREEN_WIDTH), y2);  // Opposite phase sine wave
            break;
            
        default:
            break;
    }
    
    // Check if wave is complete
    game.wave.spawnedCount++;
    if (game.wave.spawnedCount == game.wave.spawner->enemyCount) {
        SwitchEnemyWave();  // Switch to next pattern
    }   
}

// =============================================
// Collision Detection
// =============================================

// Checks if two rectangles collide
// @return TRUE if rectangles overlap, FALSE otherwise
FORCE_INLINE bool IsRectCollided(fix16 x1, fix16 y1, u16 w1, u16 h1, 
                               fix16 x2, fix16 y2, u16 w2, u16 h2) {
    return (x1 < x2 + FIX16(w2) && x1 + FIX16(w1) > x2 && 
           y1 < y2 + FIX16(h2) && y1 + FIX16(h1) > y2);
}

// Checks if two game objects collide
// @return TRUE if objects collide, FALSE otherwise
FORCE_INLINE bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2) {
    return IsRectCollided(obj1->x, obj1->y, obj1->w, obj1->h,
                         obj2->x, obj2->y, obj2->w, obj2->h);
}

void GameObject_ApplyDamageBy(GameObject *object1, GameObject *object2)
{  
     // Apply damage to enemy
     if (object1->hp - object2->damage > 0) {
         object1->hp -= object2->damage;
         SPR_setFrame(object1->sprite, 1);  // Damage frame
         object1->blinkCounter = BLINK_TICKS;  // Start blink effect
     }
     else
     	object2->hp = 0;
}

bool GameObject_CollisionUpdate(GameObject *object1, GameObject *object2)
{
	  // Check if bullet hits enemy
	 if (GameObject_IsCollided(object1, object2)) {
	     // Apply damage to enemy
	     GameObject_ApplyDamageBy(object1, object2);
	     GameObject_ApplyDamageBy(object2, object1);
	     return TRUE;
	 }
	 return FALSE;
}

// Checks for collisions between bullets and enemies
void UpdateBulletEnemyCollision() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, bullet, bulletPool) {
        if (!bullet)
            continue;
        
        FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
            if (!enemy)
                continue;
            
            // Check if bullet hits enemy & Apply damage to enemy
            if (GameObject_CollisionUpdate(bullet, (GameObject *) enemy)) {
                // Apply damage to enemy
                
                // Destroy enemy if HP reaches zero
                if (!enemy->hp)
                	GameObject_ReleaseWithExplode((GameObject *)enemy, enemyPool);
                
                // Destroy bullet on hit
                GameObject_Release(bullet, bulletPool);
                break;
            }
        }
    }
}

// Checks for collisions between player and enemies
// @param player Pointer to player to check collisions for
void UpdatePlayerEnemyCollision(Player* player) {
    FOREACH_ALLOCATED_IN_POOL(Enemy, enemy, enemyPool) {
        if (!enemy)
            continue;
        
        // Check if player hits enemy 
        if (GameObject_CollisionUpdate((GameObject *) player, (GameObject *) enemy)) {
            // Apply damage to enemy
            
            // Destroy enemy if HP reaches zero
            if (!enemy->hp)
            	GameObject_ReleaseWithExplode((GameObject *)enemy, enemyPool);
                
            // Destroy player
            if (!player->hp)
            {
            	Player_Explode(player);
            	return;
            }
        }
    }
}

void RenderMessage() {
	FOREACH_PLAYER(player) {
    	   
    	// Show join prompt if second player not present 
        blinkCounter++;
        
        if (blinkCounter == 60) 
            blinkCounter = 0;
            
        if (blinkCounter < 30) {
        	if (player && !player->hp)
            	VDP_drawTextBG(WINDOW, "PRESS START", player->index * 10 + 8, 27);   
        }
        else
        	VDP_drawTextBG(WINDOW, "                           ", 8, 27);
    }
    	
}
// =============================================
// Rendering
// =============================================

// Renders the current game frame
void RenderGame() {
#if SHOW_FPS
    // Configure text display
    VDP_setTextPalette(PAL0);
    VDP_setWindowOnBottom(1);
    VDP_setTextPlane(WINDOW);
    
    // Show performance metrics
    VDP_showCPULoad(0, 27);
    VDP_showFPS(FALSE, 6, 27);
    
#endif
	//RenderMessage();
    
    // Update scrolling backgrounds
    for (u16 ind = 0; ind < SCROLL_PLANES; ind++) {
        PlaneScrollingRule *scrollRule = &game.scrollRules[ind];
        
        if (scrollRule->autoScrollSpeed == 0)
            continue;
        
        // Update scroll offset
        scrollRule->scrollOffset += scrollRule->autoScrollSpeed;
        memsetU16((u16 *)lineOffsetX[ind], -FF32_toInt(scrollRule->scrollOffset), scrollRule->numOfLines);
        
        // Apply scrolling to VDP
        VDP_setHorizontalScrollTile(scrollRule->plane, scrollRule->startLineIndex, lineOffsetX[ind],
                                   scrollRule->numOfLines, DMA_QUEUE);
    }
    
    // Update all sprites
    SPR_update();
}