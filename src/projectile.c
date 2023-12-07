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

ProjectileDef projectile_lookup[] = {
    {
        // player
        .color = 0x0050A0FF,
        .damage = 1.0,
        .range = 128,
        .min_speed = 150.0,
        .base_speed = 215.0,
        .angle_spread = 45.0,
        .scale = 1.0,
        .num = 1,
        .charge = false,
        .charge_rate = 16,
        .ghost = false,
        .explosive = false,
        .homing = false,
        .bouncy = false,
        .penetrate = false,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // creature
        .color = 0x00FF5050,
        .damage = 1.0,
        .range = 32*6,
        .min_speed = 200.0,
        .base_speed = 200.0,
        .angle_spread = 0.0,
        .scale = 1.0,
        .num = 1,
        .charge = false,
        .charge_rate = 16,
        .ghost = false,
        .explosive = true,
        .homing = false,
        .bouncy = false,
        .penetrate = false,
        .poison_chance = 0.0,
        .cold_chance = 0.0,
    },
    {
        // creature clinger
        .color = 0x00FF5050,
        .damage = 1.0,
        .range = 32*6,
        .min_speed = 200.0,
        .base_speed = 200.0,
        .angle_spread = 0.0,
        .scale = 1.0,
        .num = 1,
        .charge = false,
        .charge_rate = 16,
        .ghost = true,
        .explosive = false,
        .homing = false,
        .bouncy = false,
        .penetrate = false,
        .poison_chance = 1.0,
        .cold_chance = 0.0,
    }
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

void projectile_add(Physics* phys, uint8_t curr_room, ProjectileDef* projdef, float angle_deg, float scale, float damage_multiplier, bool from_player)
{
    Projectile proj = {0};
    proj.proj_def = projdef;
    proj.damage = projdef->damage * damage_multiplier;
    proj.scale = scale * projdef->scale;
    proj.time = 0.0;
    proj.ttl  = 1.0;
    proj.color = projdef->color;
    proj.phys.height = gfx_images[projectile_image].visible_rects[0].h*proj.scale;
    proj.phys.width = gfx_images[projectile_image].visible_rects[0].w*proj.scale;
    proj.phys.pos.x = phys->pos.x;
    proj.phys.pos.y = phys->pos.y;
    proj.phys.pos.z = phys->height/2.0 + phys->pos.z;
    proj.phys.mass = 1.0;
    proj.phys.radius = (MAX(proj.phys.height, proj.phys.width) / 2.0) * proj.scale;
    proj.phys.amorphous = projdef->bouncy ? false : true;
    proj.phys.elasticity = projdef->bouncy ? 1.0 : 0.1;
    proj.homing = projdef->homing;
    proj.phys.ethereal = projdef->ghost;

    proj.curr_room = curr_room;
    proj.from_player = from_player;

    float min_speed = projdef->min_speed;
    float spread = projdef->angle_spread/2.0;

    for(int i = 0; i < projdef->num; ++i)
    {
        Projectile p = {0};
        memcpy(&p, &proj, sizeof(Projectile));
        p.id = get_id();

        if(!proj.homing)
        {
            p.homing = RAND_FLOAT(0.0,1.0) <= projdef->homing_chance;
        }

        if(!proj.phys.ethereal)
        {
            // printf("projdef->ghost_chance: %.2f\n", projdef->ghost_chance);
            p.phys.ethereal = RAND_FLOAT(0.0,1.0) <= projdef->ghost_chance;
        }

        // printf("proj scale: %.2f\n", p.scale);

        // add spread
        float speed = projdef->base_speed;
        float angle = angle_deg;
        if(!FEQ0(spread) && i > 0)
        {
            // printf("%.2f -> ", angle);
            angle += RAND_FLOAT(-spread, spread);
            // printf("%.2f\n", angle);
        }

        p.angle_deg = angle;
        angle = RAD(angle);

        p.phys.vel.x = +speed*cosf(angle) + phys->vel.x;
        p.phys.vel.y = -speed*sinf(angle) + phys->vel.y;
        p.phys.vel.z = 80.0;

        if(p.homing)
        {
            Physics* target = NULL;
            if(p.from_player)
                target = entity_get_closest_to(&p.phys, p.curr_room, ENTITY_TYPE_CREATURE);
            else
                target = entity_get_closest_to(&p.phys, p.curr_room, ENTITY_TYPE_PLAYER);

            if(target)
            {
                // float tx = target->pos.x;
                // float ty = target->pos.y;
                // float tz = target->pos.y;
                // Vector3f v = {tx - p.phys.pos.x, ty - p.phys.pos.y, tz - p.phys.pos.z};
                // normalize3f(&v);
                // // float m = magn(p.phys.vel);
                // float m = dist3f(tx, ty, tz, p.phys.pos.x, p.phys.pos.y, p.phys.pos.z);
                // p.phys.vel.x = v.x * m;
                // p.phys.vel.y = v.y * m;

                float tx = target->pos.x;
                float ty = target->pos.y;
                Vector2f v = {tx - p.phys.pos.x, ty - p.phys.pos.y};
                normalize(&v);
                float m = dist(tx, ty, p.phys.pos.x, p.phys.pos.y);
                p.phys.vel.x = v.x * speed;
                p.phys.vel.y = v.y * speed;

#if 0
                static bool solved = false;
                static float thetime = 0;
                if(!solved)
                {
                    // solve for time until it hits the ground
                    // vz(t) = vz(t-1) - g*dt
                    // pz(t) = pz(t-1) + dt*vz(t)
                    Physics phys = p.phys;
                    float _dt = 1.0/60.0;
                    for(;;)
                    {
                        phys_apply_gravity(&phys, 0.5, _dt);
                        thetime += _dt;
                        if(phys.pos.z <= 0)
                            break;
                    }
                    solved = true;
                }

                float tx = target->pos.x + target->coffset.x;
                float ty = target->pos.y + target->coffset.y;

                float px = p.phys.pos.x;
                float py = p.phys.pos.y;

                // float r = p.phys.radius + target->radius;
                float r = p.phys.radius;

                Vector2f v = {tx - px, ty - py};
                normalize(&v);

                float maxspeed = speed*1;


#if 0
                float total_d = dist(tx, ty, px, py);

                maxspeed = speed*2;

                float nx = px + v.x*maxspeed*thetime;
                float ny = py + v.y*maxspeed*thetime;

                float d = dist(tx, ty, nx, ny);

                // impossible to hit the target at max speed
                if(total_d > (d+r))
                {
                    printf("speed: %.2f (max)\n", maxspeed);
                    p.phys.vel.x = v.x * maxspeed;
                    p.phys.vel.y = v.y * maxspeed;

                    printf("total_d: %.2f\n", total_d);
                    printf("d+r:     %.2f\n", d+r);
                }

                else
#endif
                {
                    float s = 20.0;
                    for(;;)
                    {
                        float nx = px + v.x*s*thetime;
                        float ny = py + v.y*s*thetime;
                        float d = dist(tx, ty, nx, ny);
                        if(d <= r || s >= maxspeed)
                        {
                            // printf("speed: %.2f\n", s);
                            break;
                        }
                        s += 1.0;
                    }
                    p.phys.vel.x = v.x * s;
                    p.phys.vel.y = v.y * s;
                }
#endif
            }
        }

        float d = dist(0,0, p.phys.vel.x,p.phys.vel.y); // distance travelled per second
        p.ttl = 3.0; //projdef->range / d;

        list_add(plist, (void*)&p);
    }

}

void projectile_add_type(Physics* phys, uint8_t curr_room, ProjectileType proj_type, float angle_deg, float scale, float damage_multiplier, bool from_player)
{
    ProjectileDef* projdef = &projectile_lookup[proj_type];
    projectile_add(phys, curr_room, projdef, angle_deg, scale, damage_multiplier, from_player);
}

void projectile_kill(Projectile* proj)
{
    proj->phys.dead = true;

    ParticleEffect splash = {0};
    memcpy(&splash, &particle_effects[EFFECT_SPLASH], sizeof(ParticleEffect));

    if(!proj->from_player)
    {
        splash.color1 = 0x00CC5050;
        splash.color2 = 0x00FF8080;
        splash.color3 = 0x00550000;
    }

    particles_spawn_effect(proj->phys.pos.x,proj->phys.pos.y, 0.0, &splash, 0.5, true, false);

    if(proj->proj_def->explosive)
    {
        explosion_add(proj->phys.pos.x, proj->phys.pos.y, 15.0*proj->scale, 100.0*proj->scale, proj->curr_room, proj->from_player);
    }
}

void projectile_update(float delta_t)
{
    // printf("projectile update\n");

    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

        if(proj->phys.dead)
            continue;

        if(proj->time >= proj->ttl)
        {
            projectile_kill(proj);
            continue;
        }

        float _dt = RANGE(proj->ttl - proj->time, 0.0, delta_t);
        // printf("%3d %.4f\n", i, delta_t);

        proj->time += _dt;

        // proj->prior_pos.x = proj->phys.pos.x;
        // proj->prior_pos.y = proj->phys.pos.y;

        // if(proj->homing)
        // {
        //     if(!proj->homing_target)
        //     {
        //         // get homing target
        //         if(proj->from_player)
        //             proj->homing_target = entity_get_closest_to(&proj->phys,proj->curr_room, ENTITY_TYPE_CREATURE);
        //         else
        //             proj->homing_target = entity_get_closest_to(&proj->phys,proj->curr_room, ENTITY_TYPE_PLAYER);
        //     }

        //     if(proj->homing_target)
        //     {
        //         float m = magn(proj->phys.vel);

        //         Vector2f v = {proj->homing_target->pos.x - proj->phys.pos.x, proj->homing_target->pos.y - proj->phys.pos.y};
        //         normalize(&v);

        //         proj->phys.vel.x = v.x * m;
        //         proj->phys.vel.y = v.y * m;
        //     }
        // }

        proj->phys.prior_pos.x = proj->phys.pos.x;
        proj->phys.prior_pos.y = proj->phys.pos.y;
        proj->phys.prior_pos.z = proj->phys.pos.z;

        proj->phys.pos.x += _dt*proj->phys.vel.x;
        proj->phys.pos.y += _dt*proj->phys.vel.y;
        if(proj->homing)
            phys_apply_gravity(&proj->phys,0.5, delta_t);   //TODO
        else
            phys_apply_gravity(&proj->phys,0.5, delta_t);

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

    ProjectileDef* projdef = proj->proj_def;

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
        //hit = are_spheres_colliding(&proj_prior, &proj_curr, &check);

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
            
            if(projdef->cold_chance > 0.0)
            {
                if(RAND_FLOAT(0.0,1.0) <= projdef->cold_chance)
                    status_effects_add_type(phys,STATUS_EFFECT_COLD);
            }
            
            if(projdef->poison_chance > 0.0)
            {
                if(RAND_FLOAT(0.0,1.0) <= projdef->poison_chance)
                    status_effects_add_type(phys,STATUS_EFFECT_POISON);
            }

            switch(e->type)
            {
                case ENTITY_TYPE_PLAYER:
                    player_hurt((Player*)e->ptr, proj->damage);
                    break;
                case ENTITY_TYPE_CREATURE:
                    creature_hurt((Creature*)e->ptr, proj->damage);
                    break;
            }
        }
    }

    if(hit && projdef->explosive)
    {
        explosion_add(proj->phys.pos.x, proj->phys.pos.y, 15.0*proj->scale, 100.0*proj->scale, proj->curr_room, proj->from_player);
    }
}

void projectile_draw(Projectile* proj)
{
    if(proj->curr_room != player->curr_room)
        return; // don't draw projectile if not in same room

    float opacity = proj->phys.ethereal ? 0.3 : 1.0;

    float y = proj->phys.pos.y - 0.5*proj->phys.pos.z;
    gfx_sprite_batch_add(projectile_image, 0, proj->phys.pos.x, y, proj->color, false, proj->scale, 0.0, opacity, false, true, false);
}

void projectile_lerp(Projectile* p, double dt)
{
    p->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector2f lp = lerp2f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    //printf("prior_pos: %f %f, target_pos: %f %f, pos: %f %f, t: %f\n",p->server_state_prior.pos.x, p->server_state_prior.pos.y, p->server_state_target.pos.x, p->server_state_target.pos.y, p->phys.pos.x, p->phys.pos.y, t);
}

const char* projectile_def_get_name(ProjectileType proj_type)
{
    switch(proj_type)
    {
        case PROJECTILE_TYPE_PLAYER: return "Player";
        case PROJECTILE_TYPE_CREATURE_GENERIC: return "Creature";
        case PROJECTILE_TYPE_CREATURE_CLINGER: return "Clinger";
        default: return "???";
    }
}
