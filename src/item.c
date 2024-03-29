#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"
#include "net.h"
#include "effects.h"
#include "creature.h"
#include "ui.h"

#include "player.h"
#include "item.h"

const float iscale = 0.8;

glist* item_list = NULL;
Item prior_items[MAX_ITEMS] = {0};
Item items[MAX_ITEMS] = {0};
ItemProps item_props[ITEM_MAX] = {0};
int items_image = -1;
int chest_image = -1;
int shrine_image = -1;

static uint16_t id_counter = 1;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}


static bool item_func_chest(Item* pu, Player* p)
{
    if(pu->used) return false;
    pu->used = true;

    float x = pu->phys.pos.x;
    float y = pu->phys.pos.y;
    int croom = pu->curr_room;

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
            if(item_is_chest(t)) continue;
            if(item_is_shrine(t)) continue;
            if(t == ITEM_NEW_LEVEL) continue;
            if(t == ITEM_HEART_EMPTY) continue;
            item_add(t, x, y, croom);
            break;
        }
    }

    return true;
}

static bool item_func_revive(Item* pu, Player* p)
{
    if(pu->used) return false;

    int others[MAX_PLAYERS] = {0};
    int count = 0;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p2 = &players[i];
        if(p == p2) continue;
        if(!p2->active) continue;
        if(p2->phys.dead) others[count++] = i;
    }

    if(count > 0)
    {
        pu->used = true;
        Player* p2 = &players[others[rand()%count]];
        p2->phys.dead = false;
        p2->phys.hp = 2;
        p2->phys.floating = false; //TODO
    }

    return pu->used;
}

static bool item_func_skull(Item* pu, Player* p)
{
    if(pu->used) return false;

    const float s = 5.0;

    p->phys.vel.z += 50.0*s;
    p->phys.vel.x += RAND_FLOAT(-100.0,100.0)*s;
    p->phys.vel.y += RAND_FLOAT(-100.0,100.0)*s;

    status_effects_add_type(&p->phys, pu->curr_room, STATUS_EFFECT_POISON);

    pu->used = true;
    return pu->used;
}

static bool item_func_shrine(Item* pu, Player* p)
{
    if(pu->used) return false;
    pu->used = true;

    ParticleSpawner* ps = particles_spawn_effect(pu->phys.pos.x,pu->phys.pos.y, 0.0, &particle_effects[EFFECT_SHRINE], 2.0, true, false);
    if(ps != NULL) ps->userdata = (int)pu->curr_room;

    float x = pu->phys.pos.x;
    float y = pu->phys.pos.y;
    int croom = pu->curr_room;

    int r = rand() % 7;

    uint32_t message_color = 0x00CC00CC;
    float message_scale = 1.0;

    switch(r)
    {
        case 0:
        {
            // heart
            item_add(item_get_random_heart(), x, y, croom);
            ui_message_set_title(2.0, message_color, message_scale, "To lose thee were to lose myself");
        }   break;
        case 1:
        {
            // random creature
            Room* room = level_get_room_by_index(&level, croom);
            creature_add(room, creature_get_random(), NULL, NULL);
            room->doors_locked = true;
            ui_message_set_title(2.0, message_color, message_scale, "Death");
        }   break;
        case 2:
        {
            // poison everyone
            int num = player_get_active_count();
            for(int i = 0; i < num; ++i)
            {
                Player* x = &players[i];
                if(x->curr_room != pu->curr_room)
                    continue;

                status_effects_add_type(&x->phys, pu->curr_room, STATUS_EFFECT_POISON);
            }

            ui_message_set_title(2.0, message_color, message_scale, "Pestilence");
        }   break;
        case 3:
        {
            int num = player_get_active_count();
            for(int i = 0; i < num; ++i)
            {
                Player* x = &players[i];
                if(x->curr_room != pu->curr_room)
                    continue;

                player_add_xp(x, 300);
            }

            ui_message_set_title(2.0, message_color, message_scale, "The best gift is that of experience");
        }   break;
        case 4:
        {
            ItemType it = item_rand(true);
            item_add(it, x, y, croom);
            ui_message_set_title(2.0, message_color, message_scale, "A small treasure for your trouble");
        } break;
        case 5:
        {
            level.darkness_curse = true;
            ui_message_set_title(2.0, message_color, message_scale, "It is dark");
        } break;
        case 6:
        {
            item_add(ITEM_REVIVE, x, y, croom);
            ui_message_set_title(2.0, message_color, message_scale, "Revival item");
        } break;
    }

    return true;
}

