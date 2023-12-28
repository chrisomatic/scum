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

Creature prior_creatures[MAX_CREATURES] = {0};
Creature creatures[MAX_CREATURES] = {0};
glist* clist = NULL;

#define PROJ_COLOR 0x00FF5050

static int creature_image_slug;
static int creature_image_clinger;
static int creature_image_geizer;
static int creature_image_floater;
static int creature_image_buzzer;
static int creature_image_totem_red;
static int creature_image_totem_blue;
static int creature_image_shambler;
static int creature_image_spiked_slug;

static void creature_update_slug(Creature* c, float dt);
static void creature_update_clinger(Creature* c, float dt);
static void creature_update_geizer(Creature* c, float dt);
static void creature_update_floater(Creature* c, float dt);
static void creature_update_buzzer(Creature* c, float dt);
static void creature_update_totem_red(Creature* c, float dt);
static void creature_update_totem_blue(Creature* c, float dt);
static void creature_update_shambler(Creature* c, float dt);
static void creature_update_spiked_slug(Creature* c, float dt);

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
    creature_image_buzzer  = gfx_load_image("src/img/creature_buzzer.png", false, false, 32, 32);
    creature_image_totem_red  = gfx_load_image("src/img/creature_totem_red.png", false, false, 32, 64);
    creature_image_totem_blue  = gfx_load_image("src/img/creature_totem_blue.png", false, false, 32, 64);
    creature_image_shambler = gfx_load_image("src/img/creature_shambler.png", false, false, 32, 64);
    creature_image_spiked_slug = gfx_load_image("src/img/creature_spiked_slug.png", false, false, 32, 32);
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
        case CREATURE_TYPE_BUZZER:
            return "Buzzer";
        case CREATURE_TYPE_TOTEM_RED:
            return "Totem Red";
        case CREATURE_TYPE_TOTEM_BLUE:
            return "Totem Blue";
        case CREATURE_TYPE_SHAMBLER:
            return "Shambler";
        case CREATURE_TYPE_SPIKED_SLUG:
            return "Spiked Slug";
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
        case CREATURE_TYPE_BUZZER:
            return creature_image_buzzer;
        case CREATURE_TYPE_TOTEM_RED:
            return creature_image_totem_red;
        case CREATURE_TYPE_TOTEM_BLUE:
            return creature_image_totem_blue;
        case CREATURE_TYPE_SHAMBLER:
            return creature_image_shambler;
        case CREATURE_TYPE_SPIKED_SLUG:
            return creature_image_spiked_slug;
        default:
            return -1;
    }
}

ProjectileType creature_get_projectile_type(Creature* c)
{
    ProjectileType pt = PROJECTILE_TYPE_CREATURE_GENERIC;
    switch(c->type)
    {
        case CREATURE_TYPE_CLINGER:
            pt = PROJECTILE_TYPE_CREATURE_CLINGER;
            break;
        case CREATURE_TYPE_GEIZER:
            pt = PROJECTILE_TYPE_CREATURE_GEIZER;
            break;
        case CREATURE_TYPE_TOTEM_BLUE:
            pt = PROJECTILE_TYPE_CREATURE_TOTEM_BLUE;
            break;
        case CREATURE_TYPE_TOTEM_RED:
            break;
        default:
            break;
    }
    return pt;
}

