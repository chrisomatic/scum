#pragma once

#include "physics.h"

#define MAX_ROOMS_GRID_X 7
#define MAX_ROOMS_GRID_Y 7
#define MAX_ROOMS_GRID (MAX_ROOMS_GRID_X*MAX_ROOMS_GRID_Y)

#define MIN_DEPTH 5 
#define MAX_DEPTH 10 

#define MIN_ROOMS 10
#define MAX_ROOMS 15 

#define ROOM_TILE_SIZE_X 13
#define ROOM_TILE_SIZE_Y 7

#define TILE_SIZE 32

typedef enum
{
    TILE_NONE = 0,
    TILE_FLOOR,
    TILE_PIT,
    TILE_BOULDER,
} TileType;

typedef enum
{
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_NONE,
} Dir;

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
    SPRITE_TILE_PIT,
    SPRITE_TILE_MAX,
} SpriteTileType;

typedef struct
{
    Dir dir;
    Vector2f p0;
    Vector2f p1;
    float distance_to_player; // used for sorting walls
} Wall;

typedef struct
{
    bool valid;
    bool doors[4];
    uint32_t color;
    Wall walls[100];
    int wall_count;
    int layout;
    uint8_t index;
    bool discovered;
} Room;

typedef struct
{
    Room rooms[MAX_ROOMS_GRID_X][MAX_ROOMS_GRID_Y];
    Vector2i start;
} Level;

typedef struct
{
    TileType tiles[ROOM_TILE_SIZE_X][ROOM_TILE_SIZE_Y];
} RoomData;

extern RoomData room_list[32];
extern int room_list_count;

void level_init();
Level level_generate(unsigned int seed);
void level_draw_room(Room* room, float xoffset, float yoffset);
void level_print(Level* level);
void level_print_room(Room* room);
void level_sort_walls(Wall* walls, int wall_count, float x, float y, float radius);
void level_handle_room_collision(Room* room, Physics* phys);

Room* level_get_room(Level* level, int x, int y);
uint8_t level_get_room_index(int x, int y);
Vector2i level_get_room_coords(int index);
bool level_is_room_valid(Level* level, int x, int y);
bool level_is_room_valid_index(Level* level, int index);
bool level_is_room_discovered(Level* level, int x, int y);
bool level_is_room_discovered_index(Level* level, int index);

Vector2i get_door_offsets(Dir door);
const char* get_door_name(Dir door);
