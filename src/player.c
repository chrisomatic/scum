#include "headers.h"
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "level.h"
#include "projectile.h"
#include "creature.h"
#include "camera.h"
#include "lighting.h"
#include "status_effects.h"
#include "effects.h"
#include "ui.h"
#include "weapon.h"
#include "decal.h"
#if AUDIO_ENABLE
#include "gaudio.h"
#endif
#include "player.h"

void player_ai_move_to_target(Player* p, Player* target);

static float sprite_index_to_angle(Player* p);
static void handle_room_collision(Player* p);
static void player_set_sprite_index(Player* p, int sprite_index);

#define XP_REQ_MULT (3.0)
int xp_levels[] = {100,120,140,160,180,200};


float lookup_spaceman_strength[]                         = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
float lookup_spaceman_defense[]                          = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 };
// float lookup_spaceman_movement_speed[]                   = { 620.0, 705.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0 };
// float lookup_spaceman_movement_speed_max_vel[]           = { 110.0, 135.0, 160.0, 190.0, 220.0, 250.0, 280.0 };
// float lookup_spaceman_movement_speed_base_friction[]     = { 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0 };
float lookup_spaceman_movement_speed[]                   = { 750, 800, 850, 900, 1000, 1100, 1200 };
float lookup_spaceman_movement_speed_max_vel[]           = { 150, 160, 170, 180, 200, 220, 240 };
float lookup_spaceman_movement_speed_base_friction[]     = { 11.11, 11.85, 12.59, 13.33, 14.81, 16.3, 17.78 };
float lookup_spaceman_attack_speed[]                     = { 0.45, 0.38, 0.34, 0.31, 0.30, 0.29, 0.28 };
float lookup_spaceman_attack_range[]                     = { 0.0, 20.0, 50.0, 90.0, 140.0, 180.0, 220.0 };
float lookup_spaceman_luck[]                             = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };

float lookup_robot_strength[]                            = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
float lookup_robot_defense[]                             = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 };
float lookup_robot_movement_speed[]                      = { 520.0, 575.0, 650.0, 735.0, 780.0, 880.0, 1000.0 };
float lookup_robot_movement_speed_max_vel[]              = { 80.0, 105.0, 130.0, 155.0, 180.0, 205.0, 240.0 };
float lookup_robot_movement_speed_base_friction[]        = { 12.0, 14.0, 16.0, 20.0, 22.0, 24.0, 26.0 };
float lookup_robot_attack_speed[]                        = { 0.6, 0.55, 0.5, 0.45, 0.4, 0.35, 0.3 };
float lookup_robot_attack_range[]                        = { 0.0, 20.0, 50.0, 90.0, 140.0, 180.0, 220.0 };
float lookup_robot_luck[]                                = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };

float lookup_physicist_strength[]                        = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
float lookup_physicist_defense[]                         = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 };
float lookup_physicist_movement_speed[]                  = { 680.0, 780.0, 880.0, 1000.0, 1110.0, 1220.0, 1350.0 };
float lookup_physicist_movement_speed_max_vel[]          = { 130.0, 155.0, 180.0, 220.0, 250.0, 280.0, 315.0 };
float lookup_physicist_movement_speed_base_friction[]    = { 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0 };
float lookup_physicist_attack_speed[]                    = { 0.3, 0.26, 0.22, 0.18, 0.15, 0.12, 0.08 };
float lookup_physicist_attack_range[]                    = { 0.0, 20.0, 50.0, 90.0, 140.0, 180.0, 220.0 };
float lookup_physicist_luck[]                            = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };

float *lookup_strength                     = lookup_spaceman_strength;
float *lookup_defense                      = lookup_spaceman_defense;
float *lookup_movement_speed               = lookup_spaceman_movement_speed;
float *lookup_movement_speed_max_vel       = lookup_spaceman_movement_speed_max_vel;
float *lookup_movement_speed_base_friction = lookup_spaceman_movement_speed_base_friction;
float *lookup_attack_speed                 = lookup_spaceman_attack_speed;
float *lookup_attack_range                 = lookup_spaceman_attack_range;
float *lookup_luck                         = lookup_spaceman_luck;

int player_ignore_input = 0;

static bool _initialized = false;

char* class_strs[] = 
{
    "Spaceman",
    "Physicist",
    "Robot"
};

int class_image_spaceman = -1;
int class_image_robot = -1;
int class_image_physicist = -1;

#if AUDIO_ENABLE
static uint16_t audio_run1  = -1;
static uint16_t audio_run2  = -1;
static uint16_t audio_jump  = -1;
static uint16_t audio_shoot = -1;
#endif

Room* visible_room; // pointer to currently visible room

int shadow_image = -1;
int card_image = -1;

float jump_vel_z = 160.0;

Player players[MAX_PLAYERS] = {0};
Player* player = NULL;
Player* player2 = NULL;

text_list_t* ptext = NULL;

Rect tile_pit_rect = {0};
Rect player_pit_rect = {0};

void player_set_defaults(Player* p)
{
    p->phys.dead = false;

    Rect* vr = &gfx_images[p->image].visible_rects[0];

    p->phys.speed = 700.0;
    p->phys.speed_factor = 1.0;
    p->phys.max_velocity = 120.0;
    p->phys.base_friction = 8.0;
    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;
    p->phys.height = vr->h * 0.80;
    p->phys.width  = vr->w * 0.60;
    p->phys.length = vr->w * 0.60;
    p->phys.mass = 1.0;
    p->phys.elasticity = 0.1;
    p->phys.vr = *vr;
    p->phys.stun_timer = 0.0;
    p->phys.ethereal = false;
    p->phys.pit_damage = 1;
    p->phys.mud_ignore_factor = 0.0;
    p->phys.floating = false;
    p->phys.chance_to_block = 0.0;

    memset(p->stats,0,sizeof(uint8_t)*MAX_STAT_TYPE);

    phys_calc_collision_rect(&p->phys);
    p->phys.radius = calc_radius_from_rect(&p->phys.collision_rect)*0.8;

    p->phys.hp_max = 6;
    p->phys.hp = p->phys.hp_max;
    p->phys.mp_max = 50;
    p->phys.mp = p->phys.mp_max;

    p->invulnerable = false;
    p->invulnerable_temp_time = 0.0;
    p->gravity = GRAVITY_EARTH;
    p->artifacts = 0x00;

    p->phys.scale = 1.0;
    p->phys.falling = false;

    static bool _prnt = false;
    if(!_prnt)
    {
        _prnt = true;
        printf("[Player Phys Dimensions]\n");
        phys_print_dimensions(&p->phys);
    }

    weapon_add(WEAPON_TYPE_SPEAR,&p->phys, &p->weapon, (p->weapon.type == WEAPON_TYPE_NONE ? true : false));

#if TESTING
    char* gun_name = "machinegun2";
#else
    char* gun_name = "pistol1";
#endif

    if(!gun_get_by_name(gun_name, &p->gun))
    {
        LOGE("Couldn't find gun with name %s", gun_name);
        memcpy(&p->gun, &gun_catalog[0], sizeof(Gun));
    }

    // p->temp_room = -1;
    p->door = DIR_NONE;
    p->light_radius = 2.0;

    p->last_shoot_action = PLAYER_ACTION_SHOOT_UP;
    p->shoot_sprite_cooldown = 0.0;

    // animation
    // --------------------------------------------------------
    p->anim.curr_frame = 0;
    p->anim.max_frames = 4;
    p->anim.curr_frame_time = 0.0f;
    p->anim.max_frame_time = 0.10f;
    p->anim.finite = false;
    p->anim.curr_loop = 0;
    p->anim.max_loops = 0;
    p->anim.frame_sequence[0] = 0;
    p->anim.frame_sequence[1] = 1;
    p->anim.frame_sequence[2] = 2;
    p->anim.frame_sequence[3] = 3;


    p->total_kills = 0;
    p->total_shots = 0;
    p->total_hits = 0;
    p->level_hits = 0;
    p->room_hits = 0;

    p->coins = 2;
#if TESTING
    p->coins = 20;
#endif
    p->revives = 0;

    p->xp = 0;
    p->level = 0;
    p->new_levels = 0;

    p->highlighted_item_id = -1;
    p->highlighted_index = 0;

    player_set_class(p, p->settings.class);

    p->periodic_shot_counter = 0.0;
}

void player_init()
{
    if(!_initialized)
    {
        class_image_spaceman  = gfx_load_image("src/img/spaceman.png", false, false, 32, 32);
        class_image_robot     = gfx_load_image("src/img/robo.png", false, false, 32, 32);
        class_image_physicist = gfx_load_image("src/img/physicist.png", false, false, 32, 32);

        // player_image = class_image_spaceman;
        shadow_image = gfx_load_image("src/img/shadow.png", false, true, 32, 32);
        card_image   = gfx_load_image("src/img/card.png", false, false, 200, 100);

        ptext = text_list_init(5, 0, 0, 0.07, true, TEXT_ALIGN_LEFT, IN_WORLD, true);

        // load sounds
        //audio_run1  = gaudio_add("src/audio/footsteps1.wav", true, false, false, NULL)->id;
        //audio_run2  = gaudio_add("src/audio/footsteps2.wav", true, false, false, NULL)->id;
        //audio_jump  = gaudio_add("src/audio/jump1.wav",      true, false, false, NULL)->id;
        //audio_shoot = gaudio_add("src/audio/laserShoot.wav", true, false, false, NULL)->id;

        _initialized = true;
    }

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        p->index = i;
        p->phys.pos.x = CENTER_X;
        p->phys.pos.y = CENTER_Y;
        p->phys.pos.z = 0.0;
        p->weapon.rotation_deg = 0.0;
        player_set_sprite_index(p, 4);
        player_set_class(p, PLAYER_CLASS_SPACEMAN);

        player_set_defaults(p);

        if(strlen(p->settings.name) == 0 && role == ROLE_LOCAL)
        {
            char* nms[] = {"Randy","Jenger","Peepa","The Hammer"};
            strcpy(p->settings.name, nms[rand()%4]);
        }

        p->active = false;
    }
}

void player_print(Player* p)
{
    printf("==========================:\n");
    printf("Player:\n");
    printf("    active: %s\n", p->active ? "true" : "false");
    printf("    index:  %d\n", p->index);
    printf("    name:   %s\n", p->settings.name);
    phys_print(&p->phys);
    printf("    anim_factor: %.2f\n", p->anim_factor);
    printf("    scale:      %.2f\n", p->phys.scale);
    printf("    class:      %d\n", p->settings.class);
    printf("    xp:         %d\n", p->xp);
    printf("    level:      %d\n", p->level);
    printf("    new_levels: %d\n", p->new_levels);
    printf("    sprite_index:    %u\n", p->sprite_index);
    printf("    curr_room:       %u\n", p->phys.curr_room);
    printf("    transition_room: %u\n", p->transition_room);
    printf("    door: %d\n", p->door);
    printf("    invulnerable:      %s\n", p->invulnerable ? "true" : "false");
    printf("    invulnerable_temp: %s\n", p->invulnerable_temp ? "true" : "false");
    printf("==========================:\n");
}