void creature_init_props(Creature* c)
{
    c->image = creature_get_image(c->type);
    c->phys.width  = gfx_images[c->image].visible_rects[0].w;
    c->phys.length = gfx_images[c->image].visible_rects[0].w;
    c->phys.height = gfx_images[c->image].visible_rects[0].h;
    c->phys.radius = c->phys.width / 2.0;

    switch(c->type)
    {
        case CREATURE_TYPE_SLUG:
        {
            c->phys.speed = 30.0;
            c->act_time_min = 1.5;
            c->act_time_max = 3.0;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 15;
        } break;
        case CREATURE_TYPE_CLINGER:
        {
            c->phys.speed = 50.0;
            c->act_time_min = 0.2;
            c->act_time_max = 0.4;
            c->phys.mass = 1000.0;
            c->phys.base_friction = 0.0;
            c->phys.hp_max = 5.0;
            c->painful_touch = false;
            c->windup_max = 0.5;
            c->xp = 25;
        } break;
        case CREATURE_TYPE_GEIZER:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 3.0;
            c->act_time_max = 5.0;
            c->phys.mass = 1000.0; // so it doesn't slide when hit
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 10.0;
            c->painful_touch = false;
            c->windup_max = 0.5;
            c->xp = 35;
        } break;
        case CREATURE_TYPE_FLOATER:
        {
            c->phys.speed = 20.0;
            c->act_time_min = 0.2;
            c->act_time_max = 0.5;
            c->phys.mass = 2.0;
            c->phys.base_friction = 1.0;
            c->phys.hp_max = 3.0;
            c->phys.floating = true;
            c->painful_touch = true;
            c->xp = 10;
        } break;
        case CREATURE_TYPE_BUZZER:
        {
            c->phys.speed = 30.0;
            c->act_time_min = 0.50;
            c->act_time_max = 1.00;
            c->phys.mass = 1.0;
            c->phys.base_friction = 10.0;
            c->phys.hp_max = 10.0;
            c->phys.floating = true;
            c->painful_touch = true;
            c->windup_max = 0.5;
            c->xp = 40;
        } break;
        case CREATURE_TYPE_TOTEM_RED:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 2.00;
            c->act_time_max = 3.00;
            c->phys.mass = 1000.0;
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 128;
            c->phys.floating = false;
            c->painful_touch = false;
            c->invincible = true;
            c->windup_max = 0.5;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_TOTEM_BLUE:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 4.00;
            c->act_time_max = 6.00;
            c->phys.mass = 1000.0;
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 128;
            c->phys.floating = false;
            c->proj_type = PROJECTILE_TYPE_CREATURE_TOTEM_BLUE;
            c->painful_touch = false;
            c->invincible = true;
            c->windup_max = 0.5;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_SHAMBLER:
        {
            c->phys.speed = 50.0;
            c->act_time_min = 0.50;
            c->act_time_max = 1.00;
            c->phys.mass = 3.0;
            c->phys.base_friction = 15.0;
            c->phys.hp_max = 100.0;
            c->phys.floating = true;
            c->painful_touch = true;
            c->windup_max = 0.5;
            c->xp = 300;
        } break;
        case CREATURE_TYPE_SPIKED_SLUG:
        {
            c->phys.speed = 30.0;
            c->act_time_min = 0.3;
            c->act_time_max = 1.0;
            c->phys.mass = 1.0;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 10.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 25;
        } break;
    }

    if(c->phys.crawling)
    {
        c->phys.length = c->phys.height;
        c->phys.height = 20;
    }

    c->phys.speed_factor = 1.0;
    c->phys.coffset.x = 0;
    c->phys.coffset.y = 0;
    c->damage = 1;
}

void creature_clear_all()
{
    list_clear(clist);
}

void creature_clear_room(uint8_t room_index)
{
    for(int i = clist->count; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        if(c->curr_room == room_index)
        {
            list_remove(clist, i);
        }
    }
}

void creature_kill_all()
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        creature_die(&creatures[i]);
        list_remove(clist, i);
    }
}

void creature_kill_room(uint8_t room_index)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        if(creatures[i].curr_room == room_index)
        {
            creature_die(&creatures[i]);
            list_remove(clist, i);
        }
    }
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
    c->phys.pos.z = 0.0;

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;
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
        c.target_tile.x = -1;
        c.target_tile.y = -1;

        switch(c.type)
        {
            case CREATURE_TYPE_SLUG:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);

                c.sprite_index = DIR_DOWN;
            } break;
            case CREATURE_TYPE_CLINGER:
            {
                if(tile) add_to_wall_tile(&c, tile->x, tile->y);
                else     add_to_random_wall_tile(&c);
            } break;
            case CREATURE_TYPE_GEIZER:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
            case CREATURE_TYPE_FLOATER:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
            case CREATURE_TYPE_BUZZER:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
            case CREATURE_TYPE_TOTEM_RED:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
            case CREATURE_TYPE_TOTEM_BLUE:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
            case CREATURE_TYPE_SHAMBLER:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
            case CREATURE_TYPE_SPIKED_SLUG:
            {
                if(tile) add_to_tile(&c, tile->x, tile->y);
                else     add_to_random_tile(&c, room);
                c.sprite_index = 0;
            } break;
        }
    }

    c.base_color = COLOR_TINT_NONE;
    c.color = c.base_color;
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

    phys_add_circular_time(&c->phys, dt);

    c->curr_tile = level_get_room_coords_by_pos(CPOSX(c->phys), CPOSY(c->phys));

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
        case CREATURE_TYPE_BUZZER:
            creature_update_buzzer(c,dt);
            break;
        case CREATURE_TYPE_TOTEM_RED:
            creature_update_totem_red(c,dt);
            break;
        case CREATURE_TYPE_TOTEM_BLUE:
            creature_update_totem_blue(c,dt);
            break;
        case CREATURE_TYPE_SHAMBLER:
            creature_update_shambler(c,dt);
            break;
        case CREATURE_TYPE_SPIKED_SLUG:
            creature_update_spiked_slug(c,dt);
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
    else
    {
        // cap at speed
        normalize3f(&c->phys.vel);
        c->phys.vel.x *= speed;
        c->phys.vel.y *= speed;
    }

    // determine color
    c->color = gfx_blend_colors(COLOR_BLUE, c->base_color, c->phys.speed_factor); // cold effect
    c->color = c->windup && ((int)(c->phys.circular_dt * 100) % 2 == 0)? gfx_blend_colors(COLOR_YELLOW,c->color, 0.1) : c->color; // windup indication
    c->color = c->damaged ? COLOR_RED : c->color; // damaged indication

    if(c->damaged)
    {
        c->damaged_time += dt;
        if(c->damaged_time >= DAMAGED_TIME_MAX)
            c->damaged = false;
    }

    if(!moving)
    {
        phys_apply_friction(&c->phys,c->phys.base_friction,dt);
    }

    c->phys.pos.x += dt*c->phys.vel.x;
    c->phys.pos.y += dt*c->phys.vel.y;
    phys_apply_gravity(&c->phys, 1.0, dt);

    Rect r = RECT(c->phys.pos.x, c->phys.pos.y, 1, 1);
    Vector2f adj = limit_rect_pos(&room_area, &r);
    c->phys.pos.x += adj.x;
    c->phys.pos.y += adj.y;
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

