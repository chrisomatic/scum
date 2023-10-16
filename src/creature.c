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
    creature_image = gfx_load_image("src/img/creature_slug.png", false, false, 16, 17);
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
    c.color = room->color;
    c.sprite_index = 0;

    list_add(clist, (void*)&c);
}

void creature_update(Creature* c, float dt)
{
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

    gfx_draw_image(creature_image, c->sprite_index, c->phys.pos.x, c->phys.pos.y, c->color, 1.0, 0.0, 1.0, false, true);
}

void creature_draw_all()
{
    for(int i = clist->count; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        creature_draw(c);
    }
}