void player_drop_item(Player* p, Item* it)
{
    if(it == NULL) return;
    if(it->type == ITEM_NONE) return;

    Dir direction = DIR_NONE;
    if(p->sprite_index == SPRITE_UP)
        direction = DIR_UP;
    else if(p->sprite_index == SPRITE_DOWN)
        direction = DIR_DOWN;
    else if(p->sprite_index == SPRITE_LEFT)
        direction = DIR_LEFT;
    else if(p->sprite_index == SPRITE_RIGHT)
        direction = DIR_RIGHT;
    Vector2i o = get_dir_offsets(direction);

    float itr = it->phys.radius;

    float px = p->phys.collision_rect.x;
    float py = p->phys.collision_rect.y;
    float r = p->phys.radius*2 + itr + 2.0;
    float nx = px;
    float ny = py;

    nx += r*o.x;
    ny += r*o.y - ABS(o.x)*10.0;

    Rect rect = RECT(nx,ny,itr,itr);
    limit_rect_pos(&floor_area, &rect);
    nx = rect.x;
    ny = rect.y;

#if 0
    Room* room = level_get_room_by_index(&level, (int)p->phys.curr_room);
    if(!room) printf("room is null!\n");

    // Dir dirs[4] = {DIR_DOWN, DIR_RIGHT, DIR_LEFT, DIR_UP};
    // for(int i = 0; i < 4; ++i)
    for(int d = 0; d < DIR_NONE; ++d)
    {
        // Vector2i o = get_dir_offsets(dirs[i]);
        Vector2i o = get_dir_offsets(d);
        float _nx = px + o.x * r;
        float _ny = py + o.y * r;

        Vector2i tile_coords = level_get_room_coords_by_pos(_nx, _ny);

        Rect rect = RECT(_nx, _ny, itr, itr);
        Vector2f l = limit_rect_pos(&player_area, &rect);
        TileType tt = level_get_tile_type_by_pos(room, _nx, _ny);
        if(FEQ0(l.x) && FEQ0(l.y) && IS_SAFE_TILE(tt))
        {
            // printf("%s is good\n", get_dir_name(dirs[i]));
            nx = _nx;
            ny = _ny;

            if(!item_is_on_tile(room, tile_coords.x, tile_coords.y))
            {
                break;
            }
        }
        // else
        // {
        //     printf("no good: %s\n", get_tile_name(tt));
        //     print_rect(&rect);
        // }
    }
#endif

    // printf("dropping item at %.2f, %.2f\n", nx, ny);
    Item* a = item_add(it->type, nx, ny, p->phys.curr_room);
    a->phys.vel.x += o.x*100.0 + p->phys.vel.x;
    a->phys.vel.y += o.y*100.0 + p->phys.vel.y;
    a->phys.pos.z += p->phys.pos.z;

    item_set_description(a, "%s", it->desc);
    it->type = ITEM_NONE;
}

void player_drop_coins(Player* p)
{
    for(int i = 0; i < p->coins; ++i)
    {
        Item* it = item_add(item_get_random_coin(), p->phys.collision_rect.x, p->phys.collision_rect.y, p->phys.curr_room);
        if(it)
        {
            it->phys.vel.z = rand()%200+100.0;
            it->phys.vel.x = RAND_PN()*RAND_FLOAT(0,100);
            it->phys.vel.y = RAND_PN()*RAND_FLOAT(0,100);
        }
    }
    p->coins = 0;
}


void player_set_active(Player* p, bool active)
{
    p->active = active;
}

int player_get_active_count()
{
    int num = 0;
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(players[i].active)
            num++;
    }
    return num;
}

Player* player_get_nearest(uint8_t room_index, float x, float y)
{
    float min_dist = 100000.0;
    int min_index = 0;

    // int num_players = player_get_active_count();
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(!p->active) continue;
        if(p->phys.dead) continue;
        if(p->phys.curr_room != room_index) continue;

        float d = dist(x,y, p->phys.pos.x, p->phys.pos.y);
        if(d < min_dist)
        {
            min_dist = d;
            min_index = i;
        }
    }

    return &players[min_index];
}

void player_send_to_room(Player* p, uint8_t room_index, bool instant, Vector2i tile)
{
    // printf("player_send_to_room: %u\n");
    p->transition_room = p->phys.curr_room;
    p->phys.curr_room = room_index;
    if(instant)
    {
        p->transition_room = p->phys.curr_room;
    }

    p->room_hits = 0;

    Room* room = level_get_room_by_index(&level, room_index);
    if(!room)
    {
        LOGE("room is null");
        return;
    }

    // room->doors_locked = (creature_get_room_count(room->index, false) != 0);

    visible_room = room;
    refresh_visible_room_gun_list();
    generate_walls(&level, visible_room);

    Vector2f pos = {0};
    level_get_safe_floor_tile(room, tile, NULL, &pos);
    // printf("pos: %.2f, %.2f\n", pos.x, pos.y);
    phys_set_collision_pos(&p->phys, pos.x, pos.y);
    // printf("player send to room %u (%d, %d)\n", player->phys.curr_room, tile.x, tile.y);

    p->ignore_player_collision = true;

#if DUMB_CLIENT
    if(role == ROLE_SERVER || role == ROLE_LOCAL)
#endif
    {
       // if(level.rooms_ptr[room_index]->doors_locked)
        if(creature_get_room_count(room_index, false) != 0)
        {
            level_room_in_progress = true;
        }
        else
        {
            level_room_in_progress = false;
        }
    }
}

void player_send_to_level_start(Player* p)
{
    uint8_t idx = level_get_room_index(level.start.x, level.start.y);
    player_send_to_room(p, idx, true, level_get_door_tile_coords(DIR_NONE));
}

void player_init_keys()
{
    if(player == NULL) return;

    for(int i = 0;  i < PLAYER_ACTION_MAX; ++i)
        memset(&player->actions[i], 0, sizeof(PlayerInput));

    // window_controls_clear_keys();
    window_controls_add_key(&player->actions[PLAYER_ACTION_UP].state, GLFW_KEY_W);
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&player->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_D);
    window_controls_add_key(&player->actions[PLAYER_ACTION_JUMP].state, GLFW_KEY_SPACE);

    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_UP].state, GLFW_KEY_I);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_DOWN].state, GLFW_KEY_K);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_LEFT].state, GLFW_KEY_J);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_RIGHT].state, GLFW_KEY_L);

    // window_controls_add_key(&player->actions[PLAYER_ACTION_ACTIVATE].state, GLFW_KEY_ENTER);
    window_controls_add_key(&player->actions[PLAYER_ACTION_ACTIVATE].state, GLFW_KEY_E);

    window_controls_add_key(&player->actions[PLAYER_ACTION_USE_ITEM].state, GLFW_KEY_U);

    window_controls_add_key(&player->actions[PLAYER_ACTION_DISPLAY_MAP].state, GLFW_KEY_M);


    window_controls_add_key(&player->actions[PLAYER_ACTION_DROP_ITEM].state, GLFW_KEY_N);
    player->actions[PLAYER_ACTION_DROP_ITEM].hold_period = 0.1;

    window_controls_add_key(&player->actions[PLAYER_ACTION_REVIVE].state, GLFW_KEY_R);

    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_1].state, GLFW_KEY_1);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_2].state, GLFW_KEY_2);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_3].state, GLFW_KEY_3);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_4].state, GLFW_KEY_4);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_5].state, GLFW_KEY_5);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_6].state, GLFW_KEY_6);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_7].state, GLFW_KEY_7);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_ITEM_8].state, GLFW_KEY_8);

    window_controls_add_key(&player->actions[PLAYER_ACTION_TAB_CYCLE].state, GLFW_KEY_TAB);

    window_controls_add_key(&player->actions[PLAYER_ACTION_RSHIFT].state, GLFW_KEY_RIGHT_SHIFT);
}

void player2_init_keys()
{
    if(player2 == NULL)
    {
        printf("player2 is null\n");
        return;
    }
    window_controls_add_key(&player2->actions[PLAYER_ACTION_UP].state, GLFW_KEY_UP);
    window_controls_add_key(&player2->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_DOWN);
    window_controls_add_key(&player2->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_LEFT);
    window_controls_add_key(&player2->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_RIGHT);

    for(int i = 0;  i < PLAYER_ACTION_MAX; ++i)
        memset(&player2->actions[i], 0, sizeof(PlayerInput));
}

int get_xp_req(int level)
{
    int num = sizeof(xp_levels) / sizeof(xp_levels[0]);
    int xp_req = 0;
    if(level >= num)
        xp_req = xp_levels[num-1];
    else
        xp_req = xp_levels[level];
    return xp_req*XP_REQ_MULT;
}

void player_add_xp(Player* p, int xp)
{
    int num = sizeof(xp_levels) / sizeof(xp_levels[0]);

    p->xp += xp;
    LOGI("Added %d xp to player %d, total: %d", xp, p->index, p->xp);

    int num_new_levels = 0;

    for(;;)
    {
        int xp_req = get_xp_req(p->level);

        if(p->xp < xp_req)
            break;

        p->xp -= xp_req;
        num_new_levels++;
    }

    bool had_new_levels = p->new_levels > 0;

    p->level += num_new_levels;
    p->new_levels += num_new_levels;

    LOGI("Level: %u, new: %u", p->level, p->new_levels);
}

bool player_add_stat(Player* p, StatType stat, int val)
{
    uint8_t prev_stat_val = p->stats[stat];
    p->stats[stat] += val;
    p->stats[stat] = RANGE(p->stats[stat],0,6);

    if(prev_stat_val - p->stats[stat] == 0) // no change
        return false;

    if(p == player)
    {
        text_list_add(ptext, COLOR_WHITE, 3.0, "+ %s", stats_get_name(stat));
    }

    return true;
}

void player_add_coins(Player* p, int val)
{
    if(p == player)
    {
        text_list_add(ptext, COLOR_WHITE, 3.0, "%s%d coin%s",  val > 0 ? "+" : "", val, ABS(val) == 1 ? "" : "s");
    }

    if(val < 0 && p->coins < ABS(val))
    {
        p->coins = 0;
    }
    else
    {
        p->coins += val;
    }
}

bool player_add_mp(Player* p, int mp)
{
    uint8_t prev_mp = p->phys.mp;
    p->phys.mp += mp;
    p->phys.mp = RANGE(p->phys.mp,0,p->phys.mp_max);

    if(prev_mp - p->phys.mp == 0) // no change
        return false;

    return true;
}

bool player_add_hp(Player* p, int hp)
{
    uint8_t prev_hp = p->phys.hp;
    p->phys.hp += hp;
    p->phys.hp = RANGE(p->phys.hp,0,p->phys.hp_max);

    if(prev_hp - p->phys.hp == 0) // no change
        return false;

    return true;
}

// ignores temp invulnerability
void player_hurt_no_inv(Player* p, int damage)
{
    if(players_invincible)
        return;

    if(p->invulnerable)
        return;

    if(level_grace_time == 0)
        return;

    // printf("player_hurt_no_inv\n");

    player_add_hp(p,-damage);

    if(p->phys.hp == 0)
    {
        text_list_add(text_lst, COLOR_WHITE, 3.0, "%s died", p->settings.name);
        player_die(p);
    }
}

void player_hurt(Player* p, int damage)
{
    if(damage == 0)
        return;

    if(players_invincible)
        return;

    if(p->invulnerable)
        return;

    if(p->invulnerable_temp)
        return;

#if DUMB_CLIENT
    if(role == ROLE_CLIENT)
        return;
#endif

    float actual_damage = MAX(0.0, damage * (1.0 - lookup_defense[p->stats[DEFENSE]]));

    player_add_hp(p,-actual_damage);

    if(p->phys.hp == 0)
    {
        player_die(p);
    }
    else
    {
        p->invulnerable_temp = true;
        p->invulnerable_temp_time = 0.0;
        p->invulnerable_temp_max = 1.0;
    }
}


void player_die(Player* p)
{
    if(p->revives > 0)
    {
        p->revives--;
        p->phys.hp = 2;
        text_list_add(text_lst, COLOR_WHITE, 3.0, "Revived!");

        ParticleEffect* eff = &particle_effects[EFFECT_HEAL2];
        if(role == ROLE_SERVER)
        {
            NetEvent ev = {
                .type = EVENT_TYPE_PARTICLES,
                .data.particles.effect_index = EFFECT_HEAL2,
                .data.particles.pos = { p->phys.collision_rect.x, p->phys.collision_rect.y },
                .data.particles.scale = 1.0,
                .data.particles.color1 = eff->color1,
                .data.particles.color2 = eff->color2,
                .data.particles.color3 = eff->color3,
                .data.particles.lifetime = 0.5,
                .data.particles.room_index = p->phys.curr_room,
            };

            net_server_add_event(&ev);
        }
        else
        {
            ParticleSpawner* ps = particles_spawn_effect(p->phys.collision_rect.x,p->phys.collision_rect.y, 0.0, eff, 0.5, true, false);
            if(ps != NULL) ps->userdata = (int)p->phys.curr_room;
        }

        player_drop_coins(p);
        return;
    }

    text_list_add(text_lst, COLOR_WHITE, 3.0, "%s died.", p->settings.name);

    p->phys.hp = 0;
    p->phys.dead = true;
    p->phys.floating = true;
    status_effects_clear(&p->phys);

    // p->total_kills = 0;
    // p->total_shots = 0;
    // p->total_hits = 0;
    // p->level_hits = 0;
    // p->room_hits = 0;

    ParticleEffect* eff = &particle_effects[EFFECT_BLOOD2];
    ParticleSpawner* ps = particles_spawn_effect(p->phys.pos.x,p->phys.pos.y, 0.0, eff, 0.5, true, false);
    if(ps != NULL) ps->userdata = (int)p->phys.curr_room;

    Decal d = {0};
    d.image = particles_image;
    d.sprite_index = 0;
    d.tint = COLOR_RED;
    d.scale = 1.0;
    d.rotation = rand() % 360;
    d.opacity = 0.6;
    d.ttl = 10.0;
    d.pos.x = p->phys.pos.x;
    d.pos.y = p->phys.pos.y;
    d.room = p->phys.curr_room;
    decal_add(d);

    Item skull = {.type = ITEM_SKULL, .phys.pos.x = p->phys.pos.x, .phys.pos.y = p->phys.pos.y};
    item_set_description(&skull, "%s", p->settings.name);
    player_drop_item(p, &skull);

    player_drop_coins(p);

    p->phys.falling = false;
    p->phys.scale = 1.0;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p2 = &players[i];

        if(!p2->active) continue;
        if(!p2->phys.dead) return;
    }

    all_players_dead = true;
}

