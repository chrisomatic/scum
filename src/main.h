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



// #define VIEW_WIDTH   1200
// #define VIEW_HEIGHT  800
// #define ROOM_W  320
// #define ROOM_H  320


#define VIEW_WIDTH   600
#define VIEW_HEIGHT  400
#define ROOM_W  480
#define ROOM_H  288

// #define VIEW_WIDTH   600
// #define VIEW_HEIGHT  400
// #define ROOM_W  800
// #define ROOM_H  800


// strings
#define STR_EMPTY(x)      (x == 0 || strlen(x) == 0)
#define STR_EQUAL(x,y)    (strncmp((x),(y),strlen((x))) == 0 && strlen(x) == strlen(y))
#define STRN_EQUAL(x,y,n) (strncmp((x),(y),(n)) == 0)

#define FREE(p) do{ if(p != NULL) {free(p); p = NULL;} }while(0)

#define DEBUG()   printf("%d %s %s()\n", __LINE__, __FILE__, __func__)

typedef enum
{
    GAME_STATE_MENU,
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

void set_game_state(GameState state);
void update_input_state(PlayerInput* input, float _dt);

extern Rect room_area;
extern Level level;

extern bool initialized;
extern bool debug_enabled;
extern Timer game_timer;
extern text_list_t* text_lst;
extern uint32_t background_color;
extern unsigned int seed;
