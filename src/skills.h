#pragma once

typedef enum
{
    SKILL_TYPE_NONE,
    SKILL_TYPE_MAGIC_MISSILE,
    SKILL_TYPE_ROCK_SHOWER,
    SKILL_TYPE_SENTIENCE,
    SKILL_TYPE_MULTI_SHOT,
    SKILL_TYPE_PHASE_SHOT,
    SKILL_TYPE_HEAL,
    SKILL_TYPE_DEFLECTOR,
    SKILL_TYPE_PHASE_SHIFT,
    SKILL_TYPE_CROWN_OF_THORNS,
    SKILL_TYPE_RABBITS_FOOT,
    SKILL_TYPE_PORCUPINE,
    SKILL_TYPE_RESURRECTION,
    SKILL_TYPE_RAISE_GOLEM,
    SKILL_TYPE_FEAR,
    SKILL_TYPE_INVISIBILITY,
    SKILL_TYPE_HOLOGRAM,
    SKILL_TYPE_MAX,
} SkillType;

typedef struct
{
    SkillType type;
    int rank;
    int mp_cost;
    float cooldown;
    int slot;
} Skill;

void skills_init();
bool skills_add_skill(void* player, SkillType type);
