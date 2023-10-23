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

    int tile_x = 0;
    int tile_y = 0;
    for(;;)
    {
        tile_x = (rand() % ROOM_TILE_SIZE_X);
        tile_y = (rand() % ROOM_TILE_SIZE_Y);
        if(level_get_tile_type(room, tile_x, tile_y) == TILE_FLOOR)
        {
            Rect rp = level_get_tile_rect(tile_x, tile_y);
            c.phys.pos.x = rp.x;
            c.phys.pos.y = rp.y;

            RectXY rxy = {0};
            rect_to_rectxy(&room_area, &rxy);
            if(rp.x < rxy.x[TL] || rp.x > rxy.x[TR] || rp.y < rxy.y[TL] || rp.y > rxy.y[BL])
            {
                printf("spawned outside of area!\n");
                printf("pos: %.2f, %.2f\n", rp.x, rp.y);
                printf("tile: %d, %d (%d)\n", tile_x, tile_y, level_get_tile_type(room, tile_x, tile_y));
            }

            break;
        }
    }

    c.spawn_x = tile_x;
    c.spawn_y = tile_y;
    c.spawn.x = c.phys.pos.x;
    c.spawn.y = c.phys.pos.y;


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

    bool sim = false;
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(!players[i].active) continue;
        if(c->curr_room == players[i].curr_room)
        {
            sim = true;
            break;
        }
    }
    if(!sim) return;

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

#if 1
    static bool dbg = false;
    if(!dbg)
    {
        RectXY rxy = {0};
        rect_to_rectxy(&room_area, &rxy);
        if(c->phys.pos.x < rxy.x[TL] || c->phys.pos.x > rxy.x[TR] || c->phys.pos.y < rxy.y[TL] || c->phys.pos.y > rxy.y[BL])
        {
            dbg = true;
            text_list_add(text_lst, 10.0, "CREATURE ERROR!");
            // player->curr_room = c->curr_room;
            // player->transition_room = player->curr_room;
            Room* room = level_get_room_by_index(&level, c->curr_room);
            printf("creature is outside of area!\n");
            // print_rect(&room_area);
            printf("creature room: %u, player_room: %u\n", c->curr_room, player->curr_room);
            printf("room area X: %.2f  -  %.2f\n", rxy.x[TL], rxy.x[BR]);
            printf("room area Y: %.2f  -  %.2f\n", rxy.y[TL], rxy.y[BR]);
            printf("pos: %.2f, %.2f\n", c->phys.pos.x, c->phys.pos.y);
            printf("spawn pos: %.2f, %.2f\n", c->spawn.x, c->spawn.y);
            printf("spawn tile: %d, %d (%d)\n", c->spawn_x, c->spawn_y, level_get_tile_type(room, c->spawn_x, c->spawn_y));
        }
    }
#endif

}

void creature_update_all(float dt)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        creature_update(c, dt);
        if(c->dead)
        {
            list_remove(clist, i);
        }
    }
}

void creature_draw(Creature* c)
{
    if(c->curr_room != player->curr_room)
        return;

    if(c->dead) return;
    gfx_draw_image(c->image, c->sprite_index, c->phys.pos.x, c->phys.pos.y, c->color, 1.0, 0.0, 1.0, false, true);

    if(debug_enabled)
    {
        gfx_draw_rect(&c->hitbox, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);

        Rect r = c->hitbox;
        r.x = c->spawn.x;
        r.y = c->spawn.y;
        gfx_draw_rect(&r, COLOR_ORANGE, NOT_SCALED, NO_ROTATION, 0.2, true, true);
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

    Decal d = {0};
    d.image = particles_image;
    d.sprite_index = 0;
    d.tint = COLOR_RED;
    d.scale = 1.0;
    d.rotation = rand() % 360;
    d.opacity = 0.6;
    d.ttl = 10.0;
    d.pos.x = c->phys.pos.x;
    d.pos.y = c->phys.pos.y;
    d.room = c->curr_room;
    decal_add(d);
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
