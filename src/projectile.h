#pragma once

#include "player.h"
#include "glist.h"
#include "physics.h"
#include "net.h"

#define MAX_PROJECTILES 4096

typedef enum
{
    PROJECTILE_TYPE_LASER,
    PROJECTILE_TYPE_CREATURE_GENERIC,
    PROJECTILE_TYPE_CREATURE_CLINGER,
    PROJECTILE_TYPE_MAX
} ProjectileType;

typedef struct
{
    float damage;
    float min_speed;
    float base_speed;

    float angle_spread;
    float scale;
    uint8_t num;

    bool charge;
    uint8_t charge_rate;

    bool ghost;
    bool explosive;
    bool homing;
    bool bouncy;
    bool penetrate;

    bool poison;
    bool cold;

} ProjectileDef;

typedef struct
{
    uint16_t id;

    ProjectileType type;
    Physics phys;

    Rect hit_box;
    Rect hit_box_prior;
    float radius;
    float angle_deg;
    uint8_t curr_room;

    uint8_t player_id;

    float scale;
    float damage;
    float time;
    float ttl;
    bool from_player;

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
void projectile_add(Physics* phys, uint8_t curr_room, ProjectileType proj_type, float angle_deg, float scale, float damage_multiplier, bool from_player);
void projectile_update_hit_box(Projectile* proj);
void projectile_update(float delta_t);
void projectile_handle_collision(Projectile* p, Entity* e);
void projectile_draw(Projectile* proj, bool batch);
void projectile_lerp(Projectile* p, double dt);