void player_reset(Player* p)
{
    player_set_defaults(p);
}

void player_draw_room_transition()
{
    Player* p = player;
    if(!p->active) return;

    if(p->phys.curr_room != p->transition_room)
    {
        float dx = transition_targets.x/12.0;
        float dy = transition_targets.y/12.0;

        if(!paused)
        {
            transition_offsets.x += dx;
            transition_offsets.y += dy;
        }

        if(ABS(transition_offsets.x) >= ABS(transition_targets.x) && ABS(transition_offsets.y) >= ABS(transition_targets.y))
        {
            // printf("room transition complete\n");
            p->transition_room = p->phys.curr_room;

            bool immediate = true;
            float cx = p->phys.pos.x;
            float cy = p->phys.pos.y;
            bool check = camera_can_be_limited(cx, cy, (float)cam_zoom/100.0);
            if(!check)
            {
                // zoomed out too far
                immediate = !immediate;
            }

            camera_set(immediate);

            Room* room = level_get_room_by_index(&level, player->phys.curr_room);
            level_draw_room(room, NULL, 0, 0, 1.0, false);
        }
        else
        {
            float x0 = transition_offsets.x;
            float y0 = transition_offsets.y;

            float xoff = 0.0;
            if(!FEQ0(transition_targets.x))
            {
                if(transition_targets.x > 0)
                    xoff = -room_area.w;
                else
                    xoff = room_area.w;
            }
            float yoff = 0.0;
            if(!FEQ0(transition_targets.y))
            {
                if(transition_targets.y > 0)
                    yoff = -room_area.h;
                else
                    yoff = room_area.h;
            }

            float x1 = transition_offsets.x+xoff;
            float y1 = transition_offsets.y+yoff;

            Vector2i t = level_get_room_coords(p->transition_room);
            level_draw_room(&level.rooms[t.x][t.y], NULL, x0, y0, 1.0, false);

            Vector2i roomxy = level_get_room_coords((int)p->phys.curr_room);
            level_draw_room(&level.rooms[roomxy.x][roomxy.y], NULL, x1, y1, 1.0, false);


            // Vector2i c = level_get_room_coords(p->phys.curr_room);
            // for(int d = 0; d < 4; ++d)
            // {
            //     if(!level.rooms[roomxy.x][roomxy.y].doors[d]) continue;
            //     Vector2i o = get_dir_offsets(d);
            //     Room* r = level_get_room(&level, c.x+o.x, c.y+o.y);
            //     if(!r) continue;
            //     if(!r->valid) continue;
            //     if(r->index == p->transition_room) continue;
            //     level_draw_room(r, NULL, room_area.w*o.x+x1, room_area.h*o.y+y1);
            // }

        }
        // paused = true;  //uncommment to step through the transition
    }
}


void player_start_room_transition(Player* p)
{
    // printf("start room transition\n");
    Vector2i roomxy = level_get_room_coords((int)p->phys.curr_room);

    uint8_t room_index = 0;

#if DUMB_CLIENT
    if(role == ROLE_SERVER || role == ROLE_LOCAL)
#endif
    {
        p->transition_room = p->phys.curr_room;

        Vector2i o = get_dir_offsets(p->door);
        room_index = level_get_room_index(roomxy.x + o.x, roomxy.y + o.y);

        Dir other_door = DIR_NONE;
        uint8_t sprite_index = 0;

        switch(p->door)
        {
            case DIR_UP:
                other_door = DIR_DOWN;
                sprite_index = SPRITE_UP;
                break;

            case DIR_RIGHT:
                other_door = DIR_LEFT;
                sprite_index = SPRITE_RIGHT;
                break;

            case DIR_DOWN:
                other_door = DIR_UP;
                sprite_index = SPRITE_DOWN;
                break;

            case DIR_LEFT:
                other_door = DIR_RIGHT;
                sprite_index = SPRITE_LEFT;
                break;

            default:
                break;
        }

        Vector2i nt = level_get_door_tile_coords(other_door);

        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            Player* p2 = &players[i];
            if(!p2->active) continue;
            if(p2->phys.curr_room == room_index)
            {
                // printf("player %d is in the room already\n", i);
                continue;
            }
            player_send_to_room(p2, room_index, false, nt);
            player_set_sprite_index(p2, sprite_index);
            p2->door = p->door;
        }

        // printf("start room transition: %d -> %d\n", p->transition_room, p->phys.curr_room);

        level_grace_time = ROOM_GRACE_TIME;
        level_room_time = 0.0;
        level_room_xp = 0;

        printf("moving to room %s\n", room_files[room_list[level.rooms_ptr[room_index]->layout].file_index]);

        // // if(level.rooms_ptr[room_index]->doors_locked)
        // if(creature_get_room_count(room_index, false) != 0)
        // {
        //     level_room_in_progress = true;
        // }
        // else
        // {
        //     level_room_in_progress = false;
        // }

        for(int i = 0; i < clist->count; ++i)
        {
            if(creatures[i].type == CREATURE_TYPE_PHANTOM)
            {
                if(creatures[i].phys.curr_room == room_index) break;
                float vw = room_area.w;
                float vh = room_area.h;
                Vector2f adj = {0};
                switch(p->door)
                {
                    case DIR_UP:
                    {
                        adj.y = vh;
                    } break;
                    case DIR_RIGHT:
                    {
                        adj.x = -vw;
                    } break;
                    case DIR_DOWN:
                    {
                        adj.y = -vh;
                    } break;
                    case DIR_LEFT:
                    {
                        adj.x = vw;
                    } break;
                    default:
                        break;
                }
                adj.x += creatures[i].phys.collision_rect.x;
                adj.y += creatures[i].phys.collision_rect.y;
                phys_set_collision_pos(&creatures[i].phys, adj.x, adj.y);
                break;
            }
        }
    }



    if(role == ROLE_SERVER)
        return;

    if(p != player)
        return;

    transition_offsets.x = 0;
    transition_offsets.y = 0;
    transition_targets.x = 0;
    transition_targets.y = 0;

    Rect cr = get_camera_rect();
    float vw = cr.w;
    float vh = cr.h;

    bool check = camera_can_be_limited(cr.x, cr.y, camera_get_zoom());
    if(!check)
    {
        vw = room_area.w;
        vh = room_area.h;
    }

    switch(p->door)
    {
        case DIR_UP:
        {
            transition_targets.y = vh;
        } break;

        case DIR_RIGHT:
        {
            transition_targets.x = -vw;
        } break;

        case DIR_DOWN:
        {
            transition_targets.y = -vh;
        } break;

        case DIR_LEFT:
        {
            transition_targets.x = vw;
        } break;

        default:
            break;
    }

    // printf("transition_targets: %.2f, %.2f  door: %d     (%.2f, %.2f)\n", transition_targets.x, transition_targets.y, p->door, vw, vh);
}

static void player_set_sprite_index(Player* p, int sprite_index)
{
    p->sprite_index = sprite_index;
    p->aim_deg = sprite_index_to_angle(p);
}

static float sprite_index_to_angle(Player* p)
{
    if(p->sprite_index >= 0 && p->sprite_index <= 3)
        return 90.0;
    else if(p->sprite_index >= 4 && p->sprite_index <= 7)
        return 270.0;
    else if(p->sprite_index >= 8 && p->sprite_index <= 11)
        return 180.0;
    return 0.0;
}

static void handle_room_collision(Player* p)
{
    Room* room = level_get_room_by_index(&level, p->phys.curr_room);

    // doors
    for(int i = 0; i < 4; ++i)
    {
        if(role == ROLE_LOCAL)
        {
            if(player->phys.curr_room != player->transition_room) break;
        }

        bool is_door = room->doors[i];

        if(!is_door)
            continue;

        Vector2f door_point;

        int x0 = room_area.x - room_area.w/2.0;
        int y0 = room_area.y - room_area.h/2.0;

        switch(i)
        {
            case DIR_UP:
                door_point.x = x0 + TILE_SIZE*((ROOM_TILE_SIZE_X+2)/2.0);
                door_point.y = y0 + TILE_SIZE*(1);
                break;
            case DIR_RIGHT:
                door_point.x = x0 + TILE_SIZE*(ROOM_TILE_SIZE_X+1);
                door_point.y = y0 + TILE_SIZE*((ROOM_TILE_SIZE_Y+2)/2.0);
                break;
            case DIR_DOWN:
                door_point.x = x0 + TILE_SIZE*((ROOM_TILE_SIZE_X+2)/2.0);
                door_point.y = y0 + TILE_SIZE*(ROOM_TILE_SIZE_Y+1);
                break;
            case DIR_LEFT:
                door_point.x = x0 + TILE_SIZE*(1);
                door_point.y = y0 + TILE_SIZE*((ROOM_TILE_SIZE_Y+2)/2.0);
                break;
        }

        float d = dist(p->phys.collision_rect.x, p->phys.collision_rect.y, door_point.x, door_point.y);

        bool colliding_with_door = (d < p->phys.radius);
        if(colliding_with_door && !room->doors_locked && (p->phys.pos.z == 0.0 || p->phys.floating) && !p->phys.dead)
        {
            bool k = false;
            k |= i == DIR_UP && p->actions[PLAYER_ACTION_UP].state;
            k |= i == DIR_DOWN && p->actions[PLAYER_ACTION_DOWN].state;
            k |= i == DIR_LEFT && p->actions[PLAYER_ACTION_LEFT].state;
            k |= i == DIR_RIGHT && p->actions[PLAYER_ACTION_RIGHT].state;
            if(k)
            {
                p->phys.vel.x = 0;
                p->phys.vel.y = 0;
                p->door = i;
                player_start_room_transition(p);
            }
            break;
        }
    }
}

void player_update_all(float dt)
{
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        player_update(p, dt);
    }
}

static void player_handle_orbitals(Player* p, float dt)
{
    ProjectileOrbital* orb = projectile_orbital_get(&p->phys, p->gun.orbital_distance);
    
    if(p->actions[PLAYER_ACTION_SHOOT_UP].state)
    {
        // eject an orbital
        
        if(!orb)
            return;

        if(orb->count <= 0)
            return;

        p->proj_cooldown -= dt;
        p->proj_cooldown = MAX(p->proj_cooldown,0.0);

        if(p->proj_cooldown > 0.0)
            return;

        for(int i = plist->count -1; i >= 0; --i)
        {
            if(projectiles[i].orbital != orb)
                continue;

            // eject
            p->proj_cooldown = p->gun.cooldown;

            //gun.orbital = false;
            projectiles[i].orbital->count--;

            // adjust orbital indices for other projectiles
            for(int j =plist->count - 1; j >= 0; --j)
            {
                if(j == i)
                    continue;

                if(projectiles[j].orbital != projectiles[i].orbital)
                    continue;

                if(projectiles[j].orbital_index > projectiles[i].orbital_index)
                    projectiles[j].orbital_index--;
            }

            projectiles[i].orbital = NULL;

            break;
        }
    }
    else if(p->actions[PLAYER_ACTION_SHOOT_RIGHT].state)
    {
    }
    else if(p->actions[PLAYER_ACTION_SHOOT_DOWN].state)
    {

    }
    else if(p->actions[PLAYER_ACTION_SHOOT_LEFT].state)
    {
        Gun temp = p->gun;
        temp.damage_min += lookup_strength[p->stats[STRENGTH]];
        temp.damage_max += lookup_strength[p->stats[STRENGTH]];

        temp.speed  += lookup_attack_range[p->stats[ATTACK_RANGE]];

        int max = p->gun.orbital_max_count;
        if(orb) max -= orb->count;

        Vector3f pos = {p->phys.pos.x, p->phys.pos.y, p->phys.height + p->phys.pos.z};
        Vector3f vel = {p->phys.vel.x, p->phys.vel.y, 0.0};

        vel.z = MIN(1.0, p->gun.gravity_factor)*120.0;

        for(int i = 0; i < max; ++i)
        {
            projectile_add(pos, &vel, p->phys.curr_room, p->room_gun_index, p->aim_deg, true, &p->phys, p->index);
        }
    }
}


