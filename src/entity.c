#include "headers.h"
#include "player.h"
#include "creature.h"
#include "projectile.h"
#include "entity.h"

Entity entities[MAX_ENTITIES] = {0};
int num_entities;

static void add_entity(EntityType type, void* ptr, uint8_t curr_room, Physics* phys, bool simulate)
{
    Entity* e = &entities[num_entities++];

    e->type = type;
    e->ptr = ptr;
    e->curr_room = curr_room;
    e->phys = phys;
    e->simulate = simulate;
}

static bool is_player_room(uint8_t room_index)
{
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
    for(int i = 0; i < num_players; ++i)
    {
        Player* p = &players[i];
        add_entity(ENTITY_TYPE_PLAYER,p,p->curr_room, &p->phys, true);
    }

    // creatures
    int num_creatures = creature_get_count();
    for(int i = 0; i < num_creatures; ++i)
    {
        Creature* c = &creatures[i];

        if(!is_player_room(c->curr_room)) continue;

        bool simulate = c->dead ? false : true;
        add_entity(ENTITY_TYPE_CREATURE,c,c->curr_room, &c->phys,simulate);
    }

    // projectiles
    for(int i = 0; i < plist->count; ++i)
    {
        Projectile* p = &projectiles[i];
        add_entity(ENTITY_TYPE_PROJECTILE,p,p->curr_room, &p->phys, true);
    }

}

void entity_handle_collisions()
{
    //printf("num entities: %d\n",num_entities);

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e1 = &entities[i];
        if(!e1->simulate) continue;

        Physics* p1 = e1->phys;

        for(int j = 0; j < num_entities; ++j)
        {
            Entity* e2 = &entities[j];

            if(!e2->simulate) continue;
            if(e1 == e2) continue;
            if(e1->type == ENTITY_TYPE_PLAYER && e2->type == ENTITY_TYPE_PROJECTILE) continue;
            if(e2->type == ENTITY_TYPE_PLAYER && e1->type == ENTITY_TYPE_PROJECTILE) continue;

            Physics* p2 = e2->phys;

            Vector2f cr = {0.0,0.0};
            bool collided = phys_collision_circles(p1,p2, &cr);

            if(collided)
            {
                if(e1->type == ENTITY_TYPE_PLAYER)
                {
                    Player* p = (Player*)e1->ptr;
                    player_handle_collision(p,e2,&cr);
                }
            }
        }
    }

    for(int i = 0; i < num_entities; ++i)
    {
        Entity* e = &entities[i];
        if(!e->simulate) continue;

        Room* room = level_get_room_by_index(&level, entities[i].curr_room);
        level_handle_room_collision(room,entities[i].phys);
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
            default:
                break;
        }
    }
}
