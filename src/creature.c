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
#include "ai.h"

Creature prior_creatures[MAX_CREATURES];
Creature creatures[MAX_CREATURES];
glist* clist = NULL;

static int creature_image_slug;
static int creature_image_clinger;
static int creature_image_geizer;
static int creature_image_floater;

static void creature_update_slug(Creature* c, float dt);
static void creature_update_clinger(Creature* c, float dt);
static void creature_update_geizer(Creature* c, float dt);
static void creature_update_floater(Creature* c, float dt);

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
    creature_image_geizer = gfx_load_image("src/img/creature_geizer.png", false, false, 32, 64);
    creature_image_floater = gfx_load_image("src/img/creature_floater.png", false, false, 16, 16);
}

const char* creature_type_name(CreatureType type)
{
    switch(type)
    {
        case CREATURE_TYPE_SLUG:
            return "Slug";
        case CREATURE_TYPE_CLINGER:
            return "Clinger";
        case CREATURE_TYPE_GEIZER:
            return "Geizer";
        case CREATURE_TYPE_FLOATER:
            return "Floater";
        default:
            return "???";
    }
}

int creature_get_image(CreatureType type)
{
    switch(type)
    {
        case CREATURE_TYPE_SLUG:
            return creature_image_slug;
        case CREATURE_TYPE_CLINGER:
            return creature_image_clinger;
        case CREATURE_TYPE_GEIZER:
            return creature_image_geizer;
        case CREATURE_TYPE_FLOATER:
            return creature_image_floater;
        default:
            return -1;
    }
}


void creature_init_props(Creature* c)
{
    c->image = creature_get_image(c->type);
    c->phys.height = gfx_images[c->image].element_height;
    switch(c->type)
    {
        case CREATURE_TYPE_SLUG:
        {
            c->phys.speed = 20.0;
            // c->image = creature_image_slug;
            c->act_time_min = 0.5;
            c->act_time_max = 1.0;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            // c->phys.height = gfx_images[creature_image_slug].element_height;
            c->phys.hp_max = 3.0;
            c->painful_touch = true;
            c->xp = 160;
        } break;
        case CREATURE_TYPE_CLINGER:
        {
            c->phys.speed = 50.0;
            // c->image = creature_image_clinger;
            c->act_time_min = 0.2;
            c->act_time_max = 0.4;
            c->phys.mass = 10.0;
            c->phys.base_friction = 10.0;
            c->phys.height = gfx_images[creature_image_clinger].element_height;
            c->phys.hp_max = 5.0;
            // c->proj_type = PROJECTILE_TYPE_CREATURE_CLINGER;
            c->painful_touch = false;
            c->xp = 60;
        } break;
        case CREATURE_TYPE_GEIZER:
        {
            c->phys.speed = 0.0;
            c->image = creature_image_geizer;
            c->act_time_min = 3.0;
            c->act_time_max = 5.0;
            c->phys.mass = 1000.0; // so it doesn't slide when hit
            c->phys.base_friction = 50.0;
            // c->phys.height = gfx_images[creature_image_geizer].element_height;
            c->phys.hp_max = 10.0;
            c->proj_type = PROJECTILE_TYPE_CREATURE_GENERIC;
            c->painful_touch = false;
            c->xp = 60;
        } break;
        case CREATURE_TYPE_FLOATER:
        {
            c->phys.speed = 20.0;
            // c->image = creature_image_floater;
            c->act_time_min = 0.5;
            c->act_time_max = 1.0;
            c->phys.mass = 2.0;
            c->phys.base_friction = 10.0;
            // c->phys.height = gfx_images[creature_image_floater].element_height;
            c->phys.hp_max = 3.0;
            c->proj_type = PROJECTILE_TYPE_CREATURE_GENERIC;
            c->painful_touch = true;
            c->xp = 60;
        } break;
    }

    c->phys.speed_factor = 1.0;
    c->phys.radius = 8.0;
    c->phys.coffset.x = 0;
    c->phys.coffset.y = 0;
    c->damage = 1;
}

// Vector2f creature_get_collision(CreatureType type, float posx, float posy)
// {

