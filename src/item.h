#pragma once

#include "entity.h"

#define MAX_ITEMS 500
#define ITEM_PICKUP_RADIUS 22.0

// order matters, see these functions:
//  - item_is_gem()
//  - item_is_heart()
typedef enum
{
    ITEM_NONE = -2,

    ITEM_CHEST = 0,
    ITEM_GEM_RED,
    ITEM_GEM_GREEN,
    ITEM_GEM_BLUE,
    ITEM_GEM_WHITE,
    ITEM_GEM_YELLOW,
    ITEM_GEM_PURPLE,

    ITEM_HEART_FULL,
    ITEM_HEART_HALF,

    ITEM_MAX
} ItemType;

typedef void (*item_func)(void* item, void* player);

typedef struct
{
    ItemType type;
    int image;
    uint8_t sprite_index;
    bool touch_item;
    item_func func;
} ItemProps;

typedef struct
{
    ItemType type;
    bool picked_up;
    uint8_t curr_room;
    Physics phys;
    bool highlighted;
    bool opened;
} Item;

extern int items_image;
extern glist* item_list;
extern Item items[MAX_ITEMS];
extern ItemProps item_props[MAX_ITEMS];

void item_init();
void item_clear_all();

bool item_is_gem(ItemType type);
bool item_is_heart(ItemType type);
bool item_is_chest(ItemType type);

ItemType item_get_random_gem();
ItemType item_get_random_heart();

const char* item_get_name(ItemType type);
void item_add(ItemType type, float x, float y, uint8_t curr_room);
bool item_remove(Item* pu);
void item_update(Item* pu, float dt);
void item_update_all(float dt);
void item_draw(Item* pu, bool batch);
void item_handle_collision(Item* p, Entity* e);

