#pragma once

#define SKILL_LIST_MAX 64

typedef enum
{
    SKILL_TYPE_KINETIC_DISCHARGE,
    SKILL_TYPE_RABBITS_FOOT,
    SKILL_TYPE_STEEL_BOOTS,
    SKILL_TYPE_CROWN_OF_THORNS,
    SKILL_TYPE_PHASE_SHIFT,
    SKILL_TYPE_SENTIENCE,
    SKILL_TYPE_RESTORATION,
    SKILL_TYPE_HEALTH_BOOST,
    SKILL_TYPE_RAPID_FIRE,
    SKILL_TYPE_MULTISHOT,
    SKILL_TYPE_MORE_CHOICES,

    SKILL_TYPE_MAX
} SkillType;

typedef enum
{
    SKILL_RARITY_COMMON,
    SKILL_RARITY_RARE,
    SKILL_RARITY_EPIC,
    SKILL_RARITY_LEGENDARY,

    SKILL_RARITY_MAX
} SkillRarity;

typedef void (*skill_func)(void* skill, void* player, float dt);

typedef struct
{
    SkillType type;
    skill_func func;
    SkillRarity rarity;

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
const char* skill_rarity_str(SkillRarity rarity);
int skill_rarity_weight(SkillRarity rarity);

