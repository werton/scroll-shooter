//
// Created by weerb on 02.05.2025.
//

#include "game_object.h"
#include "defs.h"



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


// Check if two game objects are colliding
FORCE_INLINE bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2)
{
    if (obj1->y > obj2->y + FIX16(obj2->h) || obj1->y + FIX16(obj1->h) < obj2->y)
        return FALSE;
    
    if (obj1->x + FIX16(obj1->w) < obj2->x || obj1->x > obj2->x + FIX16(obj2->w))
        return FALSE;
    
    return TRUE;
}