// called when player consumes the item
static bool item_timed_func_consumable_start(Item* pu, Player* p)
{
    float ttl = 10.0;
    switch(pu->type)
    {
        case ITEM_GEM_RED:
        case ITEM_GEM_GREEN:
        case ITEM_GEM_BLUE:
        case ITEM_GEM_WHITE:
        case ITEM_GEM_YELLOW:
        case ITEM_GEM_PURPLE:
            ttl = 20.0;
            break;

        case ITEM_SHAMROCK:
            ttl = 10.0;
            break;

        case ITEM_DRAGON_EGG:
            ttl = 30.0;
            break;

        case ITEM_RUBY_RING:
            ttl = 30.0;
            break;

        case ITEM_POTION_STRENGTH:
        case ITEM_POTION_SPEED:
        case ITEM_POTION_RANGE:
        case ITEM_POTION_PURPLE:
            ttl = 15.0;
            break;

        default: break;
    }

    bool existing = false;
    bool effect_added = false;

    for(int i = 0; i < MAX_TIMED_ITEMS; ++i)
    {
        if(p->timed_items[i] == pu->type)
        {
            p->timed_items_ttl[i] += ttl;
            existing = true;
            effect_added = true;
            break;
        }
    }

    if(!effect_added)
    {

        for(int i = 0; i < MAX_TIMED_ITEMS; ++i)
        {
            if(p->timed_items[i] == ITEM_NONE)
            {
                p->timed_items[i] = pu->type;
                p->timed_items_ttl[i] = ttl;
                effect_added = true;
                break;
            }
        }
    }

    if(!effect_added)
    {
        return false;
    }

    // have to handle these effects here
    switch(pu->type)
    {
        case ITEM_SHAMROCK:
            p->invulnerable = true;
            break;

        case ITEM_DRAGON_EGG:
        {
            if(!existing)
            {
                p->phys.hp_max += 4;
                p->phys.hp = p->phys.hp_max;
            }
        } break;

        default:
            break;
    }

    return true;
}

// called after player consumes the item (every frame)
static void item_timed_func_consumable_periodic(ItemType type, Player* p)
{
    switch(type)
    {
        case ITEM_GEM_RED:
            p->proj_def.damage += 1.0;
            break;
        case ITEM_GEM_GREEN:
            p->proj_spawn.poison_chance = 1.0;
            break;
        case ITEM_GEM_BLUE:
            p->proj_spawn.cold_chance = 1.0;
            break;
        case ITEM_GEM_WHITE:
            p->proj_spawn.num += 1;
            break;
        case ITEM_GEM_YELLOW:
            p->proj_def.bouncy = true;
            break;
        case ITEM_GEM_PURPLE:
            p->proj_spawn.ghost_chance = 1.0;
            break;


        case ITEM_RUBY_RING:
            p->proj_cooldown_max -= 0.08;
            break;

        case ITEM_POTION_STRENGTH:
        case ITEM_POTION_SPEED:
        case ITEM_POTION_RANGE:
        case ITEM_POTION_PURPLE:
            p->phys.speed += 100.0;
            p->phys.max_velocity += 100.0;
            break;

        default: break;
    }

}

static void item_timed_func_consumable_end(ItemType type, Player* p)
{
    switch(type)
    {

        case ITEM_DRAGON_EGG:
        {
            p->phys.hp_max -= 4;
            if(p->phys.hp > p->phys.hp_max)
                p->phys.hp = p->phys.hp_max;
        } break;
    
        case ITEM_SHAMROCK:
            p->invulnerable = false;
            break;

        default: break;
    }
}


static bool item_func_consumable(Item* pu, Player* p)
{
    switch(pu->type)
    {
        case ITEM_HEART_FULL:
        {
            if(p->phys.hp < p->phys.hp_max)
            {
                player_add_hp(p, 2);
                // pu->picked_up = true;
            }
            else
            {
                return false;
            }
        } break;
        case ITEM_HEART_HALF:
        {
            if(p->phys.hp < p->phys.hp_max)
            {
                player_add_hp(p, 1);
                // pu->picked_up = true;
            }
            else
            {
                return false;
            }
        } break;
        case ITEM_COSMIC_HEART_FULL:
        {
            p->phys.hp_max += 2;
            // pu->picked_up = true;
            player_add_hp(p,2);
            // printf("hp_max: %d\n",p->phys.hp_max);
        } break;
        case ITEM_COSMIC_HEART_HALF:
        {
            p->phys.hp_max += 1;
            // pu->picked_up = true;
            player_add_hp(p,1);
            // printf("hp_max: %d\n",p->phys.hp_max);
        } break;
        case ITEM_GLOWING_ORB:
        {
            p->light_radius += 1.0;
            // pu->picked_up = true;
        } break;
        case ITEM_GAUNTLET_SLOT:
        {
            if(p->gauntlet_slots < PLAYER_GAUNTLET_MAX)
            {
                p->gauntlet_slots++;
                // pu->picked_up = true;
            }
        } break;
        default:
        {
            return false;
        } break;
    }
    return true;
}

