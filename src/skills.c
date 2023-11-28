#include "headers.h"
#include "main.h"
#include "projectile.h"
#include "player.h"
#include "skills.h"

Skill skill_list[SKILL_LIST_MAX] = {0};
int skill_list_count = 0;

static void skills_rabbits_foot(void* skill, void* player, float dt)
{
    Player* p = (Player*)player;

    p->phys.speed += 300.0;
    p->phys.max_velocity += 90.0;
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

void skills_init()
{
    int i = skill_list_count;

    skill_list[i].type = SKILL_TYPE_KINETIC_DISCHARGE;
    skill_list[i].func = skills_kinetic_discharge;
    skill_list[i].rank = 1;
    skill_list[i].periodic = true;
    skill_list[i].periodic_max_time = 2.0;
    strcpy(skill_list[i].name,"Kinetic Discharge I");
    strcpy(skill_list[i].desc,"Periodically fire an energy ball");
    i++;

    skill_list[i].type = SKILL_TYPE_KINETIC_DISCHARGE;
    skill_list[i].func = skills_kinetic_discharge;
    skill_list[i].rank = 2;
    skill_list[i].periodic = true;
    skill_list[i].periodic_max_time = 1.0;
    strcpy(skill_list[i].name,"Kinetic Discharge II");
    strcpy(skill_list[i].desc,"Periodically fire several energy balls");
    i++;

    skill_list[i].type = SKILL_TYPE_RABBITS_FOOT;
    skill_list[i].rank = 0;
    skill_list[i].func = skills_rabbits_foot;
    skill_list[i].periodic = false;
    strcpy(skill_list[i].name,"Rabbit's Foot");
    strcpy(skill_list[i].desc,"Increase movement speed by 1");
    i++;

    skill_list_count = i;
}
