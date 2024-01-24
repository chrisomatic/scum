#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"
#include "entity.h"
#include "player.h"
#include "creature.h"
#include "explosion.h"
#include "lighting.h"
#include "status_effects.h"
#include "projectile.h"
#include "effects.h"

Projectile projectiles[MAX_PROJECTILES];
Projectile prior_projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static int projectile_image;
static uint16_t id_counter = 0;


// .color = 0x003030FF,

ProjectileDef projectile_lookup[] = {
    {
        // player
        .damage = 200.0,
        .speed = 215.0,
        .accel = 0.0,
        .scale = 1.0,
        .ttl = 3.0,
        .explosive = false,
        .bouncy = false,
        .penetrate = false,

        .cluster = false,
        .cluster_stages = 1,
        .cluster_num = {8, 2, 2},
        .cluster_scales = {0.5, 0.5, 0.5},
    },
    {
        // player - kinetic discharge skill
        .damage = 0.25,
        .speed = 200.0,
        .accel = 0.0,
        .scale = 0.5,
        .ttl = 3.0,
        .explosive = false,
        .bouncy = false,
        .penetrate = false,
        .cluster = false,
    },
    {
        // creature generic
        .damage = 1.0,
        .speed = 200.0,
        .accel = 0.0,
        .scale = 0.8,
        .ttl = 3.0,
        .explosive = false,
        .bouncy = false,
        .penetrate = false,
        .cluster = false,
    },
    {
        // geizer
        .damage = 1.0,
        .speed = 200.0,
        .accel = 0.0,
        .scale = 0.8,
        .ttl = 3.0,
        .explosive = true,
        .bouncy = false,
        .penetrate = false,
        .cluster = false,
    },
    {
        // clinger
        .damage = 1.0,
        .speed = 200.0,
        .accel = 0.0,
        .scale = 0.8,
        .ttl = 3.0,
        .explosive = false,
        .bouncy = false,
        .penetrate = false,
        .cluster = false,
    },
    {
        // totem blue
        .damage = 1.0,
        .speed = 200.0,
        .accel = 0.0,
        .scale = 0.8,
        .ttl = 3.0,
        .explosive = false,
        .bouncy = false,
        .penetrate = false,
        .cluster = true,
    },
};

ProjectileSpawn projectile_spawn[] = {
    {
        // player
        .num = 1,
        .spread = 30.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // player - kinetic discharge skill
        .num = 1,
        .spread = 360.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // creature generic
        .num = 1,
        .spread = 0.0,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // geizer
        .num = 1,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // clinger
        .num = 1,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // clinger
        .num = 1,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .poison_chance = 0.0,
        .cold_chance = 1.0,
    },
};

static void projectile_remove(int index)
{
    list_remove(plist, index);
}

static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

void projectile_init()
{
    plist = list_create((void*)projectiles, MAX_PROJECTILES, sizeof(Projectile));
    projectile_image = gfx_load_image("src/img/projectiles.png", false, false, 16, 16);
}

void projectile_clear_all()
{
    list_clear(plist);
}

void projectile_add(Physics* phys, uint8_t curr_room, ProjectileDef* def, ProjectileSpawn* spawn, uint32_t color, float angle_deg, bool from_player)
{
    Projectile proj = {0};
    proj.id = get_id();
    proj.def = *def;
    proj.color = color;
    proj.phys.height = gfx_images[projectile_image].visible_rects[0].h * proj.def.scale;
    proj.phys.width =  gfx_images[projectile_image].visible_rects[0].w * proj.def.scale;
    proj.phys.length = gfx_images[projectile_image].visible_rects[0].w * proj.def.scale;
    proj.phys.pos.x = phys->pos.x;
    proj.phys.pos.y = phys->pos.y;
    proj.phys.pos.z = phys->height/2.0 + phys->pos.z;
    proj.phys.mass = 1.0;
    proj.phys.radius = (MAX(proj.phys.height, proj.phys.width) / 2.0) * proj.def.scale;
    proj.phys.amorphous = proj.def.bouncy ? false : true;
    proj.phys.elasticity = proj.def.bouncy ? 1.0 : 0.1;
    proj.curr_room = curr_room;
    proj.from_player = from_player;
    proj.angle_deg = angle_deg;

    proj.cluster_stage = spawn->cluster_stage;

    float spread = spawn->spread/2.0;

    for(int i = 0; i < spawn->num; ++i)
    {
        Projectile p = {0};
        memcpy(&p, &proj, sizeof(Projectile));
        p.id = get_id();

        p.phys.ethereal = RAND_FLOAT(0.0,1.0) <= spawn->ghost_chance;
        p.poison = RAND_FLOAT(0.0,1.0) <= spawn->poison_chance;
        p.cold = RAND_FLOAT(0.0,1.0) <= spawn->cold_chance;
        bool homing = RAND_FLOAT(0.0,1.0) <= spawn->homing_chance;

        if(!FEQ0(spread) && i > 0)
        {
            p.angle_deg += RAND_FLOAT(-spread, spread);
        }

        float angle = RAD(p.angle_deg);
        p.phys.vel.x = +(p.def.speed)*cosf(angle) + phys->vel.x;
        p.phys.vel.y = -(p.def.speed)*sinf(angle) + phys->vel.y;
        p.phys.vel.z = 80.0;

        if(homing)
        {
            Physics* target = NULL;
            if(p.from_player)
                target = entity_get_closest_to(&p.phys, p.curr_room, ENTITY_TYPE_CREATURE);
            else
                target = entity_get_closest_to(&p.phys, p.curr_room, ENTITY_TYPE_PLAYER);

            if(target)
            {
                float tx = target->pos.x;
                float ty = target->pos.y;
                Vector2f v = {tx - p.phys.pos.x, ty - p.phys.pos.y};
                normalize(&v);
                p.angle_deg = calc_angle_deg(p.phys.pos.x, p.phys.pos.y, tx, ty);
                p.phys.vel.x = v.x * p.def.speed;
                p.phys.vel.y = v.y * p.def.speed;
            }
        }

        // printf("%s damage: %.2f\n", __func__, p.def.damage);

        list_add(plist, (void*)&p);
    }

}

