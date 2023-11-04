#pragma once

#include "math2d.h"
#include "glist.h"

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

typedef enum
{
    STATUS_EFFECT_NONE   = 0,
    STATUS_EFFECT_POISON,
    STATUS_EFFECT_COLD,
    STATUS_EFFECT_FEAR,
} StatusEffectType;

typedef void (*effect_func)(void* e, bool end);

typedef struct
{
    StatusEffectType type;
    effect_func func;
    float lifetime;
    float lifetime_max;
    bool periodic;
    float period;
    int periods_passed;
    bool applied;
} StatusEffect;

typedef struct
{
    Vector2f pos;
    Vector2f prior_pos;

    Vector2f vel;
    Vector2f prior_vel;
    float max_velocity;
    float base_friction;
    float speed;
    float radius;
    float mass;
    float elasticity;

    uint8_t hp;
    uint8_t hp_max;

    // status-effects factors
    float se_speed_factor;

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
float phys_get_friction_rate(float friction_factor, float dt);
void phys_apply_friction_x(Physics* phys, float rate);
void phys_apply_friction_y(Physics* phys, float rate);
void phys_apply_friction(Physics* phys, float friction, float dt);
