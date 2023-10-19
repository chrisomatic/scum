#pragma once

#include "main.h"
#include "physics.h"
#include "gfx.h"
#include "net.h"

#define MAX_PLAYERS 4

#define SPRITE_UP    0
#define SPRITE_DOWN  4
#define SPRITE_LEFT  8
#define SPRITE_RIGHT 12

#define PLAYER_NAME_MAX 16

enum PlayerActions
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
    PLAYER_ACTION_DOOR,
    PLAYER_ACTION_SHOOT,
    PLAYER_ACTION_GENERATE_ROOMS,

    PLAYER_ACTION_MAX
};

typedef struct
{
    bool active;

    char name[PLAYER_NAME_MAX+1];

    Physics phys;
    float vel_factor;

    PlayerInput actions[PLAYER_ACTION_MAX];

    GFXAnimation anim;

    uint8_t sprite_index;
    Rect hitbox;
    Rect hitbox_prior;

    uint8_t curr_room;
    uint8_t transition_room;
    Dir door;

    uint8_t hp;
    uint8_t hp_max;

    Vector2i curr_tile;

    float proj_cooldown;
    float proj_cooldown_max;

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
extern int num_players;

void player_init();
void player_init_keys();
void player_set_to_level_start(Player* p);
void player_update(Player* p, float dt);
void player_draw(Player* p);
void player_reset(Player* p);
void player_lerp(Player* p, float dt);
void player_handle_net_inputs(Player* p, double dt);
void player_set_hit_box_pos(Player* p, float x, float y);
void player_set_collision_pos(Player* p, float x, float y);
void player_draw_room_transition();
void player_start_room_transition(Player* p);

int player_names_build(bool include_all, bool only_active);
