#include "headers.h"
#include "player.h"
#include "creature.h"
#include "projectile.h"
#include "physics.h"
#include "math2d.h"
#include "explosion.h"
#include "status_effects.h"
#include "entity.h"

Entity entities[MAX_ENTITIES] = {0};
int num_entities;

static void entity_update_tile(Entity* e)
{
    float cx = e->phys->collision_rect.x;
    float cy = e->phys->collision_rect.y;
    Vector2i tile_coords = level_get_room_coords_by_pos(cx, cy);
    Room* room = level_get_room_by_index(&level, e->phys->curr_room);
    e->tile = level_get_tile_type(room, tile_coords.x, tile_coords.y);
}

static void add_entity(EntityType type, void* ptr, Physics* phys)
{
    if(type == ENTITY_TYPE_PROJECTILE)
    {
        Projectile* p = (Projectile*)ptr;
        if(p->tts > 0) return;
    }

    Entity* e = &entities[num_entities++];

    e->type = type;
    e->ptr = ptr;
    e->phys = phys;
    e->draw_only = false;

    e->pos.x = phys->pos.x;
    e->pos.y = phys->pos.y;

    if(type == ENTITY_TYPE_WALL_COLUMN)
    {
        e->draw_only = true;
        e->ptr  = NULL;
        e->phys = NULL;
        return;
    }

    if(role == ROLE_CLIENT)
    {
        // need to do this in order to draw shadow and debug stuff properly
        phys_calc_collision_rect(phys);
    }
    entity_update_tile(e);
}

static bool is_player_room(uint8_t room_index)
{
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(!players[i].active) continue;
        if(players[i].phys.curr_room == room_index)
            return true;
    }

    return false;
}

static void sort_entities()
{
    // insertion sort
    int i, j;
    Entity key;

    for (i = 1; i < num_entities; ++i) 
    {
        memcpy(&key, &entities[i], sizeof(Entity));
        j = i - 1;

        while (j >= 0 && entities[j].pos.y > key.pos.y)
        {
            memcpy(&entities[j+1], &entities[j], sizeof(Entity));
            j = j - 1;
        }
        memcpy(&entities[j+1], &key, sizeof(Entity));
    }
}

static void draw_entity_shadow(Physics* phys, EntityType type)
{
    if(phys->falling) return;
    if(type == ENTITY_TYPE_ITEM) return;

    // bool horizontal = (phys->rotation_deg == 0.0 || phys->rotation_deg == 180.0);
    if(phys->crawling && phys->height < 32.0 && phys->pos.z == 0.0) return;

    // float scale = (phys->collision_rect.w/32.0);

    if(phys->vr.w == 0) phys->vr.w = 8; // @HACK: To address projectiles without shadows

    // float scale = (phys->vr.w/32.0 * 0.80);
    float scale = (phys->vr.w / gfx_images[shadow_image].visible_rects[0].w * 0.80) * phys->scale;
    float opacity = 0.6; //RANGE(0.5*(1.0 - (phys->pos.z / 128.0)),0.1,0.5);

    float shadow_x = phys->pos.x;
    float shadow_y = phys->pos.y;

    gfx_sprite_batch_add(shadow_image, 0, shadow_x, shadow_y, COLOR_TINT_NONE, false, scale, 0.0, opacity, false, false, false);
}

