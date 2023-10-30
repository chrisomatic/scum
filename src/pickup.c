#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"

#include "player.h"
#include "pickup.h"

glist* pickup_list = NULL;
Pickup pickups[MAX_PICKUPS];

int gems_image = -1;

static Rect gem_get_rect(GemType type)
{
    GFXImage* img = &gfx_images[gems_image];
    Rect* vr = &img->visible_rects[type];
    Rect r = {0};
    r.w = vr->w;
    r.h = vr->h;
    return r;
}

void pickup_init()
{
    if(pickup_list)
        return;

    pickup_list = list_create((void*)pickups, MAX_PICKUPS, sizeof(Pickup));
    gems_image = gfx_load_image("src/img/gems.png", false, false, 16, 16);
}

void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room)
{
    Pickup pu = {0};

    pu.type = type;
    pu.subtype = subtype;
    pu.sprite_index = type;
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.mass = 4.0;
    pu.phys.speed = 1.0;
    pu.phys.radius = 8;
    pu.curr_room = curr_room;

    list_add(pickup_list,&pu);
}

void pickup_update(Pickup* pu, float dt)
{
    pu->phys.pos.x += dt*pu->phys.vel.x;
    pu->phys.pos.y += dt*pu->phys.vel.y;

    float rate = 100.0; //phys_get_friction_rate(0.005*pu->phys.mass,dt);
    phys_apply_friction(&pu->phys,rate,dt);
}

void pickup_update_all(float dt)
{
    for(int i = pickup_list->count-1; i >= 0; --i)
    {
        Pickup* pu = &pickups[i];
        pickup_update(pu, dt);
        if(pu->picked_up)
        {
            list_remove(pickup_list, i);
        }
    }
}

void pickup_draw(Pickup* pu)
{
    if(pu->curr_room != player->curr_room)
        return;

    switch(pu->type)
    {
        case PICKUP_TYPE_GEM:
            gfx_draw_image(gems_image, pu->subtype, pu->phys.pos.x, pu->phys.pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, IN_WORLD);
            break;

    }
}

void pickup_handle_collision(Pickup* p, Entity* e)
{
    switch(e->type)
    {
        case ENTITY_TYPE_PICKUP:
        {
            Pickup* p2 = (Pickup*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&p->phys, &p2->phys,&ci);
            }
        } break;
    }
}
