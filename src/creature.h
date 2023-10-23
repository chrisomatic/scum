#pragma once

#include "physics.h"

#define MAX_CREATURES 1024
#define ACTION_COUNTER_MAX 1.0 // seconds

typedef enum
{
    CREATURE_TYPE_SLUG,
    CREATURE_TYPE_CLINGER,
    CREATURE_TYPE_MAX,
} CreatureType;

typedef struct
{
    CreatureType type;
    Physics phys;
    Rect hitbox;

    float hp;
    float hp_max;

    int image;
    uint32_t color;
    uint8_t sprite_index;
    uint8_t curr_room;
    bool dead;
    float h,v;

    bool painful_touch;
    int damage;

    // action
    float action_counter;

    // temp debug stuff
    Vector2f spawn;
    int spawn_x;    //room tile x
    int spawn_y;    //room tile y

} Creature;

extern Creature creatures[MAX_CREATURES];

void creature_init();
void creature_clear_all();
void creature_add(Room* room, CreatureType type);
void creature_update(Creature* c, float dt);
void creature_update_all(float dt);
void creature_draw(Creature* c);
void creature_draw_all();

void creature_hurt(Creature* c, float damage);
void creature_die(Creature* c);
int creature_get_count();
