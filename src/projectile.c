#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"
#include "player.h"
#include "creature.h"
#include "explosion.h"
#include "lighting.h"
#include "projectile.h"

Projectile projectiles[MAX_PROJECTILES];
Projectile prior_projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static int projectile_image;
static uint16_t id_counter = 0;

ProjectileDef projectile_lookup[] = {
    {
        // player/laser
        .damage = 10.0,
        .min_speed = 200.0,
        .base_speed = 200.0,
        .angle_spread = 45.0,
        .scale = 1.0,
        .num = 1,
        .charge = false,
        .charge_rate = 16,
        .ghost = false,
        .explosive = false,
        .homing = false,
        .bouncy = false,
        .penetrate = false
    },
    {
        // creature
        .damage = 1.0,
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
        .penetrate = false
    },
    {
        // creature clinger
        .damage = 1.0,
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
        .penetrate = false
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

void projectile_add(Physics* phys, uint8_t curr_room, ProjectileType proj_type, float angle_deg, float scale, float damage_multiplier, bool from_player)
{
    ProjectileDef* projdef = &projectile_lookup[proj_type];

    Projectile proj = {0};
    proj.type = proj_type;
    proj.damage = projdef->damage * damage_multiplier;
    proj.scale = scale * projdef->scale;
    proj.time = 0.0;
    proj.ttl  = 1.0; //TODO: change to range
    proj.phys.pos.x = phys->pos.x;
    proj.phys.pos.y = phys->pos.y;
    proj.phys.mass = 1.0;
    proj.phys.radius = 4.0 * proj.scale;
    proj.phys.amorphous = projdef->bouncy ? false : true;
    proj.phys.elasticity = projdef->bouncy ? 1.0 : 0.1;
    proj.phys.ethereal = projdef->ghost;
    proj.curr_room = curr_room;
    proj.from_player = from_player;

    proj.hit_box.x = proj.phys.pos.x;
    proj.hit_box.y = proj.phys.pos.y;
    Rect* vr = &gfx_images[projectile_image].visible_rects[0];
    float wh = MAX(vr->w, vr->h) * proj.scale;
    proj.hit_box.w = wh;
    proj.hit_box.h = wh;

    memcpy(&proj.hit_box_prior, &proj.hit_box, sizeof(Rect));

    float min_speed = projdef->min_speed;
    float spread = projdef->angle_spread/2.0;

    for(int i = 0; i < projdef->num; ++i)
    {
        Projectile p = {0};
        memcpy(&p, &proj, sizeof(Projectile));
        p.id = get_id();
        // printf("proj scale: %.2f\n", p.scale);

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

        float vx0 = (speed)*cosf(angle);
        float vy0 = (-speed)*sinf(angle);   // @minus

        float vx = vx0 + phys->vel.x;
        float vy = vy0 + phys->vel.y;

        p.phys.vel.x = vx0;
        p.phys.vel.y = vy0;

        // if(!FEQ0(p->vel.x))
        {
            if(vx0 > 0 && vx < 0)
            {
                // printf("x help 1\n");
                p.phys.vel.x = (min_speed)*cosf(angle);
            }
            else if(vx0 < 0 && vx > 0)
            {
                // printf("x help 2\n");
                p.phys.vel.x = (min_speed)*cosf(angle);
            }
            else
            {
                p.phys.vel.x = vx;
            }
        }
        // if(!FEQ0(p->phys.vel.y))
        {
            if(vy0 > 0 && vy < 0)
            {
                // printf("y help 1\n");
                p.phys.vel.y = (-min_speed)*sinf(angle);  // @minus
            }
            else if(vy0 < 0 && vy > 0)
            {
                // printf("y help 2\n");
                p.phys.vel.y = (-min_speed)*sinf(angle);  // @minus
            }
            else
            {
                // printf("help 3\n");
                p.phys.vel.y = vy;
            }
        }

        float a = calc_angle_rad(0,0,p.phys.vel.x, p.phys.vel.y);
        float xa = cosf(a);
        float ya = sinf(a);
        float _speed = 0;

        if(!FEQ0(xa))
        {
            _speed = p.phys.vel.x / xa;
        }
        else if(!FEQ0(ya))
        {
            _speed = p.phys.vel.y / ya;
            _speed *= -1;   // @minus
        }
        if(_speed < min_speed)
        {
            // printf("min speed\n");
            p.phys.vel.x = min_speed * xa;
            p.phys.vel.y = -min_speed * ya;   //@minus
        }


        list_add(plist, (void*)&p);
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
            proj->phys.dead = true;
            continue;
        }

        float _dt = RANGE(proj->ttl - proj->time, 0.0, delta_t);
        // printf("%3d %.4f\n", i, delta_t);

        proj->time += _dt;

        // proj->prior_pos.x = proj->phys.pos.x;
        // proj->prior_pos.y = proj->phys.pos.y;

        if(projectile_lookup[proj->type].homing)
        {
            if(!proj->homing_target)
            {
                // get homing target
                if(proj->from_player)
                    proj->homing_target = entity_get_closest_to(&proj->phys,proj->curr_room, ENTITY_TYPE_CREATURE);
                else
                    proj->homing_target = entity_get_closest_to(&proj->phys,proj->curr_room, ENTITY_TYPE_PLAYER);
            }

            if(proj->homing_target)
            {
                float m = magn(proj->phys.vel);

                Vector2f v = {proj->homing_target->pos.x - proj->phys.pos.x, proj->homing_target->pos.y - proj->phys.pos.y};
                normalize(&v);

                proj->phys.vel.x = v.x * m;
                proj->phys.vel.y = v.y * m;
            }
        }

        proj->phys.prior_pos.x = proj->phys.pos.x;
        proj->phys.prior_pos.y = proj->phys.pos.y;

        proj->phys.pos.x += _dt*proj->phys.vel.x;
        proj->phys.pos.y += _dt*proj->phys.vel.y;

        projectile_update_hit_box(proj);
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

void projectile_update_hit_box(Projectile* proj)
{
    memcpy(&proj->hit_box_prior, &proj->hit_box, sizeof(Rect));
    proj->hit_box.x = proj->phys.pos.x;
    proj->hit_box.y = proj->phys.pos.y;
    // print_rect(&proj->hit_box);
}

void projectile_handle_collision(Projectile* proj, Entity* e)
{
    if(proj->phys.dead) return;

    bool hit = false;

    ProjectileDef* projdef = &projectile_lookup[proj->type];

    if(proj->from_player && e->type == ENTITY_TYPE_CREATURE)
    {
        Creature* c = (Creature*)e->ptr;

        if(c->phys.dead) return;
        if(proj->curr_room != c->curr_room) return;

        hit = are_rects_colliding(&proj->hit_box_prior, &proj->hit_box, &c->hitbox);

        if(hit)
        {
            CollisionInfo ci = {0.0,0.0};
            creature_hurt(c, proj->damage);
            if(!projdef->penetrate)
            {
                phys_collision_correct(&proj->phys,&c->phys,&ci);
                proj->phys.dead = true;
            }
        }
    }
    else if(!proj->from_player && e->type == ENTITY_TYPE_PLAYER)
    {
        Player* p = (Player*)e->ptr;

        if(!p->active) return;
        if(p->phys.dead) return;

        if(proj->curr_room != p->curr_room) return;

        hit = are_rects_colliding(&proj->hit_box_prior, &proj->hit_box, &p->hitbox);

        if(hit)
        {
            CollisionInfo ci = {0.0,0.0};
            player_hurt(p, proj->damage);
            if(!projdef->penetrate)
            {
                phys_collision_correct(&proj->phys,&p->phys,&ci);
                proj->phys.dead = true;
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

    gfx_draw_image(projectile_image, 0, proj->phys.pos.x, proj->phys.pos.y, proj->from_player ? 0x0050A0FF : 0x00FF5050, proj->scale, 0.0, opacity, false, true);

    if(debug_enabled)
    {
        gfx_draw_rect(&proj->hit_box_prior, COLOR_GREEN, 1.0, 0.0, 1.0, false, true);
        gfx_draw_rect(&proj->hit_box, COLOR_BLUE, 1.0, 0.0, 1.0, false, true);
    }

}

void projectile_lerp(Projectile* p, double dt)
{
    p->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector2f lp = lerp2f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    projectile_update_hit_box(p);

    //printf("prior_pos: %f %f, target_pos: %f %f, pos: %f %f, t: %f\n",p->server_state_prior.pos.x, p->server_state_prior.pos.y, p->server_state_target.pos.x, p->server_state_target.pos.y, p->phys.pos.x, p->phys.pos.y, t);
}
