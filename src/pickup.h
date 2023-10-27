#pragma once

#define MAX_PICKUPS 256

typedef enum
{
    PICKUP_TYPE_GEM,
} PickupType;

typedef enum
{
    GEM_TYPE_WHITE,
    GEM_TYPE_RED,
    GEM_TYPE_GREEN,
    GEM_TYPE_BLUE,

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
} Pickup;

extern int gems_image;

void pickup_init();
void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room);
void pickup_draw(Pickup* pu);
