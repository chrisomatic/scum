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
    bool cluster;

    int cluster_stages;
    int cluster_num[3];
    float cluster_scales[3];

} ProjectileDef;

typedef struct
{
    int num;
    float spread;

    float ghost_chance;
    float homing_chance;
    float poison_chance;
    float cold_chance;
    float fire_chance;

    int cluster_stage;
} ProjectileSpawn;


typedef struct
{
    Vector3f pos;
} ProjectileNetLerp;

typedef struct
{
    uint16_t id;

    ProjectileDef def;
    Physics phys;

    // effects
    bool poison;
    bool cold;
    bool fire;

    float radius;
    float angle_deg;
    uint8_t curr_room;

    uint8_t player_id;

    uint32_t color;

    bool from_player;
    Vector3f accel_vector;

    int cluster_stage;

    // Networking
    float lerp_t;
    ProjectileNetLerp server_state_prior;
    ProjectileNetLerp server_state_target;

} Projectile;

extern Projectile projectiles[MAX_PROJECTILES];
extern Projectile prior_projectiles[MAX_PROJECTILES];
extern ProjectileDef projectile_lookup[];
extern ProjectileSpawn projectile_spawn[];
extern glist* plist;

void projectile_init();
void projectile_clear_all();
void projectile_add(Physics* phys, uint8_t curr_room, ProjectileDef* def, ProjectileSpawn* spawn, uint32_t color, float angle_deg, bool from_player);
void projectile_drop(Vector3f pos, uint8_t curr_room, ProjectileDef* def, ProjectileSpawn* spawn, uint32_t color, bool from_player);
void projectile_update_hit_box(Projectile* proj);
void projectile_update_all(float dt);
void projectile_kill(Projectile* proj);
void projectile_handle_collision(Projectile* p, Entity* e);
void projectile_draw(Projectile* proj);
void projectile_lerp(Projectile* p, double dt);
const char* projectile_def_get_name(ProjectileType proj_type);
