#pragma once

#define MAX_ROOMS_GRID_X 7
#define MAX_ROOMS_GRID_Y 7
#define MAX_ROOMS_GRID (MAX_ROOMS_GRID_X*MAX_ROOMS_GRID_Y)

#define MIN_DEPTH 5 
#define MAX_DEPTH 10 

#define MIN_ROOMS 10
#define MAX_ROOMS 15 

#define ROOM_TILE_SIZE_X 13
#define ROOM_TILE_SIZE_Y 7

typedef enum
{
    TILE_NONE = 0,
    TILE_FLOOR,
    TILE_BOULDER,
} TileType;

typedef enum
{
    DOOR_UP,
    DOOR_RIGHT,
    DOOR_DOWN,
    DOOR_LEFT,
    DOOR_NONE,
} Door;

typedef enum
{
    SPRITE_TILE_FLOOR = 0,
    SPRITE_TILE_BLOCK,
    SPRITE_TILE_WALL_UP,
    SPRITE_TILE_WALL_RIGHT,
    SPRITE_TILE_WALL_DOWN,
    SPRITE_TILE_WALL_LEFT,
    SPRITE_TILE_WALL_CORNER_LU,
    SPRITE_TILE_WALL_CORNER_UR,
    SPRITE_TILE_WALL_CORNER_RD,
    SPRITE_TILE_WALL_CORNER_DL,
    SPRITE_TILE_DOOR_UP,
    SPRITE_TILE_DOOR_RIGHT,
    SPRITE_TILE_DOOR_DOWN,
    SPRITE_TILE_DOOR_LEFT,
    SPRITE_TILE_MAX,
} SpriteTileType;

typedef struct
{
    bool valid;
    bool doors[4];
    int layout;
} Room;

typedef struct
{
    Room rooms[MAX_ROOMS_GRID_X][MAX_ROOMS_GRID_Y];
} Level;

typedef struct
{
    TileType tiles[ROOM_TILE_SIZE_X][ROOM_TILE_SIZE_Y];
} RoomData;

extern RoomData room_list[32];
extern int room_list_count;

void level_init();
Level level_generate(unsigned int seed);
void level_draw_room(Room* room);
void level_print(Level* level);
void level_print_room(Room* room);
