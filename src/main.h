#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "log.h"
#include "timer.h"
#include "math2d.h"
#include "level.h"
#include "core/text_list.h"

#include "glist.h"

#if 1
#define VIEW_WIDTH   1200
#define VIEW_HEIGHT  800
#else
#define VIEW_WIDTH   600
#define VIEW_HEIGHT  400
#endif


// strings
#define STR_EMPTY(x)      (x == 0 || strlen(x) == 0)
#define STR_EQUAL(x,y)    (strncmp((x),(y),strlen((x))) == 0 && strlen(x) == strlen(y))
#define STRN_EQUAL(x,y,n) (strncmp((x),(y),(n)) == 0)

#define FREE(p) do{ if(p != NULL) {free(p); p = NULL;} }while(0)

#define BOOLSTR(c) ((c) ? "true" : "false")

#if 1
#define DEBUG()   printf("[DEBUG] %s %s(): %d\n", __FILE__, __func__, __LINE__)
#else
#define DEBUG()
#endif

typedef enum
{
    ROLE_UNKNOWN,
    ROLE_LOCAL,
    ROLE_CLIENT,
    ROLE_SERVER,
} GameRole;

typedef enum
{
    GAME_STATE_EDITOR,
    GAME_STATE_MENU,
    GAME_STATE_SETTINGS,
    GAME_STATE_PLAYING,
} GameState;

typedef struct
{
    bool state;
    bool prior_state;
    bool toggled_on;
    bool toggled_off;

    bool hold;
    float hold_counter;
    float hold_period;
} PlayerInput;

typedef struct
{
    Rect area;
    bool show_all;

    uint32_t color_bg;
    float opacity_bg;

    uint32_t color_room;
    float opacity_room;

    //undiscovered room
    uint32_t color_uroom;
    float opacity_uroom;

    uint32_t color_player;
    float opacity_player;

    uint32_t color_border;
    float opacity_border;
} DrawLevelParams;

extern uint16_t music_id;

extern float mouse_x, mouse_y;
extern float mouse_world_x, mouse_world_y;

extern Rect room_area;
extern Rect floor_area;
extern Rect player_area;
extern Rect camera_limit;
extern Rect margin_left;
extern Rect margin_right;
extern Rect margin_top;
extern Rect margin_bottom;

extern Level level;
extern unsigned int level_seed;
extern int level_rank;
extern int level_transition;
extern int level_transition_state;
extern bool level_generate_triggered;

extern Vector2f transition_offsets;
extern Vector2f transition_targets;

extern GameRole role;
extern Timer game_timer;
extern text_list_t* text_lst;
extern bool initialized;
extern bool debug_enabled;
extern bool editor_enabled;
extern bool client_chat_enabled;
extern bool paused;
extern bool show_big_map;
extern bool show_walls;
extern bool show_tile_grid;
extern bool creatures_can_move;
extern bool players_invincible;
extern bool all_players_dead;
extern uint32_t background_color;
extern float ascale;
extern double g_timer;
extern bool g_spikes;

extern bool dynamic_zoom;
extern int cam_zoom;
extern int cam_min_zoom;

extern char* tile_names[];
extern char* creature_names[];
extern char* item_names[];

extern Rect moving_tile;
extern Rect moving_tile_prior;


void camera_set(bool immediate);
bool camera_can_be_limited(float x, float y, float z);

void trigger_generate_level(unsigned int _seed, int _rank, int transition, int line);
void game_generate_level();
void handle_room_completion(Room* room);
void set_game_state(GameState state);
void update_input_state(PlayerInput* input, float _dt);


char* string_split_index(char* str, const char* delim, int index, int* ret_len, bool split_past_index);
char* string_split_index_copy(char* str, const char* delim, int index, bool split_past_index);
