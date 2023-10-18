#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "level.h"
#include "log.h"
#include "player.h"
#include "glist.h"
#include "creature.h"

Creature creatures[MAX_CREATURES];
glist* clist = NULL;

static int creature_image;

void creature_init()
{
    clist = list_create((void*)creatures, MAX_CREATURES, sizeof(Creature));
    creature_image = gfx_load_image("src/img/creature_slug.png", false, false, 17, 17);
}

void creature_clear_all()
{
    list_clear(clist);
}


void creature_add(Room* room, CreatureType type)
{
    Creature c = {0};

    int x0 = room_area.x - room_area.w/2.0;
    int y0 = room_area.y - room_area.h/2.0;

    c.type = type;
    c.curr_room = room->index;
    c.phys.pos.x = x0 + TILE_SIZE*(rand() % ROOM_TILE_SIZE_X + 1) + 8;
    c.phys.pos.y = y0 + TILE_SIZE*(rand() % ROOM_TILE_SIZE_Y + 1) + 8;
    memcpy(&c.phys.target_pos,&c.phys.pos,sizeof(Vector2f));
    c.color = room->color;
    c.speed = 10.0;
    c.phys.radius = 8.0;
    c.phys.coffset.x = 0;
    c.phys.coffset.y = 0;
    c.action_counter = 0.0;
    c.sprite_index = DIR_DOWN;
    c.hp_max = 3.0;
    c.hp = c.hp_max;

    list_add(clist, (void*)&c);
}

void creature_update(Creature* c, float dt)
{
    if(c->dead)
        return;

    c->action_counter += dt*c->speed;

    if(c->action_counter >= ACTION_COUNTER_MAX)
    {
        c->action_counter = 0.0;
        int dir = rand() % 5;

        int v = 0;
        int h = 0;

        switch(dir)
        {
            case DIR_UP:
                c->sprite_index = DIR_UP;
                h = 0;
                v = -1;
                break;
            case DIR_RIGHT:
                c->sprite_index = DIR_RIGHT;
                h = +1;
                v = 0;
                break;
            case DIR_DOWN:
                c->sprite_index = DIR_DOWN;
                h = 0;
                v = +1;
                break;
            case DIR_LEFT:
                c->sprite_index = DIR_LEFT;
                h = -1;
                v = 0;
                break;
            default:
                // don't move
                break;
        }

        c->phys.prior_pos.x = c->phys.pos.x;
        c->phys.prior_pos.y = c->phys.pos.y;

        c->phys.target_pos.x = (c->phys.pos.x + h*TILE_SIZE);
        c->phys.target_pos.y = (c->phys.pos.y + v*TILE_SIZE);
    }

    if(c->phys.pos.x != c->phys.target_pos.x || c->phys.pos.y != c->phys.target_pos.y)
    {
        float t = c->action_counter / ACTION_COUNTER_MAX;

        c->phys.pos = lerp2f(&c->phys.prior_pos, &c->phys.target_pos, t);

        Vector2i roomxy = level_get_room_coords(c->curr_room);
        Room* room = &level.rooms[roomxy.x][roomxy.y];
        level_handle_room_collision(room,&c->phys);
    }

    // update hit box
    c->hitbox.x = c->phys.pos.x;
    c->hitbox.y = c->phys.pos.y;

    c->hitbox.w = 16;
    c->hitbox.h = 16;
}

void creature_update_all(float dt)
{
    for(int i = clist->count; i >= 0; --i)
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
        return;

    gfx_draw_image(creature_image, c->sprite_index, c->phys.pos.x, c->phys.pos.y, c->color, 1.0, 0.0, 1.0, false, true);

    if(debug_enabled)
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
