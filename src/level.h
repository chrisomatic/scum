#pragma once

#include "physics.h"
#include "room_file.h"
#include "astar.h"

#define MAX_ROOMS_GRID_X 7
#define MAX_ROOMS_GRID_Y 7
// #define MAX_ROOMS_GRID_X 3
// #define MAX_ROOMS_GRID_Y 3
#define MAX_ROOMS_GRID (MAX_ROOMS_GRID_X*MAX_ROOMS_GRID_Y)

#define MONSTER_ROOM_PERCENTAGE 80

#define MIN_DEPTH 3
#define MAX_DEPTH 8

#define MIN_ROOMS 7
#define MAX_ROOMS 12

#define ROOM_TILE_SIZE_X 13
#define ROOM_TILE_SIZE_Y 7

#define TILE_SIZE 32
#define MAX_DOORS 4
#define MAX_WALLS_PER_ROOM 200

#define MAX_ROOM_LIST_COUNT 200

#define ROOM_GRACE_TIME 1.0

// +2 for walls
#define ROOM_W  TILE_SIZE*(ROOM_TILE_SIZE_X+2)
#define ROOM_H  TILE_SIZE*(ROOM_TILE_SIZE_Y+2)

typedef enum
{
    TILE_NONE = 0,
    TILE_FLOOR,
    TILE_PIT,
    TILE_BOULDER,
    TILE_MUD,
    TILE_ICE,
    TILE_SPIKES,
    TILE_TIMED_SPIKES1,
    TILE_TIMED_SPIKES2,
    TILE_BREAKABLE_FLOOR,
    TILE_MAX
} TileType;

#define IS_SAFE_TILE(tt) (tt == TILE_FLOOR || tt == TILE_MUD || tt == TILE_ICE)


typedef enum
{
    DIR_RIGHT,
    DIR_UP,
    DIR_LEFT,
    DIR_DOWN,
    DIR_UP_RIGHT,
    DIR_UP_LEFT,
    DIR_DOWN_LEFT,
    DIR_DOWN_RIGHT,
    DIR_NONE,
} Dir;

typedef enum
{
    FLOOR_CRACKED = 0,
    FLOOR_BREAKING = 1,
    FLOOR_REALLY_BREAKING = 2,
    FLOOR_BROKE = 3,
} FloorState;

typedef enum
{
    SPRITE_TILE_FLOOR = 0,
    SPRITE_TILE_FLOOR_ALT1,
    SPRITE_TILE_FLOOR_ALT2,
    SPRITE_TILE_FLOOR_ALT3,
    SPRITE_TILE_FLOOR_ALT4,
    SPRITE_TILE_FLOOR_ALT5,
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
    SPRITE_TILE_MUD,
    SPRITE_TILE_ICE,
    SPRITE_TILE_SPIKES,
    SPRITE_TILE_SPIKES_DOWN,
    SPRITE_TILE_DOOR_UP_CLOSED,
    SPRITE_TILE_DOOR_RIGHT_CLOSED,
    SPRITE_TILE_DOOR_DOWN_CLOSED,
    SPRITE_TILE_DOOR_LEFT_CLOSED,
    SPRITE_TILE_FLOOR_BREAK1,
    SPRITE_TILE_FLOOR_BREAK2,
    SPRITE_TILE_FLOOR_BREAK3,
    SPRITE_TILE_MAX,
} SpriteTileType;

typedef enum
{
    WALL_TYPE_BLOCK,
    WALL_TYPE_PIT,
    WALL_TYPE_INNER,
    WALL_TYPE_OUTER,
} WallType;

typedef struct
{
    Dir dir;
    Vector2f p0;
    Vector2f p1;
    float distance_to_player; // used for sorting walls
    WallType type;
} Wall;

typedef enum
{
    ROOM_TYPE_EMPTY,
    ROOM_TYPE_MONSTER,
    ROOM_TYPE_TREASURE,
    ROOM_TYPE_BOSS,
    ROOM_TYPE_SHRINE,
    ROOM_TYPE_SHOP,
    ROOM_TYPE_MAX
} RoomType;

