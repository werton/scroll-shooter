//
// Created by weerb on 02.05.2025.
//
#include <genesis.h>
#include <maths.h>
#include "defs.h"
#include "globals.h"
#include "player.h"
#include "explosion.h"
#include "resources.h"
#include "game.h"


void Players_Create() {
	
	if (!game.players) {
    	// Allocate memory for new player
    	game.players = (struct Player *) MEM_alloc(sizeof(struct Player) * 2);
    
    	if (!game.players)
    		return;
    
        // Initialize player structure
    	memset(game.players, 0, sizeof(struct Player) * 2);
    
    	// Set player properties
    	game.players[0].index = 0;
    	game.players[1].index = 1;
    }
}


// Creates a new player and adds it to the player list
// @param index Player index (0 for first player, 1 for second)
// @return Pointer to created player or NULL if failed
Player* Player_Add(u8 index) {
    Player* player = &game.players[index];
    
    // Add to beginning of linked list
    player->next = game.playerListHead;
    if (game.playerListHead)
        game.playerListHead->prev = player;
    game.playerListHead = player;
    
    PAL_setPalette(PAL1, player_sprite.palette->data, DMA);
    
    // Initialize game object properties
    GameObject_Init((GameObject *)player, &player_sprite, PAL1,
                   FIX16(16), FIX16(SCREEN_HEIGHT/2 + index*48),
                   PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_HP, PLAYER_DAMAGE);
    
    // Configure sprite
    SPR_setAnimationLoop(player->sprite, FALSE);
    SPR_setVisibility(player->sprite, AUTO_FAST);
    
    // Set invincibility for 3 seconds (50 frames per second * 3 = 150 frames)
    player->invincibleTimer = 150;
    player->isDamageable = FALSE; // Player can't take damage during invincibility
    
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


// Removes a player from the game and frees its resources
// @param player Pointer to player to remove
void Player_Remove(Player* player) {
    if (!player) return;
    
    // Remove from linked list
    if (player->prev)
        player->prev->next = player->next;
    else
        game.playerListHead = player->next;
    
    if (player->next)
        player->next->prev = player->prev;
    
    // Release sprite and memory
//    SPR_releaseSprite(player->sprite);
//    MEM_free(player);
}


// Attempt to shoot bullets if cooldown allows
void Player_TryShoot(Player* player) {
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

// Updates player state (position, cooldowns)
// @param player Pointer to player to update
void Player_Update(Player* player) {
    // Handle invincibility timer and blinking
    if (!player->isDamageable) {
        player->invincibleTimer--;
        
        // Blink every 4 frames (adjust this value for different blink rates)
        if ((player->invincibleTimer % 3) == 0) {
            SPR_setVisibility(player->sprite, VISIBLE);
        } else {
            SPR_setVisibility(player->sprite, HIDDEN);
        }
        
        // End invincibility when timer runs out
        if (player->invincibleTimer == 0) {
            player->isDamageable = TRUE; // Player can now take damage
            SPR_setVisibility(player->sprite, VISIBLE); // Make sure player is visible
        }
    }
    
    // Clamp player position to screen boundaries
    player->x = clamp(player->x, FIX16(0), FIX16(SCREEN_WIDTH - player->w));
    player->y = clamp(player->y, FIX16(0), FIX16(SCREEN_HEIGHT - player->h));
    
    // Update sprite position
    SPR_setPosition(player->sprite, F16_toInt(player->x), F16_toInt(player->y));
    
    // Decrement shooting cooldown if active
    if (player->coolDownTicks)
        player->coolDownTicks--;
}


// Check collisions between player and enemies
void Player_UpdateEnemyCollision(Player* player) {
    if (player->state != PL_STATE_NORMAL)
      return;

    FOREACH_ALLOCATED_IN_POOL(Enemy , enemy, enemyPool) {
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



// Processes input for a player
// @param player Pointer to player to update
void Player_UpdateInput(Player* player) {
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
        Player_TryShoot(player);
        
}

// Initialize a bullet object at specified position
void Projectile_Spawn(Projectile *bullet, fix16 x, fix16 y, u8 ownerIndex) {
    GameObject_Init(bullet, &bullet_sprite, PAL1, x, y,
                    BULLET_WIDTH, BULLET_HEIGHT, BULLET_HP, BULLET_DAMAGE);
    SPR_setAlwaysOnTop(bullet->sprite);
    bullet->ownerIndex = ownerIndex;
}
