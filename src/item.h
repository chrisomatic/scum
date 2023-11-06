#pragma once

#include "entity.h"

#define MAX_ITEMS 256
#define NUM_GEM_TYPES 6

typedef enum
{
    ITEM_TYPE_GEM,
    ITEM_TYPE_HEALTH,
} ItemType;

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

#define ITEMS_MAX (HEALTH_TYPE_HEART_HALF+1)

#include "player.h"

typedef void (*item_func)(void* item, void* player);

typedef struct
{
    Physics phys;
    ItemType type;
    int subtype;
    int image;
    int sprite_index;
    int curr_room;
    bool touch_item;
    bool picked_up;
    item_func func;
} Item;

extern int items_image;
extern glist* item_list;
extern Item items[MAX_ITEMS];


void item_init();
int item_get_sprite_index(ItemType type, int subtype);
const char* item_get_name(ItemType type, int subtype);
void item_add(ItemType type, int subtype, float x, float y, uint8_t curr_room);
void item_update(Item* pu, float dt);
void item_update_all(float dt);
void item_draw(Item* pu);
void item_handle_collision(Item* p, Entity* e);