typedef struct
{
    bool valid;
    RoomType type;
    uint32_t rand_seed; // calculated at start, used for generating variation in room
    bool doors[MAX_DOORS];
    bool doors_locked;
    uint32_t color;

    uint8_t breakable_floor_state[ROOM_TILE_SIZE_X][ROOM_TILE_SIZE_Y];

    Wall walls[MAX_WALLS_PER_ROOM];
    int wall_count;
    int layout; // rfd room_list index

    bool discovered;

    uint8_t index;
    Vector2i grid;
} Room;

typedef struct
{
    Room rooms[MAX_ROOMS_GRID_X][MAX_ROOMS_GRID_Y];
    Room* rooms_ptr[MAX_ROOMS_GRID];
    int num_rooms;
    bool has_boss_room;
    bool has_treasure_room;
    bool has_shrine_room;
    bool darkness_curse;
    Vector2i start;
    AStar_t asd;
} Level;

typedef struct
{
    Room* rooms[MAX_ROOMS_GRID];
    Dir directions[MAX_ROOMS_GRID];
    int length;
} LevelPath;

extern int dungeon_image;
extern int dungeon_set_image1;
extern int dungeon_set_image2;
extern int dungeon_set_image3;

extern float level_grace_time;
extern float level_room_time;   // time spent in room (until completed)
extern int level_room_xp;
extern bool level_room_in_progress;

void level_init();
void level_update(float dt);
void level_load_rooms();

Level level_generate(unsigned int seed, int rank);
void level_place_entities(Level* level);
void level_set_room_pointers(Level* level);

void level_astar_to_path(Level* level, AStar_t* asd, LevelPath* path);
void level_print_path(LevelPath* path);

uint8_t level_get_tile_sprite(TileType tt);
Rect level_get_tile_rect(int x, int y);
Rect level_get_rect_by_pos(float x, float y);
TileType level_get_tile_type(Room* room, int x, int y);
TileType level_get_tile_type_by_pos(Room* room, float x, float y);
void level_get_rand_floor_tile(Room* room, Vector2i* tile_coords, Vector2f* tile_pos);
void level_get_safe_floor_tile(Room* room, Vector2i start, Vector2i* tile_coords, Vector2f* tile_pos);

// Note: DIR_NONE is center tile
Vector2i level_get_door_tile_coords(Dir dir);

void level_draw_room(Room* room, RoomFileData* room_data, float xoffset, float yoffset, float scale, bool show_entities);
void room_draw_walls(Room* room);
void level_draw_wall_column(float x, float y);
void level_print(Level* level);
void level_sort_walls(Wall* walls, int wall_count, Physics* phys);
void level_handle_room_collision(Room* room, Physics* phys, int entity_type, void* entity);
Room* level_get_room(Level* level, int x, int y);
Room* level_get_room_by_index(Level* level, int index);
int level_get_room_index(int x, int y);
Vector2i level_get_room_coords(int index);
Vector2i level_get_room_coords_by_pos(float x, float y);
Vector2f level_get_pos_by_room_coords(int x, int y);
Vector2f level_get_pos_by_room_coords_scaled(int x, int y, float scale);
bool level_is_room_valid(Level* level, int x, int y);
bool level_is_room_valid_index(Level* level, int index);
bool level_is_room_discovered(Level* level, int x, int y);
bool level_is_room_discovered_index(Level* level, int index);

int level_tile_traversable_func(int x, int y);
int level_every_tile_traversable_func(int x, int y);

float dir_to_angle_deg(Dir dir);
Dir angle_to_dir_cardinal(float angle_deg);
Dir get_opposite_dir(Dir dir);
Vector2i get_dir_offsets(Dir door);
Dir get_dir_from_coords(int x0, int y0, int x1, int y1);
Dir get_dir_from_coords2(int x0, int y0, int x1, int y1);
void level_generate_room_outer_walls(Room* room);
void generate_walls(Level* level);

const char* get_dir_name(Dir dir);
const char* get_tile_name(TileType tt);
const char* get_room_type_name(RoomType rt);