void projectile_kill(Projectile* proj)
{
    proj->phys.dead = true;

    ParticleEffect splash = {0};
    memcpy(&splash, &particle_effects[EFFECT_SPLASH], sizeof(ParticleEffect));

    if(role == ROLE_SERVER)
    {
        NetEvent ev = {
            .type = EVENT_TYPE_PARTICLES,
            .data.particles.effect_index = EFFECT_SPLASH,
            .data.particles.pos = { proj->phys.pos.x, proj->phys.pos.y },
            .data.particles.scale = proj->def.scale,
            .data.particles.color1 = proj->from_player ? 0x006484BA : 0x00CC5050,
            .data.particles.color2 = proj->from_player ? 0x001F87DC : 0x00FF8080,
            .data.particles.color3 = proj->from_player ? 0x00112837 : 0x00550000,
            .data.particles.lifetime = 0.5
        };

        net_server_add_event(&ev);
    }
    else
    {
        if(!proj->from_player)
        {
            // make splash effect red
            splash.color1 = 0x00CC5050;
            splash.color2 = 0x00FF8080;
            splash.color3 = 0x00550000;
        }

        if(proj->def.scale > 0.25)
        {
            splash.scale.init_min *= proj->def.scale;
            splash.scale.init_max *= proj->def.scale;
            ParticleSpawner* ps = particles_spawn_effect(proj->phys.pos.x,proj->phys.pos.y, 0.0, &splash, 0.5, true, false);
            ps->userdata = (int)proj->curr_room;
        }

        if(proj->def.explosive)
        {
            explosion_add(proj->phys.pos.x, proj->phys.pos.y, 15.0*proj->def.scale, 100.0*proj->def.scale, proj->curr_room, proj->from_player);
        }
    }

    if(proj->def.cluster)
    {
        // printf("cluster\n");
        ProjectileDef pd = proj->def;

        if(proj->cluster_stage >= (3) || proj->cluster_stage >= (pd.cluster_stages))
        {
            return;
        }

        pd.cluster = true;
        pd.scale = pd.cluster_scales[proj->cluster_stage];

        // pd.cluster = false;
        // pd.scale *= 0.5;
        // if(pd.scale <= 0.05)
        //     return;
        pd.speed = 50.0;

        ProjectileSpawn sp = {0};
        // sp.num = 6;
        sp.num = pd.cluster_num[proj->cluster_stage];
        sp.spread = 360.0;
        sp.cluster_stage = proj->cluster_stage+1;
        proj->phys.vel.x = 0.0;
        proj->phys.vel.y = 0.0;
        projectile_add(&proj->phys, proj->curr_room, &pd, &sp, proj->color, rand() % 360, proj->from_player);
    }
}

void projectile_update_all(float dt)
{
    // printf("projectile update\n");

    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

        if(role == ROLE_CLIENT)
        {
            projectile_lerp(proj, dt);
            continue;
        }

        if(proj->phys.dead)
            continue;

        if(proj->def.ttl <= 0)
        {
            projectile_kill(proj);
            continue;
        }

        float _dt = RANGE(proj->def.ttl, 0.0, dt);
        // printf("%3d %.4f\n", i, delta_t);

        proj->def.ttl -= _dt;

        if(proj->def.accel != 0.0 && proj->accel_vector.x == 0.0 && proj->accel_vector.y == 0.0 && proj->accel_vector.z == 0.0)
        {
            Vector3f f = {proj->phys.vel.x, proj->phys.vel.y, proj->phys.vel.z};
            normalize3f(&f);

            f.x *= proj->def.accel;
            f.y *= proj->def.accel;
            f.z *= proj->def.accel;

            proj->accel_vector.x = f.x;
            proj->accel_vector.y = f.y;
            proj->accel_vector.z = f.z;
        }

        proj->phys.vel.x += proj->accel_vector.x;
        proj->phys.vel.y += proj->accel_vector.y;
        proj->phys.vel.z += proj->accel_vector.z;

        //printf("adding %f %f; vel: %f %f\n",f.x, f.y, proj->phys.vel.x, proj->phys.vel.y);

        proj->phys.prior_pos.x = proj->phys.pos.x;
        proj->phys.prior_pos.y = proj->phys.pos.y;
        proj->phys.prior_pos.z = proj->phys.pos.z;

        proj->phys.pos.x += _dt*proj->phys.vel.x;
        proj->phys.pos.y += _dt*proj->phys.vel.y;

        phys_apply_gravity(&proj->phys, 0.5, _dt);

        if(proj->phys.amorphous && proj->phys.pos.z <= 0.0)
        {
            projectile_kill(proj);
        }
    }

    // int count = 0;
    for(int i = plist->count - 1; i >= 0; --i)
    {
        if(projectiles[i].phys.dead)
        {
            // count++;
            projectile_remove(i);
        }
    }
    // if(count > 0) printf("removed %d\n", count);

}

