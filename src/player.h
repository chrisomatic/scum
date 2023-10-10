#pragma once

#include "gfx.h"
#include "math2d.h"

#define MAX_PLAYERS 2
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

    int sprite_index;
    Rect hitbox;

    Vector2i curr_room;
    Vector2i curr_tile;

    float proj_cooldown;

    // bool in_door; // used to prevent flip-flopping between doorways

} Player;

extern Player players[MAX_PLAYERS];
extern Player* player;

void player_init();
void player_init_keys();
void player_update(Player* p, float dt);
void player_draw(Player* p);
