#pragma once

#include "glist.h"
#include "physics.h"
#include "net.h"
#include "entity.h"

#define MAX_PROJECTILES 4096

typedef enum
{
    PROJECTILE_TYPE_PLAYER,
    PROJECTILE_TYPE_CREATURE_GENERIC,
    PROJECTILE_TYPE_CREATURE_CLINGER,
    PROJECTILE_TYPE_MAX
} ProjectileType;

typedef struct
{
    uint32_t color;

    float damage;
    float range;
    float min_speed;
    float base_speed;

    float angle_spread;
    float scale;
    uint8_t num;

    bool charge;
    uint8_t charge_rate;

    bool ghost;
    bool homing;
    bool explosive;
    bool bouncy;
    bool penetrate;

    float ghost_chance;
    float homing_chance;

    // elemental
    bool poison;
    bool cold;

} ProjectileDef;

typedef struct
{
    uint16_t id;

    ProjectileDef* proj_def;
    Physics phys;

    Rect hit_box;
    Rect hit_box_prior;
    float radius;
    float angle_deg;
    uint8_t curr_room;

    uint8_t player_id;

    uint32_t color;

    float scale;
    float damage;
    float time;
    float ttl;
    bool from_player;

    bool homing;
    Physics* homing_target;
    
    // Networking
    float lerp_t;
    ObjectState server_state_prior;
    ObjectState server_state_target;
    
} Projectile;

extern Projectile projectiles[MAX_PROJECTILES];
extern Projectile prior_projectiles[MAX_PROJECTILES];
extern ProjectileDef projectile_lookup[];
extern glist* plist;

void projectile_init();
void projectile_clear_all();
void projectile_add(Physics* phys, uint8_t curr_room, ProjectileDef* projdef, float angle_deg, float scale, float damage_multiplier, bool from_player);
void projectile_add_type(Physics* phys, uint8_t curr_room, ProjectileType proj_type, float angle_deg, float scale, float damage_multiplier, bool from_player);
void projectile_update_hit_box(Projectile* proj);
void projectile_update(float delta_t);
void projectile_handle_collision(Projectile* p, Entity* e);
void projectile_draw(Projectile* proj, bool batch);
void projectile_lerp(Projectile* p, double dt);
const char* projectile_def_get_name(ProjectileType proj_type);
