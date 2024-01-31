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
    Room* room = level_get_room_by_index(&level, e->curr_room);
    e->tile = level_get_tile_type(room, tile_coords.x, tile_coords.y);
}

static void add_entity(EntityType type, void* ptr, uint8_t curr_room, Physics* phys)
{
    Entity* e = &entities[num_entities++];

    e->type = type;
    e->ptr = ptr;
    e->curr_room = curr_room;
    e->phys = phys;

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
        if(players[i].curr_room == room_index)
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

        while (j >= 0 && entities[j].phys->pos.y > key.phys->pos.y)
        {
            memcpy(&entities[j+1], &entities[j], sizeof(Entity));
            j = j - 1;
        }
        memcpy(&entities[j+1], &key, sizeof(Entity));
    }
}

static void draw_entity_shadow(Physics* phys)
{
    if(phys->falling) return;

    // bool horizontal = (phys->rotation_deg == 0.0 || phys->rotation_deg == 180.0);
    if(phys->height < 24.0 && phys->pos.z == 0.0) return;

    // float scale = (phys->collision_rect.w/32.0);
    float scale = (phys->vr.w/32.0 * 0.80);
    float opacity = RANGE(0.5*(1.0 - (phys->pos.z / 128.0)),0.1,0.5);

    float shadow_x = phys->pos.x;
    float shadow_y = phys->pos.y;

    gfx_sprite_batch_add(shadow_image, 0, shadow_x, shadow_y, COLOR_TINT_NONE, false, scale, 0.0, opacity, false, false, false);
}

void entity_build_all()
{
    num_entities = 0;

    // players
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(!p->active) continue;
        add_entity(ENTITY_TYPE_PLAYER,p,p->curr_room, &p->phys);
    }

    // creatures
    uint16_t num_creatures = creature_get_count();
    for(int i = 0; i < num_creatures; ++i)
    {
        Creature* c = &creatures[i];

        if(!is_player_room(c->curr_room)) continue;

        bool ghost = c->phys.dead ? true : false;
        add_entity(ENTITY_TYPE_CREATURE,c,c->curr_room, &c->phys);
    }

    // projectiles
    for(int i = 0; i < plist->count; ++i)
    {
        Projectile* p = &projectiles[i];
        add_entity(ENTITY_TYPE_PROJECTILE,p,p->curr_room, &p->phys);
    }

    // items
    for(int i = 0; i < item_list->count; ++i)
    {
        Item* pu = &items[i];
        add_entity(ENTITY_TYPE_ITEM,pu,pu->curr_room, &pu->phys);
    }

    // explosions
    for(int i = 0; i < explosion_list->count; ++i)
    {
        Explosion* ex = &explosions[i];
        add_entity(ENTITY_TYPE_EXPLOSION,ex,ex->curr_room, &ex->phys);
    }
}

static bool remove_status_effect(Physics* phys, int index)
{
    if(phys == NULL)
        return false;

    if(index >= MAX_STATUS_EFFECTS)
        return false;

    uint8_t* p = (uint8_t*)phys->status_effects;

    int item_size = sizeof(StatusEffect);
    memcpy(p + index*item_size, p+(phys->status_effects_count-1)*item_size, item_size);
    phys->status_effects_count--;
}

