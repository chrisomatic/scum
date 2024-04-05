#include "headers.h"
#include "main.h"
#include "player.h"
#include "skills.h"

int skills_image = -1;

int lookup_mp_cost[SKILL_TYPE_MAX][3] = {
    // {0,   0,  0}, // NONE
    {5,  10, 20}, // MAGIC MISSLE
    {10, 15, 30}, // ROCK SHOWER
    {20, 35, 50}, // SENTIENCE
    {10, 25, 40}, // MULTISHOT
    {5,  15, 25}, // PHASE SHOT
    {5,  15, 25}, // HEAL
    {5,  15, 25}, // DEFLECTOR
    {5,  15, 25}, // CROWN OF THORNS
    {5,  15, 25}, // FEAR
    {5,  15, 25}, // PHASE SHIFT
    {5,  15, 25}, // RABBITS FOOT
    {5,  15, 25}, // PORCUPINE
    {5,  15, 25}, // RESURRECTION
    {5,  15, 25}, // RAISE GOLEM
    {5,  15, 25}, // INVISIBILITY
    {5,  15, 25}, // HOLOGRAM
};

float lookup_cooldown[SKILL_TYPE_MAX][3] = {
    // {0.0, 0.0, 0.0}, // NONE
    {0.0, 0.0, 0.0}, // MAGIC MISSLE
    {0.0, 0.0, 0.0}, // ROCK SHOWER
    {0.0, 0.0, 0.0}, // SENTIENCE
    {0.0, 0.0, 0.0}, // MULTISHOT
    {0.0, 0.0, 0.0}, // PHASE SHOT
    {0.0, 0.0, 0.0}, // HEAL
    {0.0, 0.0, 0.0}, // DEFLECTOR
    {0.0, 0.0, 0.0}, // CROWN OF THORNS
    {0.0, 0.0, 0.0}, // FEAR
    {0.0, 0.0, 0.0}, // PHASE SHIFT
    {0.0, 0.0, 0.0}, // RABBITS FOOT
    {0.0, 0.0, 0.0}, // PORCUPINE
    {0.0, 0.0, 0.0}, // RESURRECTION
    {0.0, 0.0, 0.0}, // RAISE GOLEM
    {0.0, 0.0, 0.0}, // INVISIBILITY
    {0.0, 0.0, 0.0}, // HOLOGRAM
};

void skills_init()
{
    skills_image = gfx_load_image("src/img/skill_icons.png", false, false, 16, 16);
}

bool skills_use(void* player, Skill* skill)
{

    Player* p = (Player*)player;

    if(p->phys.mp < skill->mp_cost)
        return false;

    p->phys.mp -= skill->mp_cost;

    switch(skill->type)
    {
        case SKILL_TYPE_MULTI_SHOT:
        {
            ProjectileDef def = projectile_lookup[PROJECTILE_TYPE_PLAYER];
            ProjectileSpawn spawn = projectile_spawn[PROJECTILE_TYPE_PLAYER];

            def.scale += 0.20;
            spawn.num = 2 + skill->rank;
            spawn.spread = 30.0;

            projectile_add(&p->phys, p->curr_room, &def, &spawn, COLOR_WHITE, p->aim_deg, true);
            
        } break;
    }

    return true;

}

bool skills_add_skill(void* player, SkillType type)
{
    Player* p = (Player*)player;

    int n = p->skill_count;

    bool has_skill = false;
    int skill_index = -1;

    for(int i = 0; i < n; ++i)
    {
        if(p->skills[i].type == type)
        {
            has_skill = true;
            skill_index = i;
            break;
        }
    }
    
    if(has_skill)
    {
        Skill* s = &p->skills[skill_index];

        if(s->rank >= 3)
            return false;

        s->rank++;
        s->mp_cost  = lookup_mp_cost[type][s->rank-1];
        s->cooldown = lookup_cooldown[type][s->rank-1];

        return true;
    }
    
    if(n >= PLAYER_MAX_SKILLS)
        return false;

    // new skill
    p->skills[n].type = type;
    p->skills[n].rank = 1;
    p->skills[n].mp_cost  = lookup_mp_cost[type][0];
    p->skills[n].cooldown = lookup_cooldown[type][0];
    p->skill_count++;

    return true;
}

const char* skills_get_name(SkillType type)
{
    switch(type)
    {
        case SKILL_TYPE_MAGIC_MISSILE:   return "Magic Missile";
        case SKILL_TYPE_ROCK_SHOWER:     return "Rock Shower";
        case SKILL_TYPE_SENTIENCE:       return "Sentience";
        case SKILL_TYPE_MULTI_SHOT:      return "Multi-shot";
        case SKILL_TYPE_PHASE_SHOT:      return "Phase-shot";
        case SKILL_TYPE_HEAL:            return "Heal";
        case SKILL_TYPE_DEFLECTOR:       return "Deflect";
        case SKILL_TYPE_PHASE_SHIFT:     return "Phase Shift";
        case SKILL_TYPE_CROWN_OF_THORNS: return "Crown of Thorns";
        case SKILL_TYPE_RABBITS_FOOT:    return "Rabbit's Foot";
        case SKILL_TYPE_PORCUPINE:       return "Porcupine";
        case SKILL_TYPE_RESURRECTION:    return "Resurrection";
        case SKILL_TYPE_RAISE_GOLEM:     return "Raise Golem";
        case SKILL_TYPE_FEAR:            return "Fear";
        case SKILL_TYPE_INVISIBILITY:    return "Invisibility";
        case SKILL_TYPE_HOLOGRAM:        return "Hologram";
    }
    return "???";
}