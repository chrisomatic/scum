#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"

#include "player.h"
#include "item.h"

const float iscale = 0.8;



glist* item_list = NULL;
Item items[MAX_ITEMS] = {0};
ItemProps item_props[MAX_ITEMS] = {0};
int items_image = -1;
int chest_image = -1;

static uint16_t id_counter = 0;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

static void item_func_nothing(Item* pu, Player* p)
{
    return;
}

static void item_func_chest(Item* pu, Player* p)
{
    if(RAND_FLOAT(0.0,1.0) <= 0.05)
    {
        item_add(ITEM_CHEST, pu->phys.pos.x, pu->phys.pos.y, pu->curr_room);
    }

    int num = RAND_RANGE(4,8);
    for(int i = 0; i < num; ++i)
        item_add(item_get_random_gem(), pu->phys.pos.x, pu->phys.pos.y, pu->curr_room);

    if(RAND_FLOAT(0.0,1.0) <= 0.5 && num < 6)
    {
        item_add(item_get_random_heart(), pu->phys.pos.x, pu->phys.pos.y, pu->curr_room);
    }
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

static void item_func_gem(Item* pu, Player* p)
{
    switch(pu->type)
    {
        case ITEM_GEM_RED:
            p->phys.hp_max += 2;
            p->phys.hp = p->phys.hp_max;
            break;
        case ITEM_GEM_YELLOW:
            if(p->gauntlet_slots < PLAYER_GAUNTLET_MAX)
            {
                p->gauntlet_slots++;
            }
            else
            {
                item_add(pu->type, pu->phys.pos.x, pu->phys.pos.y, pu->curr_room);
            }
            break;
        default:
        {
            if(player_gauntlet_full(p))
            {
                memcpy(&p->gauntlet_item, pu, sizeof(Item));
                p->show_gauntlet = true;
            }
            else
            {
                p->gauntlet_item.type = ITEM_NONE;
                for(int i = 0; i < p->gauntlet_slots; ++i)
                {
                    if(p->gauntlet[i].type == ITEM_NONE)
                    {
                        p->gauntlet[i].type = pu->type;
                        break;
                    }
                }
            }

        } break;
    }
}

void item_init()
{
    if(item_list)
        return;

    item_list = list_create((void*)items, MAX_ITEMS, sizeof(Item));

    items_image = gfx_load_image("src/img/items.png", false, false, 16, 16);
    chest_image = gfx_load_image("src/img/chest.png", false, false, 32, 32);

    for(int i = 0; i < ITEM_MAX; ++i)
    {
        ItemProps* p = &item_props[i];
        p->type = i;
        p->image = p->type == ITEM_CHEST ? chest_image : items_image;
        p->sprite_index = p->type == ITEM_CHEST ? 0 : i;
        p->func = (void*)item_func_nothing;

        if(item_is_gem(p->type))
        {
            p->touch_item = false;
            p->func = (void*)item_func_gem;
        }
        else if(item_is_heart(p->type))
        {
            p->touch_item = true;
            p->func = (void*)item_func_heart;
        }
        else if(item_is_chest(p->type))
        {
            p->touch_item = false;
            p->func = (void*)item_func_chest;
        }
    }
}

void item_clear_all()
{
    list_clear(item_list);
}

bool item_is_chest(ItemType type)
{
    return (type == ITEM_CHEST);
}

bool item_is_gem(ItemType type)
{
    return (type >= ITEM_GEM_RED && type <= ITEM_GEM_PURPLE);
}

bool item_is_heart(ItemType type)
{
    return (type >= ITEM_HEART_FULL && type <= ITEM_HEART_HALF);
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
        case ITEM_NONE:       return "None";
        case ITEM_CHEST:      return "Chest";
        case ITEM_GEM_RED:    return "Red Gem";
        case ITEM_GEM_GREEN:  return "Green Gem";
        case ITEM_GEM_BLUE:   return "Blue Gem";
        case ITEM_GEM_WHITE:  return "White Gem";
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
    pu.id = get_id();
    pu.picked_up = false;
    pu.curr_room = curr_room;
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.speed = 1.0;

    if(type == ITEM_CHEST)
    {
        pu.phys.mass = 2.0;
        pu.phys.base_friction = 30.0;
        pu.phys.radius = 12*iscale; //TEMP
        pu.phys.elasticity = 0.2;
    }
    else
    {
        pu.phys.mass = 0.5;
        pu.phys.base_friction = 8.0;
        pu.phys.radius = 8*iscale; //TEMP
        pu.phys.elasticity = 0.5;
    }

    list_add(item_list,&pu);
}

bool item_remove(Item* pu)
{
    return list_remove_by_item(item_list, pu);
}


void item_update(Item* pu, float dt)
{
    pu->phys.pos.x += dt*pu->phys.vel.x;
    pu->phys.pos.y += dt*pu->phys.vel.y;

    phys_apply_friction(&pu->phys,pu->phys.base_friction,dt);
}


typedef struct
{
    int index;
    uint16_t id;
    float dist;
} ItemSort;

#define NUM_NEAR_ITEMS  10
ItemSort near_items_prior[NUM_NEAR_ITEMS] = {0};
int near_items_count_prior = 0;
ItemSort near_items[NUM_NEAR_ITEMS] = {0};
int near_items_count = 0;

void item_update_all(float dt)
{
    memcpy(&near_items_prior, &near_items, sizeof(ItemSort)*NUM_NEAR_ITEMS);
    near_items_count_prior = near_items_count;

    memset(&near_items, 0, sizeof(ItemSort)*NUM_NEAR_ITEMS);
    near_items_count = 0;

    for(int i = item_list->count-1; i >= 0; --i)
    {
        Item* pu = &items[i];

        if(pu->picked_up)
        {
            list_remove(item_list, i);
            continue;
        }

        item_update(pu, dt);

        if(pu->curr_room != player->curr_room)
            continue;

        Vector2f c1 = {CPOSX(player->phys), CPOSY(player->phys)};
        Vector2f c2 = {CPOSX(pu->phys), CPOSY(pu->phys)};

        float distance;
        bool in_pickup_radius = circles_colliding(&c1, player->phys.radius, &c2, ITEM_PICKUP_RADIUS, &distance);

        if(in_pickup_radius && near_items_count < NUM_NEAR_ITEMS)
        {
            near_items[near_items_count].index = i;
            near_items[near_items_count].dist = distance;
            near_items[near_items_count].id = pu->id;
            near_items_count++;
        }

        pu->highlighted = false;
    }

    if(near_items_count > 0)
    {
        // insertion sort
        int i, j;
        ItemSort key;
        for (i = 1; i < near_items_count; ++i) 
        {
            memcpy(&key, &near_items[i], sizeof(ItemSort));
            j = i - 1;

            while (j >= 0 && near_items[j].dist > key.dist)
            {
                memcpy(&near_items[j+1], &near_items[j], sizeof(ItemSort));
                j = j - 1;
            }
            memcpy(&near_items[j+1], &key, sizeof(ItemSort));
        }

        bool same_list = true;
        if(near_items_count != near_items_count_prior)
        {
            same_list = false;
        }
        else
        {
            for(int i = 0; i < near_items_count; ++i)
            {
                if(near_items[i].id != near_items_prior[i].id)
                {
                    same_list = false;
                    break;
                }
            }
        }

        if(same_list)
        {
            // text_list_add(text_lst, 3.0, "Same list");
            if(player->highlighted_index >= near_items_count)
            {
                // text_list_add(text_lst, 3.0, "same list: wrap selection");
                player->highlighted_index = 0;
            }
        }
        else
        {
            // text_list_add(text_lst, 3.0, "new list");
            player->highlighted_index = 0;
        }

        player->highlighted_item = &items[near_items[player->highlighted_index].index];
        player->highlighted_item->highlighted = true;

    }
    else
    {
        player->highlighted_item = NULL;
        player->highlighted_index = 0;
    }
}


void item_draw(Item* pu, bool batch)
{
    if(pu->curr_room != player->curr_room)
        return;

    uint32_t color = pu->highlighted ? COLOR_TINT_NONE : 0x88888888;

    int sprite_index = item_props[pu->type].sprite_index;

    if(pu->opened)
        sprite_index++;

    if(batch)
    {
        gfx_sprite_batch_add(item_props[pu->type].image, sprite_index, pu->phys.pos.x, pu->phys.pos.y, color, false, iscale, 0.0, 1.0, true, false, false);
    }
    else
    {
        gfx_draw_image(item_props[pu->type].image, sprite_index, pu->phys.pos.x, pu->phys.pos.y, color, iscale, 0.0, 1.0, true, IN_WORLD);
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