void entity_build_all()
{
    num_entities = 0;

    if(!visible_room) return;

    // players
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(!p->active) continue;
        add_entity(ENTITY_TYPE_PLAYER,p, &p->phys);
    }

    // creatures
    uint16_t num_creatures = creature_get_count();
    for(int i = 0; i < num_creatures; ++i)
    {
        Creature* c = &creatures[i];

        if(!is_player_room(c->phys.curr_room)) continue;

        bool ghost = c->phys.dead ? true : false;
        add_entity(ENTITY_TYPE_CREATURE,c, &c->phys);
    }

    // projectiles
    for(int i = 0; i < plist->count; ++i)
    {
        Projectile* p = &projectiles[i];
        add_entity(ENTITY_TYPE_PROJECTILE,p, &p->phys);
    }

    // items
    for(int i = 0; i < item_list->count; ++i)
    {
        Item* pu = &items[i];
        add_entity(ENTITY_TYPE_ITEM,pu, &pu->phys);
    }

    // explosions
    for(int i = 0; i < explosion_list->count; ++i)
    {
        Explosion* ex = &explosions[i];
        add_entity(ENTITY_TYPE_EXPLOSION,ex, &ex->phys);
    }

    // wall columns
    RoomFileData* rdata = &room_list[visible_room->layout];

    for(int _y = 0; _y < ROOM_TILE_SIZE_Y; ++_y)
    {
        for(int _x = 0; _x < ROOM_TILE_SIZE_X; ++_x)
        {
            TileType tt = rdata->tiles[_x][_y];
            if(tt == TILE_BOULDER)
            {
                Physics phys = {0};

                phys.curr_tile.x = _x;
                phys.curr_tile.y = _y;

                Vector2f coords = level_get_pos_by_room_coords(_x, _y);
                phys.pos.x = coords.x;
                phys.pos.y = coords.y - 9.0;

                add_entity(ENTITY_TYPE_WALL_COLUMN, NULL, &phys);
            }
        }
    }
}


void entity_handle_status_effects(float dt)
{
    if(role == ROLE_CLIENT) return;

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        if(!e->phys) continue;
        if(e->phys->dead) continue;

        Physics* phys = e->phys;

        for(int i = phys->status_effects_count - 1; i >= 0; --i)
        {
            StatusEffect* effect = &phys->status_effects[i];

            effect->lifetime += dt;

            if(effect->particles)
            {
                effect->particles->pos.x = phys->pos.x;
                effect->particles->pos.y = phys->pos.y;
            }

            if(effect->lifetime_max > 0.0 && effect->lifetime >= effect->lifetime_max)
            {
                // status effect is done, remove
                effect->func((void*)e, true);
                status_effect_remove(phys, i);
                continue;
            }

            if(effect->periodic)
            {
                if(effect->lifetime >= effect->period * (effect->periods_passed +1))
                {
                    printf("effect->periods_passed: %d (%.2f)\n", effect->periods_passed, effect->lifetime);
                    effect->periods_passed++;
                    effect->func((void*)e,false);
                }
            }
            else
            {
                if(!effect->applied)
                {
                    effect->func((void*)e, false);
                    effect->applied = true;
                }
            }

        }

    }
}

void entity_handle_misc(float dt)
{
    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        if(!e->phys) continue;
        if(e->phys->dead) continue;

        // stun timer
        if(e->phys->stun_timer > 0.0)
        {
            e->phys->stun_timer -= dt;

            if(e->phys->stun_timer < 0.0)
               e->phys->stun_timer = 0.0;
        }

        if(e->type == ENTITY_TYPE_PLAYER || e->type == ENTITY_TYPE_CREATURE)
        {

            uint8_t tt = level_get_tile_type(visible_room, e->phys->curr_tile.x, e->phys->curr_tile.y);
            bool on_breakable_tile = (tt == TILE_BREAKABLE_FLOOR && e->phys->pos.z <= 3.0);
            bool moved_tiles = (e->phys->curr_tile.x != e->phys->prior_tile.x || e->phys->curr_tile.y != e->phys->prior_tile.y);

            if(e->phys->on_breakable_tile && (moved_tiles || !on_breakable_tile) && !e->phys->underground)
            {
                // update breakable floor state
                uint8_t* floor_state = &visible_room->breakable_floor_state[e->phys->prior_tile.x][e->phys->prior_tile.y];

                if(*floor_state < FLOOR_BROKE)
                {
                    (*floor_state)++;

                    if(role == ROLE_SERVER)
                    {
                        NetEvent ev = {
                            .type = EVENT_TYPE_FLOOR_STATE,
                            .data.floor_state.x = e->phys->prior_tile.x,
                            .data.floor_state.y = e->phys->prior_tile.y,
                            .data.floor_state.state = (*floor_state)
                        };

                        net_server_add_event(&ev);
                    }


                }
            }

            e->phys->on_breakable_tile = on_breakable_tile;
        }
    }
}