void entity_handle_status_effects(float dt)
{
    if(role == ROLE_CLIENT) return;

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];
        if(e->phys->dead) continue;

        Physics* phys = e->phys;

        for(int i = phys->status_effects_count - 1; i >= 0; --i)
        {
            StatusEffect* effect = &phys->status_effects[i];

            effect->lifetime += dt;

            if(effect->lifetime_max > 0.0 && effect->lifetime >= effect->lifetime_max)
            {
                // status effect is done, remove
                effect->func((void*)e, true);
                remove_status_effect(phys, i);
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
        if(e1->phys->dead) continue;

        Physics* p1 = e1->phys;

        for(int j = 0; j < num_entities; ++j)
        {
            Entity* e2 = &entities[j];

            if(e2->phys->dead) continue;
            if(e1 == e2) continue;
            if(e1->curr_room != e2->curr_room) continue;
            if(!is_any_player_room(e1->curr_room)) continue;

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

        if(e->phys->ethereal) continue;
        if(e->type == ENTITY_TYPE_EXPLOSION) continue;

        Room* room = level_get_room_by_index(&level, entities[i].curr_room);
        level_handle_room_collision(room,entities[i].phys, e->type);

        if(e->phys->dead && e->type == ENTITY_TYPE_PROJECTILE)
        {
            Projectile* proj = (Projectile*)e->ptr;
            projectile_kill(proj);

            // if(proj->def.explosive)
            // {
            //     explosion_add(e->phys->pos.x, e->phys->pos.y, 15.0*proj->def.scale, 100.0*proj->def.scale, e->curr_room, proj->from_player);
            // }
        }
    }

    // // double check for players getting stuck inside walls
    // for(int i = 0; i < MAX_PLAYERS; ++i)
    // {
    //     Player* p = &players[i];
    //     if(!p->active) continue;

    // }
    // if(!p->phys.dead)
    // {
    //     float cx = p->phys.pos.x;
    //     float cy = p->phys.pos.y;
    //     Vector2i coords = level_get_room_coords_by_pos(cx, cy);
    //     TileType tt = level_get_tile_type(room, coords.x, coords.y);
    //     if(tt == TILE_BOULDER)
    //     {
    //         TileType tt = level_get_tile_type(room, p->last_safe_tile.x, p->last_safe_tile.y);
    //         if(IS_SAFE_TILE(tt))
    //         {
    //             Vector2f position = level_get_pos_by_room_coords(p->last_safe_tile.x, p->last_safe_tile.y);
    //             phys_set_collision_pos(&p->phys, position.x, position.y);
    //         }
    //     }
    // }

}

void entity_draw_all()
{
    sort_entities();

    // draw shadows
    gfx_sprite_batch_begin(true);
    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];

        if(e->curr_room != player->curr_room)
            continue;

        entity_update_tile(e);
        if(e->tile == TILE_PIT || e->tile == TILE_BOULDER)
            continue;

        // if(e->type == ENTITY_TYPE_CREATURE)
        //     printf("height %.2f\n", e->phys->height);

        // if(role == ROLE_CLIENT)
        //     phys_calc_collision_rect(e->phys);

        draw_entity_shadow(e->phys);

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

        if(e->curr_room != player->curr_room)
            continue;

        if(debug_enabled)
        {
            float vx = e->phys->pos.x;
            float vy = e->phys->pos.y - e->phys->pos.z/2.0;
            float cx = e->phys->collision_rect.x;
            float cy = e->phys->collision_rect.y;

            // draw collision circle
            gfx_draw_circle(cx, cy, e->phys->radius, COLOR_PURPLE, 1.0, false, IN_WORLD);

            // draw base dot
            gfx_draw_rect_xywh(vx, vy, 1, 1, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);

            // draw center dot
            gfx_draw_rect_xywh(cx, cy, 1, 1, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, true, true);

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
}

static Physics* get_physics_from_type(int index, uint8_t curr_room, EntityType type)
{
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
            if(!players[index].phys.dead && players[index].curr_room == curr_room)
                return &players[index].phys;
            break;
        case ENTITY_TYPE_CREATURE:
            if(!creatures[index].phys.dead && creatures[index].curr_room == curr_room)
                return &creatures[index].phys;
            break;
    }
    return NULL;
}

Physics* entity_get_closest_to(Physics* phys, uint8_t curr_room, EntityType type)
{
    int count = 0;
    switch(type)
    {
        case ENTITY_TYPE_PLAYER:
            count = player_get_active_count(); //TODO: could be empty spots in player list
            break;
        case ENTITY_TYPE_CREATURE:
            count = creature_get_count();
            break;
    }

    float min_dist = 1000.0;
    Physics* min_result = NULL;

    for(int i = 0; i < count; ++i)
    {
        Physics* phys2 = get_physics_from_type(i, curr_room, type);
        if(!phys2) continue;

        float d = dist(phys->pos.x, phys->pos.y,phys2->pos.x,phys2->pos.y);
        if(d < min_dist)
        {
            min_dist = d;
            min_result = phys2;
        }
    }

    return min_result;
}