void creature_draw(Creature* c)
{
    if(c->curr_room != player->curr_room)
        return;

    if(c->phys.dead) return;

    float y = c->phys.pos.y - 0.5*c->phys.pos.z - c->phys.width/1.5;
    gfx_sprite_batch_add(c->image, c->sprite_index, c->phys.pos.x, y, c->color, false, 1.0, 0.0, 1.0, false, false, false);
}


void creature_die(Creature* c)
{
    c->phys.dead = true;

    // player_add_xp(player, c->xp);
    Room* room = level_get_room_by_index(&level, c->curr_room);
    if(room == NULL)
    {
        LOGE("room is null");
        printf("%u\n", c->curr_room);
        return;
    }

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
    if(c->invincible)
        return;

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

bool creature_is_on_tile(Room* room, int tile_x, int tile_y)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        if(c->curr_room != room->index)
            continue;

        // simple check
        if(c->curr_tile.x == tile_x && c->curr_tile.y == tile_y)
            return true;
    }
    return false;
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
    /*
    if(ai_has_target(c))
    {
        bool at_target = ai_move_to_target(c,dt);
        if(at_target)
        {
            ai_clear_target(c);
        }
    }
    else
    {
        Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
        c->target_tile = level_get_room_coords_by_pos(p->phys.pos.x, p->phys.pos.y);

        //float x0 = room_area.x - room_area.w/2.0;
        //float y0 = room_area.y - room_area.h/2.0;
        //Vector2f pos = level_get_pos_by_room_coords(c->target_tile.x, c->target_tile.y);
    }
    */

#if 1
        bool act = ai_update_action(c, dt);

        if(act)
        {
            //Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
            //c->target_tile = level_get_room_coords_by_pos(p->phys.pos.x, p->phys.pos.y);

            ai_stop_imm(c);

            if(ai_flip_coin())
            {
                ai_random_walk(c);
            }
        }
#endif
}
static void creature_update_spiked_slug(Creature* c, float dt)
{

    Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
    Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
    normalize(&v);

    float d = 0.0;
    Vector2f dir = {0.0, 0.0};

    switch(c->sprite_index)
    {
        case DIR_UP: {
            dir.x = 0.0;
            dir.y = -1.0;
            d = vec_dot(v, dir);
        } break;
        case DIR_RIGHT: {
            dir.x = 1.0;
            dir.y = 0.0;
            d = vec_dot(v, dir);
        } break;
        case DIR_DOWN: {
            dir.x = 0.0;
            dir.y = 1.0;
            d = vec_dot(v, dir);
        } break;
        case DIR_LEFT: {
            dir.x = -1.0;
            dir.y = 0.0;
            d = vec_dot(v, dir);
        } break;
    }

    if(d >= 0.95 && d <= 1.00)
    {
        // in line of sight, charge
        c->h = dir.x * 10.0*c->phys.speed;
        c->v = dir.y * 10.0*c->phys.speed;
    }
    else
    {
        bool act = ai_update_action(c, dt);

        if(act)
        {
            //Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
            //c->target_tile = level_get_room_coords_by_pos(p->phys.pos.x, p->phys.pos.y);

            ai_stop_imm(c);

            if(ai_flip_coin())
            {
                ai_random_walk_cardinal(c);
            }
        }
    }
}
static void creature_fire_projectile(Creature* c, float angle, uint32_t color)
{
    ProjectileType pt = creature_get_projectile_type(c);
    ProjectileDef def = projectile_lookup[pt];
    ProjectileSpawn spawn = projectile_spawn[pt];

    projectile_add(&c->phys, c->curr_room, &def, &spawn, color, angle, false);
}

