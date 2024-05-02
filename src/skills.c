#include "headers.h"
#include "main.h"
#include "player.h"
#include "skills.h"
#include "creature.h"

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
    {2.0, 2.0, 2.0}, // CROWN OF THORNS
    {0.0, 0.0, 0.0}, // FEAR
    {0.0, 0.0, 0.0}, // PHASE SHIFT
    {2.0, 2.0, 2.0}, // RABBITS FOOT
    {0.0, 0.0, 0.0}, // PORCUPINE
    {0.0, 0.0, 0.0}, // RESURRECTION
    {10.0, 10.0, 10.0}, // RAISE GOLEM
    {0.0, 0.0, 0.0}, // INVISIBILITY
    {0.0, 0.0, 0.0}, // HOLOGRAM
};

void skills_init()
{
    skills_image = gfx_load_image("src/img/skill_icons.png", false, false, 16, 16);
}

bool skills_can_use(void* player, Skill* skill)
{
    if(!player || !skill)
        return false;

    Player* p = (Player*)player;
    return (p->phys.mp >= skill->mp_cost);
}

void skills_deactivate(void* player, Skill* skill)
{
    Player* p = (Player*)player;
    switch(skill->type)
    {
        case SKILL_TYPE_CROWN_OF_THORNS:
        {
            p->skill_mods.floating = false;
        } break;
        case SKILL_TYPE_RABBITS_FOOT:
        {
            p->skill_mods.speed -= skill->delta1;
            p->skill_mods.max_velocity -= skill->delta2;
            p->skill_mods.base_friction -= skill->delta3;
        } break;
    }

    skill->cooldown_timer = skill->cooldown;
}

void skills_update_timers(void* player, float dt)
{
    Player* p = (Player*)player;

    for(int i = 0; i < PLAYER_MAX_SKILLS; ++i)
    {
        Skill* s = &p->skills[i];
        if(s->cooldown_timer > 0)
        {
            s->cooldown_timer -= dt;
            s->cooldown_timer = MAX(0, s->cooldown_timer);
        }
        if(s->duration_timer > 0)
        {
            s->duration_timer -= dt;
            s->duration_timer = MAX(0, s->duration_timer);
            if(s->duration_timer == 0)
            {
                skills_deactivate(player, s);
            }
        }
    }

}


bool skills_use(void* player, Skill* skill)
{
    if(!skills_can_use(player, skill))
        return false;

    if(skill->cooldown_timer > 0 || skill->duration_timer > 0)
        return false;

    Player* p = (Player*)player;
    // p->phys.mp -= skill->mp_cost;

    ProjectileDef def = projectile_lookup[PROJECTILE_TYPE_PLAYER];
    ProjectileSpawn spawn = projectile_spawn[PROJECTILE_TYPE_PLAYER];

    bool used = true;

    switch(skill->type)
    {
        case SKILL_TYPE_MAGIC_MISSILE:
        {
            def.scale += 0.20;
            def.damage *= (1.0+skill->rank);
            def.speed += (200.0*skill->rank);

            spawn.num = 1;
            spawn.spread = 0.0;

            projectile_add(&p->phys, p->phys.curr_room, &def, &spawn, 0x0000CCFF, p->aim_deg, true);

        } break;

        case SKILL_TYPE_ROCK_SHOWER:
        {
            def.scale += 1.00;

            def.cluster = true;
            def.cluster_stages = 1;
            def.cluster_num[0] = 4;
            def.cluster_scales[0] = 0.8;

            Room* room = level_get_room_by_index(&level, p->phys.curr_room);

            float r = TILE_SIZE/2.0;

            int num_rocks = 20*skill->rank;
            for(int i = 0; i < num_rocks; ++i)
            {
                Vector2i rand_tile;
                Vector2f rand_pos;

                float xo = RAND_FLOAT(0, r) * RAND_PN();
                float yo = RAND_FLOAT(0, r) * RAND_PN();

                level_get_rand_floor_tile(room, &rand_tile, &rand_pos);

                Rect r = level_get_tile_rect(rand_tile.x, rand_tile.y);
                Vector3f pos = {r.x+xo, r.y+yo, 400.0};

                projectile_drop(pos, 0.0, p->phys.curr_room, &def, &spawn, 0x00553300, true);
            }

        } break;
        case SKILL_TYPE_SENTIENCE:
        {
            def.scale += 0.00;
            def.damage *= (1.0+skill->rank);
            spawn.num = 3 + (1*skill->rank);
            spawn.spread = 0.0;
            spawn.homing_chance = 1.0;

            projectile_add(&p->phys, p->phys.curr_room, &def, &spawn, 0x00555555, p->aim_deg, true);
        } break;

        case SKILL_TYPE_MULTI_SHOT:
        {
            def.scale += 0.20;
            spawn.num = 2 + skill->rank;
            spawn.spread = 30.0 + 10*skill->rank;

            projectile_add(&p->phys, p->phys.curr_room, &def, &spawn, COLOR_WHITE, p->aim_deg, true);
            
        } break;

        case SKILL_TYPE_PHASE_SHOT:
        {
            def.scale += 0.10;
            def.damage *= (0.5+skill->rank);

            spawn.num = 2 + skill->rank;
            spawn.spread = 60.0;
            spawn.ghost_chance = 1.0;

            projectile_add(&p->phys, p->phys.curr_room, &def, &spawn, 0x00808080, p->aim_deg, true);

        } break;
        case SKILL_TYPE_HEAL:
        {
            used = player_add_hp(p, (2*skill->rank));
        } break;
        case SKILL_TYPE_DEFLECTOR:
        {
            used = false;
        } break;
        case SKILL_TYPE_CROWN_OF_THORNS:
        {
            p->skill_mods.floating = true;
            skill->duration = 3.0;
            skill->duration_timer = skill->duration;
        } break;
        case SKILL_TYPE_FEAR:
        {
            used = false;
        } break;
        case SKILL_TYPE_PHASE_SHIFT:
        {
            used = false;
        } break;
        case SKILL_TYPE_RABBITS_FOOT:
        {
            skill->delta1 = 100.0 * skill->rank;
            skill->delta2 = 50.0 * skill->rank;
            skill->delta3 = 4.0 * skill->rank;
            p->skill_mods.speed += skill->delta1;
            p->skill_mods.max_velocity += skill->delta2;
            p->skill_mods.base_friction += skill->delta3;
            skill->duration = 5.0;
            skill->duration_timer = skill->duration;
        } break;
        case SKILL_TYPE_PORCUPINE:
        {
            used = false;
        } break;
        case SKILL_TYPE_RESURRECTION:
        {
            used = false;
        } break;
        case SKILL_TYPE_RAISE_GOLEM:
        {
            Room* room = level.rooms_ptr[p->phys.curr_room];
            Vector2i tile = p->last_safe_tile;
            creature_add(room, CREATURE_TYPE_GOLEM, &tile, NULL);
            skill->cooldown_timer = skill->cooldown;
            used = true;
        } break;
        case SKILL_TYPE_INVISIBILITY:
        {
            used = false;
        } break;
        case SKILL_TYPE_HOLOGRAM:
        {
            used = false;
        } break;
    }

    if(used)
    {
        p->phys.mp -= skill->mp_cost;
    }

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

const char* skills_rank_str(int val)
{
    if(val == 1) return "I";
    if(val == 2) return "II";
    if(val == 3) return "III";
    return "";
}
