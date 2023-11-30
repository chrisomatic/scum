#include "headers.h"
#include "main.h"
#include "projectile.h"
#include "player.h"
#include "skills.h"

Skill skill_list[SKILL_LIST_MAX] = {0};
int skill_list_count = 0;

static void skills_debug(void* skill, void* player, float dt);
static void skills_rabbits_foot(void* skill, void* player, float dt);
static void skills_steel_boots(void* skill, void* player, float dt);
static void skills_crown_of_thorns(void* skill, void* player, float dt);
static void skills_kinetic_discharge(void* skill, void* player, float dt);
static void skills_phase_shift(void* skill, void* player, float dt);
static void skills_sentience(void* skill, void* player, float dt);
static void skills_restoration(void* skill, void* player, float dt);
static void skills_health_boost(void* skill, void* player, float dt);

void skills_init()
{
    int i = skill_list_count;

    skill_list[i].type = SKILL_TYPE_DEBUG;
    skill_list[i].func = skills_debug;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_DEBUG;
    skill_list[i].rank = 1;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"DEBUG");
    strcpy(skill_list[i].desc,"good stuff");
    i++;

    skill_list[i].type = SKILL_TYPE_KINETIC_DISCHARGE;
    skill_list[i].func = skills_kinetic_discharge;
    skill_list[i].min_level = 2;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 1;
    skill_list[i].periodic = true;
    skill_list[i].periodic_max_time = 2.0;
    strcpy(skill_list[i].name,"Kinetic Discharge I");
    strcpy(skill_list[i].desc,"Periodically fire an energy ball");
    i++;

    skill_list[i].type = SKILL_TYPE_KINETIC_DISCHARGE;
    skill_list[i].func = skills_kinetic_discharge;
    skill_list[i].min_level = 3;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 2;
    skill_list[i].periodic = true;
    skill_list[i].periodic_max_time = 1.0;
    strcpy(skill_list[i].name,"Kinetic Discharge II");
    strcpy(skill_list[i].desc,"Periodically fire several energy balls");
    i++;

    skill_list[i].type = SKILL_TYPE_RABBITS_FOOT;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_rabbits_foot;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Rabbit's Foot I");
    strcpy(skill_list[i].desc,"Increase movement speed by 1");
    i++;

    skill_list[i].type = SKILL_TYPE_RABBITS_FOOT;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 2;
    skill_list[i].func = skills_rabbits_foot;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Rabbit's Foot II");
    strcpy(skill_list[i].desc,"Increase movement speed by 1");
    i++;

    skill_list[i].type = SKILL_TYPE_STEEL_BOOTS;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_steel_boots;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Steel Boots");
    strcpy(skill_list[i].desc,"Increase friction");
    i++;

    skill_list[i].type = SKILL_TYPE_CROWN_OF_THORNS;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_crown_of_thorns;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Crown of Thorns");
    strcpy(skill_list[i].desc,"Ability to float (-2 hearts)");
    i++;

    skill_list[i].type = SKILL_TYPE_PHASE_SHIFT;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_phase_shift;
    skill_list[i].periodic = true;
    strcpy(skill_list[i].name,"Phase Shift I");
    strcpy(skill_list[i].desc,"10% chance of ghost shot");
    i++;

    skill_list[i].type = SKILL_TYPE_PHASE_SHIFT;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 2;
    skill_list[i].func = skills_phase_shift;
    skill_list[i].periodic = true;
    strcpy(skill_list[i].name,"Phase Shift II");
    strcpy(skill_list[i].desc,"25% chance of ghost shot");
    i++;

    skill_list[i].type = SKILL_TYPE_SENTIENCE;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_COMMON;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_sentience;
    skill_list[i].periodic = true;
    strcpy(skill_list[i].name,"Sentience I");
    strcpy(skill_list[i].desc,"10% chance of homing shot");
    i++;

    skill_list[i].type = SKILL_TYPE_SENTIENCE;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_LEGENDARY;
    skill_list[i].rank = 2;
    skill_list[i].func = skills_sentience;
    skill_list[i].periodic = true;
    strcpy(skill_list[i].name,"Sentience II");
    strcpy(skill_list[i].desc,"25% chance of homing shot");
    i++;

    skill_list[i].type = SKILL_TYPE_RESTORATION;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_LEGENDARY;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_restoration;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Restoration");
    strcpy(skill_list[i].desc,"Fully restore all hearts");
    i++;

    skill_list[i].type = SKILL_TYPE_HEALTH_BOOST;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_LEGENDARY;
    skill_list[i].rank = 1;
    skill_list[i].func = skills_health_boost;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Health Boost I");
    strcpy(skill_list[i].desc,"Increase max health");
    i++;

    skill_list[i].type = SKILL_TYPE_HEALTH_BOOST;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_LEGENDARY;
    skill_list[i].rank = 2;
    skill_list[i].func = skills_health_boost;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Health Boost II");
    strcpy(skill_list[i].desc,"Increase max health");
    i++;

    skill_list[i].type = SKILL_TYPE_HEALTH_BOOST;
    skill_list[i].min_level = 1;
    skill_list[i].rarity = SKILL_RARITY_LEGENDARY;
    skill_list[i].rank = 3;
    skill_list[i].func = skills_health_boost;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Health Boost III");
    strcpy(skill_list[i].desc,"Increase max health");
    i++;

    skill_list_count = i;
}

