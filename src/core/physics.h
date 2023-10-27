#pragma once

#include "math2d.h"

// collision position
#define CPOSX(phys) ((phys).pos.x+(phys).coffset.x)
#define CPOSY(phys) ((phys).pos.y+(phys).coffset.y)

typedef enum
{
    COLLISION_SHAPE_CIRCLE,
    COLLISION_SHAPE_RECT,
    COLLISION_SHAPE_MAX,
} CollisionShape;

typedef struct
{
    Vector2f pos;
    Vector2f target_pos;
    Vector2f prior_pos;

    Vector2f vel;
    Vector2f prior_vel;
    float max_velocity;
    float base_friction;
    float speed;
    float radius;
    float mass;

    bool dead;
    bool amorphous; // splatters on collision
    bool ethereal;  // can pass through walls

    Vector2f coffset;
} Physics;

typedef struct
{
    Vector2f overlap;
} CollisionInfo;

// collision
bool phys_collision_circles(Physics* phys1, Physics* phys2, CollisionInfo* ci);
void phys_collision_correct(Physics* phys1, Physics* phys2, CollisionInfo* ci);

// friction
float phys_get_friction_rate(float friction_factor, float dt);
void phys_apply_friction_x(Physics* phys, float rate);
void phys_apply_friction_y(Physics* phys, float rate);
void phys_apply_friction(Physics* phys, float rate);
