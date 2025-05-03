//
// Created by weerb on 02.05.2025.
//

#include <genesis.h>
#include "explosion.h"
#include "defs.h"
#include "globals.h"
#include "game_object.h"
#include "resources.h"

// Spawns explosion effect at specified position
void Explosion_Spawn(fix16 x, fix16 y)
{
    // Try to allocate explosion object
    GameObject *explosion = (GameObject *) POOL_allocate(game.explosionPool);
    
    if (explosion)
    {
        // Initialize explosion (no HP or damage as it's just visual)
        GameObject_Init(explosion, &explosion_sprite, PAL2, x - OBJECT_SIZE/2, y,
                        OBJECT_SIZE, OBJECT_SIZE, 0, 0);
        SPR_setAlwaysOnTop(explosion->sprite);
        SPR_setAnimationLoop(explosion->sprite, FALSE);  // Play once
        XGM2_playPCM(xpcm_explosion, sizeof(xpcm_explosion), SOUND_PCM_CH3);
    }
}

// Update all active explosions animation state
void Explosions_Update()
{
    // ================================== Pool usage =======================================
    // Process all active(allocated) explosions
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, game.explosionPool)
    {
        if (!explosion)
            continue;
        
        // Update position and check animation completion
        SPR_setPosition(explosion->sprite, F16_toInt(explosion->x), F16_toInt(explosion->y));
        explosion->x += ENEMY_SPEED;
        
        // Return object back to pool if animation finished
        if (SPR_isAnimationDone(explosion->sprite))
            GameObject_Release(explosion, game.explosionPool);
    }
}