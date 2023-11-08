#pragma once

#include "math2d.h"
#include "glist.h"
#include "status_effects.h"

#define MAX_STATUS_EFFECTS 10 

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
    Vector3f pos;
    Vector3f prior_pos;

    Vector3f vel;
    Vector3f prior_vel;

    float max_velocity;
    float base_friction;
    float speed;
    float speed_factor;
    float radius;
    float mass;
    float height;
    float elasticity;

    int8_t hp;
    int8_t hp_max;

    bool dead;
    bool amorphous; // splatters on collision
    bool ethereal;  // can pass through walls

    StatusEffect status_effects[MAX_STATUS_EFFECTS];
    int status_effects_count;

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
void phys_apply_friction_x(Physics* phys, float friction, float dt);
void phys_apply_friction_y(Physics* phys, float friction, float dt);
void phys_apply_friction(Physics* phys, float friction, float dt);