// }

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
            tile_x = (rand() % (ROOM_TILE_SIZE_X-1));
            if(tile_x >= ROOM_TILE_SIZE_X/2.0)tile_x+=1;
            tile_y = -1;
            break;
        case DIR_RIGHT:
            c->sprite_index = 3;
            tile_x = ROOM_TILE_SIZE_X;
            tile_y = (rand() % (ROOM_TILE_SIZE_Y-1));
            if(tile_y >= ROOM_TILE_SIZE_Y/2.0)tile_y+=1;
            break;
        case DIR_DOWN:
            c->sprite_index = 0;
            tile_x = (rand() % (ROOM_TILE_SIZE_X-1));
            if(tile_x >= ROOM_TILE_SIZE_X/2.0)tile_x+=1;
            tile_y = ROOM_TILE_SIZE_Y;
            break;
        case DIR_LEFT:
            c->sprite_index = 1;
            tile_x = -1;
            tile_y = (rand() % (ROOM_TILE_SIZE_Y-1));
            if(tile_y >= ROOM_TILE_SIZE_Y/2.0)tile_y+=1;
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

static void add_to_wall_tile(Creature* c, int tile_x, int tile_y)
{
    Dir wall = DIR_NONE;
    if(tile_x == -1)
    {
        wall = DIR_LEFT;
    }
    else if(tile_x == ROOM_TILE_SIZE_X)
    {
        wall = DIR_RIGHT;
    }
    else if(tile_y == -1)
    {
        wall = DIR_UP;
    }
    else if(tile_y == ROOM_TILE_SIZE_Y)
    {
        wall = DIR_DOWN;
    }
    // printf("wall: %d\n", wall);

    switch(wall)
    {
        case DIR_UP:
            c->sprite_index = 2;
            break;
        case DIR_RIGHT:
            c->sprite_index = 3;
            break;
        case DIR_DOWN:
            c->sprite_index = 0;
            break;
        case DIR_LEFT:
            c->sprite_index = 1;
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
}

static void add_to_tile(Creature* c, int tile_x, int tile_y)
{
    Rect rp = level_get_tile_rect(tile_x, tile_y);
    c->phys.pos.x = rp.x;
    c->phys.pos.y = rp.y;

    RectXY rxy = {0};
    rect_to_rectxy(&room_area, &rxy);

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;
    c->spawn.x = c->phys.pos.x;
    c->spawn.y = c->phys.pos.y;
}

// specify first 3 args, or the last
Creature* creature_add(Room* room, CreatureType type, Vector2i* tile, Creature* creature)
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

        if(tile)
        {
            switch(c.type)
            {
                case CREATURE_TYPE_SLUG:
                {
                    add_to_tile(&c, tile->x, tile->y);
                    c.sprite_index = DIR_DOWN;
                } break;
                case CREATURE_TYPE_CLINGER:
                {
                    add_to_wall_tile(&c, tile->x, tile->y);
                } break;
                case CREATURE_TYPE_GEIZER:
                {
                    add_to_tile(&c, tile->x, tile->y);
                    c.sprite_index = 0;
                } break;
                case CREATURE_TYPE_FLOATER:
                {
                    add_to_tile(&c, tile->x, tile->y);
                    c.sprite_index = 0;
                } break;
            }
        }
        else
        {
            switch(c.type)
            {
                case CREATURE_TYPE_SLUG:
                {
                    add_to_random_tile(&c, room);
                    c.sprite_index = DIR_DOWN;
                } break;
                case CREATURE_TYPE_CLINGER:
                {
                    add_to_random_wall_tile(&c);
                } break;
                case CREATURE_TYPE_GEIZER:
                {
                    add_to_random_tile(&c,room);
                    c.sprite_index = 0;
                } break;
                case CREATURE_TYPE_FLOATER:
                {
                    add_to_random_tile(&c,room);
                    c.sprite_index = 0;
                } break;
            }
        }

    }

    c.color = COLOR_TINT_NONE;
    creature_init_props(&c);

    if(creature == NULL)
    {
        c.phys.hp = c.phys.hp_max;
    }

    ai_init_action(&c);

    list_add(clist, (void*)&c);

    return &creatures[clist->count-1];
}

void creature_update(Creature* c, float dt)
{
    if(c->phys.dead)
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
        case CREATURE_TYPE_GEIZER:
            creature_update_geizer(c,dt);
            break;
        case CREATURE_TYPE_FLOATER:
            creature_update_floater(c,dt);
            break;
    }


    float speed = c->phys.speed*c->phys.speed_factor;

    bool moving = (c->h != 0.0 || c->v != 0.0);

    float h_speed = speed*c->h;
    float v_speed = speed*c->v;

    float vel_magn = magn(c->phys.vel);

    if(vel_magn < speed)
    {
        c->phys.vel.x += dt*h_speed;
        c->phys.vel.y += dt*v_speed;
    }

    c->color = c->damaged ? COLOR_RED : gfx_blend_colors(COLOR_BLUE, COLOR_TINT_NONE, c->phys.speed_factor);

    if(c->damaged)
    {
        c->damaged_time += dt;
        if(c->damaged_time >= DAMAGED_TIME_MAX)
            c->damaged = false;
    }

    if(!moving || vel_magn > speed)
        phys_apply_friction(&c->phys,c->phys.base_friction,dt);

    c->phys.pos.x += dt*c->phys.vel.x;
    c->phys.pos.y += dt*c->phys.vel.y;

    // update hit box
    c->hitbox.x = c->phys.pos.x;
    c->hitbox.y = c->phys.pos.y;

    c->hitbox.w = 16;
    c->hitbox.h = 16;

}

