#pragma once

#include "math2d.h"

#define MAX_PLAYERS 2
enum PlayerActions
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
    PLAYER_ACTION_RUN,
    PLAYER_ACTION_SCUM,
    PLAYER_ACTION_TEST,
    PLAYER_ACTION_GENERATE_ROOMS,

    PLAYER_ACTION_MAX
};

typedef struct
{
    bool state;
    bool prior_state;
    bool toggled_on;
    bool toggled_off;
} PlayerAction;

typedef struct
{
    Vector2f pos;
    PlayerAction actions[PLAYER_ACTION_MAX];

    int sprite_index;
    Rect hitbox;

    int curr_room_x;
    int curr_room_y;

} Player;

extern Player players[MAX_PLAYERS];
extern Player* player;

void player_init();
void player_update();
void player_draw(Player* p);