static void player_handle_melee(Player* p, float dt)
{
    bool attacked = false;

    if(p->actions[PLAYER_ACTION_SHOOT_UP].state)
    {
        player_set_sprite_index(p, SPRITE_UP);
        p->weapon.rotation_deg = 90.0;
        attacked = true;
    }
    else if(p->actions[PLAYER_ACTION_SHOOT_RIGHT].state)
    {
        player_set_sprite_index(p, SPRITE_RIGHT);
        p->weapon.rotation_deg = 0.0;
        attacked = true;
    }
    else if(p->actions[PLAYER_ACTION_SHOOT_DOWN].state)
    {
        player_set_sprite_index(p, SPRITE_DOWN);
        p->weapon.rotation_deg = 270.0;
        attacked = true;
    }
    else if(p->actions[PLAYER_ACTION_SHOOT_LEFT].state)
    {
        player_set_sprite_index(p, SPRITE_LEFT);
        p->weapon.rotation_deg = 180.0;
        attacked = true;
    }

    if(attacked && p->weapon.state == WEAPON_STATE_NONE)
    {
        p->weapon.state = WEAPON_STATE_WINDUP;
        weapon_clear_hit_list(&p->weapon);
    }

    weapon_update(&p->weapon, dt);
}

static void player_handle_shooting(Player* p, float dt)
{
    if(p->proj_cooldown > 0.0)
    {
        p->proj_cooldown -= dt;
        p->proj_cooldown = MAX(p->proj_cooldown,0.0);
    }

    p->shoot_sprite_cooldown = MAX(p->shoot_sprite_cooldown - dt, 0.0);

    for(int i = 0; i < 4; ++i)
    {
        if(p->actions[PLAYER_ACTION_SHOOT_UP+i].toggled_on)
        {
            if(p->gun.chargeable && p->proj_cooldown == 0.0)
            {
                // Charging
                p->gun.charging = true;
            }

            p->last_shoot_action = PLAYER_ACTION_SHOOT_UP+i;
            break;
        }
    }

    if(p->last_shoot_action >= PLAYER_ACTION_SHOOT_UP && p->last_shoot_action <= PLAYER_ACTION_SHOOT_RIGHT)
    {
        if(p->gun.chargeable)
        {
            if(!p->gun.charging && p->actions[p->last_shoot_action].state && p->proj_cooldown == 0.0)
            {
                p->gun.charging = true;
            }

            if(p->gun.charging)
            {
                p->gun.charge_time += dt;
                p->gun.charge_time = MIN(p->gun.charge_time_max, p->gun.charge_time);
            }
        }

        bool shoot = p->gun.chargeable ? false : p->actions[p->last_shoot_action].state && p->proj_cooldown == 0.0;

        if(p->actions[p->last_shoot_action].toggled_off)
        {
            for(int i = 0; i < 4; ++i)
            {
                if(p->actions[PLAYER_ACTION_SHOOT_UP+i].state)
                {
                    p->last_shoot_action = PLAYER_ACTION_SHOOT_UP+i;
                    break;
                }
            }

            if(p->gun.chargeable && p->gun.charging)
            {
                // Release!
                shoot = true;
            }
        }

        int sprite_index = 0;
        if(p->last_shoot_action == PLAYER_ACTION_SHOOT_UP)
        {
            sprite_index = SPRITE_UP;
        }
        else if(p->last_shoot_action == PLAYER_ACTION_SHOOT_DOWN)
        {
            sprite_index = SPRITE_DOWN;
        }
        else if(p->last_shoot_action == PLAYER_ACTION_SHOOT_LEFT)
        {
            sprite_index = SPRITE_LEFT;
        }
        else if(p->last_shoot_action == PLAYER_ACTION_SHOOT_RIGHT)
        {
            sprite_index = SPRITE_RIGHT;
        }

        if(p->shoot_sprite_cooldown > 0)
        {
            player_set_sprite_index(p, sprite_index);
        }

        if(shoot)
        {
            player_set_sprite_index(p, sprite_index);

            if(!p->phys.dead)
            {

                Gun* proj_gun = &room_gun_list[p->room_gun_index];
                proj_gun->charging = p->gun.charging;
                proj_gun->charge_time = p->gun.charge_time;

                Vector3f pos = {p->phys.pos.x, p->phys.pos.y, p->phys.height + p->phys.pos.z};
                Vector3f vel = {p->phys.vel.x, p->phys.vel.y, 0.0};

                vel.z = MIN(1.0, p->gun.gravity_factor)*120.0;

                projectile_add(pos, &vel, p->phys.curr_room, p->room_gun_index, p->aim_deg, true, &p->phys, p->index);

                p->gun.charging = false;
                p->gun.charge_time = 0.0;

                proj_gun->charging = p->gun.charging;
                proj_gun->charge_time = p->gun.charge_time;

                //gaudio_set_pitch(audio_shoot, RAND_FLOAT(0.9,1.1));
                //gaudio_play(audio_shoot);
            }

            // text_list_add(text_lst, 5.0, "projectile");
            p->proj_cooldown = p->gun.cooldown;
            p->shoot_sprite_cooldown = 1.0;
        }
    }
}

