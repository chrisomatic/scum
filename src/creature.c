#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "level.h"
#include "log.h"
#include "player.h"
#include "glist.h"
#include "particles.h"
#include "projectile.h"
#include "creature.h"
#include "ai.h"

Creature prior_creatures[MAX_CREATURES];
Creature creatures[MAX_CREATURES];
glist* clist = NULL;

static int creature_image_slug;
static int creature_image_clinger;

static void creature_update_slug(Creature* c, float dt);
static void creature_update_clinger(Creature* c, float dt);

static uint16_t id_counter = 0;

static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

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

static void add_to_random_wall_tile(Creature* c)
{
    int tile_x = 0;
    int tile_y = 0;

    Dir wall = (Dir)rand() % 4;
    switch(wall)
    {
        case DIR_UP:
            c->sprite_index = 2;
            tile_x = (rand() % ROOM_TILE_SIZE_X);
            tile_y = -1;
            break;
        case DIR_RIGHT:
            c->sprite_index = 3;
            tile_x = ROOM_TILE_SIZE_X;
            tile_y = (rand() % ROOM_TILE_SIZE_Y);
            break;
        case DIR_DOWN:
            c->sprite_index = 0;
            tile_x = (rand() % ROOM_TILE_SIZE_X);
            tile_y = ROOM_TILE_SIZE_Y;
            break;
        case DIR_LEFT:
            c->sprite_index = 1;
            tile_x = -1;
            tile_y = (rand() % ROOM_TILE_SIZE_Y);
            break;
    }

    Rect rp = level_get_tile_rect(tile_x, tile_y);

    c->phys.pos.x = rp.x;
    c->phys.pos.y = rp.y;

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;

    c->spawn.x = c->phys.pos.x;
    c->spawn.y = c->phys.pos.y;
}

static void add_to_random_tile(Creature* c, Room* room)
{
    int tile_x = 0;
    int tile_y = 0;

    for(;;)
    {
        tile_x = (rand() % ROOM_TILE_SIZE_X);
        tile_y = (rand() % ROOM_TILE_SIZE_Y);

        if(level_get_tile_type(room, tile_x, tile_y) == TILE_FLOOR)
        {
            Rect rp = level_get_tile_rect(tile_x, tile_y);
            c->phys.pos.x = rp.x;
            c->phys.pos.y = rp.y;

            RectXY rxy = {0};
            rect_to_rectxy(&room_area, &rxy);
            break;
        }
    }

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;
    c->spawn.x = c->phys.pos.x;
    c->spawn.y = c->phys.pos.y;

    c->sprite_index = DIR_DOWN;
}

Creature* creature_add(Room* room, CreatureType type, Creature* creature)
{
    Creature c = {0};

    if(creature != NULL)
    {
        memcpy(&c, creature, sizeof(Creature));
        room = level_get_room_by_index(&level, c.curr_room);
        if(!room) return NULL;
    }
    else
    {
        if(room == NULL) return NULL;
        c.curr_room = room->index;

        c.id = get_id();
        c.type = type;

        switch(c.type)
        {
            case CREATURE_TYPE_SLUG:
            {
                add_to_random_tile(&c, room);
            } break;
            case CREATURE_TYPE_CLINGER:
            {
                add_to_random_wall_tile(&c);
            }
        }
    }

    switch(c.type)
    {
        case CREATURE_TYPE_SLUG:
        {
            c.phys.speed = 300.0;
            c.image = creature_image_slug;
            c.act_time_min = 0.5;
            c.act_time_max = 1.0;
            c.phys.mass = 1.0;
        } break;
        case CREATURE_TYPE_CLINGER:
        {
            c.phys.speed = 500.0;
            c.image = creature_image_clinger;
            c.act_time_min = 0.2;
            c.act_time_max = 0.4;
            c.phys.mass = 100.0; // so it doesn't slide when hit
        }
    }

    memcpy(&c.phys.target_pos, &c.phys.pos, sizeof(Vector2f));

    c.color = room->color;
    c.phys.radius = 8.0;
    c.phys.coffset.x = 0;
    c.phys.coffset.y = 0;
    c.hp_max = 3.0;
    c.damage = 1;
    c.painful_touch = true;

    if(creature == NULL)
    {
        c.hp = c.hp_max;
    }

    ai_init_action(&c);

    list_add(clist, (void*)&c);

    return &creatures[clist->count-1];
}

void creature_update(Creature* c, float dt)
{
    if(c->dead)
        return;

    if(!is_any_player_room(c->curr_room))
        return;

    switch(c->type)
    {
        case CREATURE_TYPE_SLUG:
            creature_update_slug(c,dt);
            break;
        case CREATURE_TYPE_CLINGER:
            creature_update_clinger(c,dt);
            break;
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

#if 0
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
            printf("spawn tile: %d, %d (%d)\n", c->spawn_tile_x, c->spawn_tile_y, level_get_tile_type(room, c->spawn_tile_x, c->spawn_tile_y));
        }
    }
