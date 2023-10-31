#pragma once

#include "physics.h"

#define MAX_ROOMS_GRID_X 7
#define MAX_ROOMS_GRID_Y 7
#define MAX_ROOMS_GRID (MAX_ROOMS_GRID_X*MAX_ROOMS_GRID_Y)

#define MONSTER_ROOM_PERCENTAGE 80

#define MIN_DEPTH 2
#define MAX_DEPTH 8

#define MIN_ROOMS 7
#define MAX_ROOMS 12

#define ROOM_TILE_SIZE_X 13
#define ROOM_TILE_SIZE_Y 7

#define TILE_SIZE 32
#define MAX_DOORS 4
#define MAX_WALLS_PER_ROOM 100

#define ROOM_W  TILE_SIZE*(ROOM_TILE_SIZE_X+2)
#define ROOM_H  TILE_SIZE*(ROOM_TILE_SIZE_Y+2)

typedef enum
{
    TILE_NONE = 0,
    TILE_FLOOR,
    TILE_PIT,
    TILE_BOULDER,
} TileType;

typedef enum
{
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_UP_RIGHT,
    DIR_DOWN_LEFT,
    DIR_DOWN_RIGHT,
    DIR_UP_LEFT,
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
    bool is_pit;
} Wall;

typedef enum
{
    ROOM_TYPE_EMPTY,
    ROOM_TYPE_MONSTER,
    ROOM_TYPE_TREASURE,
    ROOM_TYPE_BOSS,
    ROOM_TYPE_MAX,
} RoomType;

typedef struct
{
    bool valid;
    RoomType type;
    bool doors[MAX_DOORS];
    uint32_t color;
    Wall walls[MAX_WALLS_PER_ROOM];
    int wall_count;
    int layout;
    uint8_t index;
    bool discovered;
} Room;

typedef struct
{
    Room rooms[MAX_ROOMS_GRID_X][MAX_ROOMS_GRID_Y];
    int num_rooms;
    bool has_boss_room;
    bool has_treasure_room;
    Vector2i start;
} Level;

typedef struct
{
    TileType tiles[ROOM_TILE_SIZE_X][ROOM_TILE_SIZE_Y];
} RoomData;

extern RoomData room_list[32];
extern int room_list_count;
extern int dungeon_image;

void level_init();
Level level_generate(unsigned int seed);
uint8_t level_get_tile_sprite(TileType tt);
Rect level_get_tile_rect(int x, int y);
TileType level_get_tile_type(Room* room, int x, int y);
TileType level_get_tile_type_by_pos(Room* room, float x, float y);
void level_draw_room(Room* room, RoomData* room_data, float xoffset, float yoffset);
void room_draw_walls(Room* room);
void level_print(Level* level);
void level_print_room(Room* room);
void level_sort_walls(Wall* walls, int wall_count, float x, float y, float radius);
void level_handle_room_collision(Room* room, Physics* phys);
Room* level_get_room(Level* level, int x, int y);
Room* level_get_room_by_index(Level* level, int index);
uint8_t level_get_room_index(int x, int y);
Vector2i level_get_room_coords(int index);
Vector2i level_get_room_coords_by_pos(float x, float y);
bool level_is_room_valid(Level* level, int x, int y);
bool level_is_room_valid_index(Level* level, int index);
bool level_is_room_discovered(Level* level, int x, int y);
bool level_is_room_discovered_index(Level* level, int index);

char* level_tile_type_to_str(TileType tt);

Vector2i get_dir_offsets(Dir door);
const char* get_dir_name(Dir door);

void level_generate_room_outer_walls(Room* room);
