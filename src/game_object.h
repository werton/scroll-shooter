//
// Created by weerb on 02.05.2025.
//

#ifndef HEADER_GAME_OBJECT
#define HEADER_GAME_OBJECT

#include <genesis.h>

// Basic game object properties
typedef struct GameObject {
    Sprite *sprite;         // Sprite reference
    fix16 x, y;             // Position (fixed point)
    u16 w, h;               // Dimensions
    s16 hp;                 // Hit points
    s16 damage;             // Damage dealt
    u16 blinkCounter;       // Counter for damage blink effect
} GameObject;



void GameObject_ApplyDamageBy(GameObject *object1, GameObject *object2);
bool GameObject_CollisionUpdate(GameObject *object1, GameObject *object2);
void GameObject_Init(GameObject *object, const SpriteDefinition *spriteDef, u16 pal, fix16 x, fix16 y, u16 w, u16 h, s16 hp, s16 damage);
void GameObject_Release(GameObject *gameObject, Pool *pool);
void GameObject_ReleaseWithExplode(GameObject *object, Pool *pool);
bool GameObject_IsCollided(GameObject *obj1, GameObject *obj2);


#endif  // HEADER_GAME_OBJECT