void creature_lerp(Creature* c, float dt)
{
    if(c->phys.dead) return;

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
        if(c->phys.dead)
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
        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&c->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&c->phys, &p2->phys,&ci);
            }
        } break;
    }
}

void creature_draw(Creature* c, bool batch)
{
    if(c->curr_room != player->curr_room)
        return;

    if(c->phys.dead) return;

    if(batch)
    {
        gfx_sprite_batch_add(c->image, c->sprite_index, c->phys.pos.x, c->phys.pos.y, c->color, false, 1.0, 0.0, 1.0, false, false, false);
    }
    else
    {
        gfx_draw_image(c->image, c->sprite_index, c->phys.pos.x, c->phys.pos.y, c->color, 1.0, 0.0, 1.0, false, true);
    }

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
        creature_draw(c, false);
    }
}

void creature_die(Creature* c)
{
    c->phys.dead = true;

    // player_add_xp(player, c->xp);
    Room* room = level_get_room_by_index(&level, c->curr_room);
    room->xp += c->xp;

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
    c->phys.hp -= damage;

    c->damaged = true;
    c->damaged_time = 0.0;

    if(c->phys.hp <= 0.0)
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

static Player* get_nearest_player(float x, float y)
{
    float min_dist = 10000.0;
    int min_index = 0;

    int num_players = player_get_active_count();
    for(int i = 0; i < num_players; ++i)
    {
        Player* p = &players[i];
        float d = dist(x,y, p->phys.pos.x, p->phys.pos.y);
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
static void creature_fire_projectile_dir(Creature* c, Dir dir)
{
    if(dir == DIR_UP)
        projectile_add_type(&c->phys, c->curr_room, c->proj_type,90.0, 1.0, 1.0,false);
    else if(dir == DIR_RIGHT)
        projectile_add_type(&c->phys, c->curr_room, c->proj_type,0.0, 1.0, 1.0,false);
    else if(dir == DIR_DOWN)
        projectile_add_type(&c->phys, c->curr_room, c->proj_type,270.0, 1.0, 1.0,false);
    else if(dir == DIR_LEFT)
        projectile_add_type(&c->phys, c->curr_room, c->proj_type,180.0, 1.0, 1.0,false);
}

static void creature_update_clinger(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);

        int x0 = room_area.x - room_area.w/2.0;
        int y0 = room_area.y - room_area.h/2.0;

        float d = dist(p->phys.pos.x, p->phys.pos.y, c->phys.pos.x, c->phys.pos.y);
        bool horiz = (c->spawn_tile_y == -1 || c->spawn_tile_y == ROOM_TILE_SIZE_Y);

        if(d > 160.0)
        {
            // far from player, slow movement, move randomly
            c->phys.speed = 10.0;
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
        c->phys.speed = 50.0;


        float delta_x = p->phys.pos.x - c->phys.pos.x;
        float delta_y = p->phys.pos.y - c->phys.pos.y;

        if((horiz && ABS(delta_x) < 16.0) || (!horiz && ABS(delta_y) < 16.0))
        {
            c->act_time_min = 1.00;
            c->act_time_max = 2.00;

            // fire
            ai_stop_moving(c);
            if(horiz)
                creature_fire_projectile_dir(c, c->spawn_tile_y == -1 ? DIR_DOWN : DIR_UP);
            else
                creature_fire_projectile_dir(c, c->spawn_tile_x == -1 ? DIR_RIGHT : DIR_LEFT);
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

static void creature_update_geizer(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        int n_orbs = 5 + (rand() % 5);

        for(int i = 0; i < n_orbs; ++i)
        {
            int angle = rand() % 360;
            projectile_add_type(&c->phys, c->curr_room, c->proj_type, angle, 1.0, 1.0,false);
        }

    }
}

static void creature_update_floater(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    c->phys.pos.z = 10.0 + 5*sinf(PI*(c->action_counter/c->action_counter_max));

    if(act)
    {
        if(ai_flip_coin())
        {
            ai_random_walk(c);
        }
    }
}
