#pragma once

#include "entity.h"

#define MAX_PICKUPS 256
#define NUM_GEM_TYPES 6

typedef enum
{
    PICKUP_TYPE_GEM,
    PICKUP_TYPE_HEALTH,
} PickupType;

typedef enum
{
    PICKUP_TYPE_GEM_NONE = -1,
    PICKUP_TYPE_GEM_RED,
    PICKUP_TYPE_GEM_GREEN,
    PICKUP_TYPE_GEM_BLUE,
    PICKUP_TYPE_GEM_WHITE,
    PICKUP_TYPE_GEM_YELLOW,
    PICKUP_TYPE_GEM_PURPLE,
} GemType;

typedef enum
{
    PICKUP_TYPE_HEART_FULL = 6,
    PICKUP_TYPE_HEART_HALF,
} HealthType;

typedef struct
{
    Physics phys;
    PickupType type;
    int subtype;
    int image;
    int sprite_index;
    int curr_room;
    bool picked_up;
} Pickup;

extern int pickups_image;
extern glist* pickup_list;
extern Pickup pickups[MAX_PICKUPS];

void pickup_init();
void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room);
void pickup_update(Pickup* pu, float dt);
void pickup_update_all(float dt);
void pickup_draw(Pickup* pu);
void pickup_handle_collision(Pickup* p, Entity* e);
