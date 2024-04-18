#pragma once

#include "physics.h"
#include "projectile.h"
#include "net.h"
#include "entity.h"
#include "level.h"

#define MAX_CREATURES 1024
#define ACTION_COUNTER_MAX 1.0 // seconds
#define DAMAGED_TIME_MAX 0.2

typedef enum
{
    CREATURE_TYPE_SLUG,
    CREATURE_TYPE_CLINGER,
    CREATURE_TYPE_GEIZER,
    CREATURE_TYPE_FLOATER,
    CREATURE_TYPE_FLOATER_BIG,
    CREATURE_TYPE_BUZZER,
    CREATURE_TYPE_TOTEM_RED,
    CREATURE_TYPE_TOTEM_BLUE,
    CREATURE_TYPE_TOTEM_YELLOW,
    CREATURE_TYPE_SHAMBLER,
    CREATURE_TYPE_SPIKED_SLUG,
    CREATURE_TYPE_INFECTED,
    CREATURE_TYPE_GRAVITY_CRYSTAL,
    CREATURE_TYPE_PEEPER,
    CREATURE_TYPE_LEEPER,
    CREATURE_TYPE_SPAWN_EGG,
    CREATURE_TYPE_SPAWN_SPIDER,
    CREATURE_TYPE_BEHEMOTH,
    CREATURE_TYPE_BEACON_RED,
    CREATURE_TYPE_WATCHER,
    CREATURE_TYPE_MAX,
} CreatureType;

typedef struct
{
    Vector3f pos;
} CreatureNetLerp;

typedef struct
{
    uint16_t id;

    CreatureType type;
    Physics phys;

    int image;

    uint32_t base_color;
    uint32_t color;
    uint8_t sprite_index;
    float h,v;

    bool painful_touch;
    int damage;

    bool damaged;
    float damaged_time;

    bool invincible;
    bool passive;  // doesn't count toward creature total

    // AI
    float action_counter;
    float action_counter_max;

    float act_time_min;
    float act_time_max;

    float ai_counter;
    float ai_counter_max;
    int ai_state;
    int ai_value;

    bool windup;
    float windup_max;

    Vector2i target_tile;
    Vector2f target_pos;

    int xp;

    // temp debug stuff
    int spawn_tile_x;    //room tile x
    int spawn_tile_y;    //room tile y

    float lerp_t;
    CreatureNetLerp server_state_prior;
    CreatureNetLerp server_state_target;
} Creature;

extern Creature prior_creatures[MAX_CREATURES];
extern Creature creatures[MAX_CREATURES];
extern glist* clist;


void creature_init();
char* creature_type_name(CreatureType type);
int creature_get_image(CreatureType type);
ProjectileType creature_get_projectile_type(Creature* c);
void creature_init_props(Creature* c);
void creature_clear_all();
void creature_clear_room(uint8_t room_index);
void creature_kill_all();
void creature_kill_room(uint8_t room_index);
Creature* creature_add(Room* room, CreatureType type, Vector2i* tile, Creature* creature);
void creature_add_direct(Creature* c);
void creature_update(Creature* c, float dt);
void creature_lerp(Creature* c, float dt);
void creature_update_all(float dt);
void creature_draw(Creature* c);
void creature_handle_collision(Creature* c, Entity* e);
void creature_hurt(Creature* c, float damage);
void creature_die(Creature* c);
CreatureType creature_get_random();
uint16_t creature_get_count();
uint16_t creature_get_room_count(uint8_t room_index, bool count_passive);
bool creature_is_on_tile(Room* room, int tile_x, int tile_y);

