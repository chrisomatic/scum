#pragma once

#include "player.h"
#include "glist.h"
#include "net.h"

#define MAX_PROJECTILES 256

typedef enum
{
    PROJECTILE_TYPE_LASER,
    PROJECTILE_TYPE_MAX
} ProjectileType;

typedef struct
{
    float damage;
    float min_speed;
    float base_speed;
} ProjectileDef;

typedef struct
{
    uint16_t id;

    ProjectileType type;
    Vector2f pos;
    Vector2f vel;

    Rect hit_box;
    Rect hit_box_prior;
    float angle_deg;

    // Player* shooter;
    uint8_t player_id;

    float damage;
    float time;
    float ttl;
    bool dead;
    
    // Networking
    float lerp_t;
    ObjectState server_state_prior;
    ObjectState server_state_target;
    
} Projectile;

extern Projectile projectiles[MAX_PROJECTILES];
extern ProjectileDef projectile_lookup[];
extern glist* plist;

void projectile_init();
void projectile_clear_all();
void projectile_add(Player* p, float angle_deg);
void projectile_update_hit_box(Projectile* proj);
void projectile_update(float delta_t);
void projectile_handle_collisions(float delta_t);
void projectile_draw(Projectile* proj);
void projectile_lerp(Projectile* p, double dt);
