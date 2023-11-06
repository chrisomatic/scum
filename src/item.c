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
Item items[MAX_ITEMS] = {0};
ItemProps item_props[MAX_ITEMS] = {0};
int items_image = -1;

static void item_func_nothing(Item* pu, Player* p)
{
    return;
}

static void item_func_heart(Item* pu, Player* p)
{
    int add_health = 1;
    if(pu->type == ITEM_HEART_FULL)
        add_health = 2;

    if(p->phys.hp < p->phys.hp_max)
    {
        player_add_hp(p, add_health);
        pu->picked_up = true;
    }
}

void item_init()
{
    if(item_list)
        return;

    item_list = list_create((void*)items, MAX_ITEMS, sizeof(Item));
    items_image = gfx_load_image("src/img/items.png", false, false, 16, 16);

    for(int i = 0; i < ITEM_MAX; ++i)
    {
        ItemProps* p = &item_props[i];
        p->type = i;
        p->image = items_image;
        p->sprite_index = i;
        p->func = (void*)item_func_nothing;

        if(item_is_gem(p->type))
        {
            p->touch_item = false;
        }
        else if(item_is_heart(p->type))
        {
            p->touch_item = true;
            p->func = (void*)item_func_heart;
        }
    }

}

bool item_is_gem(ItemType type)
{
    if(type >= ITEM_GEM_RED && type <= ITEM_GEM_PURPLE)
        return true;
    return false;
}

bool item_is_heart(ItemType type)
{
    if(type >= ITEM_HEART_FULL && type <= ITEM_HEART_HALF)
        return true;
    return false;
}

ItemType item_get_random_gem()
{
    // kinda inefficent but no big deal
    for(;;)
    {
        ItemType it = rand() % ITEM_MAX;
        if(item_is_gem(it))
            return it;
    }
}

ItemType item_get_random_heart()
{
    for(;;)
    {
        ItemType it = rand() % ITEM_MAX;
        if(item_is_heart(it))
            return it;
    }
}

const char* item_get_name(ItemType type)
{
    switch(type)
    {
        case ITEM_NONE: return "None";
        case ITEM_GEM_RED: return "Red Gem";
        case ITEM_GEM_GREEN: return "Green Gem";
        case ITEM_GEM_BLUE: return "Blue Gem";
        case ITEM_GEM_WHITE: return "White Gem";
        case ITEM_GEM_YELLOW: return "Yellow Gem";
        case ITEM_GEM_PURPLE: return "Purple Gem";
        case ITEM_HEART_FULL: return "Full Heart";
        case ITEM_HEART_HALF: return "Half Heart";
    }
    return "???";
}

void item_add(ItemType type, float x, float y, uint8_t curr_room)
{
    Item pu = {0};

    pu.type = type;
    pu.picked_up = false;
    pu.curr_room = curr_room;
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.mass = 5.0;
    pu.phys.speed = 1.0;
    pu.phys.radius = 8;
    pu.phys.elasticity = 0.5;

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

    gfx_draw_image(item_props[pu->type].image, item_props[pu->type].sprite_index, pu->phys.pos.x, pu->phys.pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, IN_WORLD);
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