void entity_handle_collisions()
{
    if(role == ROLE_CLIENT) return;
    //printf("num entities: %d\n",num_entities);

    for(int i = 0; i < num_entities; ++i)
    {
        phys_calc_collision_rect(entities[i].phys);
    }

    // special check for players first
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        bool check = player_check_other_player_collision(&players[i]);
        if(!check) players[i].ignore_player_collision = false;
    }

    // handle entity collisions with other entities
    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e1 = &entities[i];

        if(e1->draw_only) continue;
        if(!e1->phys) continue;
        if(e1->phys->dead) continue;
        if(e1->phys->underground) continue;

        Physics* p1 = e1->phys;

        for(int j = 0; j < num_entities; ++j)
        {
            Entity* e2 = &entities[j];

            if(e1 == e2) continue;
            if(e2->draw_only) continue;
            if(!e2->phys) continue;
            if(e2->phys->dead) continue;
            if(e2->phys->underground) continue;
            if(e1->phys->curr_room != e2->phys->curr_room) continue;
            if(!is_any_player_room(e1->phys->curr_room)) continue;

            switch(e1->type)
            {
                case ENTITY_TYPE_PLAYER:
                    player_handle_collision((Player*)e1->ptr,e2);
                    break;
                case ENTITY_TYPE_CREATURE:
                    creature_handle_collision((Creature*)e1->ptr,e2);
                    break;
                case ENTITY_TYPE_PROJECTILE:
                    projectile_handle_collision((Projectile*)e1->ptr,e2);
                    break;
                case ENTITY_TYPE_ITEM:
                    item_handle_collision((Item*)e1->ptr,e2);
                    break;
                case ENTITY_TYPE_EXPLOSION:
                    explosion_handle_collision((Explosion*)e1->ptr,e2);
                    break;
            }
        }
    }

    // handle entity collisions with walls
    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        if(e->draw_only) continue;
        if(e->phys->ethereal) continue;
        if(e->type == ENTITY_TYPE_EXPLOSION) continue;

        Room* room = level_get_room_by_index(&level, entities[i].phys->curr_room);
        level_handle_room_collision(room,entities[i].phys, e->type, e);

        if(e->phys->dead && e->type == ENTITY_TYPE_PROJECTILE)
        {
            Projectile* proj = (Projectile*)e->ptr;
            projectile_kill(proj);
        }
    }
}


void entity_update_all(float dt)
{
    entity_handle_collisions();
    entity_handle_status_effects(dt);
    entity_handle_misc(dt);
}

