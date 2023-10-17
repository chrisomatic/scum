#pragma once

#include "physics.h"

#define MAX_CREATURES 256
#define ACTION_COUNTER_MAX 10.0

typedef enum
{
    CREATURE_TYPE_SLUG,
} CreatureType;

typedef struct
{
    CreatureType type;
    Physics phys;
    Rect hitbox;

    float hp;
    float hp_max;

    uint32_t color;
    uint8_t sprite_index;
    uint8_t curr_room;
    float speed;
    bool dead;

    // action
    float action_counter;

} Creature;

extern Creature creatures[MAX_CREATURES];

void creature_init();
void creature_add(Room* room, CreatureType type);
void creature_update(Creature* c, float dt);
void creature_update_all(float dt);
void creature_draw(Creature* c);
void creature_draw_all();

void creature_hurt(Creature* c, float damage);
void creature_die(Creature* c);
int creature_get_count();
