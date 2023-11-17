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
    CREATURE_TYPE_MAX,
} CreatureType;

typedef struct
{
    uint16_t id;

    CreatureType type;
    Physics phys;
    Rect hitbox;

    ProjectileType proj_type;

    int image;
    uint32_t color;
    uint8_t sprite_index;
    uint8_t curr_room;
    float h,v;

    bool painful_touch;
    int damage;

    bool damaged;
    float damaged_time;

    float action_counter;
    float action_counter_max;

    float act_time_min;
    float act_time_max;

    int xp;

    // temp debug stuff
    Vector2f spawn;
    int spawn_tile_x;    //room tile x
    int spawn_tile_y;    //room tile y

    float lerp_t;
    ObjectState server_state_prior;
    ObjectState server_state_target;
} Creature;

extern Creature prior_creatures[MAX_CREATURES];
extern Creature creatures[MAX_CREATURES];

void creature_init();
const char* creature_type_name(CreatureType type);
int creature_get_image(CreatureType type);
void creature_init_props(Creature* c);
void creature_clear_all();
Creature* creature_add(Room* room, CreatureType type, Vector2i* tile, Creature* creature);
void creature_add_direct(Creature* c);
void creature_update(Creature* c, float dt);
void creature_lerp(Creature* c, float dt);
void creature_update_all(float dt);
void creature_draw(Creature* c, bool batch);
void creature_draw_all();
void creature_handle_collision(Creature* c, Entity* e);

void creature_hurt(Creature* c, float damage);
void creature_die(Creature* c);
uint16_t creature_get_count();
uint16_t creature_get_room_count(uint8_t room_index);

