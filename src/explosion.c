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
void Explosion_Spawn(fix16 x, fix16 y) {
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


// Updates all active explosions
void Explosions_Update() {
    FOREACH_ALLOCATED_IN_POOL(GameObject, explosion, explosionPool) {
        // Remove explosion when its animation finishes
        if (explosion && SPR_isAnimationDone(explosion->sprite)) {
            GameObject_Release(explosion, explosionPool);
        }
    }
}