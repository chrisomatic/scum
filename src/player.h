#pragma once

#include "main.h"
#include "physics.h"
#include "gfx.h"
#include "net.h"
#include "item.h"
#include "skills.h"
#include "projectile.h"


#define MAX_PLAYERS 4

#define MAX_SKILL_CHOICES  6

#define SPRITE_UP    0
#define SPRITE_DOWN  4
#define SPRITE_LEFT  8
#define SPRITE_RIGHT 12

#define PLAYER_NAME_MAX 16
#define PLAYER_MAX_SKILLS 20

// #define PLAYER_SWAPPING_GEM(p) (p->gauntlet_item.type != ITEM_NONE)

#define PLAYER_GAUNTLET_MAX 8

typedef enum
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
    PLAYER_ACTION_RSHIFT,
    PLAYER_ACTION_SHOOT_UP,
    PLAYER_ACTION_SHOOT_DOWN,
    PLAYER_ACTION_SHOOT_LEFT,
    PLAYER_ACTION_SHOOT_RIGHT,
    PLAYER_ACTION_GENERATE_ROOMS,
    PLAYER_ACTION_ACTIVATE,
    PLAYER_ACTION_JUMP,
    PLAYER_ACTION_GEM_MENU,
    PLAYER_ACTION_TAB_CYCLE,
    PLAYER_ACTION_ITEM_CYCLE,

    PLAYER_ACTION_MAX
} PlayerActions;

typedef struct
{
    bool active;

    char name[PLAYER_NAME_MAX+1];

    Physics phys;
    float vel_factor;

    float scale;

    int xp;
    int level;
    int new_levels;

    PlayerInput actions[PLAYER_ACTION_MAX];

    PlayerActions last_shoot_action;
    float shoot_sprite_cooldown;

    Item gauntlet_item;
    uint8_t gauntlet_selection;
    uint8_t gauntlet_slots;
    Item gauntlet[PLAYER_GAUNTLET_MAX];

    int skills[PLAYER_MAX_SKILLS];
    int skill_count;
    int num_skill_choices;

    ProjectileDef proj_def;
    ProjectileDef proj_def_gauntlet;
    ProjectileDef proj_discharge;

    GFXAnimation anim;

    uint8_t sprite_index;
    uint8_t curr_room;
    uint8_t transition_room;
    Dir door;

    int light_index;
    float light_radius;

    Vector2i curr_tile;
    Vector2i last_safe_tile;

    uint8_t proj_charge;
    float proj_cooldown;
    float proj_cooldown_max;

    bool invulnerable;
    float invulnerable_time;
    float invulnerable_max;

    Item* highlighted_item;
    int highlighted_index;

    // periodic shot
    float periodic_shot_counter;
    float periodic_shot_counter_max;

    // Networking
    NetPlayerInput input;
    NetPlayerInput input_prior;

    float lerp_t;
    ObjectState server_state_prior;
    ObjectState server_state_target;

} Player;

extern char* player_names[MAX_PLAYERS+1]; // used for name dropdown. +1 for ALL option.
extern Player players[MAX_PLAYERS];
extern Player* player;
extern Player* player2;
extern int xp_levels[];

extern bool boost_stats;


extern int skill_selection;
extern int skill_choices[MAX_SKILL_CHOICES];
extern int num_skill_choices;
extern float jump_vel_z;

extern int shadow_image;

void player_init();
uint8_t player_get_gauntlet_count(Player* p);
void player_drop_item(Player* p, Item* it);
bool player_gauntlet_full(Player* p);
void player_init_keys();
void player2_init_keys();
void player_send_to_room(Player* p, uint8_t room_index);
void player_send_to_level_start(Player* p);
void player_update(Player* p, float dt);
void player_draw(Player* p);
void player_draw_debug(Player* p);
void player_lerp(Player* p, float dt);
void player_handle_net_inputs(Player* p, double dt);
void player_set_collision_pos(Player* p, float x, float y);
void player_add_xp(Player* p, int xp);
void player_hurt_no_inv(Player* p, int damage);
void player_hurt(Player* p, int damage);
void player_add_hp(Player* p, int hp);
void player_die(Player* p);
void player_reset(Player* p);
void player_draw_room_transition();
void player_start_room_transition(Player* p);

void player_set_active(Player* p, bool active);
int player_get_active_count();
void player_handle_collision(Player* p, Entity* e);
bool is_any_player_room(uint8_t curr_room);
int player_get_count_in_room(uint8_t curr_room);

int player_names_build(bool include_all, bool only_active);

void draw_hearts();
void draw_xp_bar();
void draw_gauntlet();
void randomize_skill_choices(Player* p);
void draw_skill_selection();

int get_xp_req(int level);