const char* skill_rarity_str(SkillRarity rarity)
{
    switch(rarity)
    {
        case SKILL_RARITY_COMMON: return "common";
        case SKILL_RARITY_RARE: return "rare";
        case SKILL_RARITY_EPIC: return "epic";
        case SKILL_RARITY_LEGENDARY: return "legendary";
    }
    return "";
}

int skill_rarity_weight(SkillRarity rarity)
{
    if(rarity == SKILL_RARITY_DEBUG)
        return 100;
    return SKILL_RARITY_MAX - rarity;
}


// ===============================
// Skills
// ==============================

static void skills_debug(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    p->phys.speed += 300.0;
    p->phys.max_velocity += 90.0;
    p->proj_cooldown_max = 0.05;
    p->proj_def.num += 4;
    p->proj_def.ghost_chance += 1.0f;
    p->proj_def.damage += 9.0;
    p->phys.hp_max += 10;
    p->phys.hp = p->phys.hp_max;
}


static void skills_rabbits_foot(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;

    p->phys.speed += 100.0;
    p->phys.max_velocity += 30.0;
}

static void skills_steel_boots(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    p->phys.base_friction *= 2.0;
}

static void skills_crown_of_thorns(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;

    p->phys.base_friction /= 2.0;
    p->phys.floating = true;
    p->phys.pos.z = 10.0;

    p->phys.hp_max -= 4;
    if(p->phys.hp > p->phys.hp_max)
        p->phys.hp = p->phys.hp_max;
}

static void skills_kinetic_discharge(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    Skill* s = (Skill*)skill;

    p->periodic_shot_counter += dt;

    if(p->periodic_shot_counter >= s->periodic_max_time)
    {
        p->periodic_shot_counter = 0.0;

        // pick angle
        float angle_deg = rand() % 360;

        memcpy(&p->proj_discharge,&projectile_lookup[PROJECTILE_TYPE_PLAYER],sizeof(ProjectileDef));
        p->proj_discharge.scale = 0.5;
        p->proj_discharge.color = COLOR_WHITE;
        
        // discharge
        if(s->rank == 1)
        {
            p->proj_discharge.damage *= 0.25;
            projectile_add(&p->phys, p->curr_room, &p->proj_discharge, angle_deg, 1.0, 1.0, true);
        }
        else if(s->rank == 2)
        {
            p->proj_discharge.damage *= 0.50;
            projectile_add(&p->phys, p->curr_room, &p->proj_discharge, angle_deg, 1.0, 1.0, true);
            projectile_add(&p->phys, p->curr_room, &p->proj_discharge, angle_deg+90, 1.0, 1.0, true);
        }
    }
}

static void skills_phase_shift(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    Skill* s = (Skill*)skill;

    if(s->rank == 1)
    {
        p->proj_def.ghost_chance += 0.10f;
    }
    else if(s->rank == 2)
    {
        p->proj_def.ghost_chance += 0.15f;
    }
}

static void skills_sentience(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    Skill* s = (Skill*)skill;

    if(s->rank == 1)
    {
        p->proj_def.homing_chance += 0.10f;
    }
    else if(s->rank == 2)
    {
        p->proj_def.homing_chance += 0.15f;
    }
}

static void skills_restoration(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    p->phys.hp = p->phys.hp_max;
}

static void skills_health_boost(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;
    p->phys.hp_max += 2;
    p->phys.hp += 2;
}