static void creature_update_clinger(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);

    float d = dist(p->phys.pos.x, p->phys.pos.y, c->phys.pos.x, c->phys.pos.y);
    bool horiz = (c->spawn_tile_y == -1 || c->spawn_tile_y == ROOM_TILE_SIZE_Y);

    if(d > 160.0)
    {
        if(act)
        {
            // far from player, slow movement, move randomly
            c->phys.speed = 10.0;
            c->act_time_min = 0.5;
            c->act_time_max = 1.0;

            if(ai_flip_coin())
            {
                //ai_walk_dir(c,horiz ? DIR_LEFT : DIR_UP);
                ai_move_imm(c, horiz ? DIR_LEFT : DIR_UP, c->phys.speed);
            }
            else
            {
                ai_move_imm(c, horiz ? DIR_RIGHT : DIR_DOWN, c->phys.speed);
            }
        }
        return;
    }

    // player is close

    // fast movement
    c->phys.speed = 100.0;

    float delta_x = p->phys.pos.x - c->phys.pos.x;
    float delta_y = p->phys.pos.y - c->phys.pos.y;

    if((horiz && ABS(delta_x) < 16.0) || (!horiz && ABS(delta_y) < 16.0))
    {
        c->act_time_min = 1.00;
        c->act_time_max = 1.00;

        c->phys.vel.x = 0.0;
        c->phys.vel.y = 0.0;

        if(act)
        {
            // fire
            if(horiz)
                creature_fire_projectile(c, dir_to_angle_deg(c->spawn_tile_y == -1 ? DIR_DOWN : DIR_UP), PROJ_COLOR);
            else
                creature_fire_projectile(c, dir_to_angle_deg(c->spawn_tile_x == -1 ? DIR_RIGHT : DIR_LEFT), PROJ_COLOR);
        }
    }
    else
    {
        c->act_time_min = 0.05;
        c->act_time_max = 0.10;

        if(act)
        {
            // move
            if(horiz)
            {
                ai_move_imm(c, delta_x < 0.0 ? DIR_LEFT : DIR_RIGHT, c->phys.speed);
            }
            else
            {
                ai_move_imm(c, delta_y < 0.0 ? DIR_UP : DIR_DOWN, c->phys.speed);
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
            creature_fire_projectile(c, angle, PROJ_COLOR);
        }
    }
}

static void creature_update_floater(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    c->phys.pos.z = c->phys.height/2.0 + 10.0 + 3*sinf(5*c->phys.circular_dt);

    if(act)
    {
        if(ai_flip_coin())
        {
            ai_random_walk(c);
        }
    }
}

static void creature_update_buzzer(Creature* c, float dt)
{
    c->phys.pos.z = c->phys.height/2.0 + 10.0 + 3*sinf(5*c->phys.circular_dt);

    if(c->ai_state == 0)
    {
        // move
        bool act = ai_update_action(c, dt);

        if(act)
        {
            if(ai_rand(4) < 3)
            {
                ai_random_walk(c);
            }
            else
            {
                c->ai_counter = 0.0;
                c->ai_state = 1;
                c->ai_value = 0;
                ai_stop_moving(c);
            }
        }

    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;//RAND_FLOAT(0.2,0.5);

        c->ai_counter += dt;

        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            // fire shots
            Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
            float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y) + RAND_FLOAT(-10.0,10.0);
            creature_fire_projectile(c, angle, PROJ_COLOR);

            c->ai_value++;
            if(c->ai_value >= 3)
            {
                c->ai_state = 0;
            }
        }
    }

}

