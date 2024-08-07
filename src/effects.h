#pragma once

#include "particles.h"

typedef enum
{
    EFFECT_GUN_SMOKE1,
    EFFECT_SPARKS1,
    EFFECT_HEAL1,
    EFFECT_HEAL2,
    EFFECT_HOLY1,
    EFFECT_BLOOD1,
    EFFECT_BLOOD2,
    EFFECT_DEBRIS1,
    EFFECT_MELEE1,
    EFFECT_BULLET_CASING,
    EFFECT_FIRE,
    EFFECT_BLOCK_DESTROY,
    EFFECT_SMOKE,
    EFFECT_SMOKE2,
    EFFECT_SPLASH,
    EFFECT_BULLET_TRAIL,
    EFFECT_GUN_BLAST,
    EFFECT_LEVEL_UP,
    EFFECT_JETS,
    EFFECT_EXPLOSION,
    EFFECT_PLAYER_ACTIVATE,
    EFFECT_SHRINE,
    EFFECT_DISSIPATE,
    EFFECT_MAX,
} Effect;

typedef struct
{
    int effect_index;
    char* file_name;
} EffectEntry;

extern ParticleEffect particle_effects[EFFECT_MAX];
extern int num_effects;

void effects_load_all();
bool effects_save(char* file_path, ParticleEffect* effect);
bool effects_load(char* file_path, ParticleEffect* effect);
