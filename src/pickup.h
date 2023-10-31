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
    GEM_TYPE_NONE = -1,
    GEM_TYPE_RED,
    GEM_TYPE_GREEN,
    GEM_TYPE_BLUE,
    GEM_TYPE_WHITE,
    GEM_TYPE_YELLOW,
    GEM_TYPE_PURPLE,

} GemType;

typedef enum
{
    HEALTH_TYPE_HEART_FULL = GEM_TYPE_PURPLE+1,
    HEALTH_TYPE_HEART_HALF,
} HealthType;

#define PICKUPS_MAX (HEALTH_TYPE_HEART_HALF+1)

#include "player.h"

typedef void (*pickup_func)(void* pickup, void* player);

typedef struct
{
    Physics phys;
    PickupType type;
    int subtype;
    int image;
    int sprite_index;
    int curr_room;
    bool touch_pickup;
    bool picked_up;
    pickup_func func;
} Pickup;

extern int pickups_image;
extern glist* pickup_list;
extern Pickup pickups[MAX_PICKUPS];


void pickup_init();
int pickup_get_sprite_index(PickupType type, int subtype);
const char* pickup_get_name(PickupType type, int subtype);
void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room);
void pickup_update(Pickup* pu, float dt);
void pickup_update_all(float dt);
void pickup_draw(Pickup* pu);
void pickup_handle_collision(Pickup* p, Entity* e);
