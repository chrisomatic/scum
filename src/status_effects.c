#include "log.h"
#include "entity.h"
#include "player.h"
#include "creature.h"
#include "gfx.h"
#include "main.h"

#include "status_effects.h"

static int status_effects_image;

static void status_effect_cold(void* entity, bool end)
{
    Entity* e = (Entity*)entity;

    e->phys->speed_factor = end ? 1.0 : 0.25;
}

static void status_effect_poison(void* entity, bool end)
{
    Entity* e = (Entity*)entity;

    if(!end && e->phys->hp > 1)
    {
        switch(e->type)
        {
            case ENTITY_TYPE_PLAYER:
                player_hurt_no_inv((Player*)e->ptr,1);
                break;
            case ENTITY_TYPE_CREATURE:
                creature_hurt((Creature*)e->ptr,1);
                break;
        }
    }
}

void status_effects_init()
{
    status_effects_image = gfx_load_image("src/img/status_effects.png", false, false, 8, 8);
}

void status_effects_draw(void* physics)
{
    Physics* phys = (Physics*)physics;

    float total_width = phys->status_effects_count*5; // 8 pixels per effect + 1 space
    float half_width  = total_width / 2.0;

    for(int i = 0; i < phys->status_effects_count; ++i)
    {
        StatusEffect* effect = &phys->status_effects[i];

        float x = phys->pos.x - half_width + (total_width*((i+1)/phys->status_effects_count));
        float y = phys->pos.y-(phys->height+8)/2.0 - sin(2.0*effect->lifetime);

        gfx_sprite_batch_add(status_effects_image, effect->type, x, y, COLOR_TINT_NONE, false, 1.0, 0.0, 0.5, false, false, false);
    }
}

void status_effects_clear(void* physics)
{
    Physics* phys = (Physics*)physics;
    phys->status_effects_count = 0;
    memset(&phys->status_effects, 0, sizeof(StatusEffect)*MAX_STATUS_EFFECTS);
}

void status_effects_add_type(void* physics, StatusEffectType type)
{
    Physics* phys = (Physics*)physics;

    if(phys->status_effects_count >= MAX_STATUS_EFFECTS)
    {
        LOGW("Too many status effects");
        return;
    }

    // check if status type already exists on object
    for(int i = 0; i < phys->status_effects_count; ++i)
    {
        StatusEffect* effect = &phys->status_effects[i];

        if(effect->type == type)
        {
            // replace effect
            effect->lifetime = 0.0;
            return;
        }
    }

    // new effect
    StatusEffect* effect = &phys->status_effects[phys->status_effects_count++];

    memset(effect,0,sizeof(StatusEffect));

    effect->type = type;
    effect->lifetime = 0.0;

    switch(type)
    {
        case STATUS_EFFECT_COLD:
            effect->periodic = false;
            effect->period = 0.0;
            effect->lifetime_max = 5.0;
            effect->func = status_effect_cold;
            break;
        case STATUS_EFFECT_POISON:
            effect->periodic = true;
            effect->period = 3.0;
            effect->lifetime_max = 9.0;
            effect->func = status_effect_poison;
            break;
    }
}



