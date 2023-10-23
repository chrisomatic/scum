#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "level.h"
#include "log.h"
#include "player.h"
#include "glist.h"
#include "particles.h"
#include "creature.h"

Creature creatures[MAX_CREATURES];
glist* clist = NULL;

static int creature_image_slug;
static int creature_image_clinger;

void creature_init()
{
    clist = list_create((void*)creatures, MAX_CREATURES, sizeof(Creature));
    creature_image_slug = gfx_load_image("src/img/creature_slug.png", false, false, 17, 17);
    creature_image_clinger = gfx_load_image("src/img/creature_clinger.png", false, false, 32, 32);
}

void creature_clear_all()
{
    list_clear(clist);
}


void creature_add(Room* room, CreatureType type)
{
    Creature c = {0};

    switch(type)
    {
        case CREATURE_TYPE_SLUG:
            c.image = creature_image_slug;
            break;
        case CREATURE_TYPE_CLINGER:
            c.image = creature_image_clinger;
            break;
    }
    c.type = type;
    c.curr_room = room->index;

    for(;;)
    {
        int x = (rand() % ROOM_TILE_SIZE_X + 1);
        int y = (rand() % ROOM_TILE_SIZE_Y + 1);
        if(level_get_tile_type(room, x, y) == TILE_FLOOR)
        {
            Rect rp = level_get_tile_rect(x, y);
            c.phys.pos.x = rp.x;
            c.phys.pos.y = rp.y;
            break;
        }
    }

    memcpy(&c.phys.target_pos,&c.phys.pos,sizeof(Vector2f));
    c.color = room->color;
    c.phys.speed = 300.0;
    c.phys.mass = 1.0;
    c.phys.radius = 8.0;
    c.phys.coffset.x = 0;
    c.phys.coffset.y = 0;
    c.action_counter = ACTION_COUNTER_MAX;
    c.sprite_index = DIR_DOWN;
    c.hp_max = 3.0;
    c.hp = c.hp_max;
    c.damage = 1;
    c.painful_touch = true;

    list_add(clist, (void*)&c);
}

void creature_update(Creature* c, float dt)
{
    if(c->dead)
        return;

    c->action_counter += dt;

    if(c->action_counter >= ACTION_COUNTER_MAX)
    {
        c->action_counter = 0.0;
        int dir = rand() % 5;

        Vector2i o = get_dir_offsets(dir);

        c->h = o.x;
        c->v = o.y;
        if(dir != DIR_NONE)
            c->sprite_index = dir;
    }

    float h_speed = c->phys.speed*c->h;
    float v_speed = c->phys.speed*c->v;

    c->phys.vel.x += dt*h_speed;
    c->phys.vel.y += dt*v_speed;

    if(ABS(c->phys.vel.x) > c->phys.speed) c->phys.vel.x = h_speed;
    if(ABS(c->phys.vel.y) > c->phys.speed) c->phys.vel.y = v_speed;

    float rate = phys_get_friction_rate(0.002,dt);
    phys_apply_friction(&c->phys,rate);

    c->phys.pos.x += dt*c->phys.vel.x;
    c->phys.pos.y += dt*c->phys.vel.y;

    // update hit box
    c->hitbox.x = c->phys.pos.x;
    c->hitbox.y = c->phys.pos.y;

    c->hitbox.w = 16;
    c->hitbox.h = 16;
}

void creature_update_all(float dt)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        creature_update(c, dt);
    }
}

void creature_draw(Creature* c)
{
    if(c->curr_room != player->curr_room)
        return;

    if(c->dead)
        gfx_draw_image(particles_image, 0, c->phys.pos.x, c->phys.pos.y, COLOR_RED, NOT_SCALED, c->blood_angle, 0.6, false, true);
    else
        gfx_draw_image(c->image, c->sprite_index, c->phys.pos.x, c->phys.pos.y, c->color, 1.0, 0.0, 1.0, false, true);

    if(debug_enabled && !c->dead)
    {
        gfx_draw_rect(&c->hitbox, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
}

void creature_draw_all()
{
    for(int i = clist->count; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        creature_draw(c);
    }
}

void creature_die(Creature* c)
{
    c->dead = true;
    c->blood_angle = rand() % 360;
}

void creature_hurt(Creature* c, float damage)
{
    c->hp -= damage;
    if(c->hp <= 0.0)
    {
        creature_die(c);
    }
}

int creature_get_count()
{
    return clist->count;
}
