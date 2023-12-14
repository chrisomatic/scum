#pragma once

#include "glist.h"
#include "physics.h"
#include "net.h"
#include "entity.h"

#define MAX_PROJECTILES 4096

typedef enum
{
    PROJECTILE_TYPE_PLAYER,
    PROJECTILE_TYPE_PLAYER_KINETIC_DISCHARGE,
    PROJECTILE_TYPE_CREATURE_GENERIC,
    PROJECTILE_TYPE_CREATURE_GEIZER,
    PROJECTILE_TYPE_CREATURE_CLINGER,
    PROJECTILE_TYPE_CREATURE_TOTEM_BLUE,
    PROJECTILE_TYPE_MAX
} ProjectileType;


typedef struct
{
    float damage;
    float speed;
    float accel;
    float scale;
    float ttl;
    bool explosive;
    bool bouncy;
    bool penetrate;
} ProjectileDef;

typedef struct
{
    int num;
    float spread;

    float ghost_chance;
    float homing_chance;
    float poison_chance;
    float cold_chance;
} ProjectileSpawn;

// typedef struct
// {
//     uint32_t color;

//     float damage;
//     float range;
//     float base_speed;
//     float accel;
//     float gravity_factor;

//     float angle_spread;
//     float scale;
//     uint8_t num;

//     bool charge;
//     uint8_t charge_rate;

//     bool ghost;
//     bool homing;
//     bool explosive;
//     bool bouncy;
//     bool penetrate;

//     float ghost_chance;
//     float homing_chance;

//     // elemental
//     float poison_chance;
//     float cold_chance;

// } ProjectileDef;

typedef struct
{
    uint16_t id;

    ProjectileDef def;
    Physics phys;

    // effects
    bool poison;
    bool cold;

    float radius;
    float angle_deg;
    uint8_t curr_room;

    uint8_t player_id;

    uint32_t color;

    // float scale;
    // float damage;
    // float time;
    // float ttl;
    bool from_player;
    Vector3f accel_vector;

    // bool homing;
    // Physics* homing_target;

    // Networking
    float lerp_t;
    ObjectState server_state_prior;
    ObjectState server_state_target;

} Projectile;

extern Projectile projectiles[MAX_PROJECTILES];
extern Projectile prior_projectiles[MAX_PROJECTILES];
extern ProjectileDef projectile_lookup[];
extern ProjectileSpawn projectile_spawn[];
extern glist* plist;

void projectile_init();
void projectile_clear_all();
void projectile_add(Physics* phys, uint8_t curr_room, ProjectileDef* def, ProjectileSpawn* spawn, uint32_t color, float angle_deg, bool from_player);
// void projectile_add(Physics* phys, uint8_t curr_room, ProjectileDef* projdef, float angle_deg, float scale, float damage_multiplier, bool from_player);
// void projectile_add_type(Physics* phys, uint8_t curr_room, ProjectileType proj_type, float angle_deg, float scale, float damage_multiplier, bool from_player);
void projectile_update_hit_box(Projectile* proj);
void projectile_update(float dt);
void projectile_kill(Projectile* proj);
void projectile_handle_collision(Projectile* p, Entity* e);
void projectile_draw(Projectile* proj);
void projectile_lerp(Projectile* p, double dt);
const char* projectile_def_get_name(ProjectileType proj_type);
