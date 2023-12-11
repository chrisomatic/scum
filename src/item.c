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

// static void item_func_nothing(Item* pu, Player* p)
// {
//     return;
// }

static void item_func_chest(Item* pu, Player* p)
{
    float x = pu->phys.pos.x;
    float y = pu->phys.pos.y;
    int croom = pu->curr_room;

    // static int dbg = 0;
    // if(dbg == 0)
    // {
    //     dbg++;
    //     for(int i = 0; i < PLAYER_GAUNTLET_MAX; ++i)
    //     {
    //         item_add(ITEM_GAUNTLET_SLOT, x, y, croom);
    //     }
    //     return;
    // }
    // else if(dbg == 1)
    // {
    //     for(int i = 0; i < ITEM_MAX; ++i)
    //     {
    //         item_add(i, x, y, croom);
    //     }
    //     dbg++;
    // }
    // else if(dbg == 2)
    // {
    //     for(int i = 0; i < 8; ++i)
    //     {
    //         item_add(ITEM_CHEST, x, y, croom);
    //     }
    //     dbg++;
    // }

    // always one gem
    item_add(item_get_random_gem(), x, y, croom);

    int r = rand()%100;

    if(r < 50)
    {
        item_add(item_get_random_gem(), x, y, croom);
    }
    else if(r < 55)
    {
        item_add(ITEM_CHEST, x, y, croom);
    }
    else
    {

        for(;;)
        {
            ItemType t = rand() % ITEM_MAX;
            if(item_is_gem(t)) continue;
            if(t == ITEM_CHEST) continue;
            if(t == ITEM_NEW_LEVEL) continue;
            if(t == ITEM_HEART_EMPTY) continue;
            item_add(t, x, y, croom);
            break;

        }
    }

}

static void item_func_consumable(Item* pu, Player* p)
{
    switch(pu->type)
    {
        case ITEM_HEART_FULL:
        {
            if(p->phys.hp < p->phys.hp_max)
            {
                player_add_hp(p, 2);
                pu->picked_up = true;
            }
        }   break;
        case ITEM_HEART_HALF:
        {
            if(p->phys.hp < p->phys.hp_max)
            {
                player_add_hp(p, 1);
                pu->picked_up = true;
            }
        } break;
        case ITEM_COSMIC_HEART_FULL:
        {
            p->phys.hp_max += 2;
            player_add_hp(p,2);
            // printf("hp_max: %d\n",p->phys.hp_max);
            pu->picked_up = true;
        } break;
        case ITEM_COSMIC_HEART_HALF:
        {
            p->phys.hp_max += 1;
            pu->picked_up = true;
            player_add_hp(p,1);
            // printf("hp_max: %d\n",p->phys.hp_max);
        } break;
        case ITEM_GLOWING_ORB:
            p->light_radius += 1.0;
            pu->picked_up = true;
            break;
        case ITEM_NEW_LEVEL:
        {
            pu->picked_up = true;
            // item_remove(pu);

            int seed = time(0)+rand()%1000;
            game_generate_level(seed, level_rank+1);
        } break;

        case ITEM_GAUNTLET_SLOT:
        {
            if(p->gauntlet_slots < PLAYER_GAUNTLET_MAX)
            {
                p->gauntlet_slots++;
                pu->picked_up = true;
            }
        } break;
        default:
            break;
    }
}

static void item_func_gem(Item* pu, Player* p)
{
    Item* it = &p->gauntlet[p->gauntlet_selection];
    player_drop_item(p, it);
    memcpy(it, pu, sizeof(Item));

    for(int i = 0; i < p->gauntlet_slots; ++i)
    {
        p->gauntlet_selection++;
        if(p->gauntlet_selection >= p->gauntlet_slots)
            p->gauntlet_selection = 0;
        if(p->gauntlet[p->gauntlet_selection].type == ITEM_NONE)
            break;
    }
    pu->picked_up = true;
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
        p->func = NULL;

        switch(p->type)
        {
            case ITEM_GEM_RED:
            case ITEM_GEM_GREEN:
            case ITEM_GEM_BLUE:
            case ITEM_GEM_WHITE:
            case ITEM_GEM_YELLOW:
            case ITEM_GEM_PURPLE:
            {
                p->touchable = false;
                p->socketable = true;
                p->func = (void*)item_func_gem;

            } break;
            case ITEM_DRAGON_EGG:
            case ITEM_SHAMROCK:
            case ITEM_RUBY_RING:
            case ITEM_POTION_STRENGTH:
            case ITEM_POTION_SPEED:
            case ITEM_POTION_RANGE:
            case ITEM_POTION_PURPLE:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = NULL;
            } break;
            case ITEM_HEART_FULL:
            case ITEM_HEART_HALF:
            case ITEM_HEART_EMPTY:
            case ITEM_COSMIC_HEART_FULL:
            case ITEM_COSMIC_HEART_HALF:
            case ITEM_GLOWING_ORB:
            case ITEM_GAUNTLET_SLOT:
            case ITEM_NEW_LEVEL:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = (void*)item_func_consumable;
            } break;
            case ITEM_CHEST:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = (void*)item_func_chest;
            }
            break;
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

