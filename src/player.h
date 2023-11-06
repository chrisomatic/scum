#pragma once

#include "main.h"
#include "physics.h"
#include "gfx.h"
#include "net.h"
#include "item.h"
#include "entity.h"

#define BOI_SHOOTING    1

#define MAX_PLAYERS 4

#define SPRITE_UP    0
#define SPRITE_DOWN  4
#define SPRITE_LEFT  8
#define SPRITE_RIGHT 12

#define PLAYER_NAME_MAX 16

#define PLAYER_GEMS_MAX 4

typedef enum
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
#if BOI_SHOOTING
    PLAYER_ACTION_SHOOT_UP,
    PLAYER_ACTION_SHOOT_DOWN,
    PLAYER_ACTION_SHOOT_LEFT,
    PLAYER_ACTION_SHOOT_RIGHT,
#else
    PLAYER_ACTION_SHOOT,
#endif
    PLAYER_ACTION_GENERATE_ROOMS,
    PLAYER_ACTION_GEM_MENU,
    PLAYER_ACTION_GEM_MENU_CYCLE,

    PLAYER_ACTION_MAX
} PlayerActions;

typedef struct
{
    bool active;

    char name[PLAYER_NAME_MAX+1];

    Physics phys;
    float vel_factor;

    PlayerInput actions[PLAYER_ACTION_MAX];

    PlayerActions last_shoot_action;
    float shoot_sprite_cooldown;

    ItemType gems[PLAYER_GEMS_MAX];

    GFXAnimation anim;

    uint8_t sprite_index;
    Rect hitbox;
    Rect hitbox_prior;

    uint8_t curr_room;
    uint8_t transition_room;
    Dir door;

    int light_index;

    Vector2i curr_tile;

    uint8_t proj_charge;
    float proj_cooldown;
    float proj_cooldown_max;

    bool invulnerable;
    float invulnerable_time;
    float invulnerable_max;

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

void player_init();
void player_init_keys();
void player2_init_keys();
void player_send_to_room(Player* p, uint8_t room_index);
void player_send_to_level_start(Player* p);
void player_update(Player* p, float dt);
void player_draw(Player* p);
void player_lerp(Player* p, float dt);
void player_handle_net_inputs(Player* p, double dt);
void player_set_hit_box_pos(Player* p, float x, float y);
void player_set_collision_pos(Player* p, float x, float y);
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
