#include "log.h"
#include "player.h"
#include "creature.h"
#include "main.h"

#include "status_effects.h"

static void status_effect_cold(void* ent, bool end)
{
    
    Entity* e = (Entity*)ent;
    e->phys->se_speed_factor = end ? 1.0 : 0.5;
}

static void status_effect_poison(void* ent, bool end)
{
    Entity* e = (Entity*)ent;

    if(!end && e->phys->hp > 1)
    {
        switch(e->type)
        {
            case ENTITY_TYPE_PLAYER:
                player_hurt((Player*)e->ptr,-1);
                break;
            case ENTITY_TYPE_CREATURE:
                creature_hurt((Creature*)e->ptr,-1);
                break;
        }
    }
}

void status_effects_add_type(Physics* phys, StatusEffectType type)
{
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
            effect->period = 1.0;
            effect->lifetime_max = 10.0;
            effect->func = status_effect_poison;
            break;
    }
}