static void creature_update_totem_red(Creature* c, float dt)
{
    int creature_count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].curr_room == c->curr_room)
        {
            // same room
            if(creatures[i].type != CREATURE_TYPE_TOTEM_RED)
            {
                creature_count++;
            }
        }
            
    }

    if(creature_count == 0)
    {
        // deactivate totem
        creature_die(c);
    }

    if(c->ai_state == 0)
    {
        // move
        bool act = ai_update_action(c, dt);

        if(act)
        {
            c->ai_counter = 0.0;
            c->ai_state = 1;
            c->ai_value = 0;
        }
    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;

        c->ai_counter += dt;

        if(c->windup)
        {
            if(c->ai_counter >= c->windup_max)
            {
                c->ai_counter = 0.0;
                c->windup = false;
            }
        }
        else
        {
            if(c->ai_counter >= c->ai_counter_max)
            {
                c->ai_counter = 0.0;

                // fire shots
                Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
                creature_fire_projectile(c, angle, PROJ_COLOR);

                c->ai_value++;
                if(c->ai_value >= 3)
                {
                    c->ai_state = 0;
                }
            }
        }
    }
}

static void creature_update_totem_blue(Creature* c, float dt)
{
    int creature_count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].curr_room == c->curr_room)
        {
            // same room
            if(!creatures[i].invincible)
            {
                creature_count++;
            }
        }
    }

    if(creature_count == 0)
    {
        // deactivate totem
        creature_die(c);
    }

    if(c->ai_state == 0)
    {
        bool act = ai_update_action(c, dt);

        if(act)
        {
            c->ai_counter = 0.0;
            c->ai_state = 1;
            c->ai_value = 0;
            c->windup = true;
        }
    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;

        c->ai_counter += dt;

        if(c->windup)
        {
            if(c->ai_counter >= c->windup_max)
            {
                c->ai_counter = 0.0;
                c->windup = false;
            }
        }
        else
        {
            if(c->ai_counter >= c->ai_counter_max)
            {
                c->ai_counter = 0.0;

                // fire shots
                Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);

                uint32_t color = 0x003030FF;
                creature_fire_projectile(c, angle, color);
                creature_fire_projectile(c, angle-30, color);
                creature_fire_projectile(c, angle-60, color);
                creature_fire_projectile(c, angle+30, color);
                creature_fire_projectile(c, angle+60, color);

                c->ai_value++;
                c->ai_state = 0;
            }
        }

    }
}

static void creature_update_shambler(Creature* c, float dt)
{
    c->phys.pos.z = 10.0 + 3*sinf(5*c->phys.circular_dt);

    bool low_health = (c->phys.hp < 0.30*c->phys.hp_max);
    if(low_health)
    {
        // insane-mode
        c->phys.speed = 150.0;
        c->base_color = COLOR_RED;
    }

    if(c->ai_state > 0)
    {
        c->ai_counter += dt;
        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            if(c->ai_state == 1)
            {
                // fire 5 shots
                Player* p = get_nearest_player(c->phys.pos.x, c->phys.pos.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
                creature_fire_projectile(c, angle, PROJ_COLOR);

                c->ai_value++;
                if(c->ai_value >= 5)
                {
                    c->ai_state = 0;
                }
            }
            else if(c->ai_state == 2)
            {
                // fire 4 orthogonal shots
                float aoffset = c->ai_value*10.0;
                creature_fire_projectile(c, 0.0 + aoffset, PROJ_COLOR);
                creature_fire_projectile(c, 90.0 + aoffset, PROJ_COLOR);
                creature_fire_projectile(c, 180.0 + aoffset, PROJ_COLOR);
                creature_fire_projectile(c, 270.0 + aoffset, PROJ_COLOR);

                c->ai_value++;
                if(c->ai_value >= 18)
                {
                    c->ai_state = 0;
                }

            }
        }
        return;
    }

    if(c->ai_counter == 0.0)
    {
        c->ai_counter_max = low_health ? RAND_FLOAT(0.5,1.0) : RAND_FLOAT(2.0,5.0);
    }

    c->ai_counter += dt;

    bool act = ai_update_action(c, dt);

    if(act)
    {
        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            int choice = ai_rand(low_health ? 3 : 2);

            if(choice == 0)
            {
                // teleport
                Room* room = level_get_room_by_index(&level, c->curr_room);
                add_to_random_tile(c, room);

                // explode projectiles
                int num_projectiles = (rand() % 24) + 12;
                for(int i = 0; i < num_projectiles; ++i)
                {
                    int angle = rand() % 360;
                    creature_fire_projectile(c, angle, PROJ_COLOR);
                }
            }
            else if(choice == 1)
            {
                c->ai_state = 1;
                c->ai_value = 0;
                c->ai_counter = 0;
                c->ai_counter_max = 0.2;
            }
            else if(choice == 2)
            {
                c->ai_state = 2;
                c->ai_value = 0;
                c->ai_counter = 0;
                c->ai_counter_max = 0.1;
            }
        }
        else
        {
            ai_random_walk(c);
        }

    }
}