void player_update(Player* p, float dt)
{
    if(!p->active) return;


    if(all_players_dead)
    {
        PlayerInput* pa = &p->actions[PLAYER_ACTION_REVIVE];
        update_input_state(pa, dt);

        bool revive = p->actions[PLAYER_ACTION_REVIVE].toggled_on;

        if(revive)
        {
            trigger_generate_level(rand(), level_rank, 2, __LINE__);

            for(int i = 0; i < MAX_PLAYERS; ++i)
            {
                player_set_defaults(&players[i]);
            }

            if(role == ROLE_SERVER)
            {
                NetEvent ev = {.type = EVENT_TYPE_NEW_LEVEL};
                net_server_add_event(&ev);
            }

        }

        if(role == ROLE_LOCAL)
            return;
    }

    // has to be role local because server doesn't know about transition_room
    if(role == ROLE_LOCAL && p == player)
    {
        if(player->phys.curr_room != player->transition_room) return;
    }

    if(role == ROLE_CLIENT)
    {
        bool _all_players_dead = true;
        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            Player* p2 = &players[i];
            
            if(!p2->active) continue;
            if(!p2->phys.dead)
            {
                _all_players_dead = false;
                break;
            }
        }

        all_players_dead = _all_players_dead;

        if(p == player && !p->phys.dead)
        {
            p->light_index = lighting_point_light_add(p->phys.pos.x, p->phys.pos.y, 1.0, 1.0, 1.0, p->light_radius,0.0);
        }

#if DUMB_CLIENT
        player_lerp(p, dt);
#else
        if(p != player)
            player_lerp(p, dt);
#endif

        Room* room = level_get_room_by_index(&level, (int)p->phys.curr_room);
        if(!room) return;

        room->discovered = true;
        
#if DUMB_CLIENT
        return;
#else
        if(p != player)
            return;
#endif
    }

    float prior_x = p->phys.pos.x;
    float prior_y = p->phys.pos.y;

    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerInput* pa = &p->actions[i];
        update_input_state(pa, dt);
    }

    // artifacts... @HACK
    if(!p->phys.falling)
    {
        if(player_has_artifact(p, ARTIFACT_SLOT_GEM_YELLOW))
        {
            p->phys.scale = 0.8;
        }

        if(player_has_artifact(p, ARTIFACT_SLOT_DRAGON_EGG))
        {
            p->phys.scale = 1.2;
        }
    }

    bool activate = !all_players_dead && p->actions[PLAYER_ACTION_ACTIVATE].toggled_on;
    bool action_use = !all_players_dead && p->actions[PLAYER_ACTION_USE_ITEM].toggled_on;
    bool tabbed = !all_players_dead && p->actions[PLAYER_ACTION_TAB_CYCLE].toggled_on;
    bool rshift = !all_players_dead && p->actions[PLAYER_ACTION_RSHIFT].state;
    bool action_drop = !all_players_dead && p->actions[PLAYER_ACTION_DROP_ITEM].toggled_on || p->actions[PLAYER_ACTION_DROP_ITEM].hold;
    bool revive = p->actions[PLAYER_ACTION_REVIVE].toggled_on;

    if(p->revives > 0 && revive)
    {
        uint8_t others[MAX_PLAYERS] = {0};
        int count = 0;
        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            Player* p2 = &players[i];
            // if(p == p2) continue;
            if(!p2->active) continue;
            if(p2->phys.dead) others[count++] = i;
        }

        if(count > 0)
        {
            Player* p2 = &players[others[rand()%count]];
            p2->phys.dead = false;
            p2->phys.hp = 2;
            p2->phys.floating = false;
            p->revives--;
        }
    }

    if(action_drop)
    {
        if(p->coins > 0)
        {
            Item it = {0};
            it.type = ITEM_COIN_GOLD;
            player_drop_item(p, &it);
            p->coins--;
        }
    }

    Item* highlighted_item = NULL;
    if(p->highlighted_item_id != -1)
    {
        highlighted_item = item_get_by_id(p->highlighted_item_id);
    }

    if(highlighted_item)
    {
        if(tabbed)
        {
            if(rshift)
                p->highlighted_index--;
            else
                p->highlighted_index++;
        }
    }

    if(activate)
    {
        // printf("activate\n");
        Item* pu = highlighted_item;
        if(pu)
        {
            ItemType type = pu->type;
            ItemProps* pr = &item_props[type];

            LOGI("Using item type: %s (%d)", item_get_name(type), type);
            bool used = item_use(pu, p);
        }
    }

    // apply stats
    p->phys.speed         = lookup_movement_speed[p->stats[MOVEMENT_SPEED]];
    p->phys.max_velocity  = lookup_movement_speed_max_vel[p->stats[MOVEMENT_SPEED]];
    p->phys.base_friction = lookup_movement_speed_base_friction[p->stats[MOVEMENT_SPEED]];

    float cx = p->phys.collision_rect.x;
    float cy = p->phys.collision_rect.y;
    Room* room = level_get_room_by_index(&level, p->phys.curr_room);

    float mud_factor = 1.0;
    float friction_factor = 1.0;

    if(!room)
    {
        LOGW("Failed to load room, player curr room: %u (number of rooms in level: %d)",p->phys.curr_room, level.num_rooms);
    }
    else
    {
        Vector2i c_tile = level_get_room_coords_by_pos(cx, cy);

        p->phys.prior_tile.x = p->phys.curr_tile.x;
        p->phys.prior_tile.y = p->phys.curr_tile.y;

        float prior_tile_counter = p->phys.curr_tile_counter;
        p->phys.curr_tile.x = RANGE(c_tile.x, 0, ROOM_TILE_SIZE_X-1);
        p->phys.curr_tile.y = RANGE(c_tile.y, 0, ROOM_TILE_SIZE_Y-1);
        if(p->phys.curr_tile.x == p->phys.prior_tile.x && p->phys.curr_tile.y == p->phys.prior_tile.y)
        {
            p->phys.curr_tile_counter += dt;
        }
        else
        {
            p->phys.curr_tile_counter = 0.0;
        }

        TileType tt = level_get_tile_type(room, p->phys.curr_tile.x, p->phys.curr_tile.y);
        if(tt == TILE_BREAKABLE_FLOOR && !p->phys.floating && !p->phys.dead)
        {
            int tc1 = (int)prior_tile_counter;
            int tc2 = (int)p->phys.curr_tile_counter;

            if(tc2-tc1 >= 1)
            {
                room->breakable_floor_state[p->phys.curr_tile.x][p->phys.curr_tile.y]++;

                if(role == ROLE_SERVER)
                {
                    NetEvent ev = {
                        .type = EVENT_TYPE_FLOOR_STATE,
                        .data.floor_state.x = p->phys.curr_tile.x,
                        .data.floor_state.y = p->phys.curr_tile.y,
                        .data.floor_state.state = room->breakable_floor_state[p->phys.curr_tile.x][p->phys.curr_tile.y],
                    };

                    net_server_add_event(&ev);
                }

            }
        }

        if(IS_SAFE_TILE(tt))
        {
            p->last_safe_tile = p->phys.curr_tile;
        }

        if(p->phys.pos.z == 0.0)
        {
            switch(tt)
            {
                case TILE_MUD:
                    mud_factor = 0.4;
                    mud_factor += p->phys.mud_ignore_factor;
                    mud_factor = MIN(1.0, mud_factor);
                    break;
                case TILE_ICE:
                    mud_factor = 0.4;
                    friction_factor = 0.04;
                    break;
                case TILE_SPIKES:
                    player_hurt(p,1);
                    break;
                case TILE_TIMED_SPIKES1:
                case TILE_TIMED_SPIKES2:
                    if(level_get_tile_sprite(tt) == SPRITE_TILE_SPIKES)
                    {
                        player_hurt(p,1);
                    }
                    break;
            }
        }

        bool on_edge = memcmp(&p->phys.curr_tile, &c_tile, sizeof(Vector2i)) != 0;
        bool dir_edge[4] = {0};
        if(on_edge)
        {
            dir_edge[DIR_UP] = c_tile.y < p->phys.curr_tile.y;
            dir_edge[DIR_DOWN] = c_tile.y > p->phys.curr_tile.y;
            dir_edge[DIR_LEFT] = c_tile.x < p->phys.curr_tile.x;
            dir_edge[DIR_RIGHT] = c_tile.x > p->phys.curr_tile.x;
        }

        TileType check_tile = tt;
        int _curr_tile_x = p->phys.curr_tile.x;
        int _curr_tile_y = p->phys.curr_tile.y;

        if(check_tile == TILE_PIT && p->phys.pos.z == 0.0 && !p->phys.falling)
        {

#if 0
            if(on_edge)
            {
                printf("on_edge!\n");
                for(int dir = 0; dir < 4; ++dir)
                {
                    if(!dir_edge[dir]) continue;
                    printf(" %s\n", get_dir_name(dir));
                }
            }
#endif

            player_pit_rect.x = cx;
            player_pit_rect.y = cy;
            player_pit_rect.w = 1;
            player_pit_rect.h = 1;

            if(dir_edge[DIR_UP] || dir_edge[DIR_DOWN])
                player_pit_rect.h = 5;

            if(dir_edge[DIR_LEFT] || dir_edge[DIR_RIGHT])
                player_pit_rect.w = 5;

            Rect pit_rect = level_get_tile_rect(_curr_tile_x, _curr_tile_y);

            float shrink_fac = 0.7;
            float adj_fac = (1.0 - shrink_fac) / 2.0;

            tile_pit_rect = pit_rect;
            tile_pit_rect.w = pit_rect.w*shrink_fac;
            tile_pit_rect.h = pit_rect.h*shrink_fac;

            for(int dir = 0; dir < 4; ++dir)
            {
                Vector2i o = get_dir_offsets(dir);
                int nx = _curr_tile_x + o.x;
                int ny = _curr_tile_y + o.y;
                TileType _tt = level_get_tile_type(room, nx, ny);

                float xadj = 0;
                float yadj = 0;

                if(_tt == TILE_PIT || _tt == TILE_BOULDER || _tt == TILE_NONE)  //TILE_NONE means it's against the wall
                {
                    if(dir == DIR_LEFT)
                    {
                        // printf(" left");
                        xadj = -pit_rect.w*adj_fac;
                    }
                    else if(dir == DIR_RIGHT)
                    {
                        // printf(" right");
                        xadj = pit_rect.w*adj_fac;
                    }
                    else if(dir == DIR_UP)
                    {
                        // printf(" up");
                        yadj = -pit_rect.h*adj_fac;
                    }
                    else if(dir == DIR_DOWN)
                    {
                        // printf(" down");
                        yadj = pit_rect.h*adj_fac;
                    }
                }

                tile_pit_rect.x += xadj/2.0;
                tile_pit_rect.w += ABS(xadj);
                tile_pit_rect.y += yadj/2.0;
                tile_pit_rect.h += ABS(yadj);
            }
            // printf("\n");

            if(rectangles_colliding(&tile_pit_rect, &player_pit_rect))
            {
                p->phys.falling = true;
                player_hurt(p, p->phys.pit_damage);
            }
        }
    }

    if(p->phys.falling)
    {
        p->phys.scale -= 0.04;
        if(p->phys.scale < 0)
        {
            p->phys.scale = 1.0;
            p->phys.falling = false;

            TileType tt = level_get_tile_type(room, p->last_safe_tile.x, p->last_safe_tile.y);
            if(IS_SAFE_TILE(tt))
            {
                Vector2f position = level_get_pos_by_room_coords(p->last_safe_tile.x, p->last_safe_tile.y);
                phys_set_collision_pos(&p->phys, position.x, position.y);
            }
            else
            {
                LOGW("Sending to center!");
                player_send_to_room(p, p->phys.curr_room, true, level_get_door_tile_coords(DIR_NONE));
                // player_send_to_level_start(p);
            }

        }
    }

    phys_add_circular_time(&p->phys, dt);

    bool up    = !all_players_dead && p->actions[PLAYER_ACTION_UP].state;
    bool down  = !all_players_dead && p->actions[PLAYER_ACTION_DOWN].state;
    bool left  = !all_players_dead && p->actions[PLAYER_ACTION_LEFT].state;
    bool right = !all_players_dead && p->actions[PLAYER_ACTION_RIGHT].state;

    // handle map navigation
    bool show_map = !all_players_dead && p->actions[PLAYER_ACTION_DISPLAY_MAP].state;

    if(show_map != p->show_map)
    {
        p->show_map = show_map;

        if(p->show_map)
        {
            Room* room = level_get_room_by_index(&level, p->phys.curr_room);
            if(room)
            {
                p->nav_sel.x = room->grid.x;
                p->nav_sel.y = room->grid.y;
            }
        }
    }

    if(p->show_map)
    {
        up    = p->actions[PLAYER_ACTION_UP].toggled_on;
        down  = p->actions[PLAYER_ACTION_DOWN].toggled_on;
        left  = p->actions[PLAYER_ACTION_LEFT].toggled_on;
        right = p->actions[PLAYER_ACTION_RIGHT].toggled_on;

        Dir dir = DIR_NONE;
        if(up) dir = DIR_UP;
        if(down) dir = DIR_DOWN;
        if(left) dir = DIR_LEFT;
        if(right) dir = DIR_RIGHT;

        if(dir != DIR_NONE)
        {
            Vector2i o = get_dir_offsets(dir);
            // Vector2i temp = big_map_sel;

            o.x += p->nav_sel.x;
            o.y += p->nav_sel.y;

            if(level_is_room_valid(&level, o.x, o.y))
            {
                Room* troom = &level.rooms[o.x][o.y];

                bool can_travel = false;

                if(role == ROLE_SERVER && troom->discovered)
                    can_travel = true;
                else if(role == ROLE_LOCAL && (troom->discovered || debug_enabled))
                    can_travel = true;

                if(can_travel)
                {
                    p->nav_sel.x = o.x;
                    p->nav_sel.y = o.y;
                }

            }
        }

        if(activate)
        {
            Room* target_room = &level.rooms[p->nav_sel.x][p->nav_sel.y];
            if(target_room->valid && !visible_room->doors_locked)
            {
                Dir door = DIR_NONE;
                for(int d = 0; d < MAX_DOORS; ++d)
                {
                    if(!target_room->doors[d]) continue;
                    door = d;
                    break;
                }

                Vector2i nt = level_get_door_tile_coords(door);
                bool sent_someone = false;
                for(int i = 0; i < MAX_PLAYERS; ++i)
                {
                    Player* p2 = &players[i];
                    if(!p2->active) continue;
                    if(p2->phys.curr_room == target_room->index)
                    {
                        continue;
                    }
                    sent_someone = true;
                    player_send_to_room(p2, target_room->index, false, nt);
                }

                if(sent_someone)
                {
                    level_grace_time = ROOM_GRACE_TIME;
                    level_room_time = 0.0;
                    level_room_xp = 0;
                }
            }

            //p->show_map = false;
        }

        up = false;
        down = false;
        left = false;
        right = false;
    }

    // update velocity

    Vector2f vel_dir = {0.0,0.0};

    bool moving = false;
    Vector2f vel_max = {0};

    if(!p->phys.falling)
    {
        if(up)
        {
            player_set_sprite_index(p, SPRITE_UP);
            vel_dir.y = -1.0;
        }

        if(down)
        {
            player_set_sprite_index(p, SPRITE_DOWN);
            vel_dir.y = 1.0;
        }

        if(left)
        {
            player_set_sprite_index(p, SPRITE_LEFT);
            vel_dir.x = -1.0;
        }

        if(right)
        {
            player_set_sprite_index(p, SPRITE_RIGHT);
            vel_dir.x = 1.0;
        }

        if((up && (left || right)) || (down && (left || right)))
        {
            // moving diagonally
            vel_dir.x *= 0.7071f;
            vel_dir.y *= 0.7071f;
        }

        vel_max.x = p->phys.max_velocity*p->phys.speed_factor*vel_dir.x*mud_factor;
        vel_max.y = p->phys.max_velocity*p->phys.speed_factor*vel_dir.y*mud_factor;

        moving = (up || down || left || right);

        float speed = p->phys.speed*p->phys.speed_factor*mud_factor;
        if(p->phys.stun_timer > 0.0)
            speed = 0.0;

        if(moving)
        {
            // trying to move
            p->phys.vel.x += vel_dir.x*speed*dt;
            p->phys.vel.y += vel_dir.y*speed*dt;

            if(ABS(vel_dir.x) > 0.0)
                if(ABS(p->phys.vel.x) > ABS(vel_max.x))
                    p->phys.vel.x = vel_max.x;

            if(ABS(vel_dir.y) > 0.0)
                if(ABS(p->phys.vel.y) > ABS(vel_max.y))
                    p->phys.vel.y = vel_max.y;
        }

    }

    phys_apply_gravity(&p->phys,p->gravity,dt);

    bool jump = !all_players_dead && p->actions[PLAYER_ACTION_JUMP].state;
    if(p->phys.falling) jump = false;
    if(jump && p->phys.pos.z == 0.0)
    {
        p->phys.vel.z = jump_vel_z;

        //gaudio_play(audio_jump);
    }

    if(!p->phys.falling)
    {
        // handle friction
        int vel_x_dir = (p->phys.vel.x >= 0) ? 1 : -1;
        int vel_y_dir = (p->phys.vel.y >= 0) ? 1 : -1;

        float vel_magn_x = ABS(p->phys.vel.x);
        float vel_magn_y = ABS(p->phys.vel.y);

        float applied_friction_x = vel_x_dir*MIN(vel_magn_x,p->phys.base_friction*friction_factor);
        float applied_friction_y = vel_y_dir*MIN(vel_magn_y,p->phys.base_friction*friction_factor);

        if(!left && !right)
        {
            p->phys.vel.x -= applied_friction_x;
        }

        if(!up && !down)
        {
            p->phys.vel.y -= applied_friction_y;
        }
    }

    if(ABS(p->phys.vel.x) < 0.1) p->phys.vel.x = 0.0;
    if(ABS(p->phys.vel.y) < 0.1) p->phys.vel.y = 0.0;

    float m1 = magn2f(p->phys.vel.x,p->phys.vel.y);
    float m2 = magn2f(vel_max.x, vel_max.y);

    // for animation speed
    float additional_anim_factor = p->phys.speed_factor*(lookup_movement_speed_max_vel[p->stats[MOVEMENT_SPEED]] / lookup_movement_speed_max_vel[0]) - 1.0;

    p->anim_factor = RANGE(m1/m2,0.3,1.0) + additional_anim_factor;
    if(p->phys.pos.z > 0.0)
    {
        p->anim_factor = RANGE(p->anim_factor, 0.1,0.4);
    }
    //printf("anim_factor: %f\n",p->anim_factor);

    if(p->phys.falling)
    {
        p->phys.vel.x = 0;
        p->phys.vel.y = 0;
    }

    p->phys.prior_vel.x = p->phys.vel.x;
    p->phys.prior_vel.y = p->phys.vel.y;
    p->phys.prior_vel.z = p->phys.vel.z;

    // update position
    p->phys.pos.x += p->phys.vel.x*dt;
    p->phys.pos.y += p->phys.vel.y*dt;



    if(debug_enabled)
    {
        Rect tr = RECT(p->phys.pos.x, p->phys.pos.y, 2, 2);
        if(rectangles_colliding(&tr, &moving_tile))
        {
            // printf("colliding, %.2f, %.2f\n", (moving_tile_prior.x - moving_tile.x), (moving_tile_prior.y - moving_tile.y));
            p->phys.pos.x += -(moving_tile_prior.x - moving_tile.x);
            p->phys.pos.y += -(moving_tile_prior.y - moving_tile.y);
        }
    }


    /*
    Vector2f adj = limit_rect_pos(&player_area, &p->hitbox);
    if(!FEQ0(adj.x) || !FEQ0(adj.y))
    {
        p->phys.pos.x += adj.x;
        p->phys.pos.y += adj.y;
    }
    */

    // update player current tile
    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];

