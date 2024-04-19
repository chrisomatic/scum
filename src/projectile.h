#pragma once

#include "glist.h"
#include "physics.h"
#include "net.h"
#include "entity.h"

#define MAX_ORBITALS 32
#define MAX_PROJECTILES 4096

typedef enum
{
    PROJECTILE_TYPE_PLAYER,
    PROJECTILE_TYPE_PLAYER_KINETIC_DISCHARGE,
    PROJECTILE_TYPE_CREATURE_GENERIC,
    PROJECTILE_TYPE_CREATURE_GEIZER,
    PROJECTILE_TYPE_CREATURE_CLINGER,
    PROJECTILE_TYPE_CREATURE_TOTEM_BLUE,
    PROJECTILE_TYPE_CREATURE_WATCHER,
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

    // cluster
    bool cluster;
    int cluster_stages;
    int cluster_num[3];
    float cluster_scales[3];

    // orbital
    bool is_orbital;
    float orbital_distance;
    float orbital_speed_factor;

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


typedef enum
{
    PROJ_ORB_EVOLUTION_NONE = 0,
    PROJ_ORB_EVOLUTION_GROW_SHRINK,
} OrbEvolution;

typedef struct
{
    Physics* body;  // origin of orbital
    float distance; // radius of orbital
    float speed_factor; // speed of rotation, negative spins opposite way. 1.0 is 1 spin/6 seconds
    int count;
    float base_angle;  // used for rotation of projectiles
    float lerp_t;
    OrbEvolution evolution;
} ProjectileOrbital;

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

    ProjectileOrbital* orbital;
    int orbital_index;

    Vector2f orbital_pos;
    Vector2f orbital_pos_prior;
    Vector2f orbital_pos_target;

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
void projectile_drop(Vector3f pos, float vel0_z, uint8_t curr_room, ProjectileDef* def, ProjectileSpawn* spawn, uint32_t color, bool from_player);
void projectile_update_hit_box(Projectile* proj);
void projectile_update_all(float dt);
void projectile_kill(Projectile* proj);
void projectile_handle_collision(Projectile* p, Entity* e);
void projectile_draw(Projectile* proj);
void projectile_lerp(Projectile* p, double dt);
const char* projectile_def_get_name(ProjectileType proj_type);