void item_init()
{
    if(item_list)
        return;

    item_list = list_create((void*)items, MAX_ITEMS, sizeof(Item), false);

    items_image = gfx_load_image("src/img/items.png", false, false, 16, 16);
    chest_image = gfx_load_image("src/img/chest.png", false, false, 32, 32);
    shrine_image = gfx_load_image("src/img/shrine.png", false, false, 32, 48);

    for(int i = 0; i < ITEM_MAX; ++i)
    {
        ItemProps* p = &item_props[i];

        p->type = i;

        if(p->type == ITEM_CHEST)
        {
            p->image = chest_image;
            p->sprite_index = 0;
        }
        else if(p->type == ITEM_SHRINE)
        {
            p->image = shrine_image;
            p->sprite_index = 0;
        }
        else
        {
            p->image = items_image;
            p->sprite_index = i;
        }

        p->func = NULL;

        p->socketable = false;

        switch(p->type)
        {
            case ITEM_GEM_RED:
            case ITEM_GEM_GREEN:
            case ITEM_GEM_BLUE:
            case ITEM_GEM_WHITE:
            case ITEM_GEM_YELLOW:
            case ITEM_GEM_PURPLE:
            {
                p->socketable = true;
                p->touchable = false;
                p->func = (void*)item_timed_func_consumable_start;
                p->timed_func = (void*)item_timed_func_consumable_periodic;
                // p->timed_func_end = (void*)item_timed_func_consumable_end;

            } break;
            case ITEM_DRAGON_EGG:
            case ITEM_SHAMROCK:
            case ITEM_RUBY_RING:
            case ITEM_POTION_STRENGTH:
            case ITEM_POTION_SPEED:
            case ITEM_POTION_RANGE:
            case ITEM_POTION_PURPLE:
            {
                p->socketable = true;
                p->touchable = false;
                p->func = (void*)item_timed_func_consumable_start;
                p->timed_func = (void*)item_timed_func_consumable_periodic;
                p->timed_func_end = (void*)item_timed_func_consumable_end;
            } break;
            case ITEM_HEART_FULL:
            case ITEM_HEART_HALF:
            case ITEM_HEART_EMPTY:
            case ITEM_COSMIC_HEART_FULL:
            case ITEM_COSMIC_HEART_HALF:
            case ITEM_GLOWING_ORB:
            case ITEM_GAUNTLET_SLOT:
            {
                p->socketable = true;
                p->touchable = false;
                p->func = (void*)item_func_consumable;
            } break;
            case ITEM_CHEST:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = (void*)item_func_chest;
            } break;
            case ITEM_SHRINE:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = (void*)item_func_shrine;
            } break;
            case ITEM_NEW_LEVEL:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = NULL;
            } break;
            case ITEM_REVIVE:
            {
                p->touchable = false;
                p->socketable = false;
                p->func = (void*)item_func_revive;
            } break;
            case ITEM_SKULL:
            {
                p->touchable = false;
                p->socketable = true;
                p->func = (void*)item_func_skull;
            } break;
        }

    }
}

void item_clear_all()
{
    list_clear(item_list);
}

Item* item_get_by_id(uint16_t id)
{
    for(int i = 0; i < item_list->count; ++i)
    {
        if(items[i].id == id)
        {
            return &items[i];
        }
    }
    return NULL;
}