void projectile_handle_collision(Projectile* proj, Entity* e)
{
    if(proj->phys.dead) return;

    ProjectileDef* projdef = &proj->def;

    uint8_t curr_room = 0;
    Physics* phys = NULL;

    if(proj->from_player && e->type == ENTITY_TYPE_CREATURE)
    {
        Creature* c = (Creature*)e->ptr;

        curr_room = c->curr_room;
        phys = &c->phys;
    }
    else if(!proj->from_player && e->type == ENTITY_TYPE_PLAYER)
    {
        Player* p = (Player*)e->ptr;

        curr_room = p->curr_room;
        phys = &p->phys;
    }

    bool hit = false;

    if(phys)
    {
        if(phys->dead) return;
        if(proj->curr_room != curr_room) return;

        Box proj_curr = {
            proj->phys.pos.x,
            proj->phys.pos.y,
            proj->phys.pos.z + proj->phys.height/2.0,
            proj->phys.width,
            proj->phys.width,
            proj->phys.height*2,
        };

        Box check = {
            phys->pos.x,
            phys->pos.y,
            phys->pos.z + phys->height/2.0,
            phys->width,
            phys->width,
            phys->height*2,
        };

        hit = boxes_colliding(&proj_curr, &check);

        if(hit)
        {
            if(!projdef->penetrate)
            {
                CollisionInfo ci = {0.0,0.0};
                proj->phys.pos.x = proj->phys.prior_pos.x;
                proj->phys.pos.y = proj->phys.prior_pos.y;
                phys_collision_correct(&proj->phys,phys,&ci);
                projectile_kill(proj);
            }

            if(proj->cold)
            {
                status_effects_add_type(phys,STATUS_EFFECT_COLD);
            }
            
            if(proj->poison > 0.0)
            {
                status_effects_add_type(phys,STATUS_EFFECT_POISON);
            }

            switch(e->type)
            {
                case ENTITY_TYPE_PLAYER:
                    player_hurt((Player*)e->ptr, projdef->damage);
                    break;
                case ENTITY_TYPE_CREATURE:
                    // printf("%s damage: %.2f\n", __func__, projdef->damage);
                    creature_hurt((Creature*)e->ptr, projdef->damage);
                    break;
            }
        }
    }

    if(hit && projdef->explosive)
    {
        explosion_add(proj->phys.pos.x, proj->phys.pos.y, 15.0*proj->def.scale, 100.0*proj->def.scale, proj->curr_room, proj->from_player);
    }
}

void projectile_draw(Projectile* proj)
{
    if(proj->curr_room != player->curr_room)
        return; // don't draw projectile if not in same room

    float opacity = proj->phys.ethereal ? 0.3 : 1.0;

    float y = proj->phys.pos.y - 0.5*proj->phys.pos.z;
    gfx_sprite_batch_add(projectile_image, 0, proj->phys.pos.x, y, proj->color, false, proj->def.scale, 0.0, opacity, false, true, false);
}

void projectile_lerp(Projectile* p, double dt)
{
    p->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector3f lp = lerp3f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;
    p->phys.pos.z = lp.z;

    //printf("prior_pos: %f %f, target_pos: %f %f, pos: %f %f, t: %f\n",p->server_state_prior.pos.x, p->server_state_prior.pos.y, p->server_state_target.pos.x, p->server_state_target.pos.y, p->phys.pos.x, p->phys.pos.y, t);
}

const char* projectile_def_get_name(ProjectileType proj_type)
{
    switch(proj_type)
    {
        case PROJECTILE_TYPE_PLAYER: return "Player";
        case PROJECTILE_TYPE_PLAYER_KINETIC_DISCHARGE: return "Kinetic Discharge";
        case PROJECTILE_TYPE_CREATURE_GENERIC: return "Creature";
        case PROJECTILE_TYPE_CREATURE_GEIZER: return "Geizer";
        case PROJECTILE_TYPE_CREATURE_CLINGER: return "Clinger";
        case PROJECTILE_TYPE_CREATURE_TOTEM_BLUE: return "Totem Blue";
        default: return "???";
    }
}
