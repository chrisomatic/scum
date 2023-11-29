#pragma once

#define SKILL_LIST_MAX 20

typedef enum
{
    SKILL_TYPE_KINETIC_DISCHARGE,
    SKILL_TYPE_RABBITS_FOOT,
    SKILL_TYPE_STEEL_BOOTS,
    SKILL_TYPE_CROWN_OF_THORNS,
    SKILL_TYPE_PHASE_SHIFT,
    SKILL_TYPE_SENTIENCE,
    SKILL_TYPE_RESTORATION,
    SKILL_TYPE_MAX
} SkillType;

typedef void (*skill_func)(void* skill, void* player, float dt);

typedef struct
{
    SkillType type;
    skill_func func;

    int rank;
    int min_level;

    bool periodic;
    float periodic_max_time;
    char name[32];
    char desc[100];
} Skill;

extern Skill skill_list[SKILL_LIST_MAX];
extern int skill_list_count;

void skills_init();
