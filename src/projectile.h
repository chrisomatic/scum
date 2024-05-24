#pragma once

#include "glist.h"
#include "physics.h"
#include "net.h"
#include "entity.h"

#define MAX_ORBITALS 32
#define MAX_PROJECTILES 4096
#define MAX_CLUSTER_STAGES 2
#define MAX_GUNS 100
#define GUN_NAME_MAX_LEN 50 
#define GUN_DESC_MAX_LEN 100

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

typedef enum
{
    SPREAD_TYPE_RANDOM = 0,
    SPREAD_TYPE_UNIFORM,
    SPREAD_TYPE_COUNT,
} SpreadType;

typedef enum
{
    CHARGE_TYPE_SCALE_DAMAGE,
    CHARGE_TYPE_SCALE_BURST_COUNT,
    CHARGE_TYPE_COUNT,
} ChargeType;

typedef struct
{
    char  name[GUN_NAME_MAX_LEN+1];
    char  desc[GUN_DESC_MAX_LEN+1];
    float damage_min;
    float damage_max;
    
    float cooldown;
    float speed;
    float scale1;
    float scale2;
    float lifetime;

    float explosion_chance;
    float bounce_chance;
    float penetration_chance;

    float explosion_radius;
    float explosion_rate;

    float gravity_factor;
    float directional_accel;
    float air_friction;

    float knockback_factor;
    float critical_hit_chance;

    float wave_amplitude;
    float wave_period;

    float spin_factor;

    int sprite_index;

    uint32_t color1;
    uint32_t color2;

    // cluster
    bool cluster;
    int cluster_stages;
    int cluster_num[MAX_CLUSTER_STAGES];
    float cluster_scales[MAX_CLUSTER_STAGES];
    int cluster_stage;

    // orbital
    bool  orbital;
    float orbital_distance;
    float orbital_speed_factor;
    int   orbital_max_count;

    // charge
    bool charging;
    bool chargeable;
    float charge_time;
    float charge_time_max;
    ChargeType charge_type;

    // spawn properties
    int num;

    SpreadType spread_type;
    float spread;

    float ghost_chance;
    float homing_chance;

    // elemental
    float fire_damage;
    float cold_damage;
    float lightning_damage;
    float poison_damage;

    // bursting
    int burst_count;
    float burst_rate;

} Gun;

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
    int max_count;
    float base_angle;  // used for rotation of projectiles
    float lerp_t;
    float lerp_factor;
    OrbEvolution evolution;
} ProjectileOrbital;

typedef struct
{
    Vector3f pos;
} ProjectileNetLerp;

typedef struct
{
    float tts;
    Physics* shooter;

    uint16_t id;

    Gun gun;
    Physics phys;

    float ttl;

    // effects
    bool fire;
    bool cold;
    bool lightning;
    bool poison;

    float radius;
    float angle_deg;
    float wave_time;
    float charge_time;

    int  wave_dir;
    uint8_t curr_room;

    uint8_t player_id;

    uint8_t sprite_index;
    uint32_t color;

    bool from_player;
    int cluster_stage;

    ProjectileOrbital* orbital;
    int orbital_index;

    Vector2f orbital_pos;
    Vector2f orbital_pos_prior;
    Vector2f orbital_pos_target;

    // Audio
    int source_explode;

    // Networking
    float lerp_t;
    ProjectileNetLerp server_state_prior;
    ProjectileNetLerp server_state_target;

} Projectile;

extern Projectile projectiles[MAX_PROJECTILES];
extern Projectile prior_projectiles[MAX_PROJECTILES];
extern Gun gun_list[];
extern int gun_list_count;
extern glist* plist;

void projectile_init();
void projectile_clear_all();

void projectile_add(Vector3f pos, Vector3f* vel, uint8_t curr_room, Gun* gun, uint32_t color, float angle_deg, bool from_player, Physics* phys);

void projectile_fire(Physics* phys, uint8_t curr_room, Gun* gun, uint32_t color, float angle_deg, bool from_player);

void projectile_lob(Physics* phys, float vel0_z, uint8_t curr_room, Gun* gun, uint32_t color, float angle_deg, bool from_player);

void projectile_update_hit_box(Projectile* proj);
void projectile_update_all(float dt);
void projectile_kill(Projectile* proj);
void projectile_handle_collision(Projectile* p, Entity* e);
void projectile_draw(Projectile* proj);
void projectile_lerp(Projectile* p, double dt);
const char* projectile_def_get_name(ProjectileType proj_type);
const char* projectile_spread_type_get_name(SpreadType spread_type);
const char* projectile_charge_type_get_name(ChargeType charge_type);

ProjectileOrbital* projectile_orbital_get(Physics* body, float distance);
void projectile_orbital_kill(ProjectileOrbital* orb);

void gun_save_to_file(Gun* gun);
void gun_load_from_file(Gun* gun, const char* file_name);
void gun_refresh_list();
void gun_print(Gun* gun);
bool gun_get_by_name(char* gun_name, Gun* gun);
