#pragma once

#define MAX_WEAPON_HITS 32

typedef enum
{
    WEAPON_TYPE_NONE,
    WEAPON_TYPE_SPEAR,
} WeaponType;

typedef enum
{
    WEAPON_STATE_NONE,
    WEAPON_STATE_WINDUP,
    WEAPON_STATE_RELEASE,
    WEAPON_STATE_RETRACT,
} WeaponState;

typedef struct
{
    uint16_t id;
    WeaponType type;
    WeaponState state;
    Physics* phys; // pointer to whatever physics it is attached to
    Vector3f pos;
    float rotation_deg;
    int image;
    float scale;
    float damage;
    float time;
    float windup_time;
    float release_time;
    float retract_time;
    uint32_t color;
    uint16_t hit_ids[MAX_WEAPON_HITS];
    uint8_t  hit_id_count;
} Weapon;

void weapon_init();
void weapon_add(WeaponType type, Physics* phys, Weapon* w, bool _new);
void weapon_update(Weapon* w, float dt);
void weapon_draw(Weapon* w);

// hit list management
bool weapon_is_in_hit_list(Weapon* w, uint16_t id);
void weapon_add_hit_id(Weapon* w, uint16_t id);
void weapon_clear_hit_list(Weapon* w);
