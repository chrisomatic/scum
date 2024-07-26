#pragma once

#include "physics.h"

#define MAX_ENTITIES 10000

typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_CREATURE,
    ENTITY_TYPE_CREATURE_SEGMENT,
    ENTITY_TYPE_PROJECTILE,
    ENTITY_TYPE_ITEM,
    ENTITY_TYPE_EXPLOSION,
    ENTITY_TYPE_WEAPON,
    ENTITY_TYPE_WALL_COLUMN,
    ENTITY_TYPE_MAX,
} EntityType;

typedef struct
{
    EntityType type;
    void* ptr;
    int tile;   //TileType
    Physics* phys;
    Vector2f pos;
    bool draw_only;
    Rect* collision;
    //bool shadow;
} Entity;

extern Entity entities[MAX_ENTITIES];
extern int num_entities;

void entity_build_all(); // handles sorting too
void entity_update_all(float dt);
void entity_draw_all();

Physics* entity_get_closest_to(Physics* phys, uint8_t curr_room, EntityType type, uint16_t* exclude_ids, int exclude_count, uint16_t* target_id);
