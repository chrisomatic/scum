#pragma once

#define MAX_ITEMS 500
#define ITEM_PICKUP_RADIUS 28.0

// order matters!
typedef enum
{
    ITEM_NONE = -1,

    ITEM_GEM_RED = 0,
    ITEM_GEM_GREEN,
    ITEM_GEM_BLUE,
    ITEM_GEM_WHITE,
    ITEM_GEM_YELLOW,
    ITEM_GEM_PURPLE,

    ITEM_HEART_FULL,
    ITEM_HEART_HALF,
    ITEM_HEART_EMPTY,

    ITEM_COSMIC_HEART_FULL,
    ITEM_COSMIC_HEART_HALF,
    ITEM_DRAGON_EGG,
    ITEM_SHAMROCK,
    ITEM_RUBY_RING,
    ITEM_POTION_STRENGTH,
    ITEM_POTION_MANA,
    ITEM_POTION_GREAT_MANA,
    ITEM_POTION_PURPLE,
    ITEM_GAUNTLET_SLOT,
    ITEM_NEW_LEVEL,
    ITEM_UPGRADE_ORB,
    ITEM_SILVER_STAR,
    ITEM_GOLD_STAR,
    ITEM_PURPLE_STAR,
    ITEM_REVIVE,
    ITEM_SKULL,
    ITEM_GALAXY_PENDANT,
    ITEM_FEATHER,
    ITEM_SHIELD,
    ITEM_WING,
    ITEM_LOOKING_GLASS,
    ITEM_COIN_COPPER,
    ITEM_COIN_SILVER,
    ITEM_COIN_GOLD,
    ITEM_SKILL_BOOK,
    ITEM_SOCCER_BALL,

    ITEM_CHEST,
    ITEM_CHEST_GUN,
    ITEM_SHRINE,
    ITEM_PODIUM,
    ITEM_GUN,

    ITEM_MAX
} ItemType;

typedef bool (*item_func)(void* item, void* player);
typedef void (*item_timed_func)(ItemType type, void* player);

typedef struct
{
    ItemType type;
    int image;
    uint8_t sprite_index;
    bool touchable;
    bool chestable;
} ItemProps;

typedef struct
{
    Vector3f pos;
    float angle;
} ItemNetLerp;

typedef struct
{
    ItemType type;
    uint16_t id;
    Physics phys;
    bool highlighted;
    bool used;
    float angle;
    char desc[32];
    uint8_t user_data;
    uint32_t user_data2;

    // Networking
    float lerp_t;
    ItemNetLerp server_state_prior;
    ItemNetLerp server_state_target;
} Item;

typedef struct
{
    int index;
    uint16_t id;
    float dist;
} ItemSort;

extern int items_image;
extern int chest_image;
extern int shrine_image;
extern int podium_image;
extern int guns_image;

extern glist* item_list;
extern Item prior_items[MAX_ITEMS];
extern Item items[MAX_ITEMS];
extern ItemProps item_props[ITEM_MAX];

#include "entity.h"

void item_init();
void item_clear_all();
Item* item_get_by_id(uint16_t id);

bool item_is_gem(ItemType type);
bool item_is_heart(ItemType type);
bool item_is_chest(ItemType type);
bool item_is_shrine(ItemType type);

ItemType item_get_random_heart();
ItemType item_rand(bool include_chest);
ItemType item_get_random_chestable();
ItemType item_get_random_coin();

const char* item_get_name(ItemType type);
const char* item_get_description(ItemType type, Item* it);
Item* item_add(ItemType type, float x, float y, uint8_t curr_room);
Item* item_add_gun(uint8_t gun_index, uint32_t seed, float x, float y, uint8_t curr_room);
void item_set_description(Item* it, char* fmt, ...);
bool item_use(Item* it, void* _player);
bool item_remove(Item* it);
void item_update(Item* it, float dt);
void item_update_all(float dt);
void item_draw(Item* it);
void item_lerp(Item* it, double dt);
void item_handle_collision(Item* it, Entity* e);
bool item_is_on_tile(Room* room, int tile_x, int tile_y);
void item_calc_phys_props(Item* it);