#if 0
    if(p->settings.class == PLAYER_CLASS_ROBOT)
    {
        player_handle_melee(p, dt);
    }
    else if(p->settings.class == PLAYER_CLASS_PHYSICIST)
    {
        player_handle_orbitals(p,dt);
    }
    else
#endif
    {
        player_handle_shooting(p, dt);
    }
    

    // check tiles around player
    handle_room_collision(p);

    player_check_stuck_in_wall(p);

    // if(p == player)  //removed: maybe important idk?
    {
        Room* room = level_get_room_by_index(&level, (int)p->phys.curr_room);

        if(room->type == ROOM_TYPE_BOSS && !room->discovered)
        {
            RoomFileData* rfd = &room_list[room->layout];
            int ctype = rfd->creature_types[0];
            ui_message_set_title(2.0, 0x00880000, 1.2, creature_type_name(ctype));
        }

        room->discovered = true;
    }

    // update animation
    if(moving)
    {
        gfx_anim_update(&p->anim, p->anim_factor*dt);

        if(p->phys.pos.z == 0.0)
        {
            if(p->anim.curr_frame == 1)
            {
                //gaudio_play(audio_run1);
            }
            else if(p->anim.curr_frame == 3)
            {
                //gaudio_play(audio_run2);
            }
        }
    }
    else
    {
        p->anim.curr_frame = 0;
    }


    if(p->phys.floating)
    {
        p->phys.pos.z = 15 + 2*sin(5*p->phys.circular_dt);
        p->anim.curr_frame = 0;
    }

    if(isnan(p->phys.pos.x) || isnan(p->phys.pos.y))
    {
        text_list_add(text_lst, COLOR_RED, 10.0, "player pos was nan!");
        p->phys.pos.x = prior_x;
        p->phys.pos.y = prior_y;
    }

    if(p == player)
    {
        ptext->x = p->phys.pos.x - p->phys.radius/2.0 - 2.0;
        ptext->y = (p->phys.pos.y - p->phys.pos.z/2.0) - p->phys.vr.h - ptext->text_height - 4.0;
        text_list_update(ptext, dt);
    }

    if(role != ROLE_SERVER)
    {
        if(p->phys.curr_room == player->phys.curr_room)
        {
            p->light_index = lighting_point_light_add(p->phys.pos.x, p->phys.pos.y, 1.0, 1.0, 1.0, p->light_radius,0.0);
        }
    }

    if(p->invulnerable_temp)
    {
        p->invulnerable_temp_time += dt;
        if(p->invulnerable_temp_time >= p->invulnerable_temp_max)
        {
            p->invulnerable_temp = false;
        }
    }

#if AUDIO_ENABLE
    // update audio positions
    
    audio_source_update_position(p->source_run1,  p->phys.pos.x, p->phys.pos.y, p->phys.pos.z);
    audio_source_update_position(p->source_run2,  p->phys.pos.x, p->phys.pos.y, p->phys.pos.z);
    audio_source_update_position(p->source_jump, p->phys.pos.x,p->phys.pos.y, p->phys.pos.z);

    if(player == p)
    {
        audio_set_listener_pos(p->phys.pos.x, p->phys.pos.y, p->phys.pos.z);
    }
#endif
}

void player_update_with_input(Player* p, float dt)
{

}

void player_ai_move_to_target(Player* p, Player* target)
{
    if(target->phys.curr_room != p->phys.curr_room)
    {
        p->actions[PLAYER_ACTION_LEFT].state = false;
        p->actions[PLAYER_ACTION_RIGHT].state = false;
        p->actions[PLAYER_ACTION_UP].state = false;
        p->actions[PLAYER_ACTION_DOWN].state = false;
        return;
    }

    // const float threshold = 2.0;
    float threshold = p->phys.radius*2.0 + 6.0;

    float tx = target->phys.pos.x;
    float ty = target->phys.pos.y;

    float dx = p->phys.pos.x - tx;
    float dy = p->phys.pos.y - ty;

    if(ABS(dx) > threshold)
    {
        if(dx < 0)
        {
            p->actions[PLAYER_ACTION_LEFT].state = false;
            p->actions[PLAYER_ACTION_RIGHT].state = true;
        }
        else
        {
            p->actions[PLAYER_ACTION_LEFT].state = true;
            p->actions[PLAYER_ACTION_RIGHT].state = false;
        }
    }
    else
    {
        p->actions[PLAYER_ACTION_LEFT].state = false;
        p->actions[PLAYER_ACTION_RIGHT].state = false;
    }

    if(ABS(dy) > threshold)
    {
        if(dy < 0)
        {
            p->actions[PLAYER_ACTION_UP].state = false;
            p->actions[PLAYER_ACTION_DOWN].state = true;
        }
        else
        {
            p->actions[PLAYER_ACTION_UP].state = true;
            p->actions[PLAYER_ACTION_DOWN].state = false;
        }
    }
    else
    {
        p->actions[PLAYER_ACTION_UP].state = false;
        p->actions[PLAYER_ACTION_DOWN].state = false;
    }

}

void draw_other_player_info(Player* p)
{
    int num = p->phys.hp / 2;
    int rem = p->phys.hp % 2;

    int sprite_index = p->sprite_index+p->anim.curr_frame;
    float ph = gfx_images[p->image].visible_rects[sprite_index].h;
    float pw = gfx_images[p->image].visible_rects[sprite_index].w;

    float x = p->phys.pos.x;
    float y = p->phys.pos.y;

    y += 4.0*ascale;

    float l = 4.0*ascale; // image size
    float pad = 1.0*ascale;

    uint8_t image_full = item_props[ITEM_HEART_FULL].image;
    int si_full = item_props[ITEM_HEART_FULL].sprite_index;
    int image_half = item_props[ITEM_HEART_FULL].image;
    uint8_t si_half = item_props[ITEM_HEART_HALF].sprite_index;
    int image_empty = item_props[ITEM_HEART_EMPTY].image;
    // uint8_t si_empty = item_props[ITEM_HEART_EMPTY].sprite_index;

    // float w = gfx_images[image_full].visible_rects[si_full].w;
    float w = gfx_images[image_full].element_width;
    float scale = l / w;

    // start from the left
    x -= (num+rem)*l/2.0;
    x -= (num+rem-1)*pad/2.0;

    // float name_x = x;
    float name_scale = 0.06*ascale;
    Vector2f text_size = gfx_string_get_size(name_scale, p->settings.name);

    float name_x = p->phys.pos.x - text_size.x/2.0;
    float name_y = y + l;

    // draw image from center
    x += l/2.0;
    y += l/2.0;

    float opacity = 0.5;

    for(int i = 0; i < num; ++i)
    {
        gfx_sprite_batch_add(image_full, si_full, x, y, COLOR_TINT_NONE, false, scale, 0.0, opacity, false, false, false);
        x += (l+pad);
    }

    if(rem == 1)
    {
        gfx_sprite_batch_add(image_half, si_half, x, y, COLOR_TINT_NONE, false, scale, 0.0, opacity, false, false, false);
        x += (l+pad);
    }

    gfx_draw_string(name_x, name_y, COLOR_WHITE, name_scale, NO_ROTATION, 0.7, IN_WORLD, DROP_SHADOW, 0, "%s", p->settings.name);
}

void draw_all_other_player_info()
{
    gfx_sprite_batch_begin(true);

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(p == player) continue;
        if(!p->active) continue;
        if(p->phys.curr_room != player->phys.curr_room) continue;
        draw_other_player_info(p);
    }

    gfx_sprite_batch_draw();
}


float coins_y = 0.0;
void draw_hearts()
{
#define TOP_MARGIN  1

    int max_num = ceill((float)player->phys.hp_max/2.0);
    int num = player->phys.hp / 2;
    int rem = player->phys.hp % 2;

    float pad = 3.0*ascale;
    float l = 30.0*ascale; // image size
    float x = margin_left.w;

#if TOP_MARGIN
    Rect* marg = &margin_top;
    // float y = margin_top.h - 5.0 - l;
    float y = 5.0;
    x = 5.0;
#else
    Rect* marg = &margin_bottom;
    float y = 5.0;
#endif


    uint8_t image_full = item_props[ITEM_HEART_FULL].image;
    int si_full = item_props[ITEM_HEART_FULL].sprite_index;
    int image_half = item_props[ITEM_HEART_FULL].image;
    uint8_t si_half = item_props[ITEM_HEART_HALF].sprite_index;
    int image_empty = item_props[ITEM_HEART_EMPTY].image;
    uint8_t si_empty = item_props[ITEM_HEART_EMPTY].sprite_index;

    // float w = gfx_images[image_full].visible_rects[si_full].w;
    float w = gfx_images[image_full].element_width;
    float scale = l / w;

    Rect area = RECT(x, y, l, l);
    gfx_get_absolute_coords(&area, ALIGN_TOP_LEFT, marg, ALIGN_CENTER);

    x = area.x;
    y = area.y;
    coins_y = area.y + area.h/2.0;
    for(int i = 0; i < num; ++i)
    {
        gfx_draw_image_ignore_light(image_full, si_full, x, y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);
        // Rect r = RECT(x, y, l, l);
        // gfx_draw_rect(&r, COLOR_BLACK, NOT_SCALED, NO_ROTATION, scale, false, NOT_IN_WORLD);
        x += (l+pad);
    }

    if(rem == 1)
    {
        gfx_draw_image_ignore_light(image_half, si_half, x, y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);
        x += (l+pad);
    }

    int empties = max_num - num - rem;
    for(int i = 0; i < empties; ++i)
    {
        gfx_draw_image_ignore_light(image_empty, si_empty, x, y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);
        x += (l+pad);
    }

    // draw_hearts_other_player(player);

}

float revives_y = 0.0;
void draw_coins()
{
    float x = 16.0;
    float y = coins_y + 15.0;
    float scale = 3.0*ascale;
    int sprite = item_props[ITEM_COIN_GOLD].sprite_index;
    int image = item_props[ITEM_COIN_GOLD].image;
    float h = gfx_images[image].visible_rects[sprite].h;
    revives_y = y + h;

    gfx_draw_image_ignore_light(image, sprite, x, y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);
    gfx_draw_string(x+12, y-9, 0xD4AF37, 0.25*ascale, NO_ROTATION, 1.0, NOT_IN_WORLD, DROP_SHADOW, 0, "%u", player->coins);
}

void draw_revives()
{
    float x = 16.0;
    float y = revives_y + 15.0;
    float scale = 1.4*ascale;
    int sprite = item_props[ITEM_REVIVE].sprite_index;
    int image = item_props[ITEM_REVIVE].image;
    float h = gfx_images[image].visible_rects[sprite].h;

    gfx_draw_image_ignore_light(image, sprite, x, y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);
    gfx_draw_string(x+12, y-9, COLOR_TINT_NONE, 0.25*ascale, NO_ROTATION, 1.0, NOT_IN_WORLD, DROP_SHADOW, 0, "%u", player->revives);
}

