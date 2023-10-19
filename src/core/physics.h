#pragma once

#include "math2d.h"

// collision position
#define CPOSX(phys) ((phys).pos.x+(phys).coffset.x)
#define CPOSY(phys) ((phys).pos.y+(phys).coffset.y)

typedef struct
{
    Vector2f pos;
    Vector2f target_pos;
    Vector2f prior_pos;

    Vector2f vel;
    float speed;
    float radius;
    float mass;

    Vector2f coffset;
} Physics;

// collision
void phys_collision_circles(Physics* phys1, Physics* phys2);

// friction
float phys_get_friction_rate(float friction_factor, float dt);
void phys_apply_friction_x(Physics* phys, float rate);
void phys_apply_friction_y(Physics* phys, float rate);
void phys_apply_friction(Physics* phys, float rate);
