#pragma once

#include "entity.h"

#define MAX_EXPLOSIONS 256

typedef struct
{
    Physics phys;
    float max_radius;
    float rate;
    uint8_t curr_room;
    bool from_player;
} Explosion;

extern glist* explosion_list;
extern Explosion explosions[MAX_EXPLOSIONS];

void explosion_init();
void explosion_add(float x, float y, float max_radius, float rate, uint8_t curr_room, bool from_player);
void explosion_update(Explosion* ex, float dt);
void explosion_update_all(float dt);
void explosion_draw(Explosion* ex);
void explosion_draw_all();
void explosion_handle_collision(Explosion* ex, Entity* e);
