#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "level.h"



static inline bool flip_coin()
{
    return (rand()%2==0);
}

static void generate_rooms(Level* level, int x, int y, Door came_from, int depth)
{
    Room* room = &level->rooms[x][y];
    room->valid = true;

    switch(came_from)
    {
        case DOOR_UP:    room->doors[DOOR_DOWN]  = true; break;
        case DOOR_RIGHT: room->doors[DOOR_LEFT]  = true; break;
        case DOOR_DOWN:  room->doors[DOOR_UP]    = true; break;
        case DOOR_LEFT:  room->doors[DOOR_RIGHT] = true; break;
        default: break;
    }

    if(depth >= MAX_DEPTH)
        return;

    if(y > 0 && !level->rooms[x][y-1].valid) // check up
    {
        bool door = flip_coin();
        room->doors[DOOR_UP] = door;
        if(door)
            generate_rooms(level, x,y-1,DOOR_UP,depth+1);
    }

    if(x < MAX_ROOMS_X-1 && !level->rooms[x+1][y].valid) // check right
    {
        bool door = flip_coin();
        room->doors[DOOR_RIGHT] = door;
        if(door)
            generate_rooms(level, x+1,y,DOOR_RIGHT,depth+1);
    }

    if(y < MAX_ROOMS_Y-1 && !level->rooms[x][y+1].valid) // check down
    {
        bool door = flip_coin();
        room->doors[DOOR_DOWN] = door;
        if(door)
            generate_rooms(level, x,y+1,DOOR_DOWN,depth+1);
    }

    if(x > 0 && !level->rooms[x-1][y].valid) // check left
    {
        bool door = flip_coin();
        room->doors[DOOR_LEFT] = door;
        if(door)
            generate_rooms(level, x-1,y,DOOR_LEFT,depth+1);
    }

    if(depth < MIN_DEPTH)
    {
        if( room->doors[DOOR_UP]    == false &&
            room->doors[DOOR_RIGHT] == false &&
            room->doors[DOOR_DOWN]  == false &&
            room->doors[DOOR_LEFT]  == false )
        {
            if(y > 0 && !level->rooms[x][y-1].valid) // check up
            {
                room->doors[DOOR_UP] = true;
                generate_rooms(level, x,y-1,DOOR_UP,depth+1);
            }
            else if(x < MAX_ROOMS_X-1 && !level->rooms[x+1][y].valid) // check right
            {
                room->doors[DOOR_RIGHT] = true;
                generate_rooms(level, x+1,y,DOOR_RIGHT,depth+1);
            }
            else if(y < MAX_ROOMS_Y-1 && !level->rooms[x][y+1].valid) // check down
            {
                room->doors[DOOR_DOWN] = true;
                generate_rooms(level, x,y+1,DOOR_DOWN,depth+1);
            }
            else if(x > 0 && !level->rooms[x-1][y].valid) // check left
            {
                room->doors[DOOR_LEFT] = true;
                generate_rooms(level, x-1,y,DOOR_LEFT,depth+1);
            }
        }
    }
}

void level_draw(Level* level)
{
    printf("\n");
    for(int j = 0; j < MAX_ROOMS_Y; ++j)
    {
        for(int i = 0; i < MAX_ROOMS_X; ++i)
        {
            Room* room = &level->rooms[i][j];
            printf("%c", room->valid ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

Level level_generate(unsigned int seed)
{
    int start_x, start_y;

    Level level = {0};

    srand(seed);

    start_x = MAX_ROOMS_X / 2; //rand() % MAX_ROOMS_X;
    start_y = MAX_ROOMS_Y / 2; //rand() % MAX_ROOMS_Y;

    // memset(level->rooms,0, sizeof(level->rooms));
    generate_rooms(&level, start_x, start_y, DOOR_NONE, 0);

    return level;
}
