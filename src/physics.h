#pragma once

#include "math2d.h"
#include "glist.h"
#include "status_effects.h"

#define MAX_STATUS_EFFECTS 10 
#define GRAVITY_EARTH 1.0
#define CIRCULAR_DT_MAX 2*PI

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
    float width;
    float length;
    float height;
    float elasticity;
    float rotation_deg;
    float scale;

    Vector2i prior_tile;
    Vector2i curr_tile;
    uint8_t curr_room;
    float curr_tile_counter;    //used for breakable floors

    bool on_breakable_tile;

    Rect vr;
    Rect collision_rect_prior;
    Rect collision_rect;

    int8_t hp;
    int8_t hp_max;

    int16_t mp;
    int16_t mp_max;

    float circular_dt; // cumulative time mod 2*PI

    bool dead;
    bool amorphous; // splatters on collision
    bool ethereal;  // can pass through walls

    bool collided; // just a flag to indicate collision with something
    bool falling; // falling down a pit

    bool floating;
    bool bobbing;
    bool crawling;
    bool underground; // doesn't interact

    float stun_timer; // seconds. 0.0 = not stunned, counts down

    StatusEffect status_effects[MAX_STATUS_EFFECTS];
    int status_effects_count;

} Physics;

typedef struct
{
    Vector2f overlap;
} CollisionInfo;

// collision
bool phys_collision_circles(Physics* phys1, Physics* phys2, CollisionInfo* ci);
void phys_collision_correct(Physics* phys1, Physics* phys2, CollisionInfo* ci);

// gravity
void phys_apply_gravity(Physics* phys, float gravity_factor, float dt);

// friction
void phys_apply_friction_x(Physics* phys, float friction, float dt);
void phys_apply_friction_y(Physics* phys, float friction, float dt);
void phys_apply_friction(Physics* phys, float friction, float dt);

// other
void phys_add_circular_time(Physics* phys, float dt);
void phys_calc_collision_rect(Physics* phys);
void phys_set_collision_pos(Physics* phys, float new_x, float new_y);
void phys_print(Physics* phys);
void phys_print_dimensions(Physics* phys);
