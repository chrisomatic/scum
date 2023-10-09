#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"
#include "player.h"
#include "projectile.h"

Projectile projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static int projectile_image;
static uint16_t id_counter = 0;

ProjectileDef projectile_lookup[] = {
    {10.0, 100.0, 400.0} // laser
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

void projectile_add(Player* p, float angle_deg)
{
    Projectile proj = {0};

    proj.id = get_id();
    //proj.player_id = p->id;

    proj.type = PROJECTILE_TYPE_LASER;

    ProjectileDef* projdef = &projectile_lookup[proj.type];

    proj.dead = false;
    proj.damage = projdef->damage;

    proj.pos.x = p->pos.x;
    proj.pos.y = p->pos.y;

    proj.angle_deg = angle_deg;
    float angle = RAD(proj.angle_deg);
    float speed = projdef->base_speed;
    float min_speed = projdef->min_speed;

    float vx0 = (speed)*cosf(angle);
    float vy0 = (-speed)*sinf(angle);   // @minus

    float vx = vx0 + p->vel.x;
    float vy = vy0 + p->vel.y;

    proj.vel.x = vx0;
    proj.vel.y = vy0;

    // handle angle
    // -----------------------------------------------------------------------------------

    // if(!FEQ0(p->vel.x))
    {
        if(vx0 > 0 && vx < 0)
        {
            // printf("x help 1\n");
            proj.vel.x = (min_speed)*cosf(angle);
        }
        else if(vx0 < 0 && vx > 0)
        {
            // printf("x help 2\n");
            proj.vel.x = (min_speed)*cosf(angle);
        }
        else
        {
            proj.vel.x = vx;
        }
    }
    // if(!FEQ0(p->vel.y))
    {
        if(vy0 > 0 && vy < 0)
        {
            // printf("y help 1\n");
            proj.vel.y = (-min_speed)*sinf(angle);  // @minus
        }
        else if(vy0 < 0 && vy > 0)
        {
            // printf("y help 2\n");
            proj.vel.y = (-min_speed)*sinf(angle);  // @minus
        }
        else
        {
            // printf("help 3\n");
            proj.vel.y = vy;
        }
    }

    // handle minimum speed
    // -----------------------------------------------------------------------------------
    float a = calc_angle_rad(0,0,proj.vel.x, proj.vel.y);
    float xa = cosf(a);
    float ya = sinf(a);
    float _speed = 0;

    if(!FEQ0(xa))
    {
        _speed = proj.vel.x / xa;
    }
    else if(!FEQ0(ya))
    {
        _speed = proj.vel.y / ya;
        _speed *= -1;   // @minus
    }
    if(_speed < min_speed)
    {
        // printf("min speed\n");
        proj.vel.x = min_speed * xa;
        proj.vel.y = -min_speed * ya;   //@minus
    }

    proj.time = 0.0;
    proj.ttl  = 5.0;

    proj.hit_box.x = proj.pos.x;
    proj.hit_box.y = proj.pos.y;
    GFXImage* img = &gfx_images[projectile_image];
    float wh = MAX(img->element_width, img->element_height);
    proj.hit_box.w = wh;
    proj.hit_box.h = wh;

    memcpy(&proj.hit_box_prior, &proj.hit_box, sizeof(Rect));

    list_add(plist, (void*)&proj);
}


void projectile_update(float delta_t)
{
    // printf("projectile update\n");

    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

        if(proj->dead)
            continue;

        if(proj->time >= proj->ttl)
        {
            proj->dead = true;
            continue;
        }

        float _dt = RANGE(proj->ttl - proj->time, 0.0, delta_t);
        // printf("%3d %.4f\n", i, delta_t);

        proj->time += _dt;

        // proj->prior_pos.x = proj->pos.x;
        // proj->prior_pos.y = proj->pos.y;

        proj->pos.x += _dt*proj->vel.x;
        proj->pos.y += _dt*proj->vel.y;

        projectile_update_hit_box(proj);

        if(!rectangles_colliding(&proj->hit_box, &room_area))
        {
            proj->dead = true;
            continue;
        }
    }

    // int count = 0;
    for(int i = plist->count - 1; i >= 0; --i)
    {
        if(projectiles[i].dead)
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
    proj->hit_box.x = proj->pos.x;
    proj->hit_box.y = proj->pos.y;
    // print_rect(&proj->hit_box);
}

void projectile_handle_collisions(float delta_t)
{
    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* p = &projectiles[i];
        if(p->dead) continue;
    }
    return;
}

void projectile_draw(Projectile* proj)
{
    gfx_draw_image(projectile_image, 0, proj->pos.x, proj->pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, true, true);

    if(debug_enabled)
    {
        gfx_draw_rect(&proj->hit_box_prior, COLOR_GREEN, 0, 1.0, 1.0, false, true);
        gfx_draw_rect(&proj->hit_box, COLOR_BLUE, 0, 1.0, 1.0, false, true);
    }
}
