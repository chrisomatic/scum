#pragma once

typedef enum
{
    STATUS_EFFECT_NONE   = 0,
    STATUS_EFFECT_POISON,
    STATUS_EFFECT_COLD,
    STATUS_EFFECT_FEAR,
} StatusEffectType;

typedef void (*effect_func)(void* entity, bool end);

typedef struct
{
    StatusEffectType type;
    effect_func func;
    float lifetime;
    float lifetime_max;
    bool periodic;
    float period;
    int periods_passed;
    bool applied;
} StatusEffect;

void status_effects_init();
void status_effects_clear(void* physics);
void status_effects_add_type(void* physics, StatusEffectType type);
void status_effects_draw(void* physics);
