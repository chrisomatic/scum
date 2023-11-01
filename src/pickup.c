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

int pickups_image = -1;

static Rect gem_get_rect(GemType type)
{
    GFXImage* img = &gfx_images[pickups_image];
    Rect* vr = &img->visible_rects[type];
    Rect r = {0};
    r.w = vr->w;
    r.h = vr->h;
    return r;
}

static void pickup_func_nothing(Pickup* pu, Player* p)
{
    return;
}
static void pickup_func_heart_full(Pickup* pu, Player* p)
{
    if(p->hp < p->hp_max)
    {
        player_add_hp(p,2);
        pu->picked_up = true;
    }
}

static void pickup_func_heart_half(Pickup* pu, Player* p)
{
    if(p->hp < p->hp_max)
    {
        player_add_hp(p,1);
        pu->picked_up = true;
    }
}

void pickup_init()
{
    if(pickup_list)
        return;

    pickup_list = list_create((void*)pickups, MAX_PICKUPS, sizeof(Pickup));
    pickups_image = gfx_load_image("src/img/pickups.png", false, false, 16, 16);
}

int pickup_get_sprite_index(PickupType type, int subtype)
{
    return subtype;
}

const char* pickup_get_name(PickupType type, int subtype)
{
    if(type == PICKUP_TYPE_GEM)
    {
        switch(subtype)
        {
            case GEM_TYPE_RED: return "Red Gem";
            case GEM_TYPE_GREEN: return "Green Gem";
            case GEM_TYPE_BLUE: return "Blue Gem";
            case GEM_TYPE_WHITE: return "White Gem";
            case GEM_TYPE_YELLOW: return "Yellow Gem";
            case GEM_TYPE_PURPLE: return "Purple Gem";
        }
    }
    else if(type == PICKUP_TYPE_HEALTH)
    {
        switch(subtype)
        {
            case HEALTH_TYPE_HEART_FULL: return "Full Heart";
            case HEALTH_TYPE_HEART_HALF: return "Half Heart";
        }
    }
    return "???";
}

void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room)
{
    Pickup pu = {0};

    pu.type = type;
    pu.subtype = subtype;
    pu.sprite_index = pickup_get_sprite_index(type, subtype);
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.mass = 5.0;
    pu.phys.speed = 1.0;
    pu.phys.radius = 8;
    pu.phys.elasticity = 0.5;
    pu.curr_room = curr_room;
    pu.touch_pickup = type == PICKUP_TYPE_HEALTH ? true : false;

    if(pu.type == PICKUP_TYPE_HEALTH)
    {
        switch(pu.subtype)
        {
            case HEALTH_TYPE_HEART_FULL:
                pu.func = (void*)pickup_func_heart_full;
                break;
            case HEALTH_TYPE_HEART_HALF:
                pu.func = (void*)pickup_func_heart_half;
                break;
            default:
                pu.func = (void*)pickup_func_nothing;
        }
    }

    list_add(pickup_list,&pu);
}

void pickup_update(Pickup* pu, float dt)
{
    pu->phys.pos.x += dt*pu->phys.vel.x;
    pu->phys.pos.y += dt*pu->phys.vel.y;

    phys_apply_friction(&pu->phys,10.0,dt);
}

void pickup_update_all(float dt)
{
    for(int i = pickup_list->count-1; i >= 0; --i)
    {
        Pickup* pu = &pickups[i];

        if(pu->picked_up)
        {
            list_remove(pickup_list, i);
            continue;
        }

        pickup_update(pu, dt);
    }
}

void pickup_draw(Pickup* pu)
{
    if(pu->curr_room != player->curr_room)
        return;

    switch(pu->type)
    {
        case PICKUP_TYPE_GEM:
            gfx_draw_image(pickups_image, pu->sprite_index, pu->phys.pos.x, pu->phys.pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, IN_WORLD);
            break;
        case PICKUP_TYPE_HEALTH:
            gfx_draw_image(pickups_image, pu->sprite_index, pu->phys.pos.x, pu->phys.pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, IN_WORLD);
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
