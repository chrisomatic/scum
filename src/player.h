#pragma once

#include "gfx.h"
#include "net.h"

#define MAX_PLAYERS 4

#define SPRITE_UP    0
#define SPRITE_DOWN  4
#define SPRITE_LEFT  8
#define SPRITE_RIGHT 12

enum PlayerActions
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
    PLAYER_ACTION_RUN,
    PLAYER_ACTION_SHOW_MAP,
    PLAYER_ACTION_DOOR,
    PLAYER_ACTION_SCUM,
    PLAYER_ACTION_SHOOT,
    PLAYER_ACTION_GENERATE_ROOMS,

    PLAYER_ACTION_MAX
};

typedef struct
{
    bool active;

    Vector2f vel;
    Vector2f pos;

    PlayerInput actions[PLAYER_ACTION_MAX];

    GFXAnimation anim;

    uint8_t sprite_index;
    Rect hitbox;
    Rect hitbox_prior;
    float radius;

    Vector2i curr_room;

    float proj_cooldown;

    // Networking
    NetPlayerInput input;
    NetPlayerInput input_prior;

    float lerp_t;
    ObjectState server_state_prior;
    ObjectState server_state_target;

} Player;

extern Player players[MAX_PLAYERS];
extern Player* player;
extern int num_players;

void player_init();
void player_init_keys();
void player_update(Player* p, float dt);
void player_draw(Player* p);
void player_reset(Player* p);
void player_lerp(Player* p, float dt);
void player_handle_net_inputs(Player* p, double dt);
void player_set_hit_box_pos(Player* p, float x, float y);
