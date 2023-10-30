#pragma once

#include "entity.h"

#define MAX_PICKUPS 256

typedef enum
{
    PICKUP_TYPE_GEM,
} PickupType;

typedef enum
{
    GEM_TYPE_RED,
    GEM_TYPE_GREEN,
    GEM_TYPE_BLUE,
    GEM_TYPE_WHITE,
    GEM_TYPE_YELLOW,
    GEM_TYPE_PURPLE,
    GEM_TYPE_NONE,
    GEM_TYPE_MAX
} GemType;

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

extern int gems_image;
extern glist* pickup_list;
extern Pickup pickups[MAX_PICKUPS];

void pickup_init();
void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room);
void pickup_update(Pickup* pu, float dt);
void pickup_update_all(float dt);
void pickup_draw(Pickup* pu);
void pickup_handle_collision(Pickup* p, Entity* e);