void entity_draw_all()
{
    sort_entities();

    // draw shadows
    gfx_sprite_batch_begin(true);
    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        if(e->type == ENTITY_TYPE_WALL_COLUMN)
            continue;

        if(e->phys->curr_room != player->phys.curr_room)
            continue;

        entity_update_tile(e);
        if(e->tile == TILE_PIT || e->tile == TILE_BOULDER)
            continue;

        draw_entity_shadow(e->phys, e->type);
    }

    gfx_sprite_batch_draw();

    gfx_sprite_batch_begin(true);

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        switch(e->type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                player_draw((Player*)e->ptr);
            }   break;
            case ENTITY_TYPE_CREATURE:
            {
                creature_draw((Creature*)e->ptr);
            }   break;
            case ENTITY_TYPE_PROJECTILE:
            {
                projectile_draw((Projectile*)e->ptr);
            }   break;
            case ENTITY_TYPE_ITEM:
            {
                item_draw((Item*)e->ptr);
            }   break;
            case ENTITY_TYPE_WALL_COLUMN:
            {
                level_draw_wall_column(e->pos.x, e->pos.y);
                continue;
            }
            default:
                break;
        }

        // draw any status effects
        status_effects_draw((void*)e->phys);

    }

    gfx_sprite_batch_draw();

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        if(!e->phys)
            continue;

        if(e->phys->curr_room != player->phys.curr_room)
            continue;

        if(debug_enabled)
        {
            float vx = e->phys->pos.x;
            float vy = e->phys->pos.y - e->phys->pos.z/2.0;
            float cx = e->phys->collision_rect.x;
            float cy = e->phys->collision_rect.y - e->phys->pos.z/2.0;

            // draw collision circle
            gfx_draw_circle(cx, cy, e->phys->radius, COLOR_PURPLE, 1.0, false, IN_WORLD);

            // draw base dot
            gfx_draw_rect_xywh(vx, vy, 1, 1, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);

            // draw center dot
            gfx_draw_rect_xywh(cx, cy, 1, 1, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, true, true);


            // float vry = vy - (e->phys->vr.h*e->phys->scale + e->phys->pos.z)/2.0;
            // gfx_draw_rect_xywh(vx, vry, e->phys->vr.w*e->phys->scale, e->phys->vr.h*e->phys->scale, COLOR_YELLOW, NOT_SCALED, NO_ROTATION, 1.0, false, true); // bottom rect


            // draw boundingbox
            gfx_draw_rect_xywh(cx, cy, e->phys->collision_rect.w, e->phys->collision_rect.h, COLOR_YELLOW, NOT_SCALED, NO_ROTATION, 1.0, false, true); // bottom rect
            gfx_draw_rect_xywh(cx, cy - e->phys->height, e->phys->collision_rect.w, e->phys->collision_rect.h, COLOR_YELLOW, NOT_SCALED, NO_ROTATION, 1.0, false, true); // top rect

            float x0 = vx;
            float y0 = vy;
            float x1 = x0 + e->phys->vel.x;
            float y1 = y0 + e->phys->vel.y;
            gfx_add_line(x0, y0, x1, y1, COLOR_RED);
        }
    }

    // gfx_sprite_batch_begin(true);
    // gfx_sprite_batch_add(podium_image, 0, 500, 400, COLOR_TINT_NONE, false, 1.0, 0.0, 1.0, true, false, false);
    // gfx_sprite_batch_draw();

    // GFXImage* img = &gfx_images[podium_image];
    // gfx_draw_rect_xywh(500, 400, img->visible_rects[0].w*1.0, img->visible_rects[0].h*1.0, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, true);

}

static Physics* get_physics_from_type(int index, uint8_t curr_room, EntityType type)
{
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
            if(!players[index].phys.dead && players[index].phys.curr_room == curr_room && players[index].active)
                return &players[index].phys;
        case ENTITY_TYPE_CREATURE:
            if(!creatures[index].phys.dead && creatures[index].phys.curr_room == curr_room)
                return &creatures[index].phys;
    }
    return NULL;
}

static uint16_t get_id_from_type(int index, uint8_t curr_room, EntityType type)
{
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
            return index;
        case ENTITY_TYPE_CREATURE:
            return creatures[index].id;
    }
    return 0;
}

Physics* entity_get_closest_to(Physics* phys, uint8_t curr_room, EntityType type, uint16_t* exclude_ids, int exclude_count, uint16_t* target_id)
{
    int count = 0;
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
            count = MAX_PLAYERS;
            break;
        case ENTITY_TYPE_CREATURE:
            count = creature_get_count();
            break;
    }

    float min_dist = 10000.0;
    Physics* min_result = NULL;

    for(int i = 0; i < count; ++i)
    {
        Physics* phys2 = get_physics_from_type(i, curr_room, type);
        if(!phys2) continue;

        uint16_t id = get_id_from_type(i, curr_room, type);
        if(type == ENTITY_TYPE_CREATURE)
        {
            Creature* c = creature_get_by_id(id);
            if(c->friendly) continue;
        }

        bool exclude = false;
        for(int j = 0; j < exclude_count; ++j)
        {
            if(id == exclude_ids[j])
            {
                exclude = true;
                break;
            }
        }
        if(exclude) continue;

        float d = dist(phys->pos.x, phys->pos.y,phys2->pos.x,phys2->pos.y);
        if(d < min_dist)
        {
            min_dist = d;
            min_result = phys2;
            if(target_id) *target_id = id;
        }
    }

    return min_result;
}