#if 0
void draw_gauntlet()
{
    float len = 50.0 * ascale;
    float margin = 5.0 * ascale;

    int w = gfx_images[skills_image].element_width;
    float scale = len / (float)w;

    float total_len = (PLAYER_MAX_SKILLS * len) + ((PLAYER_MAX_SKILLS-1) * margin);

    float x = (view_width - total_len) / 2.0;   // left side of first rect
    x += len/2.0;   // centered

    // float y = (float)view_height * 0.75;
    float y = view_height - len/2.0 - margin - 10.0;

    Rect r = {0};
    r.w = len+1;
    r.h = len+1;
    r.y = y;
    r.x = x;

    // for(int i = 0; i < player->skill_count; ++i)
    for(int i = 0; i < PLAYER_MAX_SKILLS; ++i)
    {
        uint32_t color = COLOR_BLACK;
        Skill skill = player->skills[i];

        bool selected_skill = (i == player->gauntlet_selection);
        if(selected_skill)
        {
            if(skill.type != SKILL_TYPE_NONE)
            {
                const char* name = skills_get_name(skill.type);
                float tscale = 0.12 * ascale;
                Vector2f size = gfx_string_get_size(tscale, (char*)name);

                float tlx = r.x - r.w/2.0;
                // float tly = r.y - r.h/2.0 + size.y + 2.0;
                float bly = r.y + r.h/2.0 + 2.0;

                gfx_draw_string(tlx, bly, COLOR_WHITE, tscale, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "%s (%d)", name, skill.rank);

            }

            color = 0x00b0b0b0;
        }

        gfx_draw_rect(&r, color, NOT_SCALED, NO_ROTATION, 0.3, true, NOT_IN_WORLD);

        float tlx = r.x - r.w/2.0;
        float tly = r.y - r.h/2.0;

        if(skill.type != SKILL_TYPE_NONE)
        {
            gfx_draw_image_ignore_light(skills_image, skill.type, r.x, r.y, COLOR_TINT_NONE, 0.8*scale, 0.0, 1.0, false, NOT_IN_WORLD);

            if(skill.duration_timer > 0)
            {
                Rect tr = {0};
                tr.h = r.h * (skill.duration_timer / skill.duration);
                tr.w = r.w;
                tr.x =  tlx;
                tr.y = tly + (r.h-tr.h);
                gfx_draw_rect_tl(&tr, 0x0020f020, 1.0, NO_ROTATION, 0.3, true, NOT_IN_WORLD);
            }
            else if(skill.cooldown_timer > 0)
            {
                Rect tr = {0};
                tr.h = r.h * (1.0 - skill.cooldown_timer / skill.cooldown);
                tr.w = r.w;
                tr.x = tlx;
                tr.y = tly + (r.h-tr.h);
                gfx_draw_rect_tl(&tr, 0x00f02020, 1.0, NO_ROTATION, 0.3, true, NOT_IN_WORLD);
            }
            else if(!skills_can_use((void*)player, &skill))
            {
                gfx_draw_rect(&r, 0x00202020, 0.8, NO_ROTATION, 0.8, true, NOT_IN_WORLD);
            }

        }


        gfx_draw_string(tlx, tly, COLOR_WHITE, 0.15 * ascale, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "%d", i+1);


        r.x += len;
        r.x += margin;
    }
}
#endif

void draw_stats()
{
    float x = 10.0;
    float y = 180.0;

    float margin = 5.0;
    float len = 15.0;

    Vector2i im = stats_get_img(0);
    GFXImage* img = &gfx_images[im.x];
    float scale = len / img->element_width;
    

    for(int i = 0; i < MAX_STAT_TYPE; ++i)
    {
        Vector2i im = stats_get_img(i);
        GFXImage* img = &gfx_images[im.x];

        float _w = scale * img->visible_rects[im.y].w;
        float _h = scale * img->visible_rects[im.y].h;

        gfx_draw_image_ignore_light(im.x, im.y, x, y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);

        float _x = x + len + 1.0;

        for(int j = 0; j < 7; ++j)
        {
            // border
            Rect r = {0};
            r.x = _x;
            r.y = y - len/2.0;
            r.w = 3.0;
            r.h = len;

            if(player->stats[i] >= j)
            {
                float adj = 0.2;
                Rect r2 = r;
                r2.w -= adj*2;
                r2.h -= adj*2;
                r2.x += adj;
                r2.y += adj;
                gfx_draw_rect_tl(&r2, COLOR_WHITE, NOT_SCALED, NO_ROTATION, 0.8, true, NOT_IN_WORLD);
            }

            gfx_draw_rect_tl(&r, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 1.0, false, NOT_IN_WORLD);

            _x += 6.0;
        }

        y += len + margin;
    }

    // gfx_draw_string(x, y, COLOR_WHITE, 0.17, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "%u, %u %u, %u, %u", player->total_kills, player->total_shots, player->total_hits, player->level_hits, player->room_hits);

}

void draw_statistics()
{
    // if(!debug_enabled) return;
    float scale = 0.16*ascale;
    float x = 10.0;
    float y = 90.0;
    uint32_t color = COLOR_WHITE;
    Vector2f size = gfx_string_get_size(scale, "|");

    gfx_draw_string(x, y, color, scale, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "Kills: %u", player->total_kills); y += size.y + 3.0;
    gfx_draw_string(x, y, color, scale, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "Hits:  %u/%u", player->total_hits, player->total_shots); y += size.y + 3.0;

    // other stuff
    // gfx_draw_string(10, 90, COLOR_WHITE, 0.17, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "%u, %u %u, %u, %u", player->total_kills, player->total_shots, player->total_hits, player->level_hits, player->room_hits);
}

void draw_artifacts()
{

    const float spacer = 20;
    float x = VIEW_WIDTH - spacer;
    float y = VIEW_HEIGHT - 32;

    uint32_t artifact_count = 0;
    for(int i  = 0; i < 32; ++i)
    {
        int has_artifact = ((player->artifacts >> i) & 0x1);
        if(has_artifact)
        {
            ItemType it = -1;
            switch(i)
            {
                case ARTIFACT_SLOT_DRAGON_EGG:        it = ITEM_DRAGON_EGG; break;
                case ARTIFACT_SLOT_GEM_YELLOW:        it = ITEM_GEM_YELLOW; break;
                case ARTIFACT_SLOT_WING:              it = ITEM_WING; break;
                case ARTIFACT_SLOT_FEATHER:           it = ITEM_FEATHER; break;
                case ARTIFACT_SLOT_RUBY_RING:         it = ITEM_RUBY_RING; break;
                case ARTIFACT_SLOT_POTION_STRENGTH:   it = ITEM_POTION_STRENGTH; break;
                case ARTIFACT_SLOT_POTION_MANA:       it = ITEM_POTION_MANA; break;
                case ARTIFACT_SLOT_POTION_GREAT_MANA: it = ITEM_POTION_GREAT_MANA; break;
                case ARTIFACT_SLOT_POTION_PURPLE:     it = ITEM_POTION_PURPLE; break;
                case ARTIFACT_SLOT_SHAMROCK:          it = ITEM_SHAMROCK; break;
                case ARTIFACT_SLOT_GAUNTLET_SLOT:     it = ITEM_GAUNTLET_SLOT; break;
                case ARTIFACT_SLOT_UPGRADE_ORB:       it = ITEM_UPGRADE_ORB; break;
                case ARTIFACT_SLOT_GALAXY_PENDANT:    it = ITEM_GALAXY_PENDANT; break;
                case ARTIFACT_SLOT_SHIELD:            it = ITEM_SHIELD; break;
                case ARTIFACT_SLOT_LOOKING_GLASS:     it = ITEM_LOOKING_GLASS; break;
                case ARTIFACT_SLOT_BOOK:              it = ITEM_BOOK; break;
                case ARTIFACT_SLOT_GEM_WHITE:         it = ITEM_GEM_WHITE; break;
                case ARTIFACT_SLOT_GEM_PURPLE:        it = ITEM_GEM_PURPLE; break;
            }

            gfx_draw_image_ignore_light(items_image, it, x - (spacer*artifact_count), y, COLOR_TINT_NONE, 1.0, 0.0, 0.8, false, NOT_IN_WORLD);

            artifact_count ++;
        }
    }
}

void draw_equipped_gun()
{
    float box_w = 60.0;
    float box_h = 60.0;
    float margin = 5.0;

    Gun* gun = &player->gun;

    Rect r = {box_w/2.0 + margin, view_height-box_h/2.0 - margin, box_w, box_h};

    gfx_draw_rect(&r, COLOR_WHITE, NOT_SCALED, NO_ROTATION, 0.3, true, NOT_IN_WORLD);
    gfx_draw_image_ignore_light(guns_image, gun->gun_sprite_index, r.x, r.y, gun->color1, 3.0, 0.0, 0.8, false, NOT_IN_WORLD);

    float x = r.x+r.w/2.0 + 5;
    float y = r.y-r.h/2.0 + 5;

    Vector2f sbig = gfx_string_get_size(0.25, "Gun: %s", player->gun.name);
    Vector2f ssmall = gfx_string_get_size(0.15, "Damage");

    gfx_draw_string(x, y, COLOR_CYAN, 0.25, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "%s", gun->name);
    y += sbig.y + 2;
    gfx_draw_string(x, y, COLOR_WHITE, 0.15, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "Damage: %.2f - %.2f", gun->damage_min, gun->damage_max);
    y += ssmall.y + 2;
    gfx_draw_string(x, y, COLOR_WHITE, 0.15, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "Crit: %.2f Knbk: %.2f", gun->critical_hit_chance, gun->knockback_factor);
    y += ssmall.y + 2;
    gfx_draw_string(x, y, COLOR_WHITE, 0.15, NO_ROTATION, 0.9, NOT_IN_WORLD, DROP_SHADOW, 0, "\"%s\"", gun->desc);
}

void player_set_class(Player* p, PlayerClass class)
{
    p->settings.class = class;

#if 1
    lookup_strength                     = lookup_spaceman_strength;
    lookup_defense                      = lookup_spaceman_defense;
    lookup_movement_speed               = lookup_spaceman_movement_speed;
    lookup_movement_speed_max_vel       = lookup_spaceman_movement_speed_max_vel;
    lookup_movement_speed_base_friction = lookup_spaceman_movement_speed_base_friction;
    lookup_attack_speed                 = lookup_spaceman_attack_speed;
    lookup_attack_range                 = lookup_spaceman_attack_range;
    lookup_luck                         = lookup_spaceman_luck;

    switch(class)
    {
        case PLAYER_CLASS_SPACEMAN:
            p->image = class_image_spaceman;
            break;
        case PLAYER_CLASS_PHYSICIST:
            p->image = class_image_physicist;
            break;
        case PLAYER_CLASS_ROBOT:
            p->image = class_image_robot;
            break;
        default:
            LOGE("Invalid class: %d", class);
            p->image = class_image_robot;
            break;
    }
#else
    switch(class)
    {
        case PLAYER_CLASS_SPACEMAN:
            p->image = class_image_spaceman;
            p->gun.orbital = false;
            p->gun.ghost_chance = 0.0f;

            lookup_strength                     = lookup_spaceman_strength;
            lookup_defense                      = lookup_spaceman_defense;
            lookup_movement_speed               = lookup_spaceman_movement_speed;
            lookup_movement_speed_max_vel       = lookup_spaceman_movement_speed_max_vel;
            lookup_movement_speed_base_friction = lookup_spaceman_movement_speed_base_friction;
            lookup_attack_speed                 = lookup_spaceman_attack_speed;
            lookup_attack_range                 = lookup_spaceman_attack_range;
            lookup_luck                         = lookup_spaceman_luck;

            break;
        case PLAYER_CLASS_PHYSICIST:
            p->image = class_image_physicist;
            p->gun.orbital = true;
            p->gun.ghost_chance = 1.0f;

            lookup_strength                     = lookup_physicist_strength;
            lookup_defense                      = lookup_physicist_defense;
            lookup_movement_speed               = lookup_physicist_movement_speed;
            lookup_movement_speed_max_vel       = lookup_physicist_movement_speed_max_vel;
            lookup_movement_speed_base_friction = lookup_physicist_movement_speed_base_friction;
            lookup_attack_speed                 = lookup_physicist_attack_speed;
            lookup_attack_range                 = lookup_physicist_attack_range;
            lookup_luck                         = lookup_physicist_luck;

            break;
        case PLAYER_CLASS_ROBOT:
            p->image = class_image_robot;
            p->gun.orbital = false;
            p->gun.ghost_chance = 0.0f;

            lookup_strength                     = lookup_robot_strength;
            lookup_defense                      = lookup_robot_defense;
            lookup_movement_speed               = lookup_robot_movement_speed;
            lookup_movement_speed_max_vel       = lookup_robot_movement_speed_max_vel;
            lookup_movement_speed_base_friction = lookup_robot_movement_speed_base_friction;
            lookup_attack_speed                 = lookup_robot_attack_speed;
            lookup_attack_range                 = lookup_robot_attack_range;
            lookup_luck                         = lookup_robot_luck;

            break;
        default:
            LOGE("Invalid class: %d", class);
            p->image = class_image_robot;
            break;
    }
#endif

    Rect* vr = &gfx_images[p->image].visible_rects[0];
    // print_rect(vr);
    p->phys.vr = *vr;
    phys_calc_collision_rect(&p->phys);
    p->phys.radius = calc_radius_from_rect(&p->phys.collision_rect)*0.75;
}

