#pragma once

#define MAX_ROOMS_X 7
#define MAX_ROOMS_Y 7
#define MAX_ROOMS (MAX_ROOMS_X*MAX_ROOMS_Y)

#define MIN_DEPTH 5 
#define MAX_DEPTH 10 

typedef enum
{
    DOOR_UP,
    DOOR_RIGHT,
    DOOR_DOWN,
    DOOR_LEFT,
    DOOR_NONE,
} Door;

typedef struct
{
    bool valid;
    bool doors[4];
} Room;

typedef struct
{
    Room rooms[MAX_ROOMS_X][MAX_ROOMS_Y];
} Level;


Level level_generate(unsigned int seed);
void level_draw(Level* level);
