
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ROOMS_X 5
#define MAX_ROOMS_Y 5
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

struct
{
    Room rooms[5][5];
} level;


static inline bool flip_coin()
{
    return (rand()%2==0);
}

static void generate_rooms(int x, int y, Door came_from, int depth)
{
    Room* room = &level.rooms[x][y];
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

    if(y > 0 && !level.rooms[x][y-1].valid) // check up
    {
        bool door = flip_coin();
        room->doors[DOOR_UP] = door;
        if(door)
            generate_rooms(x,y-1,DOOR_UP,depth+1);
    }

    if(x < MAX_ROOMS_X && !level.rooms[x+1][y].valid) // check right
    {
        bool door = flip_coin();
        room->doors[DOOR_RIGHT] = door;
        if(door)
            generate_rooms(x+1,y,DOOR_RIGHT,depth+1);
    }

    if(y < MAX_ROOMS_Y && !level.rooms[x][y+1].valid) // check down
    {
        bool door = flip_coin();
        room->doors[DOOR_DOWN] = door;
        if(door)
            generate_rooms(x,y+1,DOOR_DOWN,depth+1);
    }

    if(x > 0 && !level.rooms[x-1][y].valid) // check left
    {
        bool door = flip_coin();
        room->doors[DOOR_LEFT] = door;
        if(door)
            generate_rooms(x-1,y,DOOR_LEFT,depth+1);
    }

    if(depth < MIN_DEPTH)
    {
        if( room->doors[DOOR_UP]    == false &&
            room->doors[DOOR_RIGHT] == false &&
            room->doors[DOOR_DOWN]  == false &&
            room->doors[DOOR_LEFT]  == false )
        {
            if(y > 0 && !level.rooms[x][y-1].valid) // check up
            {
                room->doors[DOOR_UP] = true;
                generate_rooms(x,y-1,DOOR_UP,depth+1);
            }
            else if(x < MAX_ROOMS_X && !level.rooms[x+1][y].valid) // check right
            {
                room->doors[DOOR_RIGHT] = true;
                generate_rooms(x+1,y,DOOR_RIGHT,depth+1);
            }
            else if(y < MAX_ROOMS_Y && !level.rooms[x][y+1].valid) // check down
            {
                room->doors[DOOR_DOWN] = true;
                generate_rooms(x,y+1,DOOR_DOWN,depth+1);
            }
            else if(x > 0 && !level.rooms[x-1][y].valid) // check left
            {
                room->doors[DOOR_LEFT] = true;
                generate_rooms(x-1,y,DOOR_LEFT,depth+1);
            }
        }
    }
}

void generate_level()
{
    int start_x, start_y;

    start_x = rand() % MAX_ROOMS_X;
    start_y = rand() % MAX_ROOMS_Y;

    generate_rooms(start_x, start_y, DOOR_NONE, 0);

    printf("\n");
    for(int j = 0; j < MAX_ROOMS_Y; ++j)
    {
        for(int i = 0; i < MAX_ROOMS_X; ++i)
        {
            Room* room = &level.rooms[i][j];
            printf("%c", room->valid ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}


int main()
{
    srand((unsigned)time(0));
    generate_level();
    return 0;
}