void player_draw(Player* p)
{
    if(!p->active) return;
    if(p->phys.curr_room != player->phys.curr_room) return;

    if(all_players_dead)
        return;

    // Room* room = level_get_room_by_index(&level, (int)p->phys.curr_room);

    bool blink = p->invulnerable_temp ? ((int)(p->invulnerable_temp_time * 100)) % 2 == 0 : false;
    float opacity = p->phys.dead ? 0.3 : 1.0;

    opacity = blink ? 0.3 : opacity;

    if(p->weapon.type != WEAPON_TYPE_NONE && p->weapon.rotation_deg == 90.0)
    {
        weapon_draw(&p->weapon);
    }

    //uint32_t color = gfx_blend_colors(COLOR_BLUE, COLOR_TINT_NONE, p->phys.speed_factor);
    float y = p->phys.pos.y - (p->phys.vr.h*p->phys.scale + p->phys.pos.z)/2.0;
    bool ret = gfx_sprite_batch_add(p->image, p->sprite_index+p->anim.curr_frame, p->phys.pos.x, y, p->settings.color, true, p->phys.scale, 0.0, opacity, false, false, false);
    if(!ret) printf("Failed to add player to batch!\n");

    if(p->weapon.type != WEAPON_TYPE_NONE && p->weapon.rotation_deg != 90.0)
    {
        weapon_draw(&p->weapon);
    }

    if(p->gun.charging)
    {
        const float bar_width = 20;
        const float bar_height = 2;

        const float x = p->phys.pos.x - bar_width/2.0;
        const float y = p->phys.pos.y - p->phys.height - bar_height - 10;

        gfx_draw_rect_xywh_tl(x, y, bar_width, bar_height, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 0.4, true, true);
        gfx_draw_rect_xywh_tl(x, y, bar_width*(p->gun.charge_time / p->gun.charge_time_max), bar_height, COLOR_YELLOW, NOT_SCALED, NO_ROTATION, 0.4, true, true);

    }

    if(debug_enabled)
    {
        if(p->weapon.state == WEAPON_STATE_RELEASE)
        {
            GFXImage* img = &gfx_images[p->weapon.image];
            Rect* vr = &img->visible_rects[0];

            bool vertical = (p->weapon.rotation_deg == 90.0 || p->weapon.rotation_deg == 270.0);
            float w = vertical ? vr->h : vr->w;
            float h = vertical ? vr->w : vr->h;

            gfx_draw_rect_xywh(p->weapon.pos.x, p->weapon.pos.y, w,h, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        }
    }

    if(p == player)
    {

        Item* highlighted_item = NULL;
        if(p->highlighted_item_id != -1)
        {
            highlighted_item = item_get_by_id(p->highlighted_item_id);
        }
        if(highlighted_item)
        {
            highlighted_item->highlighted = true;

            const char* desc = item_get_description(highlighted_item->type, highlighted_item);
            const char* name = item_get_name(highlighted_item->type);

            int cost = 0;
            if(highlighted_item->type == ITEM_GUN)
            {
                if(highlighted_item->user_data2 < MAX_ROOM_GUNS)
                    cost = room_gun_list[highlighted_item->user_data2].cost;
            }

            char cost_str[32] = {0};
            if(cost > 0)
            {
                sprintf(cost_str, " [Cost: %u]", cost);
            }

            if(strlen(desc) > 0)
            {
                ui_message_set_small(0.1, "%s (%s)%s", name, desc, cost_str);
            }
            else
            {
                ui_message_set_small(0.1, "%s%s", name, cost_str);
            }
        }

    }
    // else
    // {
    //     draw_hearts_other_player(p);
    // }
}

void player_lerp(Player* p, float dt)
{
    if(!p->active) return;

    const float decay = 16.0;

    p->phys.pos = exp_decay3f(p->phys.pos, p->server_state_target.pos, decay, dt);
    p->invulnerable_temp_time = exp_decay(p->invulnerable_temp_time, p->server_state_target.invulnerable_temp_time, decay, dt);
    p->weapon.scale = exp_decay(p->weapon.scale, p->server_state_target.weapon_scale, decay, dt);
}

void player_handle_net_inputs(Player* p, double dt)
{
    // handle input
    memcpy(&p->input_prior, &p->input, sizeof(NetPlayerInput));

    p->input.delta_t = dt;
    p->input.keys = 0;

    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        if(p->actions[i].state && (player_ignore_input == 0))
        {
            p->input.keys |= (1<<i);
        }
    }

    if(p->phys.curr_room != p->transition_room || paused)
    {
        p->input.keys = 0;
    }

    if(role != ROLE_CLIENT)
        return;

    if(p->input.keys != p->input_prior.keys && (player_ignore_input == 0))
    {
        // printf("enter state: %d\n", p->actions[PLAYER_ACTION_ACTIVATE].state);
        // printf("add player input: %d\n", player_ignore_input);
        WorldState s = {
            .players[0].pos = p->phys.pos
        };
        net_client_add_player_input(&p->input, &s);
    }

    if(player_ignore_input > 0)
    {
        player_ignore_input--;
        // printf("player_ignore_input: %d\n", player_ignore_input);
    }
}

bool player_check_other_player_collision(Player* p)
{
    if(p->phys.dead)
        return false;
    if(!p->active)
        return false;

    CollisionInfo ci = {0};
    bool check = false;
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p2 = &players[i];
        if(p == p2) continue;
        if(!p2->active) continue;
        if(p2->phys.dead) continue;

        check = phys_collision_circles(&p->phys,&p2->phys, &ci);
        if(check) break;
    }

    return check;
}

void player_check_stuck_in_wall(Player* p)
{
    if(!p->active) return;
    if(p->phys.dead) return;

    float cx = p->phys.collision_rect.x;
    float cy = p->phys.collision_rect.y;
    Vector2i coords = level_get_room_coords_by_pos(cx, cy);
    Room* room = level_get_room_by_index(&level, (int)p->phys.curr_room);
    TileType tt = level_get_tile_type(room, coords.x, coords.y);
    if(tt == TILE_BOULDER)
    {
        TileType tt = level_get_tile_type(room, p->last_safe_tile.x, p->last_safe_tile.y);
        if(IS_SAFE_TILE(tt))
        {
            Vector2f position = level_get_pos_by_room_coords(p->last_safe_tile.x, p->last_safe_tile.y);
            phys_set_collision_pos(&p->phys, position.x, position.y);
        }
    }
}


void player_handle_collision(Player* p, Entity* e)
{
    if(all_players_dead)
        return;
    if(p->phys.dead)
        return;

    switch(e->type)
    {
        case ENTITY_TYPE_CREATURE:
        {
            Creature* c = (Creature*)e->ptr;

            Physics cphys = c->phys;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&cphys, &ci);

            if(c->segmented)
            {
                for(int i = 0; i < creature_segment_count; ++i)
                {
                    if(collided)
                        break;

                    CreatureSegment* cs = &creature_segments[i];
                    cphys.collision_rect = cs->collision_rect;

                    collided = phys_collision_circles(&p->phys,&cphys, &ci);
                }
            }

            if(collided)
            {
                if(p->phys.pos.z <= 3 && !p->phys.floating)
                {
                    phys_collision_correct(&p->phys, &cphys,&ci);
                }

                if(c->painful_touch)
                {
                    player_hurt(p,c->damage);
                }
            }

            // check for weapon collision here?
            // it may be better to add weapons to the entity list
            // so they can be used by creatures too, or thrown or something

            if(p->weapon.state == WEAPON_STATE_RELEASE)
            {
                GFXImage* img = &gfx_images[p->weapon.image];
                Rect* vr = &img->visible_rects[0];

                bool vertical = (p->weapon.rotation_deg == 90.0 || p->weapon.rotation_deg == 270.0);

                float w = vertical ? vr->h : vr->w;
                float h = vertical ? vr->w : vr->h;

                Box weap_box = {
                    p->weapon.pos.x,
                    p->weapon.pos.y,
                    p->weapon.pos.z + 32,
                    w,
                    h,
                    64,
                };
                
                Box check = {
                    cphys.collision_rect.x,
                    cphys.collision_rect.y,
                    cphys.pos.z + p->phys.height/2.0,
                    cphys.collision_rect.w,
                    cphys.collision_rect.h,
                    cphys.height*2,
                };

                bool hit = boxes_colliding(&weap_box, &check);

                if(hit)
                {
                    if(!weapon_is_in_hit_list(&p->weapon, c->id))
                    {
                        weapon_add_hit_id(&p->weapon, c->id);
                        creature_hurt(c, p->weapon.damage, p->index);
                    }
                }
            }

        } break;
        case ENTITY_TYPE_PLAYER:
        {
            Player* p2 = (Player*)e->ptr;

            if(p->ignore_player_collision && p2->ignore_player_collision) return;

            CollisionInfo ci = {0};

            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);
            if(collided)
            {
                phys_collision_correct(&p->phys, &p2->phys,&ci);
            }

        } break;

        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            if(p2->used) break;


            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);

            if(collided)
            {

                // if(item_props[p2->type].touchable && p2->phys.pos.z <= 0)
                if(item_props[p2->type].touchable)
                {
                    item_use(p2, (void*)p);
                }
                else
                {
                    phys_collision_correct(&p->phys, &p2->phys,&ci);
                    //phys_collision_correct_no_bounce(&p->phys, &p2->phys, &ci);
                }

            }

        } break;
    }
}

bool is_any_player_room(uint8_t curr_room)
{
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(!players[i].active) continue;
        if(curr_room == players[i].phys.curr_room)
        {
            return true;
        }
    }

    return false;
}

bool player_has_artifact(Player* p, uint32_t artifact_slot)
{
    return BIT_CHECK(p->artifacts, artifact_slot);
}

int player_get_count_in_room(uint8_t curr_room)
{
    int count = 0;
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(!players[i].active) continue;
        if(curr_room == players[i].phys.curr_room)
        {
            count++;
        }
    }
    return count;
}

const char* stats_get_name(StatType stat)
{
    switch(stat)
    {
        case STRENGTH: return "Strength";
        case DEFENSE: return "Defense";
        case MOVEMENT_SPEED: return "Movement Speed";
        case ATTACK_SPEED: return "Attack Speed";
        case ATTACK_RANGE: return "Attack Range";
        case LUCK: return "Luck";
    }
    return "???";
}

ItemType stats_get_item(StatType stat)
{
    switch(stat)
    {
        case STRENGTH: return ITEM_POTION_STRENGTH;
        case DEFENSE: return ITEM_SHIELD;
        case MOVEMENT_SPEED: return ITEM_FEATHER;
        case ATTACK_SPEED: return ITEM_WING;
        case ATTACK_RANGE: return ITEM_LOOKING_GLASS;
        case LUCK: return ITEM_SHAMROCK;
    }
    return ITEM_NONE;
}

Vector2i stats_get_img(StatType stat)
{
    ItemProps* p = &item_props[stats_get_item(stat)];
    Vector2i ret = {0};
    ret.x = p->image;
    ret.y = p->sprite_index;
    return ret;
}

void player_set_gun(Player* p, uint8_t room_gun_index, bool drop_old_gun)
{
    p->proj_cooldown = 0.0;

    uint8_t prior_room_gun_index = p->room_gun_index;
    p->room_gun_index = room_gun_index;

    memcpy(&p->gun, &room_gun_list[room_gun_index], sizeof(Gun));

    printf("Gun Perks (%d):\n", p->gun.num_perks);
    for(int i = 0; i < p->gun.num_perks; ++i)
    {
        printf("[%d] %s\n", i, get_gun_perk_name(p->gun.perks[i]));
    }

    Gun* gun_prior = &room_gun_list[prior_room_gun_index];

    if(drop_old_gun)
    {
        for(int i = 0; i < gun_catalog_count; ++i)
        {
            if(STR_EQUAL(gun_prior->name, gun_catalog[i].name))
            {
                float x = p->phys.pos.x;
                float y = p->phys.pos.y;
                int croom = p->phys.curr_room;
                Item* it = item_add_gun(i, 0, x, y, croom);
                it->user_data2 = prior_room_gun_index;
                return;
            }
        }
    }
}
