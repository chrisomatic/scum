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

glist* item_list = NULL;
Item prior_items[MAX_ITEMS] = {0};
Item items[MAX_ITEMS] = {0};
ItemProps item_props[ITEM_MAX] = {0};

int items_image = -1;
int chest_image = -1;
int shrine_image = -1;
int podium_image = -1;
int portal_image = -1;
int guns_image = -1;

static int chestables_index[ITEM_MAX] = {0};
static int num_chestables = 0;

#define ITEM_USED_EXISTS(_type_) (_type_ == ITEM_CHEST || _type_ == ITEM_CHEST_GUN || _type_ == ITEM_SHRINE || _type_ == ITEM_PODIUM || _type_ == ITEM_PORTAL)

static uint16_t id_counter = 1;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

static bool item_func_chest(Item* it, Player* p)
{
    // int* d = NULL;
    // printf("%d\n",*d);

    if(it->used) return false;
    it->used = true;

    it->phys.vr = gfx_images[ item_props[it->type].image ].visible_rects[ item_props[it->type].sprite_index+1 ];
    item_calc_phys_props(it);

    float x = it->phys.pos.x;
    float y = it->phys.pos.y;
    int croom = it->phys.curr_room;

    int num = player_get_active_count();
    ItemType lst[10] = {0};

    for(int i = 0; i < num; ++i)
    {
#if 1
        item_add(item_get_random_chestable(), x, y, croom);
#else
        // this code drops unique items
        for(;;)
        {
            ItemType type = item_get_random_chestable();
            if(type == ITEM_GUN) continue;
            bool dup = false;

            for(int j = 0; j < i; ++j)
            {
                if(lst[j] == type)
                {
                    dup = true;
                    break;
                }
            }
            if(i >= num_chestables) dup = false;
            if(dup) continue;    //ignore dup check for now

            item_add(type, x, y, croom);
            lst[i] = type;
            break;
        }
#endif
    }

    return true;
}

static bool item_func_chest_gun(Item* it, Player* p)
{
    // int* d = NULL;
    // printf("%d\n",*d);

    if(it->used) return false;
    it->used = true;

    it->phys.vr = gfx_images[ item_props[it->type].image ].visible_rects[ item_props[it->type].sprite_index+1 ];
    item_calc_phys_props(it);

    float x = it->phys.pos.x;
    float y = it->phys.pos.y;
    int croom = it->phys.curr_room;

    int num = player_get_active_count() + 1;

    for(int i = 0; i < num; ++i)
    {
        int r = rand() % gun_catalog_count;
        Item* it = item_add_gun(r, rand(), x, y, croom);
        if(!it) continue;
        refresh_visible_room_gun_list();
        // add_to_room_gun_list(it);
    }

    return true;
}

static bool item_func_revive(Item* it, Player* p)
{
    if(it->used) return false;
    p->revives++;
    it->used = true;
    return true;

    // int others[MAX_PLAYERS] = {0};
    // int count = 0;

    // for(int i = 0; i < MAX_PLAYERS; ++i)
    // {
    //     Player* p2 = &players[i];
    //     // if(p == p2) continue;
    //     if(!p2->active) continue;
    //     if(p2->phys.dead) others[count++] = i;
    // }

    // if(count > 0)
    // {
    //     it->used = true;
    //     Player* p2 = &players[others[rand()%count]];
    //     p2->phys.dead = false;
    //     p2->phys.hp = 2;
    //     p2->phys.floating = false; //TODO
    // }

    // return it->used;
}

static bool item_func_skull(Item* it, Player* p)
{
    if(it->used) return false;

    const float s = 5.0;

    p->phys.vel.z += 50.0*s;
    p->phys.vel.x += RAND_FLOAT(-100.0,100.0)*s;
    p->phys.vel.y += RAND_FLOAT(-100.0,100.0)*s;

    status_effects_add_type(&p->phys, it->phys.curr_room, STATUS_EFFECT_POISON);

    it->used = true;
    return it->used;
}

