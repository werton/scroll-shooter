//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_DEFS
#define HEADER_DEFS

// Game settings
#define PLAY_MUSIC                      0
#define SHOW_FPS                        1
#define BLINK_TICKS                     3

// Game balance
#define BULLET_DAMAGE                   10
#define BULLET_HP                       2
#define ENEMY_DAMAGE                    10
#define PLAYER_DAMAGE                   10
#define PLAYER_HP                       10
#define FIRE_RATE                       14
#define WAVE_DURATION                   180  // frames per wave
#define WAVE_INTERVAL                   300  // frames between waves

// Movement and positioning
#define BULLET_OFFSET_X                 FIX16(8)
#define ENEMY_SPEED                     FIX16(2.5)
#define PLAYER_SPEED                    FIX16(2)

// Screen dimensions
#define SCREEN_HEIGHT                   224
#define SCREEN_WIDTH                    320
#define SCREEN_TILE_ROWS                28
#define SCROLL_PLANES                   5

// Object dimensions
#define BULLET_HEIGHT                   10
#define BULLET_WIDTH                    22
#define ENEMY_HEIGHT                    24
#define ENEMY_WIDTH                     24
#define ENEMY_HP                        10
#define ENEMY_SCORE_VALUE               10
#define OBJECT_SIZE                     16
#define PLAYER_HEIGHT                   24
#define PLAYER_WIDTH                    24

// Object limits
#define MAX_BULLETS                     20
#define MAX_ENEMIES                     16
#define MAX_EXPLOSION                   10

// Player settings
#define PLAYER_INITIAL_X                16
#define PLAYER_INITIAL_Y_OFFSET         48
#define PLAYER_INVINCIBILITY_DURATION   150
#define PLAYER_RESPAWN_DELAY            60
#define PLAYER_BLINK_RATE               4
#define PLAYER_BLINK_VISIBLE_FRAMES     2
#define PLAYER_LIVES                    3

// UI settings
#define JOIN_MESSAGE_BLINK_INTERVAL     60
#define JOIN_MESSAGE_VISIBLE_FRAMES     30
#define PLAYER1_JOIN_TEXT_POS_X         1
#define PLAYER2_JOIN_TEXT_POS_X         25
#define JOIN_TEXT_POS_Y                 27
#define JOIN_TEXT_WIDTH                 15
#define FPS_CPU_LOAD_POS_X              17
#define FPS_CPU_LOAD_POS_Y              27
#define FPS_POS_X                       21
#define FPS_POS_Y                       27

// Animation and effects
#define EXPLOSION_X_OFFSET              8
#define NORMAL_FRAME                    0
#define DAMAGE_FRAME                    1
#define PLAYER_NEUTRAL_ANIM             0
#define PLAYER_UP_ANIM                  1
#define PLAYER_DOWN_ANIM                2

// Sound settings
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

#endif //HEADER_DEFS

