#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"

#include "player.h"
#include "item.h"

glist* item_list = NULL;
Item items[MAX_ITEMS];

int items_image = -1;

static Rect gem_get_rect(GemType type)
{
    GFXImage* img = &gfx_images[items_image];
    Rect* vr = &img->visible_rects[type];
    Rect r = {0};
    r.w = vr->w;
    r.h = vr->h;
    return r;
}

static void item_func_nothing(Item* pu, Player* p)
{
    return;
}
static void item_func_heart_full(Item* pu, Player* p)
{
    if(p->phys.hp < p->phys.hp_max)
    {
        player_add_hp(p,2);
        pu->picked_up = true;
    }
}

static void item_func_heart_half(Item* pu, Player* p)
{
    if(p->phys.hp < p->phys.hp_max)
    {
        player_add_hp(p,1);
        pu->picked_up = true;
    }
}

void item_init()
{
    if(item_list)
        return;

    item_list = list_create((void*)items, MAX_ITEMS, sizeof(Item));
    items_image = gfx_load_image("src/img/items.png", false, false, 16, 16);
}

int item_get_sprite_index(ItemType type, int subtype)
{
    return subtype;
}

const char* item_get_name(ItemType type, int subtype)
{
    if(type == ITEM_TYPE_GEM)
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
    else if(type == ITEM_TYPE_HEALTH)
    {
        switch(subtype)
        {
            case HEALTH_TYPE_HEART_FULL: return "Full Heart";
            case HEALTH_TYPE_HEART_HALF: return "Half Heart";
        }
    }
    return "???";
}

void item_add(ItemType type, int subtype, float x, float y, uint8_t curr_room)
{
    Item pu = {0};

    pu.type = type;
    pu.subtype = subtype;
    pu.sprite_index = item_get_sprite_index(type, subtype);
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.mass = 5.0;
    pu.phys.speed = 1.0;
    pu.phys.radius = 8;
    pu.phys.elasticity = 0.5;
    pu.curr_room = curr_room;
    pu.touch_item = type == ITEM_TYPE_HEALTH ? true : false;

    if(pu.type == ITEM_TYPE_HEALTH)
    {
        switch(pu.subtype)
        {
            case HEALTH_TYPE_HEART_FULL:
                pu.func = (void*)item_func_heart_full;
                break;
            case HEALTH_TYPE_HEART_HALF:
                pu.func = (void*)item_func_heart_half;
                break;
            default:
                pu.func = (void*)item_func_nothing;
        }
    }

    list_add(item_list,&pu);
}

void item_update(Item* pu, float dt)
{
    pu->phys.pos.x += dt*pu->phys.vel.x;
    pu->phys.pos.y += dt*pu->phys.vel.y;

    phys_apply_friction(&pu->phys,10.0,dt);
}

void item_update_all(float dt)
{
    for(int i = item_list->count-1; i >= 0; --i)
    {
        Item* pu = &items[i];

        if(pu->picked_up)
        {
            list_remove(item_list, i);
            continue;
        }

        item_update(pu, dt);
    }
}

void item_draw(Item* pu)
{
    if(pu->curr_room != player->curr_room)
        return;

    switch(pu->type)
    {
        case ITEM_TYPE_GEM:
            gfx_draw_image(items_image, pu->sprite_index, pu->phys.pos.x, pu->phys.pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, IN_WORLD);
            break;
        case ITEM_TYPE_HEALTH:
            gfx_draw_image(items_image, pu->sprite_index, pu->phys.pos.x, pu->phys.pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, IN_WORLD);
            break;
    }
}

void item_handle_collision(Item* p, Entity* e)
{
    switch(e->type)
    {
        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&p->phys, &p2->phys,&ci);
            }
        } break;
    }
}
