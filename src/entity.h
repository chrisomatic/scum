#pragma once

#include "physics.h"

#define MAX_ENTITIES 10000

typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_CREATURE,
    ENTITY_TYPE_PROJECTILE,
    ENTITY_TYPE_PICKUP,
    ENTITY_TYPE_EXPLOSION,
    ENTITY_TYPE_MAX,
} EntityType;

typedef struct
{
    EntityType type;
    void* ptr;
    uint8_t curr_room;
    Physics* phys;
} Entity;

extern Entity entities[MAX_ENTITIES];
extern int num_entities;

void entity_build_all(); // handles sorting too
void entity_handle_collisions();
void entity_handle_status_effects(float dt);
void entity_draw_all();

Physics* entity_get_closest_to(Physics* phys, uint8_t curr_room, EntityType type);
