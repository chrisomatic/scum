#pragma once

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
    Physics* phys;
    Vector3f pos;
    int image;
    float damage;
    float time;
    float windup_time;
    float release_time;
    float retract_time;
    uint32_t color;
} Weapon;

void weapon_init();
void weapon_add(WeaponType type, Physics* phys, Weapon* w, bool _new);
void weapon_update(Weapon* w, float dt);
void weapon_draw(Weapon* w);