static bool item_func_podium(Item* it, Player* p)
{
    if(it->used) return false;
    it->used = true;

    float x = it->phys.pos.x;
    float y = it->phys.pos.y;
    int croom = it->phys.curr_room;

    return true;
}

static bool item_func_portal(Item* it, Player* p)
{
    if(it->used) return false;

    it->used = true;

    if(level_grace_time > 0) return false;

    trigger_generate_level(rand(), level_rank+1, 2, __LINE__);
    if(role == ROLE_SERVER)
    {
        NetEvent ev = {.type = EVENT_TYPE_NEW_LEVEL};
        net_server_add_event(&ev);
    }

    return true;
}

static bool item_func_shrine(Item* it, Player* p)
{
    if(it->used) return false;
    it->used = true;

    ParticleSpawner* ps = particles_spawn_effect(it->phys.pos.x,it->phys.pos.y, 0.0, &particle_effects[EFFECT_SHRINE], 2.0, true, false);
    if(ps != NULL) ps->userdata = (int)it->phys.curr_room;

    float x = it->phys.pos.x;
    float y = it->phys.pos.y;
    int croom = it->phys.curr_room;

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
            // room->doors_locked = true;
            ui_message_set_title(2.0, message_color, message_scale, "Death");
        }   break;
        case 2:
        // {
        //     // poison everyone
        //     int num = player_get_active_count();
        //     for(int i = 0; i < num; ++i)
        //     {
        //         Player* x = &players[i];
        //         if(x->phys.curr_room != it->phys.curr_room)
        //             continue;
        //         status_effects_add_type(&x->phys, it->phys.curr_room, STATUS_EFFECT_POISON);
        //     }
        //     ui_message_set_title(2.0, message_color, message_scale, "Pestilence");
        // }   break;
        case 3:
        {
            creature_add(level.rooms_ptr[croom], CREATURE_TYPE_PHANTOM, NULL, NULL);
            ui_message_set_title(2.0, message_color, message_scale, "Phantom");
            // int num = player_get_active_count();
            // for(int i = 0; i < num; ++i)
            // {
            //     Player* x = &players[i];
            //     if(x->phys.curr_room != it->phys.curr_room)
            //         continue;
            //     player_add_xp(x, 300);
            // }
            // ui_message_set_title(2.0, message_color, message_scale, "The best gift is that of experience");
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


/*

+ ITEM_COSMIC_HEART_FULL: Increase Hearts by 1
+ ITEM_COSMIC_HEART_HALF: Increase Hearts by 1/2
+ ITEM_DRAGON_EGG:        Giant (size increase)
+ ITEM_GEM_YELLOW:        Dwarf (size decrease)
+ ITEM_WING:              Floating
+ ITEM_FEATHER:           Floating jump
+ ITEM_RUBY_RING:         Falling in pits does half damage
+ ITEM_POTION_STRENGTH:   Massive (Push Heavy things)
+ ITEM_POTION_MANA:       Speed Increase
+ ITEM_POTION_GREAT_MANA: Decrease Mud Slowdown
+ ITEM_POTION_PURPLE:     Ignore Mud
+ ITEM_SHAMROCK:          Higher chance to drop coin/heart after room clear
ITEM_GAUNTLET_SLOT:     Increase Chance to Block Hits
ITEM_UPGRADE_ORB:       Periodically shoot projectiles from self
ITEM_GALAXY_PENDANT:    Chance to shoot homing projectiles when taking damage
ITEM_SHIELD:            Defence Increase
ITEM_LOOKING_GLASS:     Reveal Map
ITEM_BOOK:              Chance to Open Doors early
ITEM_GEM_WHITE:         Chance of Ethereal per room (pass through walls)
ITEM_GEM_PURPLE:        Falling rocks on death


*/

static bool internal_item_use(Item* it, void* _player)
{
    Player* p = (Player*)_player;

    switch(it->type)
    {
        case ITEM_GUN:
        {
            if(visible_room)
            {
                if(visible_room->type == ROOM_TYPE_SHOP)
                {
                    int cost = room_gun_list[it->user_data2].cost;

                    if(p->coins >= cost)
                    {
                        player_add_coins(p, -cost);
                        player_set_gun(p, it->user_data2, true);
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    player_set_gun(p, it->user_data2, true);
                }
            }
            else
            {
                player_set_gun(p, it->user_data2, true);
            }
        } break;
        case ITEM_HEART_FULL:
        {
            return player_add_hp(p, 2);
        } break;

        case ITEM_HEART_HALF:
        {
            return player_add_hp(p, 1);
        } break;

        case ITEM_POTION_MANA:
        {
            return player_add_stat(p, MOVEMENT_SPEED, 1);
        } break;

        case ITEM_POTION_GREAT_MANA:
        {
            p->phys.mud_ignore_factor = 0.3;
        } break;

        case ITEM_POTION_PURPLE:
        {
            p->phys.mud_ignore_factor = 0.6;
        } break;

        case ITEM_COSMIC_HEART_FULL:
        {
            p->phys.hp_max += 2;
            player_add_hp(p,2);
        } break;

        case ITEM_COSMIC_HEART_HALF:
        {
            p->phys.hp_max += 1;
            player_add_hp(p,1);
        } break;

        case ITEM_GALAXY_PENDANT:
        {
            p->phys.mp_max += 30;
            player_add_mp(p,30);
        } break;

        case ITEM_POTION_STRENGTH:
        {
            p->phys.mass = 10.0;
        } break;

        case ITEM_SHIELD:
        {
            return player_add_stat(p, DEFENSE, 1);
        } break;

        case ITEM_FEATHER:
        {
            p->gravity = 0.5;
        } break;

        case ITEM_WING:
        {
            p->phys.floating = true;
        } break;

        case ITEM_DRAGON_EGG:
        {
            p->phys.scale = 1.2;
        } break;

        case ITEM_GEM_YELLOW:
        {
            p->phys.scale = 0.8;
        } break;

        case ITEM_RUBY_RING:
        {
            p->phys.pit_damage = 0;
        } break;

        // [STAT] Range
        case ITEM_LOOKING_GLASS:
        {
            return player_add_stat(p, ATTACK_RANGE, 1);
        } break;

        // [STAT] Luck
        case ITEM_SHAMROCK:
        {
            return player_add_stat(p, LUCK, 1);
        } break;

        // [STAT]
        case ITEM_UPGRADE_ORB:
        {
            return player_add_stat(p, rand() % MAX_STAT_TYPE, 1);
        } break;

        case ITEM_COIN_COPPER:
        {
            player_add_coins(p, 1);
        } break;
        case ITEM_COIN_SILVER:
        {
            player_add_coins(p, 1);
        } break;
        case ITEM_COIN_GOLD:
        {
            player_add_coins(p, 1);
        } break;

        case ITEM_GAUNTLET_SLOT:
        {

        } break;

        case ITEM_NEW_LEVEL:
        {
            if(level_grace_time > 0) return false;

            trigger_generate_level(rand(), level_rank+1, 2, __LINE__);
            if(role == ROLE_SERVER)
            {
                NetEvent ev = {.type = EVENT_TYPE_NEW_LEVEL};
                net_server_add_event(&ev);
            }
        } break;

        case ITEM_REVIVE:
        {
            return item_func_revive(it, p);
        } break;

        case ITEM_CHEST:
        {
            item_func_chest(it, p);
        } break;

        case ITEM_CHEST_GUN:
        {
            item_func_chest_gun(it, p);
        } break;

        case ITEM_SHRINE:
        {
            item_func_shrine(it, p);
        } break;

        case ITEM_PODIUM:
        {
            item_func_podium(it, p);
        } break;

        case ITEM_PORTAL:
        {
            item_func_portal(it, p);
        } break;

        default:
        {
            return false;
        } break;
    }
    return true;
}

bool item_use(Item* it, void* _player)
{
    if(it->used) return false;

    Player* p = (Player*)_player;

    bool ret = internal_item_use(it, _player);

    if(ret)
    {
        it->used = true;
    }

    return ret;
}

void item_init()
{
    if(item_list)
        return;

    item_list = list_create((void*)items, MAX_ITEMS, sizeof(Item), false);

    items_image = gfx_load_image("src/img/items.png", false, false, 16, 16);
    chest_image = gfx_load_image("src/img/chest.png", false, false, 32, 32);
    shrine_image = gfx_load_image("src/img/shrine.png", false, false, 32, 48);
    podium_image = gfx_load_image("src/img/podium.png", false, false, 32, 48);
    portal_image = gfx_load_image("src/img/portal.png", false, false, 40, 56);
    guns_image = gfx_load_image("src/img/guns.png", false, false, 32, 32);

    for(int i = 0; i < ITEM_MAX; ++i)
    {
        ItemProps* p = &item_props[i];

        p->type = i;

        if(p->type == ITEM_CHEST)
        {
            p->image = chest_image;
            p->sprite_index = 0;
        }
        else if(p->type == ITEM_CHEST_GUN)
        {
            p->image = chest_image;
            p->sprite_index = 2;
        }
        else if(p->type == ITEM_SHRINE)
        {
            p->image = shrine_image;
            p->sprite_index = 0;
        }
        else if(p->type == ITEM_PODIUM)
        {
            p->image = podium_image;
            p->sprite_index = 0;
        }
        else if(p->type == ITEM_PORTAL)
        {
            p->image = portal_image;
            p->sprite_index = 0;
        }
        else if(p->type == ITEM_GUN)
        {
            p->image = guns_image;
            p->sprite_index = 0;
        }
        else
        {
            p->image = items_image;
            p->sprite_index = i;
        }

        p->touchable = false;
        p->chestable = false;

        switch(p->type)
        {
            case ITEM_COSMIC_HEART_FULL:
            case ITEM_COSMIC_HEART_HALF:
            case ITEM_HEART_FULL:
            case ITEM_HEART_HALF:
            case ITEM_DRAGON_EGG:
            case ITEM_GEM_YELLOW:
            case ITEM_WING:
            case ITEM_FEATHER:
            case ITEM_RUBY_RING:
            case ITEM_POTION_STRENGTH:
            case ITEM_POTION_MANA:
            case ITEM_POTION_GREAT_MANA:
            case ITEM_POTION_PURPLE:
            case ITEM_SHAMROCK:
            {
                p->chestable = true;
                p->touchable = false;
            } break;

            case ITEM_GUN:
            case ITEM_GALAXY_PENDANT:
            case ITEM_SHIELD:
            case ITEM_LOOKING_GLASS:
            case ITEM_UPGRADE_ORB:
            {
                p->chestable = false;
                p->touchable = false;
            } break;

            case ITEM_COIN_COPPER:
            case ITEM_COIN_SILVER:
            case ITEM_COIN_GOLD:
            {
                p->chestable = true;
                p->touchable = true;
            } break;
        }
    }

    // create index of chestables
    for(int i = 0; i < ITEM_MAX; ++i)
    {
        if(item_props[i].chestable)
            chestables_index[num_chestables++] = i;
    }
}

void item_clear_all()
{
    list_clear(item_list);
    memset(prior_items, 0, sizeof(Item)*MAX_ITEMS);
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
    return (type == ITEM_CHEST || type == ITEM_CHEST_GUN);
}

bool item_is_gem(ItemType type)
{
    return (type >= ITEM_GEM_RED && type <= ITEM_GEM_PURPLE);
}

bool item_is_heart(ItemType type)
{
    return (type >= ITEM_HEART_FULL && type <= ITEM_HEART_HALF);
}

bool item_is_coin(ItemType type)
{
    return (type >= ITEM_COIN_COPPER && type <= ITEM_COIN_GOLD);
}

ItemType item_get_random_chestable()
{
    int r = rand() % num_chestables;
    return item_props[chestables_index[r]].type;
}

ItemType item_get_random_coin()
{
    int r = rand() % 3;
    if(r == 0) return ITEM_COIN_COPPER;
    if(r == 1) return ITEM_COIN_SILVER;
    return ITEM_COIN_GOLD;
}

ItemType item_get_random_heart()
{
    int r = rand() % 2;
    if(r == 0) return ITEM_HEART_HALF;
    return ITEM_HEART_FULL;
}

// junk
ItemType item_rand(bool include_chest)
{
    for(;;)
    {
        ItemType t = rand() % ITEM_MAX;
        if(item_is_chest(t) && !include_chest) continue;
        if(item_is_shrine(t)) continue;
        if(t == ITEM_NEW_LEVEL) continue;
        if(t == ITEM_PORTAL) continue;
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
        case ITEM_SHRINE:     return "Shrine";
        case ITEM_CHEST:      return "Chest";
        case ITEM_CHEST_GUN:  return "Gun Chest";
        case ITEM_PODIUM:      return "Podium";
        case ITEM_PORTAL:      return "Portal";

        case ITEM_NEW_LEVEL:  return "New Level";
        case ITEM_SKULL:      return "Skull";
        case ITEM_GEM_RED:    return "Red Gem";
        case ITEM_GEM_GREEN:  return "Green Gem";
        case ITEM_GEM_BLUE:   return "Blue Gem";
        case ITEM_GEM_WHITE:  return "White Gem";
        case ITEM_GEM_YELLOW: return "Yellow Gem";
        case ITEM_GEM_PURPLE: return "Purple Gem";
        case ITEM_HEART_EMPTY: return "Empty Heart";
        case ITEM_DRAGON_EGG: return "Dragon Egg";
        case ITEM_RUBY_RING: return "Ruby Ring";
        case ITEM_POTION_PURPLE: return "Potion of Purple";
        case ITEM_GAUNTLET_SLOT: return "+1 Gauntlet Slot";

        case ITEM_HEART_FULL: return "Full Heart";
        case ITEM_HEART_HALF: return "Half Heart";
        case ITEM_POTION_MANA: return "Potion of Mana";
        case ITEM_POTION_GREAT_MANA: return "Potion of Great Mana";
        case ITEM_COSMIC_HEART_FULL: return "Cosmic Full Heart";
        case ITEM_COSMIC_HEART_HALF: return "Cosmic Half Heart";

        case ITEM_GALAXY_PENDANT: return "Galaxy Pendant";
        case ITEM_POTION_STRENGTH: return "Potion of Strength";
        case ITEM_SHIELD: return "Shield";
        case ITEM_FEATHER: return "Feather";
        case ITEM_WING: return "Wing";
        case ITEM_LOOKING_GLASS: return "Looking Glass";
        case ITEM_SHAMROCK: return "Shamrock";
        case ITEM_UPGRADE_ORB: return "Upgrade Orb";

        case ITEM_COIN_COPPER: return "Copper Coin";
        case ITEM_COIN_SILVER: return "Silver Coin";
        case ITEM_COIN_GOLD: return "Gold Coin";
        case ITEM_GUN: return "Gun";
    }
    return "???";
}

const char* item_get_description(ItemType type, Item* it)
{
    if(it)
    {
        if(strlen(it->desc) > 0)
        {
            return it->desc;
        }
    }

    switch(type)
    {
        case ITEM_NONE:       return "";
        case ITEM_REVIVE:     return "cheat death";
        case ITEM_SHRINE:     return "hmmmm";
        case ITEM_CHEST:      return "contains loot";
        case ITEM_CHEST_GUN:  return "contains guns";
        case ITEM_PODIUM:     return "this looks interesting";
        case ITEM_PORTAL:     return "Looks otherworldly";

        case ITEM_NEW_LEVEL:  return "enter new level";
        case ITEM_SKULL:      return "unknown";
        case ITEM_GEM_RED:    return "";
        case ITEM_GEM_GREEN:  return "";
        case ITEM_GEM_BLUE:   return "";
        case ITEM_GEM_WHITE:  return "";
        case ITEM_GEM_YELLOW: return "Dwarfism";
        case ITEM_GEM_PURPLE: return "";
        case ITEM_HEART_EMPTY: return "";
        case ITEM_DRAGON_EGG: return "Gigantism";
        case ITEM_RUBY_RING: return "Ignore Pit Damage";
        case ITEM_POTION_PURPLE: return "Ignore Mud";
        case ITEM_GAUNTLET_SLOT: return "";

        case ITEM_HEART_FULL: return "heal 1 heart";
        case ITEM_HEART_HALF: return "heal 1/2 heart";
        case ITEM_POTION_MANA: return "Speed Increase";
        case ITEM_POTION_GREAT_MANA: return "Decrease Mud Slowdown";
        case ITEM_COSMIC_HEART_FULL: return "increase hp by 1 heart";
        case ITEM_COSMIC_HEART_HALF: return "increase hp by 1/2 heart";

        case ITEM_GALAXY_PENDANT: return "increase mana";
        case ITEM_POTION_STRENGTH: return "Massive";
        case ITEM_SHIELD: return "+1 defense";
        case ITEM_FEATHER: return "Slow Fall";
        case ITEM_WING: return "Float";
        case ITEM_LOOKING_GLASS: return "+1 range";
        case ITEM_SHAMROCK: return "+1 luck";
        case ITEM_UPGRADE_ORB: return "upgrade stuff";

        case ITEM_COIN_COPPER: return "1 coin";
        case ITEM_COIN_SILVER: return "1 coin";
        case ITEM_COIN_GOLD:   return "1 coin";
    }
    return "???";
}

Item* item_add_gun(uint8_t gun_index, uint32_t seed, float x, float y, uint8_t curr_room)
{
    Item* it = item_add(ITEM_GUN, x, y, curr_room);
    it->user_data = gun_index;
    it->seed = seed;

    Gun* gun = &gun_catalog[it->user_data];
    item_set_description(it, "%s: %s", gun->name, gun->desc);

    it->phys.vr = gfx_images[ item_props[ITEM_GUN].image ].visible_rects[ gun->gun_sprite_index ];
    phys_calc_collision_rect(&it->phys);

    return it;
}

Item* item_add(ItemType type, float x, float y, uint8_t curr_room)
{
    Item it = {0};
    it.type = type;
    it.id = get_id();
    it.phys.scale = 0.8;
    it.phys.curr_room = curr_room;
    it.phys.pos.x = x;
    it.phys.pos.y = y;
    it.phys.pos.z = 0.0;
    it.phys.speed = 1.0;
    it.phys.crawling = true;

    switch(type)
    {
        case ITEM_CHEST:
        case ITEM_CHEST_GUN:
        {
            it.phys.mass = 3.0;
            it.phys.base_friction = 50.0;
            it.phys.elasticity = 0.2;
            it.phys.rotation_deg = 1.01;    // avoid horizontal bool in phys_calc_collision_rect()
        } break;

        case ITEM_SHRINE:
        {
            it.phys.crawling = false;
            it.phys.mass = 1000.0;
            it.phys.base_friction = 50.0;
            it.phys.elasticity = 0.2;
        } break;

        case ITEM_PODIUM:
        {
            it.phys.crawling = false;
            it.phys.mass = 1000.0;
            it.phys.base_friction = 50.0;
            it.phys.elasticity = 0.2;
        } break;
        case ITEM_PORTAL:
        {
            it.phys.crawling = false;
            it.phys.mass = 1000.0;
            it.phys.base_friction = 50.0;
            it.phys.elasticity = 0.2;
        } break;

        case ITEM_COIN_COPPER:
        case ITEM_COIN_SILVER:
        case ITEM_COIN_GOLD:
        {
            it.phys.mass = 0.5;
            it.phys.base_friction = 5.0;
            it.phys.elasticity = 0.5;
            it.phys.vel.z = 100.0;
        } break;
        case ITEM_SOCCER_BALL:
        {
            it.phys.mass = 0.3;
            it.phys.base_friction = 2.0;
            it.phys.elasticity = 0.8;
            it.phys.vel.z = 200.0;
        } break;
        default:
        {
            it.phys.mass = 0.5;
            it.phys.base_friction = 5.0;
            it.phys.elasticity = 0.4;
            it.phys.vel.z = 200.0;
        } break;

    }

    it.phys.vr = gfx_images[ item_props[it.type].image ].visible_rects[ item_props[it.type].sprite_index ];
    item_calc_phys_props(&it);

    // if(item_is_coin(type))
    // {
    //     it.phys.radius *= 3.0;
    // }

    // x,y position passed in are meant to be the collision positions
    phys_calc_collision_rect(&it.phys);
    phys_set_collision_pos(&it.phys, x, y);

    bool ret = list_add(item_list, &it);
    if(!ret) return NULL;
    return &items[item_list->count-1];
}

Item* item_add_to_tile(ItemType type, int x, int y, uint8_t curr_room)
{
    Vector2f pos = level_get_pos_by_room_coords(x, y);
    return item_add(type, pos.x, pos.y, curr_room);
}

void item_calc_phys_props(Item* it)
{
    if(it->phys.crawling)
    {
        it->phys.width  = it->phys.vr.w;
        it->phys.length = it->phys.vr.h;
        it->phys.height = MIN(it->phys.width, it->phys.length);
    }
    else
    {
        it->phys.width  = it->phys.vr.w;
        it->phys.length = it->phys.vr.w;
        it->phys.height = it->phys.vr.h;
    }
    it->phys.radius = MIN(it->phys.vr.w, it->phys.vr.h)/2.0 * it->phys.scale * 0.90;

    if(!it->phys.crawling)
    {
        it->phys.radius *= 0.8;
    }
    // if(type == ITEM_PODIUM)
    //     it->phys.radius *= 0.8;
    // else if(type == ITEM_SHRINE)
    //     it->phys.radius *= 0.8;

    // if(type == ITEM_CHEST)
    // {
    //     printf("[CHEST] \n");
    //     print_rect(&it->phys.vr);
    // }
}

void item_set_description(Item* it, char* fmt, ...)
{
    if(!it) return;
    memset(it->desc, 0, sizeof(char)*32);

    va_list args;
    va_start(args, fmt);
    vsnprintf(it->desc,32, fmt, args);
    va_end(args);
}


bool item_remove(Item* it)
{
    // return list_remove_by_item(item_list, it);
    for(int i = item_list->count-1; i >= 0; --i)
    {
        if(it->id == items[i].id)
        {
            list_remove(item_list, i);
            return true;
        }
    }
    return false;
}

void item_update(Item* it, float dt)
{
    it->phys.pos.x += dt*it->phys.vel.x;
    it->phys.pos.y += dt*it->phys.vel.y;

    phys_apply_gravity(&it->phys, GRAVITY_EARTH, dt);
    phys_apply_friction(&it->phys, it->phys.base_friction, dt);

    limit_rect_pos(&floor_area, &it->phys.collision_rect);

    if(it->type == ITEM_NEW_LEVEL)
    {
        if(role == ROLE_LOCAL)
        {
            it->angle += 80.0 * dt;
            it->angle = fmod(it->angle,360.0f);
        }
    }

    it->highlighted = false;
}

void item_update_all(float dt)
{

    for(int i = item_list->count-1; i >= 0; --i)
    {
        Item* it = &items[i];

        // remove previously used items
        if(it->used)
        {
            if(!ITEM_USED_EXISTS(it->type))
            {
                item_remove(it);
                continue;
            }
        }

        if(role == ROLE_CLIENT)
        {
            item_lerp(it, dt);
            continue;
        }

        item_update(it, dt);
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
            Item* it = &items[j];

            if(it->phys.curr_room != p->phys.curr_room) continue;
            if(it->used) continue;

            Vector2f c1 = {p->phys.pos.x, p->phys.pos.y};
            Vector2f c2 = {it->phys.pos.x, it->phys.pos.y};

            float distance;
            bool in_pickup_radius = circles_colliding(&c1, p->phys.radius, &c2, ITEM_PICKUP_RADIUS, &distance);

            if(in_pickup_radius && p->near_items_count < NUM_NEAR_ITEMS)
            {
                p->near_items[p->near_items_count].index = j;
                p->near_items[p->near_items_count].dist = distance;
                p->near_items[p->near_items_count].id = it->id;
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

void item_draw(Item* it)
{
    if(it->phys.curr_room != player->phys.curr_room)
        return;

    uint32_t color = 0x88888888;
    if(it->highlighted || it->id == player->highlighted_item_id)
    {
        color = COLOR_TINT_NONE;
    }

    int sprite_index = item_props[it->type].sprite_index;

    if(it->used)
    {
        if(ITEM_USED_EXISTS(it->type)) sprite_index++;
        else return;
    }

    if(it->type == ITEM_GUN)
    {
        Gun* gun = &gun_catalog[it->user_data];

        sprite_index = gun->gun_sprite_index;
        color = gfx_blend_colors(COLOR_BLACK, gun->color1, 0.7);

        if(it->highlighted || it->id == player->highlighted_item_id)
        {
            color = gun->color1;
        }
    }

    float y = it->phys.pos.y - (it->phys.vr.h * it->phys.scale + it->phys.pos.z)/2.0;

    gfx_sprite_batch_add(item_props[it->type].image, sprite_index, it->phys.pos.x, y, color, false, it->phys.scale, it->angle, 1.0, false, false, false);
}

void item_lerp(Item* it, double dt)
{
    const float decay = 16.0;
    it->phys.pos = exp_decay3f(it->phys.pos, it->server_state_target.pos, decay, dt);
    //it->angle = lerp_angle_deg(it->server_state_prior.angle, it->server_state_target.angle, t);

#if 1
    if(it->type == ITEM_NEW_LEVEL)
    {
        it->angle += 80.0 * dt;
        it->angle = fmod(it->angle, 360.0f);
    }
#endif
}

void item_handle_collision(Item* it, Entity* e)
{
    Vector3f ppos = it->phys.pos;

    switch(e->type)
    {
        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            if(p2->used || it->used) return;

            // if(item_is_chest(p2->type) &&  p2->used)
            //     return;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&it->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&it->phys, &p2->phys,&ci);
            }

            // if(item_is_chest(it->type) && it->used)
            // {
            //     it->phys.pos = ppos;
            // }

        } break;
    }
}

bool item_is_on_tile(Room* room, int tile_x, int tile_y)
{
    Rect rp = level_get_tile_rect(tile_x, tile_y);

    for(int i = item_list->count-1; i >= 0; --i)
    {
        Item* it = &items[i];

        if(it->phys.curr_room != room->index)
            continue;

        float _x = it->phys.pos.x;
        float _y = it->phys.pos.y;

        // simple check
        Rect ir = RECT(_x, _y, 1, 1);

        if(rectangles_colliding(&ir, &rp))
        {
            return true;
        }

    }

    return false;
}
