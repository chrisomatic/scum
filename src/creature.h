#pragma once

#include "physics.h"

#define MAX_CREATURES 256

typedef enum
{
    CREATURE_TYPE_SLUG,
} CreatureType;

typedef struct
{
    CreatureType type;
    Physics phys;
    uint32_t color;
    uint8_t sprite_index;
    uint8_t curr_room;
} Creature;

void creature_init();
void creature_add(Room* room, CreatureType type);
void creature_update(Creature* c, float dt);
void creature_update_all(float dt);
void creature_draw(Creature* c);
void creature_draw_all();
