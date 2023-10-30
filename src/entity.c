#include "headers.h"
#include "player.h"
#include "creature.h"
#include "projectile.h"
#include "explosion.h"
#include "entity.h"

Entity entities[MAX_ENTITIES] = {0};
int num_entities;

static void add_entity(EntityType type, void* ptr, uint8_t curr_room, Physics* phys)
{
    Entity* e = &entities[num_entities++];

    e->type = type;
    e->ptr = ptr;
    e->curr_room = curr_room;
    e->phys = phys;
}

static bool is_player_room(uint8_t room_index)
{
    int num_players = player_get_active_count();
    for(int i = 0; i < num_players; ++i)
    {
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
    
    // pickups
    for(int i = 0; i < pickup_list->count; ++i)
    {
        Pickup* pu = &pickups[i];
        add_entity(ENTITY_TYPE_PICKUP,pu,pu->curr_room, &pu->phys);
    }
}

void entity_handle_collisions()
{
    //printf("num entities: %d\n",num_entities);

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
                case ENTITY_TYPE_PICKUP:
                    pickup_handle_collision((Pickup*)e1->ptr,e2);
                    break;
            }
        }
    }

    // handle entity collisions with walls
    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];
        if(e->phys->dead || e->phys->ethereal) continue;

        Room* room = level_get_room_by_index(&level, entities[i].curr_room);
        level_handle_room_collision(room,entities[i].phys);

        if(e->phys->dead && e->type == ENTITY_TYPE_PROJECTILE)
        {
            Projectile* proj = (Projectile*)e->ptr;
            if(projectile_lookup[proj->type].explosive)
            {
                explosion_add(e->phys->pos.x, e->phys->pos.y, 15.0*proj->scale, 100.0, e->curr_room);
            }
        }
    }
}

void entity_draw_all()
{
    sort_entities();

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];
        switch(e->type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                Player* p = (Player*)e->ptr;
                if(p->curr_room == player->curr_room)
                    player_draw(p);
            }   break;
            case ENTITY_TYPE_CREATURE:
            {
                Creature* c = (Creature*)e->ptr;
                if(c->curr_room == player->curr_room)
                    creature_draw(c);
            }   break;
            case ENTITY_TYPE_PROJECTILE:
            {
                projectile_draw((Projectile*)e->ptr);
            }   break;
            case ENTITY_TYPE_PICKUP:
            {
                pickup_draw((Pickup*)e->ptr);
            }   break;
            default:
                break;
        }
    }
}