#endif

}

void creature_lerp(Creature* c, float dt)
{
    if(c->dead) return;

    c->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (c->lerp_t / tick_time);


    Vector2f lp = lerp2f(&c->server_state_prior.pos, &c->server_state_target.pos, t);
    c->phys.pos.x = lp.x;
    c->phys.pos.y = lp.y;

    // printf("%.1f | %.1f -> %.1f  =  %.1f\n", t, c->server_state_prior.pos.x, c->server_state_target.pos.x, lp.x);
    // creature_update_hit_box(c);
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

void creature_handle_collision(Creature* c, Entity* e)
{
    switch(e->type)
    {
        case ENTITY_TYPE_CREATURE:
        {
            Creature* c2 = (Creature*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&c->phys,&c2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&c->phys, &c2->phys,&ci);
            }
        } break;
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

uint16_t creature_get_count()
{
    return (uint16_t)clist->count;
}

uint16_t creature_get_room_count(uint8_t room_index)
{
    uint16_t count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].curr_room == room_index)
            count++;
    }
    return count;
}

static Player* get_nearest_player(Vector2f* pt)
{
    float min_dist = 10000.0;
    int min_index = 0;

    int num_players = player_get_active_count();
    for(int i = 0; i < num_players; ++i)
    {
        Player* p = &players[i];
        float d = dist(pt->x,pt->y, p->phys.pos.x, p->phys.pos.y);
        if(d < min_dist)
        {
            min_dist = d;
            min_index = i;
        }
    }

    return &players[min_index];
}

static void creature_update_slug(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        if(ai_flip_coin())
        {
            ai_random_walk(c);
        }
    }
}
static void creature_fire_projectile(Creature* c, Dir dir)
{
    if(dir == DIR_UP)
        projectile_add(&c->phys, c->curr_room, 90.0, 1.0, 1.0,false);
    else if(dir == DIR_RIGHT)
        projectile_add(&c->phys, c->curr_room, 0.0, 1.0, 1.0,false);
    else if(dir == DIR_DOWN)
        projectile_add(&c->phys, c->curr_room, 270.0, 1.0, 1.0,false);
    else if(dir == DIR_LEFT)
        projectile_add(&c->phys, c->curr_room, 180.0, 1.0, 1.0,false);
}

static void creature_update_clinger(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        Player* p = get_nearest_player(&c->phys.pos);

        int x0 = room_area.x - room_area.w/2.0;
        int y0 = room_area.y - room_area.h/2.0;

        float d = dist(p->phys.pos.x, p->phys.pos.y, c->phys.pos.x, c->phys.pos.y);
        bool horiz = (c->spawn_tile_y == -1 || c->spawn_tile_y == ROOM_TILE_SIZE_Y);

        if(d > 160.0)
        {
            // far from player, slow movement, move randomly
            c->phys.speed = 100.0;
            c->act_time_min = 0.5;
            c->act_time_max = 1.0;

            if(ai_flip_coin())
            {
                ai_walk_dir(c,horiz ? DIR_LEFT : DIR_UP);
            }
            else
            {
                ai_walk_dir(c,horiz ? DIR_RIGHT : DIR_DOWN);
            }
            return;
        }

        // fast movement
        c->phys.speed = 400.0;


        float delta_x = p->phys.pos.x - c->phys.pos.x;
        float delta_y = p->phys.pos.y - c->phys.pos.y;

        if((horiz && ABS(delta_x) < 16.0) || (!horiz && ABS(delta_y) < 16.0))
        {
            c->act_time_min = 1.00;
            c->act_time_max = 2.00;

            // fire
            ai_stop_moving(c);
            if(horiz)
                creature_fire_projectile(c, c->spawn_tile_y == -1 ? DIR_DOWN : DIR_UP);
            else
                creature_fire_projectile(c, c->spawn_tile_x == -1 ? DIR_RIGHT : DIR_LEFT);
        }
        else
        {
            c->act_time_min = 0.05;
            c->act_time_max = 0.10;

            // move
            if(horiz)
            {
                if(delta_x < 0.0)
                    ai_walk_dir(c,DIR_LEFT);
                else
                    ai_walk_dir(c,DIR_RIGHT);
            }
            else
            {
                if(delta_y < 0.0)
                    ai_walk_dir(c,DIR_UP);
                else
                    ai_walk_dir(c,DIR_DOWN);
            }
        }
    }
    return;
}