void item_apply_gauntlet(void* _proj_def, Item* gauntlet, uint8_t num_slots)
{
    ProjectileDef* proj_def = (ProjectileDef*)_proj_def;

    for(int i = 0; i < num_slots; ++i)
    {
        Item* item = &gauntlet[i];
        if(item->type == ITEM_NONE)
            continue;

        switch(item->type)
        {
            case ITEM_GEM_RED:
                proj_def->damage += 1.0;
                break;
            case ITEM_GEM_GREEN:
                proj_def->poison_chance = 1.0;
                break;
            case ITEM_GEM_BLUE:
                proj_def->cold_chance = 1.0;
                break;
            case ITEM_GEM_WHITE:
                proj_def->num += 1;
                break;
            case ITEM_GEM_YELLOW:
                proj_def->bouncy = true;
                break;
            case ITEM_GEM_PURPLE:
                proj_def->ghost = true;
                break;
            default:
                break;
        }
    }
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
        case ITEM_HEART_EMPTY: return "Empty Heart";
        case ITEM_COSMIC_HEART_FULL: return "Cosmic Full Heart";
        case ITEM_COSMIC_HEART_HALF: return "Cosmic Half Heart";
        case ITEM_GLOWING_ORB: return "Glowing Orb";
        case ITEM_DRAGON_EGG: return "Dragon Egg";
        case ITEM_SHAMROCK: return "Shamrock";
        case ITEM_RUBY_RING: return "Ruby Ring";
        case ITEM_POTION_STRENGTH: return "Potion of Strength";
        case ITEM_POTION_SPEED: return "Potion of Speed";
        case ITEM_POTION_RANGE: return "Potion of Range";
        case ITEM_POTION_PURPLE: return "Potion of Purple";
        case ITEM_NEW_LEVEL:  return "New Level";
        case ITEM_GAUNTLET_SLOT: return "+1 Gauntlet Slot";
    }
    return "???";
}

const char* item_get_description(ItemType type)
{
    switch(type)
    {
        case ITEM_NONE:       return "";
        case ITEM_CHEST:      return "contains loot";
        case ITEM_GEM_RED:    return "increase projectile damage";
        case ITEM_GEM_GREEN:  return "poison projectiles";
        case ITEM_GEM_BLUE:   return "cold projectiles";
        case ITEM_GEM_WHITE:  return "+1 projectile";
        case ITEM_GEM_YELLOW: return "bouncy projectiles";
        case ITEM_GEM_PURPLE: return "ghost projectiles";
        case ITEM_HEART_FULL: return "heal 1 heart";
        case ITEM_HEART_HALF: return "heal 1/2 heart";
        case ITEM_HEART_EMPTY: return "heal 0 heart";
        case ITEM_COSMIC_HEART_FULL: return "increase max health by 1 heart";
        case ITEM_COSMIC_HEART_HALF: return "increase max health by 1/2 heart";
        case ITEM_GLOWING_ORB: return "increase light radius";
        case ITEM_DRAGON_EGG: return "";
        case ITEM_SHAMROCK: return "";
        case ITEM_RUBY_RING: return "";
        case ITEM_POTION_STRENGTH: return "";
        case ITEM_POTION_SPEED: return "";
        case ITEM_POTION_RANGE: return "";
        case ITEM_POTION_PURPLE: return "";
        case ITEM_NEW_LEVEL:  return "enter new level";
        case ITEM_GAUNTLET_SLOT: return "+1 gauntlet slot";
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
        pu.phys.vel.z = 200.0;
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

    phys_apply_gravity(&pu->phys,GRAVITY_EARTH,dt);
    phys_apply_friction(&pu->phys,pu->phys.base_friction,dt);
}

typedef struct
{
    int index;
    uint16_t id;
    float dist;
} ItemSort;
#define NUM_NEAR_ITEMS  12
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

        if(pu->used)
            continue;

        if(item_props[pu->type].func == NULL)
            continue;

        // Vector2f c1 = {CPOSX(player->phys), CPOSY(player->phys)};
        Vector2f c1 = {player->phys.pos.x, player->phys.pos.y};
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
            else if(player->highlighted_index < 0)
            {
                player->highlighted_index = near_items_count-1;
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


void item_draw(Item* pu)
{
    if(pu->curr_room != player->curr_room)
        return;

    uint32_t color = pu->highlighted ? COLOR_TINT_NONE : 0x88888888;

    int sprite_index = item_props[pu->type].sprite_index;

    if(pu->used)
        sprite_index++;

    if(pu->type == ITEM_NEW_LEVEL)
    {
        pu->angle += 5.0;
    }

    float y = pu->phys.pos.y - 0.5*pu->phys.pos.z;
    gfx_sprite_batch_add(item_props[pu->type].image, sprite_index, pu->phys.pos.x, y, color, false, iscale, pu->angle, 1.0, true, false, false);
}

void item_handle_collision(Item* p, Entity* e)
{
    Vector3f ppos = p->phys.pos;

    switch(e->type)
    {
        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            if(p2->type == ITEM_CHEST && p2->used)
                return;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&p->phys, &p2->phys,&ci);
            }

            if(p->type == ITEM_CHEST && p->used)
            {
                p->phys.pos = ppos;
            }

        } break;
    }
}

bool item_is_on_tile(Room* room, int tile_x, int tile_y)
{
    Rect rp = level_get_tile_rect(tile_x, tile_y);

    for(int i = item_list->count-1; i >= 0; --i)
    {
        Item* pu = &items[i];

        if(pu->curr_room != room->index)
            continue;

        float _x = CPOSX(pu->phys);
        float _y = CPOSY(pu->phys);

        // simple check
        Rect ir = RECT(_x, _y, 1, 1);

        if(rectangles_colliding(&ir, &rp))
        {
            return true;
        }

    }

    return false;
}