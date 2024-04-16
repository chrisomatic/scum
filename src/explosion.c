#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"

#include "player.h"
#include "creature.h"
#include "explosion.h"

glist* explosion_list = NULL;
Explosion explosions[MAX_EXPLOSIONS];

void explosion_init()
{
    if(explosion_list)
        return;

    explosion_list = list_create((void*)explosions, MAX_EXPLOSIONS, sizeof(Explosion), false);
}

void explosion_add(float x, float y, float max_radius, float rate, uint8_t curr_room, bool from_player)
{
    Explosion ex = {0};

    ex.phys.pos.x = x;
    ex.phys.pos.y = y;
    ex.phys.mass = 4.0;
    ex.phys.speed = 0.0;
    ex.phys.radius = 0.0;
    ex.phys.dead = false;
    ex.max_radius = max_radius;
    ex.rate = rate;
    ex.phys.curr_room = curr_room;
    ex.from_player = from_player;

    list_add(explosion_list, &ex);
}

void explosion_update(Explosion* ex, float dt)
{
    ex->phys.radius += dt * ex->rate;
    // printf("ex->phys.radius: %.2f\n", ex->phys.radius);
    if(ex->phys.radius >= ex->max_radius)
    {
        ex->phys.radius = ex->max_radius;
        ex->rate *= -1.0;
    }
    else if(ex->phys.radius <= 0.0)
    {
        ex->phys.dead = true;
    }
}

void explosion_update_all(float dt)
{
    for(int i = explosion_list->count-1; i >= 0; --i)
    {
        Explosion* ex = &explosions[i];
        explosion_update(ex, dt);
        if(ex->phys.dead)
        {
            list_remove(explosion_list, i);
        }
    }
}

void explosion_draw(Explosion* ex)
{
    if(ex->phys.curr_room != player->phys.curr_room)
        return;

    gfx_draw_circle(ex->phys.pos.x, ex->phys.pos.y, ex->phys.radius, COLOR_RAND, 1.0, true, IN_WORLD);
}

void explosion_draw_all()
{
    for(int i = explosion_list->count-1; i >= 0; --i)
    {
        explosion_draw(&explosions[i]);
    }
}

void explosion_handle_collision(Explosion* ex, Entity* e)
{
    // TODO: apply damage

    switch(e->type)
    {
        case ENTITY_TYPE_CREATURE:
        {
            if(!ex->from_player) return;
            Creature* p2 = (Creature*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&ex->phys, &p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&ex->phys, &p2->phys, &ci);
            }
        } break;
        case ENTITY_TYPE_PLAYER:
        {
            if(ex->from_player) return;
            Player* p2 = (Player*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&ex->phys, &p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&ex->phys, &p2->phys, &ci);
            }
        } break;
    }
}