bool item_is_shrine(ItemType type)
{
    return (type == ITEM_SHRINE);
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

void item_apply_gauntlet(void* _proj_def, void* _proj_spawn, Item* gauntlet, uint8_t num_slots)
{
    ProjectileDef* proj_def = (ProjectileDef*)_proj_def;
    ProjectileSpawn* proj_spawn = (ProjectileSpawn*)_proj_spawn;

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
                proj_spawn->poison_chance = 1.0;
                break;
            case ITEM_GEM_BLUE:
                proj_spawn->cold_chance = 1.0;
                break;
            case ITEM_GEM_WHITE:
                proj_spawn->num += 1;
                break;
            case ITEM_GEM_YELLOW:
                proj_def->bouncy = true;
                break;
            case ITEM_GEM_PURPLE:
                proj_spawn->ghost_chance = 1.0;
                break;
            default:
                break;
        }
    }
}

ItemType item_get_random_gem()
{
    // kinda inefficient but no big deal
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

ItemType item_rand(bool include_chest)
{
    for(;;)
    {
        ItemType t = rand() % ITEM_MAX;
        if(item_is_chest(t) && !include_chest) continue;
        if(item_is_shrine(t)) continue;
        if(t == ITEM_NEW_LEVEL) continue;
        if(t == ITEM_HEART_EMPTY) continue;

        //TEMP
        if(t == ITEM_SILVER_STAR) continue;
        if(t == ITEM_GOLD_STAR) continue;
        if(t == ITEM_PURPLE_STAR) continue;

        return t;
    }
}

const char* item_get_name(ItemType type)
{
    switch(type)
    {
        case ITEM_NONE:       return "None";
        case ITEM_REVIVE:     return "Revive";
        case ITEM_SKULL:      return "Skull";
        case ITEM_SHRINE:     return "Shrine";
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
        case ITEM_REVIVE:     return "cheat death";
        case ITEM_SKULL:      return "Probably a good sign";
        case ITEM_SHRINE:     return "hmmmmmm";
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

Item* item_add(ItemType type, float x, float y, uint8_t curr_room)
{
    Item pu = {0};
    pu.type = type;
    pu.id = get_id();
    pu.picked_up = false;
    pu.curr_room = curr_room;
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.speed = 1.0;
    pu.phys.crawling = true;

    if(type == ITEM_CHEST)
    {
        pu.phys.mass = 2.0;
        pu.phys.base_friction = 30.0;
        pu.phys.radius = 12*iscale; //TEMP
        pu.phys.elasticity = 0.2;
    }
    else if(type == ITEM_SHRINE)
    {
        pu.phys.mass = 1000.0;
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

    pu.phys.width  = 1.6f*pu.phys.radius;
    pu.phys.length = 1.6f*pu.phys.radius;
    pu.phys.height = 1.6f*pu.phys.radius;
    pu.phys.vr = gfx_images[ item_props[pu.type].image ].visible_rects[0];

    bool ret = list_add(item_list,&pu);

    if(!ret) return NULL;

    return &items[item_list->count];
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

    if(pu->type == ITEM_NEW_LEVEL)
    {
        if(role != ROLE_SERVER)
        {
            pu->angle += 80.0 * dt;
            pu->angle = fmod(pu->angle,360.0f);
        }
    }

    pu->highlighted = false;
}

void item_update_all(float dt)
{
    for(int i = item_list->count-1; i >= 0; --i)
    {
        Item* pu = &items[i];

        if(role == ROLE_CLIENT)
        {
            item_lerp(pu, dt);
            continue;
        }

        if(pu->picked_up)
        {
            list_remove(item_list, i);
            continue;
        }

        item_update(pu, dt);
    }

    if(role == ROLE_CLIENT)
        return;

    for(int k = 0; k < MAX_PLAYERS; ++k)
    {
        Player* p = &players[k];

        if(role == ROLE_LOCAL && p != player) continue;
        if(!p->active) continue;
        if(p->phys.dead) continue;

        ItemSort near_items_prior[NUM_NEAR_ITEMS] = {0};
        int near_items_count_prior = 0;

        memcpy(near_items_prior, p->near_items, sizeof(ItemSort)*NUM_NEAR_ITEMS);
        near_items_count_prior = p->near_items_count;

        memset(p->near_items, 0, sizeof(ItemSort)*NUM_NEAR_ITEMS);
        p->near_items_count = 0;

        for(int j = 0; j < item_list->count; ++j)
        {
            Item* pu = &items[j];

            if(pu->curr_room != p->curr_room) continue;
            if(pu->used) continue;

            Vector2f c1 = {p->phys.pos.x, p->phys.pos.y};
            Vector2f c2 = {pu->phys.pos.x, pu->phys.pos.y};

            float distance;
            bool in_pickup_radius = circles_colliding(&c1, p->phys.radius, &c2, ITEM_PICKUP_RADIUS, &distance);

            if(in_pickup_radius && p->near_items_count < NUM_NEAR_ITEMS)
            {
                p->near_items[p->near_items_count].index = j;
                p->near_items[p->near_items_count].dist = distance;
                p->near_items[p->near_items_count].id = pu->id;
                p->near_items_count++;
            }

        }   //items

        if(p->near_items_count > 0)
        {
            // insertion sort
            int i, j;
            ItemSort key;
            for (i = 1; i < p->near_items_count; ++i) 
            {
                memcpy(&key, &p->near_items[i], sizeof(ItemSort));
                j = i - 1;

                while (j >= 0 && p->near_items[j].dist > key.dist)
                {
                    memcpy(&p->near_items[j+1], &p->near_items[j], sizeof(ItemSort));
                    j = j - 1;
                }
                memcpy(&p->near_items[j+1], &key, sizeof(ItemSort));
            }

            bool same_list = true;
            if(p->near_items_count != near_items_count_prior)
            {
                same_list = false;
            }
            else
            {
                for(int i = 0; i < p->near_items_count; ++i)
                {
                    if(p->near_items[i].id != near_items_prior[i].id)
                    {
                        same_list = false;
                        break;
                    }
                }
            }

            if(same_list)
            {
                // text_list_add(text_lst, 3.0, "Same list");
                if(p->highlighted_index >= p->near_items_count)
                {
                    // text_list_add(text_lst, 3.0, "same list: wrap selection -> 0");
                    p->highlighted_index = 0;
                }
                else if(p->highlighted_index < 0)
                {
                    // text_list_add(text_lst, 3.0, "same list: wrap selection -> %d", p->near_items_count-1);
                    p->highlighted_index = p->near_items_count-1;
                }
            }
            else
            {
                // text_list_add(text_lst, 3.0, "new list");
                p->highlighted_index = 0;
            }

            Item* it = &items[p->near_items[p->highlighted_index].index];
            it->highlighted = true;

            // if(p->highlighted_item_id != it->id)
            // {
            //     text_list_add(text_lst, 3.0, "new id: %u", it->id);
            // }
            p->highlighted_item_id = it->id;

            // p->highlighted_item = it;
            // player->highlighted_item = &items[near_items[player->highlighted_index].index];
            // player->highlighted_item->highlighted = true;

        }
        else
        {
            // p->highlighted_item = NULL;
            p->highlighted_item_id = -1;
            p->highlighted_index = 0;
        }
    }   //players

}

void item_draw(Item* pu)
{
    if(pu->curr_room != player->curr_room)
        return;

    uint32_t color = 0x88888888;
    if(pu->highlighted || pu->id == player->highlighted_item_id)
    {
        color = COLOR_TINT_NONE;
    }

    int sprite_index = item_props[pu->type].sprite_index;

    if(pu->used)
        sprite_index++;

    // float y = pu->phys.pos.y - 0.5*pu->phys.pos.z;
    float y = pu->phys.pos.y - (pu->phys.vr.h + pu->phys.pos.z)/2.0;

    gfx_sprite_batch_add(item_props[pu->type].image, sprite_index, pu->phys.pos.x, y, color, false, iscale, pu->angle, 1.0, true, false, false);
}

void item_lerp(Item* it, double dt)
{
    it->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (it->lerp_t / tick_time);

    Vector3f lp = lerp3f(&it->server_state_prior.pos,&it->server_state_target.pos,t);
    it->phys.pos.x = lp.x;
    it->phys.pos.y = lp.y;
    it->phys.pos.z = lp.z;
    //printf("prior_pos: %f %f, target_pos: %f %f, pos: %f %f, t: %f\n",p->server_state_prior.pos.x, p->server_state_prior.pos.y, p->server_state_target.pos.x, p->server_state_target.pos.y, p->phys.pos.x, p->phys.pos.y, t);

#if 1
    if(it->type == ITEM_NEW_LEVEL)
    {
        it->angle += 80.0 * dt;
        it->angle = fmod(it->angle,360.0f);
    }
#else
    it->angle = lerp_angle_deg(it->server_state_prior.angle, it->server_state_target.angle, t);
#endif
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

        float _x = pu->phys.pos.x;
        float _y = pu->phys.pos.y;

        // simple check
        Rect ir = RECT(_x, _y, 1, 1);

        if(rectangles_colliding(&ir, &rp))
        {
            return true;
        }

    }

    return false;
}
