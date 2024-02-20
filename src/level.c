#include "headers.h"
#include "core/files.h"
#include "core/gfx.h"
#include "core/window.h"
#include "main.h"
#include "creature.h"
#include "item.h"
#include "room_file.h"
#include "entity.h"
#include "physics.h"
#include "player.h"

#define NEW_LEVEL_GENERATION 1

int dungeon_image = -1;
int dungeon_set_image1 = -1;
int dungeon_set_image2 = -1;
int dungeon_set_image3 = -1;
float level_grace_time = 0.0;

static void generate_rooms(Level* level, int x, int y, Dir came_from, int depth);

static int room_list_monster[MAX_ROOM_LIST_COUNT] = {0};
static int room_list_empty[MAX_ROOM_LIST_COUNT] = {0};
static int room_list_treasure[MAX_ROOM_LIST_COUNT] = {0};
static int room_list_boss[MAX_ROOM_LIST_COUNT] = {0};

static int room_count_monster  = 0;
static int room_count_empty    = 0;
static int room_count_treasure = 0;
static int room_count_boss     = 0;

static int used_room_list_monster[MAX_ROOM_LIST_COUNT] = {0};
static int used_room_count_monster = 0;

static bool min_depth_reached = false;

#if NEW_LEVEL_GENERATION

static void set_doors_from_path(Level* level, LevelPath* path);
static Room* place_room(Level* level, RoomType type, int min_dist, int max_dist);
static int get_room_count(Level* level);
static void print_room(Level* level, Room* room);
static int rand_from_probs(int weights[], int num);
static void print_path(LevelPath* path);
static bool generate_room_path(Level* level, Room* start, Room* end, LevelPath* path, int tries);
static bool find_direct_room_path(Level* level, Room* start, Room* end, LevelPath* path);
static int get_viable_rooms(RoomType type, bool doors[4], int* ret_list);
static int level_traversable_func(int x, int y);

#endif


// rand
// ------------------------------------------------------------------------
static unsigned long int lrand_next = 1;

static int lrand()
{
    lrand_next = lrand_next * 1103515245 + 12345;
    return (unsigned int)(lrand_next / 65536) % 32768;
}

static void slrand(unsigned int seed)
{
    lrand_next = seed;
}

static inline bool flip_coin()
{
    return (lrand()%2==0);
}
// ------------------------------------------------------------------------

int get_room_dist(int x0, int y0, int x1, int y1)
{
    return ABS(x0-x1) + ABS(y0-y1);
}

#if NEW_LEVEL_GENERATION

#define GENERATE_ROOMS_TEST 0
#define TEST_COUNT          10000
#define START_SEED          0

Level level_generate(unsigned int seed, int rank)
{
    Level level = {0};

#if GENERATE_ROOMS_TEST
    seed = START_SEED-1;
    for(int t = 0; t < TEST_COUNT; ++t)
    {
        memset(&level, 0, sizeof(Level));
        seed += 1;
#endif

    // seed PRNG
    slrand(seed);

    if(rank != 5)
    {
#if !GENERATE_ROOMS_TEST
        LOGW("Overriding rank!");
#endif
        rank = 5;
        level_rank = rank;
    }

#if !GENERATE_ROOMS_TEST
    LOGI("Generating level, seed: %u, rank: %d", seed, rank);
#endif

    used_room_count_monster = 0;

    // fill out helpful room information
    room_count_monster = 0;
    room_count_empty = 0;
    room_count_treasure = 0;
    room_count_boss = 0;

    for(int i = 0; i < room_list_count; ++i)
    {
        RoomFileData* rfd = &room_list[i];
        if(rfd->rank > rank)
            continue;
        switch(rfd->type)
        {
            case ROOM_TYPE_MONSTER:  room_list_monster[room_count_monster++]   = i; break;
            case ROOM_TYPE_EMPTY:    room_list_empty[room_count_empty++]       = i; break;
            case ROOM_TYPE_TREASURE: room_list_treasure[room_count_treasure++] = i; break;
            case ROOM_TYPE_BOSS:     room_list_boss[room_count_boss++]         = i; break;
            default: break;
        }
    }
#if !GENERATE_ROOMS_TEST
    printf("Total Rooms: %d\n", room_list_count);
    printf("   # monster rooms: %d\n",room_count_monster);
    printf("   # empty rooms: %d\n",room_count_empty);
    printf("   # treasure rooms: %d\n",room_count_treasure);
    printf("   # boss rooms: %d\n",room_count_boss);
#endif

    item_clear_all();
    creature_clear_all();

    for(int _x = 0; _x < MAX_ROOMS_GRID_X; ++_x)
    {
        for(int _y = 0; _y < MAX_ROOMS_GRID_Y; ++_y)
        {
            uint8_t idx = level_get_room_index(_x, _y);
            level.rooms_ptr[idx] = &level.rooms[_x][_y];
            level.rooms[_x][_y].index = idx;
            level.rooms[_x][_y].grid.x = _x;
            level.rooms[_x][_y].grid.y = _y;
            level.rooms[_x][_y].xp = 0;
            level.rooms[_x][_y].doors_locked = false;
        }
    }

#if 0
    {
        level.start.x = 0;
        level.start.y = 0;
        Room* sroom = &level.rooms[level.start.x][level.start.y];
        sroom->valid = true;
        sroom->type = ROOM_TYPE_EMPTY;

        Room* troom = &level.rooms[MAX_ROOMS_GRID_X-1][MAX_ROOMS_GRID_Y-1];
        troom->valid = true;
        troom->type = ROOM_TYPE_EMPTY;

        level.rooms[MAX_ROOMS_GRID_X-1][MAX_ROOMS_GRID_Y-2].type = ROOM_TYPE_BOSS;
        level.rooms[MAX_ROOMS_GRID_X-1][MAX_ROOMS_GRID_Y-2].valid = true;

        // level.rooms[MAX_ROOMS_GRID_X-2][MAX_ROOMS_GRID_Y-1].type = ROOM_TYPE_BOSS;

        find_direct_room_path(&level, sroom, troom, NULL);
        printf("finished\n");

        // memset(&level, 0, sizeof(Level));
        // for(int _x = 0; _x < MAX_ROOMS_GRID_X; ++_x)
        // {
        //     for(int _y = 0; _y < MAX_ROOMS_GRID_Y; ++_y)
        //     {
        //         uint8_t idx = level_get_room_index(_x, _y);
        //         level.rooms_ptr[idx] = &level.rooms[_x][_y];
        //         level.rooms[_x][_y].index = idx;
        //         level.rooms[_x][_y].grid.x = _x;
        //         level.rooms[_x][_y].grid.y = _y;
        //         level.rooms[_x][_y].xp = 0;
        //         level.rooms[_x][_y].doors_locked = false;
        //     }
        // }
    }
#endif

    level.start.x = (lrand()%(MAX_ROOMS_GRID_X-2))+1;
    level.start.y = (lrand()%(MAX_ROOMS_GRID_Y-2))+1;
    Room* sroom = &level.rooms[level.start.x][level.start.y];
    sroom->valid = true;
    sroom->type = ROOM_TYPE_EMPTY;
    sroom->layout = 0;
    if(role != ROLE_CLIENT)
    {
        //TEMP
        item_add(ITEM_NEW_LEVEL, CENTER_X, CENTER_Y, sroom->index);
    }

    Room* broom = place_room(&level, ROOM_TYPE_BOSS, 4,6);
    Room* troom = place_room(&level, ROOM_TYPE_TREASURE, 4,6);

    LevelPath bpath = {0};
    LevelPath tpath = {0};
    LevelPath epath = {0};
    LevelPath epath2 = {0};

    bool ret = true;
    bool tret = true;

    // boss room path
    for(;;)
    {
        ret = generate_room_path(&level, sroom, broom, &bpath, 5);
        if(ret)
            break;
        else
        {
            broom->valid = false;
            broom->type = ROOM_TYPE_EMPTY;
            broom = place_room(&level, ROOM_TYPE_BOSS, 4,6);
        }
    }


    // treasure room path
    for(;;)
    {
        ret = generate_room_path(&level, sroom, troom, &tpath, 5);
        if(ret)
            break;
        else
        {
            troom->valid = false;
            troom->type = ROOM_TYPE_EMPTY;
            troom = place_room(&level, ROOM_TYPE_TREASURE, 4,6);
        }
    }

    // extra room path
    Room* eroom = place_room(&level, ROOM_TYPE_EMPTY, 3,5);
    for(int i = 0; i < 5; ++i)
    {
        ret = generate_room_path(&level, sroom, eroom, &epath, 5);
        if(ret)
            break;
        else
        {
            eroom->valid = false;
            eroom->type = ROOM_TYPE_EMPTY;
            eroom = place_room(&level, ROOM_TYPE_EMPTY, 3,5);
        }
    }

    // extra room path
    Room* eroom2 = place_room(&level, ROOM_TYPE_EMPTY, 3,5);
    for(int i = 0; i < 5; ++i)
    {
        ret = generate_room_path(&level, sroom, eroom2, &epath2, 5);
        if(ret)
            break;
        else
        {
            eroom2->valid = false;
            eroom2->type = ROOM_TYPE_EMPTY;
            eroom2 = place_room(&level, ROOM_TYPE_EMPTY, 3,5);
        }
    }

    // ret = generate_room_path(&level, sroom, troom, &bpath, 5);
    // tret &= ret;
    // if(!ret) LOGE("boss");

    // ret = generate_room_path(&level, sroom, troom, &tpath, 5);
    // tret &= ret;
    // if(!ret) LOGE("treasure");

    // //TODO: need to add in check for place_room
    // Room* eroom = place_room(&level, ROOM_TYPE_EMPTY, 2,6);
    // ret = generate_room_path(&level, sroom, eroom, &epath, 5);
    // tret &= ret;
    // if(!ret) LOGE("extra");

    set_doors_from_path(&level, &bpath);
    set_doors_from_path(&level, &tpath);
    set_doors_from_path(&level, &epath);
    set_doors_from_path(&level, &epath2);

//     if(!tret)
//     {
//         LOGE("Level Generation Error (seed: %u, rank: %d)", seed, rank);
// #if GENERATE_ROOMS_TEST
//         continue;
// #endif
//     }

#if !GENERATE_ROOMS_TEST
    print_room(&level, broom);
    // print_path(&bpath);
    print_room(&level, troom);
    // print_path(&tpath);
    print_room(&level, eroom);
    // print_path(&epath);
    print_room(&level, eroom2);
    // print_path(&epath2);
#endif


    // add some doors between surrounding rooms
    for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
    {
        for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
        {
            Room* room = &level.rooms[x][y];
            // printf("%2u) %2d, %-2d\n", level.rooms[x][y].index, x, y);
            if(!room->valid) continue;
            bool start = x == level.start.x && y == level.start.y;
            bool boss = room->type == ROOM_TYPE_BOSS;
            bool treasure = room->type == ROOM_TYPE_TREASURE;
            if(start || boss || treasure) continue;

            // check surrounding rooms
            for(int d = 0; d < 4; ++d)
            {
                if(room->doors[d]) continue;
                Vector2i o = get_dir_offsets(d);
                int _x = x+o.x;
                int _y = y+o.y;

                if(!level_is_room_valid(&level, _x, _y)) continue;
                Room* aroom = &level.rooms[_x][_y];
                if(lrand()%100 <= 10)
                {
                    room->doors[d] = true;
                    aroom->doors[get_opposite_dir(d)] = true;
                }
            }
        }
    }


    int rfd_list[100] = {0};

    for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
    {
        for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
        {
            Room* room = &level.rooms[x][y];
            // printf("%2u) %2d, %-2d\n", level.rooms[x][y].index, x, y);
            if(!room->valid) continue;

            bool start = x == level.start.x && y == level.start.y;
            bool boss = room->type == ROOM_TYPE_BOSS;
            bool treasure = room->type == ROOM_TYPE_TREASURE;

            if(!start)
            {
                int rfd_count = 0;

                if(boss)
                {
                    rfd_count = get_viable_rooms(room->type, room->doors, rfd_list);
                    level.has_boss_room = rfd_count > 0;
                }
                else if(treasure)
                {
                    rfd_count = get_viable_rooms(room->type, room->doors, rfd_list);
                    level.has_treasure_room = rfd_count > 0;
                }
                else
                {

                    room->type = ROOM_TYPE_EMPTY;
                    if(lrand() % 100 < MONSTER_ROOM_PERCENTAGE)
                        room->type = ROOM_TYPE_MONSTER;
                    rfd_count = get_viable_rooms(room->type, room->doors, rfd_list);
                }

                if(rfd_count == 0)
                {
                    // there should always be an empty room available
                    room->type = ROOM_TYPE_EMPTY;
                    rfd_count = get_viable_rooms(ROOM_TYPE_EMPTY, room->doors, rfd_list);
                }
                room->layout = rfd_list[lrand()%rfd_count];
            }

            if(room->type == ROOM_TYPE_BOSS)
                room->color = COLOR(100,100,200);
            else if(room->type == ROOM_TYPE_TREASURE)
                room->color = COLOR(200,200,100);
            else if(room->type == ROOM_TYPE_MONSTER)
                room->color = COLOR(200,100,100);
            else
                room->color = COLOR(200,200,200);

            if(room->type == ROOM_TYPE_MONSTER)
            {
                used_room_list_monster[used_room_count_monster++] = room->layout;
            }


            if(role != ROLE_CLIENT)
            {

                RoomFileData* rfd = &room_list[room->layout];
                for(int i = 0; i < rfd->item_count; ++i)
                {
                    Vector2f pos = level_get_pos_by_room_coords(rfd->item_locations_x[i]-1, rfd->item_locations_y[i]-1); // @convert room objects to tile grid coordinates

                    // printf("Adding item of type: %s to (%f %f)\n", item_get_name(rfd->item_types[i]), pos.x, pos.y);
                    item_add(rfd->item_types[i], pos.x, pos.y, room->index);
                }

                if(room->type == ROOM_TYPE_MONSTER || room->type == ROOM_TYPE_BOSS)
                {
                    for(int i = 0; i < rfd->creature_count; ++i)
                    {
                        Vector2i g = {rfd->creature_locations_x[i], rfd->creature_locations_y[i]};
                        g.x--; g.y--; // @convert room objects to tile grid coordinates
                        Creature* c = creature_add(room, rfd->creature_types[i], &g, NULL);
                    }
                }

            }

            room->doors_locked = (creature_get_room_count(room->index) != 0);
        }
    }

    if(!level.has_treasure_room || !level.has_boss_room)
    {
        LOGE("Level Generation Error (seed: %u, rank: %d)", seed, rank);
        if(!level.has_treasure_room) LOGE("No treasure room");
        if(!level.has_boss_room) LOGE("No boss room");
    }

#if GENERATE_ROOMS_TEST
    }
#endif

    generate_walls(&level);

    int room_count = get_room_count(&level);
    printf("Total Room Count: %d\n", room_count);
    level_print(&level);
    level_grace_time = 2.0;

    astar_create(&level.asd, ROOM_TILE_SIZE_X, ROOM_TILE_SIZE_Y);
    astar_set_traversable_func(&level.asd, level_traversable_func);

    return level;
}

static void set_doors_from_path(Level* level, LevelPath* path)
{
    for(int i = 1; i < path->length; ++i)
    {
        Dir dir = path->directions[i-1];
        path->directions[i-1] = dir;
        path->rooms[i-1]->doors[dir] = true; //set door on prior room
        path->rooms[i]->doors[get_opposite_dir(dir)] = true; // set door on new room
    }
}

#if 0

#if 1
#define dprint_path(_path_index_, curr_path_index, fmt, ...) if(curr_path_index == _path_index_ || _path_index_ < 0) printf(fmt, __VA_ARGS__)
#else
#define dprint_path(_path_index_, curr_path_index, fmt, ...)
#endif


#if 1
#define dprint_start_end(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define dprint_start_end(fmt, ...)
#endif

#if 0
#define dprint_new_path(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define dprint_new_path(fmt, ...)
#endif


#define MAX_PATHS   300000

bool next_room(Level* level, Room* end, uint8_t* ipath, int* len, bool* valid, int path_index, int* curr_min)
{
    static int s = 0;
    s++;
    // printf("s: %d\n", s);
    // if(s > 100) return false;

    const int _path_index = 0;

    int index = path_index * MAX_ROOMS_GRID;
    Vector2i start = level_get_room_coords(ipath[index+0]);
    Vector2i curr = level_get_room_coords(ipath[index+len[path_index]-1]);
    dprint_path(_path_index, path_index, "[%d : %d] curr: %d, %d (%u)\n", path_index, len[path_index], curr.x, curr.y, ipath[index+len[path_index]-1]);

    if(len[path_index]+1 >= *curr_min)
    {
        dprint_path(_path_index, path_index, "[%d : %d] exceeded min len: %d", path_index, len[path_index], *curr_min);
        dprint_start_end("ended:%d:%d\n", path_index, len[path_index]);
        len[path_index] = 1;
        return false;
    }

    // if(path_index == _path_index)
    //     getchar();

    int valid_dirs[4] = {0};
    int valid_count = 0;

    for(int dir = 0; dir < 4; ++dir)
    {
        Vector2i o = get_dir_offsets(dir);
        int _x = curr.x+o.x;
        int _y = curr.y+o.y;
        int _idx = level_get_room_index(_x, _y);

        if(_x == start.x && _y == start.y) continue;
        if(_x < 0 || _y < 0) continue;
        if(_x >= MAX_ROOMS_GRID_X || _y >= MAX_ROOMS_GRID_Y) continue;

        if(_idx == end->index)
        {
            ipath[index + len[path_index]] = _idx;
            len[path_index]++;
            dprint_path(_path_index, path_index, "  [%d : %d] found the end\n", path_index, len[path_index]);
            printf("[%d : %d] found the end\n", path_index, len[path_index]);
            valid[path_index] = true;
            *curr_min = len[path_index];
            return true;
        }

        if(level->rooms[_x][_y].type == ROOM_TYPE_TREASURE) continue;
        if(level->rooms[_x][_y].type == ROOM_TYPE_BOSS) continue;

        // part of the path already
        bool in_path = false;
        for(int i = 0; i < len[path_index]; ++i)
        {
            // if(path_index == 1)
            // {
            //     printf("     [%d] checking prior room: %u (index: %d)\n", path_index, ipath[index + i], i);
            // }

            if(ipath[index + i] == _idx)
            {
                in_path = true;
                break;
            }
        }
        if(in_path) continue;

        valid_dirs[valid_count++] = dir;

    }

    if(valid_count == 0)
    {
        dprint_path(_path_index, path_index, "  [%d : %d] dead end\n", path_index, len[path_index]);
        dprint_start_end("ended:%d:%d\n", path_index, len[path_index]);
        return false;
    }

    int new_paths[4] = {0};
    int new_indexes[4] = {0};
    int new_count = 0;

    // always do original path first
    new_paths[0] = path_index;
    new_indexes[0] = index;
    new_count++;



    for(int i = 1; i < valid_count; ++i)
    {
        for(int j = 1; j < MAX_PATHS; ++j)
        {
            if(len[j] == 1)
            {
                new_paths[new_count] = j;
                new_indexes[new_count] = j*MAX_ROOMS_GRID;
                new_count++;
                break;
            }
        }
    }

    for(int i = 1; i < new_count; ++i)
    {
        memcpy(&ipath[new_indexes[i]], &ipath[index], sizeof(uint8_t)*MAX_ROOMS_GRID);
        len[new_paths[i]] = len[path_index];
    }

    for(int i = 0; i < new_count; ++i)
    {

        Vector2i o = get_dir_offsets(valid_dirs[i]);
        int _x = curr.x+o.x;
        int _y = curr.y+o.y;
        int _idx = level_get_room_index(_x, _y);
        ipath[new_indexes[i] + len[new_paths[i]]] = _idx;
        len[new_paths[i]]++;

        if(new_paths[i] != path_index)
        {
            dprint_path(_path_index, new_paths[i], "  [%d : %d] (from: %d) (%d, %d) moving %s to (%d, %d)\n", new_paths[i], len[new_paths[i]], path_index, curr.x, curr.y, get_dir_name(valid_dirs[i]), _x, _y);
            dprint_start_end("started:%d:%d:%d\n", new_paths[i], path_index, len[new_paths[i]]);
            dprint_new_path("started: %d, from: %d\n", new_paths[i], path_index);
        }
        else
        {
            dprint_path(_path_index, new_paths[i], "  [%d : %d] (%d, %d) moving %s to (%d, %d)\n", new_paths[i], len[new_paths[i]], curr.x, curr.y, get_dir_name(valid_dirs[i]), _x, _y);
        }
        next_room(level, end, ipath, len, valid, new_paths[i], curr_min);
    }






#if 0
    // for(int i = 0; i < valid_count; ++i)
    for(int i = valid_count-1; i >= 0; --i)
    {
        int new_path = path_index;
        int new_index = index;
        if(i > 0)
        {
            for(int j = 1; j < MAX_PATHS; ++j)
            {
                if(len[j] == 1)
                {
                    // printf("found slot: %d\n", j);
                    new_index = j*MAX_ROOMS_GRID;
                    new_path = j;
                    break;
                }
            }

            if(new_path == path_index)
            {
                printf("exceeded MAX_PATHS: %d\n", MAX_PATHS);
                return false;
            }

            if(i > 0)
            {
                memcpy(&ipath[new_index], &ipath[index], sizeof(uint8_t)*MAX_ROOMS_GRID);
                len[new_path] = len[path_index];
            }
        }

        Vector2i o = get_dir_offsets(valid_dirs[i]);
        int _x = curr.x+o.x;
        int _y = curr.y+o.y;
        int _idx = level_get_room_index(_x, _y);
        ipath[new_index + len[new_path]] = _idx;
        len[new_path]++;

        if(new_path != path_index)
        {
            dprint_path(_path_index, new_path, "  [%d : %d] (from: %d) (%d, %d) moving %s to (%d, %d)\n", new_path, len[new_path], path_index, curr.x, curr.y, get_dir_name(valid_dirs[i]), _x, _y);
            dprint_start_end("started:%d:%d:%d\n", new_path, path_index, len[new_path]);
            dprint_new_path("started: %d, from: %d\n", new_path, path_index);
        }
        else
        {
            dprint_path(_path_index, new_path, "  [%d : %d] (%d, %d) moving %s to (%d, %d)\n", new_path, len[new_path], curr.x, curr.y, get_dir_name(valid_dirs[i]), _x, _y);
        }
        next_room(level, end, ipath, len, valid, new_path, curr_min);
    }

#endif
    return false;
}

static bool find_direct_room_path(Level* level, Room* start, Room* end, LevelPath* path)
{
    // int x = start->grid.x;
    // int y = start->grid.y;

    // Level level_copy = {0};
    // memcpy(&level_copy, level, sizeof(Level));

    // Dir end_dir_x = DIR_NONE;
    // if(end->grid.x > start->grid.x) end_dir_x = DIR_RIGHT;
    // else if(end->grid.x < start->grid.x) end_dir_x = DIR_LEFT;

    // Dir end_dir_y = DIR_NONE;
    // if(end->grid.y > start->grid.y) end_dir_y = DIR_DOWN;
    // else if(end->grid.y < start->grid.y) end_dir_y = DIR_UP;

    // uint8_t ipaths[MAX_PATHS][MAX_ROOMS_GRID] = {0};
    uint8_t ipaths[MAX_PATHS*MAX_ROOMS_GRID] = {0};
    int lens[MAX_PATHS] = {0};
    bool valids[MAX_PATHS] = {0};

    for(int i = 0; i < MAX_PATHS; ++i)
    {
        // ipaths[i][lens[i]++] = start->index;
        int index = i * MAX_ROOMS_GRID;
        ipaths[index] = start->index;
        lens[i]++;
        valids[i] = false;
    }

    dprint_start_end("started:%d:%d:%d\n", 0, 0, lens[0]);

    int min_len = 99999;
    next_room(level, end, ipaths, lens, valids, 0, &min_len);

    for(int i = 0; i < MAX_PATHS; ++i)
    {
        if(valids[i])
        {
            printf("path: %d is valid, length: %d\n", i, lens[i]);
        }
    }



    return false;

}

#endif

static bool generate_room_path(Level* level, Room* start, Room* end, LevelPath* path, int tries)
{

    for(int t = 0; t < tries; ++t)
    {

        int x = start->grid.x;
        int y = start->grid.y;

        Level level_copy = {0};
        memcpy(&level_copy, level, sizeof(Level));
        memset(path, 0, sizeof(LevelPath));

        int total_dist = get_room_dist(start->grid.x, start->grid.y, end->grid.x, end->grid.y);

        path->rooms[0] = start;
        path->length++;

        for(;;)
        {
            Dir directions[4] = {0};
            int dcount = 0;
            int probs[4] = {0};
            int dists[4] = {0}; // distance from end room

            Dir end_dir_x = DIR_NONE;
            if(end->grid.x > x) end_dir_x = DIR_RIGHT;
            else if(end->grid.x < x) end_dir_x = DIR_LEFT;

            Dir end_dir_y = DIR_NONE;
            if(end->grid.y > y) end_dir_y = DIR_DOWN;
            else if(end->grid.y < y) end_dir_y = DIR_UP;

            for(int dir = 0; dir < 4; ++dir)
            {
                // printf("(%d, %d) dir: %s\n", x, y, get_dir_name(dir));
                Vector2i o = get_dir_offsets(dir);
                int _x = x+o.x;
                int _y = y+o.y;

                if(_x == end->grid.x && _y == end->grid.y)
                {
                    path->directions[path->length-1] = dir;
                    path->directions[path->length] = DIR_NONE;
                    path->rooms[path->length] = end;
                    path->length++;
                    return true;
                }

                if(_x == start->grid.x && _y == start->grid.y) continue;
                // if(_x == _level.start.x && _y == _level.start.y) continue;
                if(_x < 0 || _y < 0) continue;
                if(_x >= MAX_ROOMS_GRID_X || _y >= MAX_ROOMS_GRID_Y) continue;
                if(level->rooms[_x][_y].type == ROOM_TYPE_TREASURE) continue;
                if(level->rooms[_x][_y].type == ROOM_TYPE_BOSS) continue;

                // part of the path already
                bool in_path = false;
                for(int i = 0; i < path->length; ++i)
                {
                    if(_x == path->rooms[i]->grid.x && _y == path->rooms[i]->grid.y)
                    {
                        in_path = true;
                        break;
                    }
                }
                if(in_path) continue;

                directions[dcount] = dir;

                int prior_weight = 0;
                if(dcount > 0) prior_weight = probs[dcount-1];

                if(dir == end_dir_x || dir == end_dir_y)
                {
                    if(path->length >= 1.3*total_dist)
                    {
                        probs[dcount] = prior_weight+100;
                    }
                    else
                    {
                        probs[dcount] = prior_weight+10;
                    }
                }
                else
                {
                    probs[dcount] = prior_weight+1;
                }

                dists[dcount] = get_room_dist(_x, _y, end->grid.x, end->grid.y);
                dcount++;
            }

            if(dcount == 0)
            {
                // print_path(path);
                // LOGW("trying again");
                memcpy(level, &level_copy, sizeof(Level));
                break;
            }

            Dir choose_directions[4] = {0};
            int ccount = 0;
            for(int d = 0; d < dcount; ++d)
            {
                if(dists[d] == 1)
                {
                    choose_directions[ccount++] = directions[d];
                }
            }

            Dir dir = DIR_NONE;
            if(ccount >= 1)
            {
                dir = choose_directions[lrand()%ccount];
            }
            else
            {
                // same as function in player.c
                int choice = rand_from_probs(probs, dcount);
                dir = directions[choice];
            }

            Vector2i o = get_dir_offsets(dir);
            x += o.x;
            y += o.y;

            path->directions[path->length-1] = dir;

            Room* proom = &level->rooms[x][y];
            path->rooms[path->length] = proom;
            // path->rooms[path->length-1]->doors[dir] = true; // set door on prior room
            // path->rooms[path->length]->doors[get_opposite_dir(dir)] = true; // set door on new room
            proom->valid = true;
            path->length++;
        }
    }

    // printf("no path!\n");
    return false;

}

static int get_viable_rooms(RoomType type, bool doors[4], int* ret_list)
{
    int list_count = 0;
    int* list;
    int index = 0;

    int temp_monster_list[MAX_ROOM_LIST_COUNT] = {0};

    switch(type)
    {
        case ROOM_TYPE_BOSS:
            list_count = room_count_boss;
            list = room_list_boss;
            break;
        case ROOM_TYPE_TREASURE:
            list_count = room_count_treasure;
            list = room_list_treasure;
            break;
        case ROOM_TYPE_MONSTER:
        {
            for(int i = 0; i < room_count_monster; ++i)
            {
                bool add = true;
                for(int j = 0; j < used_room_count_monster; ++j)
                {
                    if(room_list_monster[i] == used_room_list_monster[j])
                    {
                        add = false;
                        break;
                    }
                }
                if(add)
                {
                    temp_monster_list[list_count++] = room_list_monster[i];
                }
            }
            list = temp_monster_list;
        } break;
        case ROOM_TYPE_EMPTY:
            list_count = room_count_empty;
            list = room_list_empty;
            break;
    }

    if(list_count == 0) return 0;

    int ret_count = 0;
    for(int i = 0; i < list_count; ++i)
    {
        RoomFileData* rfd = &room_list[list[i]];

        bool valid = true;
        for(int d = 0; d < 4; ++d)
        {
            if(!doors[d]) continue;
            if(!rfd->doors[d])
            {
                valid = false;
                break;
            }
        }
        if(!valid) continue;

        ret_list[ret_count++] = list[i];
    }

    return ret_count;

}


static Room* place_room(Level* level, RoomType type, int min_dist, int max_dist)
{
    for(;;)
    {
        int _x = lrand()%MAX_ROOMS_GRID_X;
        int _y = lrand()%MAX_ROOMS_GRID_Y;
        Room* room = &level->rooms[_x][_y];
        if(room->valid) continue;
        int d = get_room_dist(_x, _y, level->start.x, level->start.y);
        if(d >= min_dist && d <= max_dist)
        {
            room->valid = true;
            room->type = type;
            return room;
        }
    }
}

static int get_room_count(Level* level)
{
    int count = 0;
    for(int i = 0; i < MAX_ROOMS_GRID; ++i)
    {
        if(level->rooms_ptr[i]->valid) count++;
    }
    return count;
}

static void print_room(Level* level, Room* room)
{
    printf("%-8s room: %d, %d (%2u) (dist: %d)\n", get_room_type_name(room->type), room->grid.x, room->grid.y, room->index, get_room_dist(room->grid.x, room->grid.y, level->start.x, level->start.y));
}

static int rand_from_probs(int probs[], int num)
{
    int r = (lrand() % probs[num-1]) + 1;
    for(int i = 0; i < num; ++i)
    {
        if(r <= probs[i])
        {
            return i;
        }
    }
    return num-1;
}

void print_path(LevelPath* path)
{
    Room* start = path->rooms[0];
    Room* end = path->rooms[path->length-1];
    printf("START: %d, %d\n", start->grid.x, start->grid.y);
    for(int i = 1; i < path->length-1; ++i)
    {
        printf("       %d, %d\n", path->rooms[i]->grid.x, path->rooms[i]->grid.y);
    }
    printf("END:   %d, %d\n", end->grid.x, end->grid.y);


    for(int _y = 0; _y < MAX_ROOMS_GRID_Y; ++_y)
    {
        for(int _x = 0; _x < MAX_ROOMS_GRID_X; ++_x)
        {
            if(_x == start->grid.x && _y == start->grid.y)
                printf("S");
            else if(_x == end->grid.x && _y == end->grid.y)
                printf("E");
            else
            {
                bool p = false;
                for(int i = 1; i < path->length-1; ++i)
                {
                    if(_x == path->rooms[i]->grid.x && _y == path->rooms[i]->grid.y)
                    {
                        int idx = i;  // dirs length is actually length+1
                        p = true;
                        if(path->directions[idx] == DIR_LEFT)
                            printf("<");
                        else if(path->directions[idx] == DIR_RIGHT)
                            printf(">");
                        else if(path->directions[idx] == DIR_UP)
                            printf("^");
                        else if(path->directions[idx] == DIR_DOWN)
                            printf("v");
                        break;
                    }
                }
                if(!p) printf(".");
            }
        }
        printf("\n");
    }
    printf("\n");
}

#else

static void branch_room(Level* level, int x, int y, int depth)
{
    // printf("%s\n", __func__);

    Room* room = &level->rooms[x][y];
    RoomFileData* rfd = &room_list[room->layout];

    bool can_go_up    = rfd->doors[DIR_UP] && (y > 0 && !level->rooms[x][y-1].valid);
    bool can_go_right = rfd->doors[DIR_RIGHT] && (x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid);
    bool can_go_down  = rfd->doors[DIR_DOWN] && (y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid);
    bool can_go_left  = rfd->doors[DIR_LEFT] && (x > 0 && !level->rooms[x-1][y].valid);

    int tries = 0;

try_again: ;

    bool random_doors[4] = {0};

    random_doors[DIR_UP]    = can_go_up && flip_coin();
    random_doors[DIR_RIGHT] = can_go_right && flip_coin();
    random_doors[DIR_DOWN]  = can_go_down && flip_coin();
    random_doors[DIR_LEFT]  = can_go_left && flip_coin();

    int door_count = 0;
    for(int i = 0; i < 4; ++i)
        if(random_doors[i]) door_count++;

    if(!min_depth_reached && depth < MIN_DEPTH)
    {
        if(depth < MIN_DEPTH)
        {
            if(door_count < 2)
            {
                tries++;
                if(tries < 10)
                    goto try_again;
            }
        }
        else
        {
            min_depth_reached = true;
        }
    }

    if(random_doors[DIR_UP] && rfd->doors[DIR_UP] && (y > 0 && !level->rooms[x][y-1].valid))
    {
        room->doors[DIR_UP] = true;
        generate_rooms(level, x,y-1,DIR_DOWN,depth+1);
    }

    if(random_doors[DIR_RIGHT] && rfd->doors[DIR_RIGHT] && (x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid))
    {
        room->doors[DIR_RIGHT] = true;
        generate_rooms(level, x+1,y,DIR_LEFT,depth+1);
    }

    if(random_doors[DIR_DOWN] && rfd->doors[DIR_DOWN] && (y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid))
    {
        room->doors[DIR_DOWN] = true;
        generate_rooms(level, x,y+1,DIR_UP,depth+1);
    }

    if(random_doors[DIR_LEFT] && rfd->doors[DIR_LEFT] && (x > 0 && !level->rooms[x-1][y].valid))
    {
        room->doors[DIR_LEFT] = true;
        generate_rooms(level, x-1,y,DIR_RIGHT,depth+1);
    }
}

static int get_rand_room_index(RoomType type, Dir came_from)
{
    int list_count = 0;
    int* list;
    int index = 0;

    int temp_monster_list[MAX_ROOM_LIST_COUNT] = {0};

    switch(type)
    {
        case ROOM_TYPE_BOSS:
            index = room_list_boss[lrand() % room_count_boss];
            list_count = room_count_boss;
            list = room_list_boss;
            break;
        case ROOM_TYPE_TREASURE:
            index = room_list_treasure[lrand() % room_count_treasure];
            list_count = room_count_treasure;
            list = room_list_treasure;
            break;
        case ROOM_TYPE_MONSTER:
        {
            for(int i = 0; i < room_count_monster; ++i)
            {
                bool add = true;
                for(int j = 0; j < used_room_count_monster; ++j)
                {
                    if(room_list_monster[i] == used_room_list_monster[j])
                    {
                        add = false;
                        break;
                    }
                }
                if(add)
                {
                    temp_monster_list[list_count++] = room_list_monster[i];
                }

            }
            index = temp_monster_list[lrand() % list_count];
            list = temp_monster_list;
        } break;
        case ROOM_TYPE_EMPTY:
            index = room_list_empty[lrand() % room_count_empty];
            list_count = room_count_empty;
            list = room_list_empty;
            break;
    }

    RoomFileData* rfd = &room_list[index];

    if(rfd->doors[came_from] == 0)
    {
        // can't use this room
        // find a room that you can

        int new_index = 0;

        for(int i = 0; i < list_count; ++i)
        {
            RoomFileData* rfd_test = &room_list[list[i]];

            if(rfd_test->doors[came_from])
            {
                new_index = list[i];
            }
        }

        index = new_index;
    }
    
    return index;
}

static void generate_rooms(Level* level, int x, int y, Dir came_from, int depth)
{
    // printf("%s\n", __func__);

    Room* room = &level->rooms[x][y];

    if(room->valid)
        goto exit_conditions; // shouldn't happen

    level->num_rooms++;

    room->valid = true;
    room->discovered = false;
    room->color = COLOR_TINT_NONE;//COLOR((lrand() % 159) + 96,(lrand() % 159) + 96,(lrand() % 159) + 96);
    room->index = level_get_room_index(x,y);

    // printf("adding room, depth: %d\n",depth);

    switch(came_from)
    {
        case DIR_UP:    room->doors[DIR_UP]    = true; break;
        case DIR_RIGHT: room->doors[DIR_RIGHT] = true; break;
        case DIR_DOWN:  room->doors[DIR_DOWN]  = true; break;
        case DIR_LEFT:  room->doors[DIR_LEFT]  = true; break;
        default: break;
    }

    bool is_start_room = (x == level->start.x && y == level->start.y);

#if 0 
    if(role != ROLE_CLIENT)
    {
        if(is_start_room)
            item_add(ITEM_NEW_LEVEL, CENTER_X, CENTER_Y, room->index);
    }
#endif

    if(is_start_room)
    {
        room->type   = ROOM_TYPE_EMPTY;
        room->layout = 0;

        goto exit_conditions;
    }

    bool is_boss_room = !level->has_boss_room && ((level->num_rooms == MAX_ROOMS) || (level->num_rooms > 4 && (lrand()%4 == 0)));

    if(is_boss_room)
    {
        room->type = ROOM_TYPE_BOSS;
        room->layout = get_rand_room_index(ROOM_TYPE_BOSS, came_from);
        room->color = COLOR(100,100,200);
        level->has_boss_room = true;

        // add monsters
        if(role != ROLE_CLIENT)
        {

            RoomFileData* rfd = &room_list[room->layout];
            for(int i = 0; i < rfd->creature_count; ++i)
            {
                Vector2i g = {rfd->creature_locations_x[i], rfd->creature_locations_y[i]};
                g.x--; g.y--; // @convert room objects to tile grid coordinates
                Creature* c = creature_add(room, rfd->creature_types[i], &g, NULL);
            }

        }

        goto exit_conditions;
    }

    bool is_treasure_room = !level->has_treasure_room && ((level->num_rooms == MAX_ROOMS) || (level->num_rooms > 4 && (lrand()%4 == 0)));

    if(is_treasure_room)
    {
        room->type = ROOM_TYPE_TREASURE;
        room->layout = get_rand_room_index(ROOM_TYPE_TREASURE, came_from);
        room->color = COLOR(200,200,100);
        level->has_treasure_room = true;
        goto exit_conditions;
    }

    bool is_monster_room = (lrand() % 100 < MONSTER_ROOM_PERCENTAGE);
    is_monster_room &= (used_room_count_monster < room_count_monster);

    if(is_monster_room)
    {
        room->type = ROOM_TYPE_MONSTER;
        room->layout = get_rand_room_index(ROOM_TYPE_MONSTER, came_from);
        room->color = COLOR(200,100,100);
        used_room_list_monster[used_room_count_monster++] = room->layout;

        // add monsters
        if(role != ROLE_CLIENT)
        {

            RoomFileData* rfd = &room_list[room->layout];
            for(int i = 0; i < rfd->creature_count; ++i)
            {
                Vector2i g = {rfd->creature_locations_x[i], rfd->creature_locations_y[i]};
                g.x--; g.y--; // @convert room objects to tile grid coordinates
                Creature* c = creature_add(room, rfd->creature_types[i], &g, NULL);
            }
        
        }
    }
    else
    {
        // empty room
        room->type = ROOM_TYPE_EMPTY;
        room->layout = get_rand_room_index(ROOM_TYPE_EMPTY, came_from);
        room->color = COLOR(200,200,200);
    }

exit_conditions: ;

    RoomFileData* rfd = &room_list[room->layout];

    if(role != ROLE_CLIENT)
    {

        for(int i = 0; i < rfd->item_count; ++i)
        {
            Vector2f pos = level_get_pos_by_room_coords(rfd->item_locations_x[i]-1, rfd->item_locations_y[i]-1); // @convert room objects to tile grid coordinates

            // printf("Adding item of type: %s to (%f %f)\n", item_get_name(rfd->item_types[i]), pos.x, pos.y);
            item_add(rfd->item_types[i], pos.x, pos.y, room->index);
        }

    }

    // printf("  %s\n", get_room_type_name(room->type));

    if(level->num_rooms >= MAX_ROOMS)
        return;

    if(room->type == ROOM_TYPE_BOSS)
        return; // boss room is a leaf node

    if(room->type == ROOM_TYPE_TREASURE)
        return; // treasure room is a leaf node

    // branch
    branch_room(level,x,y,depth);
}
#endif

void level_generate_room_outer_walls(Room* room)
{
    const float wall_offset = 7.0;
    int x0 = room_area.x - room_area.w/2.0;
    int y0 = room_area.y - room_area.h/2.0;

    // inner walls
    Wall* wall_top = &room->walls[room->wall_count];
    wall_top->p0.x = x0-wall_offset;
    wall_top->p0.y = y0+TILE_SIZE-wall_offset;
    wall_top->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2)+wall_offset;
    wall_top->p1.y = wall_top->p0.y;
    wall_top->dir = DIR_DOWN;
    wall_top->type = WALL_TYPE_INNER;
    room->wall_count++;

    Wall* wall_right = &room->walls[room->wall_count];
    wall_right->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+wall_offset;
    wall_right->p0.y = y0-wall_offset;
    wall_right->p1.x = wall_right->p0.x;
    wall_right->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2)+wall_offset;
    wall_right->dir = DIR_LEFT;
    wall_right->type = WALL_TYPE_INNER;
    room->wall_count++;

    Wall* wall_bottom = &room->walls[room->wall_count];
    wall_bottom->p0.x = x0-wall_offset;
    wall_bottom->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+wall_offset;
    wall_bottom->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2)+wall_offset;
    wall_bottom->p1.y = wall_bottom->p0.y;
    wall_bottom->dir = DIR_UP;
    wall_bottom->type = WALL_TYPE_INNER;
    room->wall_count++;

    Wall* wall_left = &room->walls[room->wall_count];
    wall_left->p0.x = x0+TILE_SIZE-wall_offset;
    wall_left->p0.y = y0-wall_offset;
    wall_left->p1.x = wall_left->p0.x;
    wall_left->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2)+wall_offset;
    wall_left->dir = DIR_RIGHT;
    wall_left->type = WALL_TYPE_INNER;
    room->wall_count++;

    // outer walls
    Wall* wall_outer_top = &room->walls[room->wall_count];
    wall_outer_top->p0.x = x0;
    wall_outer_top->p0.y = y0;
    wall_outer_top->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
    wall_outer_top->p1.y = wall_outer_top->p0.y;
    wall_outer_top->type = WALL_TYPE_OUTER;
    wall_outer_top->dir = DIR_DOWN;
    room->wall_count++;

    Wall* wall_outer_right = &room->walls[room->wall_count];
    wall_outer_right->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
    wall_outer_right->p0.y = y0;
    wall_outer_right->p1.x = wall_outer_right->p0.x;
    wall_outer_right->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
    wall_outer_right->dir = DIR_LEFT;
    wall_outer_right->type = WALL_TYPE_OUTER;
    room->wall_count++;

    Wall* wall_outer_bottom = &room->walls[room->wall_count];
    wall_outer_bottom->p0.x = x0;
    wall_outer_bottom->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
    wall_outer_bottom->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
    wall_outer_bottom->p1.y = wall_outer_bottom->p0.y;
    wall_outer_bottom->dir = DIR_UP;
    wall_outer_bottom->type = WALL_TYPE_OUTER;
    room->wall_count++;

    Wall* wall_outer_left = &room->walls[room->wall_count];
    wall_outer_left->p0.x = x0;
    wall_outer_left->p0.y = y0;
    wall_outer_left->p1.x = wall_outer_left->p0.x;
    wall_outer_left->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
    wall_outer_left->dir = DIR_RIGHT;
    wall_outer_left->type = WALL_TYPE_OUTER;
    room->wall_count++;

    // walls around doors
    if(room->doors[DIR_UP])
    {
        Wall* wall_door_up1 = &room->walls[room->wall_count];
        wall_door_up1->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+1);
        wall_door_up1->p0.y = y0;
        wall_door_up1->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+1);
        wall_door_up1->p1.y = y0+TILE_SIZE-7;
        wall_door_up1->dir = DIR_LEFT;
        wall_door_up1->type = WALL_TYPE_OUTER;
        room->wall_count++;

        Wall* wall_door_up2 = &room->walls[room->wall_count];
        wall_door_up2->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_up2->p0.y = y0;
        wall_door_up2->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_up2->p1.y = y0+TILE_SIZE-7;
        wall_door_up2->dir = DIR_RIGHT;
        wall_door_up2->type = WALL_TYPE_OUTER;
        room->wall_count++;
    }

    if(room->doors[DIR_RIGHT])
    {
        Wall* wall_door_right1 = &room->walls[room->wall_count];
        wall_door_right1->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+7;
        wall_door_right1->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+1);
        wall_door_right1->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
        wall_door_right1->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+1);
        wall_door_right1->dir = DIR_UP;
        wall_door_right1->type = WALL_TYPE_OUTER;
        room->wall_count++;

        Wall* wall_door_right2 = &room->walls[room->wall_count];
        wall_door_right2->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+7;
        wall_door_right2->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_right2->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
        wall_door_right2->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_right2->dir = DIR_DOWN;
        wall_door_right2->type = WALL_TYPE_OUTER;
        room->wall_count++;
    }

    if(room->doors[DIR_DOWN])
    {
        Wall* wall_door_down1 = &room->walls[room->wall_count];
        wall_door_down1->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+1);
        wall_door_down1->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+7;
        wall_door_down1->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+1);
        wall_door_down1->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
        wall_door_down1->dir = DIR_LEFT;
        wall_door_down1->type = WALL_TYPE_OUTER;
        room->wall_count++;

        Wall* wall_door_down2 = &room->walls[room->wall_count];
        wall_door_down2->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_down2->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+7;
        wall_door_down2->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_down2->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
        wall_door_down2->dir = DIR_RIGHT;
        wall_door_down2->type = WALL_TYPE_OUTER;
        room->wall_count++;
    }

    if(room->doors[DIR_LEFT])
    {
        Wall* wall_door_left1 = &room->walls[room->wall_count];
        wall_door_left1->p0.x = x0;
        wall_door_left1->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+1);
        wall_door_left1->p1.x = x0+TILE_SIZE-7;
        wall_door_left1->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+1);
        wall_door_left1->dir = DIR_UP;
        wall_door_left1->type = WALL_TYPE_OUTER;
        room->wall_count++;

        Wall* wall_door_left2 = &room->walls[room->wall_count];
        wall_door_left2->p0.x = x0;
        wall_door_left2->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_left2->p1.x = x0+TILE_SIZE-7;
        wall_door_left2->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_left2->dir = DIR_DOWN;
        wall_door_left2->type = WALL_TYPE_OUTER;
        room->wall_count++;
    }
}

static void add_wall(Room* room, Dir dir, WallType t, Vector2f* p0, Vector2f* p1)
{
    Wall* w = &room->walls[room->wall_count++];

    w->dir = dir;
    w->type = t;
    w->p0.x = p0->x; w->p0.y = p0->y;
    w->p1.x = p1->x; w->p1.y = p1->y;

    p0->x = 0.0; p0->y = 0.0;
    p1->x = 0.0; p1->y = 0.0;
}

void generate_walls(Level* level)
{
    int x0 = room_area.x - room_area.w/2.0;
    int y0 = room_area.y - room_area.h/2.0;

    for(int j = 0; j < MAX_ROOMS_GRID_Y; ++j)
    {
        for(int i = 0; i < MAX_ROOMS_GRID_X; ++i)
        {
            Room* room = &level->rooms[i][j];

            if(room->valid)
            {
                level_generate_room_outer_walls(room);

                RoomFileData* rdata = &room_list[room->layout];

                // create horizontal walls
                for(int rj = 0; rj < ROOM_TILE_SIZE_Y; ++rj)
                {
                    Vector2f up0 = {0.0, 0.0}; Vector2f up1 = {0.0, 0.0};
                    Vector2f dn0 = {0.0, 0.0}; Vector2f dn1 = {0.0, 0.0};

                    TileType tt = TILE_NONE;
                    TileType tt_prior;

                    for(int ri = 0; ri < ROOM_TILE_SIZE_X; ++ri)
                    {
                        tt_prior = tt;
                        tt = rdata->tiles[ri][rj];

                        // check up and down
                        TileType tt_up = rdata->tiles[ri][rj-1];
                        TileType tt_dn = rdata->tiles[ri][rj+1];

                        bool _up = (tt == TILE_PIT && tt_up != TILE_PIT) || (tt == TILE_BOULDER && tt_up != TILE_BOULDER);
                        bool _dn = (tt == TILE_PIT && tt_dn != TILE_PIT) || (tt == TILE_BOULDER && tt_dn != TILE_BOULDER);

                        if(_up && tt != tt_prior)
                        {
                            // wall switched from pit to boulder (or boulder to pit) in same line
                            add_wall(room, DIR_UP, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &up0, &up1);
                        }

                        if(_up)
                        {
                            // start at top left
                            Vector2f _p0 = {x0+TILE_SIZE*(ri+1), y0+TILE_SIZE*(rj+1)};
                            Vector2f _p1 = {x0+TILE_SIZE*(ri+1) + TILE_SIZE, y0+TILE_SIZE*(rj+1)};

                            if(up0.x == 0.0 && up0.y == 0.0)
                            {
                                // first segment
                                up0.x = _p0.x; up0.y = _p0.y;
                                up1.x = _p1.x; up1.y = _p1.y;
                            }
                            else if(up1.x == _p0.x && up1.y == _p0.y)
                            {
                                // continue segment
                                up1.x = _p1.x; up1.y = _p1.y;
                            }
                        }
                        else if(up0.x > 0.0 && up0.y > 0.0)
                        {
                            add_wall(room, DIR_UP, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &up0, &up1);
                        }

                        if(_dn && tt != tt_prior)
                        {
                            add_wall(room, DIR_DOWN, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &dn0, &dn1);
                        }

                        if(_dn)
                        {
                            // start at top left
                            Vector2f _p0 = {x0+TILE_SIZE*(ri+1), y0+TILE_SIZE*(rj+1) + TILE_SIZE};
                            Vector2f _p1 = {x0+TILE_SIZE*(ri+1) + TILE_SIZE, y0+TILE_SIZE*(rj+1) + TILE_SIZE};

                            if(dn0.x == 0.0 && dn0.y == 0.0)
                            {
                                // first segment
                                dn0.x = _p0.x; dn0.y = _p0.y;
                                dn1.x = _p1.x; dn1.y = _p1.y;
                            }
                            else if(dn1.x == _p0.x && dn1.y == _p0.y)
                            {
                                // continue segment
                                dn1.x = _p1.x; dn1.y = _p1.y;
                            }
                        }
                        else if(dn0.x > 0.0 && dn0.y > 0.0)
                        {
                            add_wall(room, DIR_DOWN, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &dn0, &dn1);
                        }
                    }

                    if(up0.x > 0.0 && up0.y > 0.0)
                    {
                        add_wall(room, DIR_UP, (tt == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &up0, &up1);
                    }

                    if(dn0.x > 0.0 && dn0.y > 0.0)
                    {
                        add_wall(room, DIR_DOWN, (tt == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &dn0, &dn1);
                    }

                }
                
                // create vertical walls
                for(int ri = 0; ri < ROOM_TILE_SIZE_X; ++ri)
                {
                    Vector2f left0 = {0.0, 0.0}; Vector2f left1 = {0.0, 0.0};
                    Vector2f right0 = {0.0, 0.0}; Vector2f right1 = {0.0, 0.0};

                    TileType tt = TILE_NONE;
                    TileType tt_prior;

                    for(int rj = 0; rj < ROOM_TILE_SIZE_Y; ++rj)
                    {
                        tt_prior = tt;
                        tt = rdata->tiles[ri][rj];

                        // check left and down
                        TileType tt_left = rdata->tiles[ri-1][rj];
                        TileType tt_right = rdata->tiles[ri+1][rj];

                        bool _left  = (tt == TILE_PIT && tt_left != TILE_PIT) || (tt == TILE_BOULDER && tt_left != TILE_BOULDER);
                        bool _right = (tt == TILE_PIT && tt_right != TILE_PIT) || (tt == TILE_BOULDER && tt_right != TILE_BOULDER);

                        if(_left && (tt != tt_prior))
                        {
                            add_wall(room, DIR_LEFT, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &left0, &left1);
                        }

                        if(_left)
                        {
                            // start at top left
                            Vector2f _p0 = {x0+TILE_SIZE*(ri+1), y0+TILE_SIZE*(rj+1)};
                            Vector2f _p1 = {x0+TILE_SIZE*(ri+1), y0+TILE_SIZE*(rj+1) + TILE_SIZE};

                            if(left0.x == 0.0 && left0.y == 0.0)
                            {
                                // first segment
                                left0.x = _p0.x; left0.y = _p0.y;
                                left1.x = _p1.x; left1.y = _p1.y;
                            }
                            else if(left1.x == _p0.x && left1.y == _p0.y)
                            {
                                // continue segment
                                left1.x = _p1.x; left1.y = _p1.y;
                            }
                        }
                        else if(left0.x > 0.0 && left0.y > 0.0)
                        {
                            add_wall(room, DIR_LEFT, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &left0, &left1);
                        }

                        if(_right && (tt != tt_prior))
                        {
                            add_wall(room, DIR_RIGHT, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &right0, &right1);
                        }

                        if(_right)
                        {
                            // start at top left
                            Vector2f _p0 = {x0+TILE_SIZE*(ri+1) + TILE_SIZE, y0+TILE_SIZE*(rj+1)};
                            Vector2f _p1 = {x0+TILE_SIZE*(ri+1) + TILE_SIZE, y0+TILE_SIZE*(rj+1) + TILE_SIZE};

                            if(right0.x == 0.0 && right0.y == 0.0)
                            {
                                // first segment
                                right0.x = _p0.x; right0.y = _p0.y;
                                right1.x = _p1.x; right1.y = _p1.y;
                            }
                            else if(right1.x == _p0.x && right1.y == _p0.y)
                            {
                                // continue segment
                                right1.x = _p1.x; right1.y = _p1.y;
                            }
                        }
                        else if(right0.x > 0.0 && right0.y > 0.0)
                        {
                            add_wall(room, DIR_RIGHT, (tt_prior == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &right0, &right1);
                        }
                    }

                    if(left0.x > 0.0 && left0.y > 0.0)
                    {
                        add_wall(room, DIR_LEFT, (tt == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &left0, &left1);
                    }

                    if(right0.x > 0.0 && right0.y > 0.0)
                    {
                        add_wall(room, DIR_RIGHT, (tt == TILE_PIT ? WALL_TYPE_PIT : WALL_TYPE_BLOCK), &right0, &right1);
                    }
                }
            }
        }
    }
}

void level_handle_room_collision(Room* room, Physics* phys, int entity_type, void* entity)
{
    if(!room)
        return;

    Entity* e = (Entity*)entity;

    level_sort_walls(room->walls,room->wall_count, phys);

    for(int i = 0; i < room->wall_count; ++i)
    {
        phys_calc_collision_rect(phys);

        Wall* wall = &room->walls[i];

        float px = phys->pos.x;
        float py = phys->pos.y;

        if(entity_type == ENTITY_TYPE_PROJECTILE && (wall->type == WALL_TYPE_PIT || wall->type == WALL_TYPE_INNER))
            continue;

        if(entity_type == ENTITY_TYPE_CREATURE && (wall->type == WALL_TYPE_INNER))
        {
            Creature* c = (Creature*)e->ptr;
            if(c->type == CREATURE_TYPE_CLINGER)
            {
                continue;
            }
        }

        if(entity_type == ENTITY_TYPE_PROJECTILE && wall->type == WALL_TYPE_BLOCK && phys->pos.z > 36.0)
            continue;

        if(entity_type == ENTITY_TYPE_PLAYER && wall->type == WALL_TYPE_PIT)
            continue;

        if(phys->pos.z > 0.0 && wall->type == WALL_TYPE_PIT)
            continue;

        if(wall->distance_to_player > phys->radius)
            continue; // ignore wall if too far away

        if(wall->dir == DIR_UP    && phys->vel.y < 0.0) continue;
        if(wall->dir == DIR_RIGHT && phys->vel.x > 0.0) continue;
        if(wall->dir == DIR_DOWN  && phys->vel.y > 0.0) continue;
        if(wall->dir == DIR_LEFT  && phys->vel.x < 0.0) continue;

        if(phys->dead && wall->type != WALL_TYPE_OUTER) continue;

        bool collision = false;

        // p0 ------------ p1
        // |               |
        // |               |
        // |               |
        // p2 ------------ p3

        Rect* r = &phys->collision_rect;

        Vector2f p0 = {r->x - r->w/2.0, r->y - r->h/2.0};
        Vector2f p1 = {r->x + r->w/2.0, r->y - r->h/2.0};
        Vector2f p2 = {r->x - r->w/2.0, r->y + r->h/2.0};
        Vector2f p3 = {r->x + r->w/2.0, r->y + r->h/2.0};

        float delta = 0.0;

        switch(wall->dir)
        {
            case DIR_UP:    collision = (p2.y > wall->p0.y) && (p2.x < wall->p1.x && p3.x > wall->p0.x); delta = p2.y - wall->p0.y; break;
            case DIR_RIGHT: collision = (p0.x < wall->p0.x) && (p0.y < wall->p1.y && p2.y > wall->p0.y); delta = wall->p0.x - p0.x; break;
            case DIR_DOWN:  collision = (p0.y < wall->p0.y) && (p0.x < wall->p1.x && p1.x > wall->p0.x); delta = wall->p0.y - p0.y; break;
            case DIR_LEFT:  collision = (p1.x > wall->p0.x) && (p1.y < wall->p1.y && p3.y > wall->p0.y); delta = p1.x - wall->p0.x; break;
        }

        delta += 0.01;

        if(collision)
        {
            if(phys->amorphous)
            {
                phys->dead = true;
                return;
            }

            switch(wall->dir)
            {
                case DIR_UP:    phys->pos.y -= delta; phys->vel.y *= -phys->elasticity; break;
                case DIR_DOWN:  phys->pos.y += delta; phys->vel.y *= -phys->elasticity; break;
                case DIR_LEFT:  phys->pos.x -= delta; phys->vel.x *= -phys->elasticity; break;
                case DIR_RIGHT: phys->pos.x += delta; phys->vel.x *= -phys->elasticity; break;
            }

            // add convenient sliding to get around walls
#if 0
            const int threshold = 10;

            if(wall->dir == DIR_UP || wall->dir == DIR_DOWN)
            {
                bool can_slide_left   = px-threshold < wall->p0.x;
                bool can_slide_right  = px+threshold > wall->p1.x;

                if(can_slide_left || can_slide_right)
                {
                    Vector2i tile_coords = level_get_room_coords_by_pos(px, py);

                    tile_coords.x += wall->dir == DIR_UP ? +1 : -1;
                    //tile_coords.y += can_slide_up ? -1 : +1;

                    TileType tt = level_get_tile_type(room, tile_coords.x, tile_coords.y);

                    if(tt == TILE_FLOOR)
                    {
                        phys->pos.x += can_slide_left ? -1 : +1;
                    }
                } 
            }
            else
            {
                bool can_slide_up   = py-threshold < wall->p0.y;
                bool can_slide_down = py+threshold > wall->p1.y;

                if(can_slide_up || can_slide_down)
                {
                    Vector2i tile_coords = level_get_room_coords_by_pos(px, py);

                    tile_coords.x += wall->dir == DIR_LEFT ? +1 : -1;
                    //tile_coords.y += can_slide_up ? -1 : +1;

                    TileType tt = level_get_tile_type(room, tile_coords.x, tile_coords.y);

                    if(tt == TILE_FLOOR)
                    {
                        phys->pos.y += can_slide_up ? -1.0 : +1.0;
                    }
                } 
            }
#endif
        }
    }

}

void level_init()
{
    if(dungeon_image > 0) return;

    dungeon_set_image1 = gfx_load_image("src/img/dungeon_set1.png", false, false, TILE_SIZE, TILE_SIZE);
    dungeon_set_image2 = gfx_load_image("src/img/dungeon_set2.png", false, false, TILE_SIZE, TILE_SIZE);
    dungeon_set_image3 = gfx_load_image("src/img/dungeon_set3.png", false, false, TILE_SIZE, TILE_SIZE);

    dungeon_image = dungeon_set_image2;

    room_file_load_all(false);
    printf("Done with room files\n");
}

void level_update(float dt)
{
    level_grace_time -= dt;
    level_grace_time = MAX(0.0, level_grace_time);
}

void level_print(Level* level)
{
    printf("\n");
    for(int j = 0; j < MAX_ROOMS_GRID_Y; ++j)
    {
        for(int i = 0; i < MAX_ROOMS_GRID_X; ++i)
        {
            Room* room = &level->rooms[i][j];
            printf("%c", room->valid ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

static int level_traversable_func(int x, int y)
{
    Room* room  = level_get_room_by_index(&level, player->curr_room);
    TileType tt = level_get_tile_type(room, x, y);

    if(tt == TILE_BOULDER) return 0;
    if(tt == TILE_PIT)     return 0;
    if(tt == TILE_SPIKES)  return 1;
    if(tt == TILE_MUD)     return 2;
    if(tt == TILE_ICE)     return 2;

    return 3;
}

Rect level_get_tile_rect(int x, int y)
{
    float _x = room_area.x - room_area.w/2.0;
    float _y = room_area.y - room_area.h/2.0;

    _x += (x+1)*TILE_SIZE;
    _y += (y+1)*TILE_SIZE;

    Rect r = RECT((_x+TILE_SIZE/2.0), (_y+TILE_SIZE/2.0), TILE_SIZE, TILE_SIZE);
    return r;
}

Rect level_get_rect_by_pos(float x, float y)
{
    Vector2i tile_coords = level_get_room_coords_by_pos(x, y);
    return level_get_tile_rect(tile_coords.x, tile_coords.y);
}

uint8_t level_get_tile_sprite(TileType tt)
{
    SpriteTileType sprite = SPRITE_TILE_MAX;
    switch(tt)
    {
        case TILE_FLOOR:         sprite = SPRITE_TILE_FLOOR;  break;
        case TILE_PIT:           sprite = SPRITE_TILE_PIT;    break;
        case TILE_BOULDER:       sprite = SPRITE_TILE_BLOCK;  break;
        case TILE_MUD:           sprite = SPRITE_TILE_MUD;    break;
        case TILE_ICE:           sprite = SPRITE_TILE_ICE;    break;
        case TILE_SPIKES:        sprite = SPRITE_TILE_SPIKES; break;
        case TILE_TIMED_SPIKES:  sprite = SPRITE_TILE_SPIKES; break;
        default: break;
    }
    return (uint8_t)sprite;
}

TileType level_get_tile_type(Room* room, int x, int y)
{
    if(x < 0 || x >= ROOM_TILE_SIZE_X) return TILE_NONE;
    if(y < 0 || y >= ROOM_TILE_SIZE_Y) return TILE_NONE;
    if(!room) return TILE_NONE;
    if(room->layout >= MAX_ROOM_LIST_COUNT) return TILE_NONE;

    RoomFileData* rdata = &room_list[room->layout];
    if(!rdata) return TILE_NONE;

    return rdata->tiles[x][y];
}

TileType level_get_tile_type_by_pos(Room* room, float x, float y)
{
    Vector2i tile_coords = level_get_room_coords_by_pos(x, y);
    return level_get_tile_type(room, tile_coords.x, tile_coords.y);
}


void level_get_safe_floor_tile(Room* room, Vector2i start, Vector2i* tile_coords, Vector2f* tile_pos)
{
    for(int x = 0; x < ROOM_TILE_SIZE_X; ++x)
    {
        int _x = start.x;
        _x += ((x % 2 == 0) ? -1 : 1) * x/2;
        if(_x < 0 || _x >= ROOM_TILE_SIZE_X) continue;

        for(int y = 0; y < ROOM_TILE_SIZE_Y; ++y)
        {
            int _y = start.y;
            _y += ((y % 2 == 0) ? -1 : 1) * y/2;
            if( _y < 0 || _y >= ROOM_TILE_SIZE_Y) continue;

            // TODO: item_is_on_tile and creature_is_on_tile only do simple collision checks
            if(IS_SAFE_TILE(level_get_tile_type(room, _x, _y)) && !item_is_on_tile(room, _x, _y) && !creature_is_on_tile(room, _x, _y))
            {
                if(tile_coords)
                {
                    tile_coords->x = _x;
                    tile_coords->y = _y;
                }
                if(tile_pos)
                {
                    Rect rp = level_get_tile_rect(_x, _y);
                    tile_pos->x = rp.x;
                    tile_pos->y = rp.y;
                    // printf("setting tile_pos: %.2f, %.2f (%d, %d)\n", tile_pos->x, tile_pos->y, _x, _y);
                }
                return;
            }
        }
    }
}


void level_get_rand_floor_tile(Room* room, Vector2i* tile_coords, Vector2f* tile_pos)
{
    for(;;)
    {
        int _x = rand() % ROOM_TILE_SIZE_X;
        int _y = rand() % ROOM_TILE_SIZE_Y;
        if(level_get_tile_type(room, _x, _y) == TILE_FLOOR)
        {
            if(tile_coords)
            {
                tile_coords->x = _x;
                tile_coords->y = _y;
            }
            if(tile_pos)
            {
                Rect rp = level_get_tile_rect(_x, _y);
                tile_pos->x = rp.x;
                tile_pos->y = rp.y;
            }
            return;
        }
    }
}

Vector2i level_get_door_tile_coords(Dir dir)
{
    Vector2i c = {0};
    if(dir == DIR_NONE)
    {
        c.x = ROOM_TILE_SIZE_X/2;
        c.y = ROOM_TILE_SIZE_Y/2;
    }
    else if(dir == DIR_LEFT)
    {
        c.x = 0;
        c.y = ROOM_TILE_SIZE_Y/2;
    }
    else if(dir == DIR_RIGHT)
    {
        c.x = ROOM_TILE_SIZE_X-1;
        c.y = ROOM_TILE_SIZE_Y/2;
    }
    else if(dir == DIR_UP)
    {
        c.x = ROOM_TILE_SIZE_X/2;
        c.y = 0;
    }
    else if(dir == DIR_DOWN)
    {
        c.x = ROOM_TILE_SIZE_X/2;
        c.y = ROOM_TILE_SIZE_Y-1;
    }
    return c;
}

void level_draw_room(Room* room, RoomFileData* room_data, float xoffset, float yoffset)
{
    if(!room)
        return;

    if(!room->valid)
        return;

    float x = room_area.x - room_area.w/2.0 + xoffset;
    float y = room_area.y - room_area.h/2.0 + yoffset;
    float w = TILE_SIZE;
    float h = TILE_SIZE;

    Rect r = {(x+w/2.0), (y+h/2.0), w,h};   // top left wall corner

    uint32_t color = COLOR_TINT_NONE; //room->color;

    gfx_sprite_batch_begin(true);

    // draw walls
    for(int i = 1; i < ROOM_TILE_SIZE_X+1; ++i) // top
        gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_UP, r.x + i*w,r.y, color, false, 1.0, 0.0, 1.0, false, false, false);
   
    for(int i = 1; i < ROOM_TILE_SIZE_Y+1; ++i) // right
        gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_RIGHT, r.x + (ROOM_TILE_SIZE_X+1)*w,r.y+i*h, color,false,  1.0, 0.0, 1.0, false, false, false);

    for(int i = 1; i < ROOM_TILE_SIZE_X+1; ++i) // bottom
        gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_DOWN, r.x + i*w,r.y+(ROOM_TILE_SIZE_Y+1)*h, color, false, 1.0, 0.0, 1.0, false, false, false);

    for(int i = 1; i < ROOM_TILE_SIZE_Y+1; ++i) // left
        gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_LEFT, r.x,r.y+i*h, color, false, 1.0, 0.0, 1.0, false, false, false);

    // wall corners
    gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_CORNER_LU, r.x,r.y, color, false, 1.0, 0.0, 1.0, false, false, false);
    gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_CORNER_UR, r.x+(ROOM_TILE_SIZE_X+1)*w,r.y, color, false, 1.0, 0.0, 1.0, false, false, false);
    gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_CORNER_RD, r.x+(ROOM_TILE_SIZE_X+1)*w,r.y+(ROOM_TILE_SIZE_Y+1)*h, color, false, 1.0, 0.0, 1.0, false, false, false);
    gfx_sprite_batch_add(dungeon_image, SPRITE_TILE_WALL_CORNER_DL, r.x,r.y+(ROOM_TILE_SIZE_Y+1)*h, color, false, 1.0, 0.0, 1.0, false, false, false);

    uint8_t door_sprites[4] = {SPRITE_TILE_DOOR_UP, SPRITE_TILE_DOOR_RIGHT, SPRITE_TILE_DOOR_DOWN, SPRITE_TILE_DOOR_LEFT};
    if(room->doors_locked)
    {
        door_sprites[0] = SPRITE_TILE_DOOR_UP_CLOSED;
        door_sprites[1] = SPRITE_TILE_DOOR_RIGHT_CLOSED;
        door_sprites[2] = SPRITE_TILE_DOOR_DOWN_CLOSED;
        door_sprites[3] = SPRITE_TILE_DOOR_LEFT_CLOSED;
    }

    // center of the room
    float halfw = TILE_SIZE*(ROOM_TILE_SIZE_X+1)/2.0;
    float halfh = TILE_SIZE*(ROOM_TILE_SIZE_Y+1)/2.0;
    float centerx = r.x + halfw;
    float centery = r.y + halfh;

    for(int i = 0; i < 4; ++i)
    {
        if(!room->doors[i]) continue;

        Vector2i c = level_get_room_coords(room->index);
        Vector2i o = get_dir_offsets(i);

        uint32_t dcolor = color;

        Room* aroom = level_get_room(&level, c.x+o.x, c.y+o.y);
        if(aroom != NULL)
        {
            if(aroom->type == ROOM_TYPE_TREASURE)
                dcolor = aroom->color;
            else if(aroom->type == ROOM_TYPE_BOSS)
                dcolor = aroom->color;
        }
        float _x = centerx + halfw*o.x;
        float _y = centery + halfh*o.y;
        gfx_sprite_batch_add(dungeon_image, door_sprites[i], _x, _y, dcolor, false, 1.0, 0.0, 1.0, false, false, false);
    }

    RoomFileData* rdata;
    if(room_data != NULL)
        rdata = room_data;
    else
        rdata = &room_list[room->layout];

    // draw room
    for(int _y = 0; _y < ROOM_TILE_SIZE_Y; ++_y)
    {
        for(int _x = 0; _x < ROOM_TILE_SIZE_X; ++_x)
        {
            TileType tt = rdata->tiles[_x][_y];
            uint8_t sprite = level_get_tile_sprite(tt);

            // +1 for walls
            float draw_x = r.x + (_x+1)*w;
            float draw_y = r.y + (_y+1)*h;

            uint32_t tcolor = color;
            // if(debug_enabled)
            // {
            //     if(player->curr_tile.x == _x && player->curr_tile.y == _y)
            //     {
            //         tcolor = COLOR_RED;
            //     }
            // }

            gfx_sprite_batch_add(dungeon_image, sprite, draw_x, draw_y, tcolor, false, 1.0, 0.0, 1.0, false, false, false);

            // if(debug_enabled && show_tile_grid)
            // {
            //     gfx_draw_rect_xywh(draw_x, draw_y, TILE_SIZE, TILE_SIZE, COLOR_CYAN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
            // }
        }
    }

    gfx_sprite_batch_draw();
}

void room_draw_walls(Room* room)
{
    if(!room) return;
    for(int i = 0; i < room->wall_count; ++i)
    {
        Wall* wall = &room->walls[i];

        float x = wall->p0.x;
        float y = wall->p0.y;
        float w = ABS(wall->p0.x - wall->p1.x)+1;
        float h = ABS(wall->p0.y - wall->p1.y)+1;

        uint32_t color = COLOR_WHITE;

        switch(wall->type)
        {
            case WALL_TYPE_BLOCK: color = COLOR_WHITE;  break;
            case WALL_TYPE_PIT:   color = COLOR_YELLOW; break;
            case WALL_TYPE_INNER: color = COLOR_CYAN;   break;
            case WALL_TYPE_OUTER: color = COLOR_RED;    break;
        }

        gfx_draw_rect_xywh_tl(x, y, w, h, color, NOT_SCALED, NO_ROTATION, FULL_OPACITY, true, IN_WORLD);
    }
}

#if !NEW_LEVEL_GENERATION
#define GENERATE_ROOMS_TEST 0
#define TEST_COUNT          1000
#define START_SEED          0

Level level_generate(unsigned int seed, int rank)
{
    // printf("%s\n", __func__);
    Level level = {0};

#if GENERATE_ROOMS_TEST
    seed = START_SEED-1;
    for(int t = 0; t < TEST_COUNT; ++t)
    {
        memset(&level, 0, sizeof(Level));
        seed += 1;
#endif

    // seed PRNG
    slrand(seed);

    if(rank != 5)
    {
#if !GENERATE_ROOMS_TEST
        LOGW("Overriding rank!");
#endif
        rank = 5;
        level_rank = rank;
    }

#if !GENERATE_ROOMS_TEST
    LOGI("Generating level, seed: %u, rank: %d", seed, rank);
#endif

    // start in center of grid
    level.start.x = floor(MAX_ROOMS_GRID_X/2);
    level.start.y = floor(MAX_ROOMS_GRID_Y/2);

    used_room_count_monster = 0;

    // fill out helpful room information
    room_count_monster = 0;
    room_count_empty = 0;
    room_count_treasure = 0;
    room_count_boss = 0;

    for(int i = 0; i < room_list_count; ++i)
    {
        RoomFileData* rfd = &room_list[i];

        if(rfd->rank > rank)
            continue;

        switch(rfd->type)
        {
            case ROOM_TYPE_MONSTER:  room_list_monster[room_count_monster++]   = i; break;
            case ROOM_TYPE_EMPTY:    room_list_empty[room_count_empty++]       = i; break;
            case ROOM_TYPE_TREASURE: room_list_treasure[room_count_treasure++] = i; break;
            case ROOM_TYPE_BOSS:     room_list_boss[room_count_boss++]         = i; break;
            default: break;
        }
    }

#if !GENERATE_ROOMS_TEST
    printf("Total Rooms: %d\n", room_list_count);
    printf("   # monster rooms: %d\n",room_count_monster);
    printf("   # empty rooms: %d\n",room_count_empty);
    printf("   # treasure rooms: %d\n",room_count_treasure);
    printf("   # boss rooms: %d\n",room_count_boss);
#endif

    item_clear_all();
    creature_clear_all();
    min_depth_reached = false;

    generate_rooms(&level, level.start.x, level.start.y, DIR_NONE, 0);

    for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
    {
        for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
        {
            uint8_t index = level_get_room_index(x,y);
            level.rooms[x][y].index = index;
            // printf("%2u) %2d, %-2d\n", level.rooms[x][y].index, x, y);
            level.rooms[x][y].doors_locked = (creature_get_room_count(index) != 0);
            level.rooms[x][y].xp = 0;
        }
    }

    if(!level.has_treasure_room || !level.has_boss_room)
    {
        LOGE("Level Generation Error (seed: %u, rank: %d)", seed, rank);
        if(!level.has_treasure_room) LOGE("No treasure room");
        if(!level.has_boss_room) LOGE("No boss room");
    }

#if GENERATE_ROOMS_TEST
    }
#endif


    generate_walls(&level);
    level_print(&level);
    level_grace_time = 2.0;

    return level;
}
#endif

void level_sort_walls(Wall* walls, int wall_count, Physics* phys)
{
    if(!walls)
        return;

    float x = phys->collision_rect.x;
    float y = phys->collision_rect.y;

    // calculate and store distance to player
    for(int i = 0; i < wall_count; ++i)
    {
        Wall* wall = &walls[i];

        Vector2f check_point = {0.0,0.0};

        if(wall->dir == DIR_UP || wall->dir == DIR_DOWN)
        {
            check_point.x = x;
            check_point.y = wall->p0.y;

            if(x < wall->p0.x)      check_point.x = wall->p0.x;
            else if(x > wall->p1.x) check_point.x = wall->p1.x;
        }
        else if(wall->dir == DIR_LEFT || wall->dir == DIR_RIGHT)
        {
            check_point.x = wall->p0.x;
            check_point.y = y;

            if(y < wall->p0.y)      check_point.y = wall->p0.y;
            else if(y > wall->p1.y) check_point.y = wall->p1.y;
        }

        wall->distance_to_player = dist(x,y,check_point.x,check_point.y);

    }

    // insertion sort
    int i, j;
    Wall key;

    for (i = 1; i < wall_count; ++i) 
    {
        memcpy(&key, &walls[i], sizeof(Wall));
        j = i - 1;

        while (j >= 0 && walls[j].distance_to_player > key.distance_to_player)
        {
            memcpy(&walls[j+1], &walls[j], sizeof(Wall));
            j = j - 1;
        }
        memcpy(&walls[j+1], &key, sizeof(Wall));
    }

    return;
}

// for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
// {
//     for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
//     {
//         printf("%2d | %d, %d\n", level_get_room_index(x,y), x, y);
//     }
// }

// for(int i = 0; i < MAX_ROOMS_GRID_X*MAX_ROOMS_GRID_Y; ++i)
// {
//     Vector2i xy = level_get_room_coords(i);
//     printf("%2d | %d, %d\n", i, xy.x, xy.y);
// }


Room* level_get_room(Level* level, int x, int y)
{
    if(!level_is_room_valid(level, x, y))
    {
        // LOGE("Room not valid, %d %d",x,y); //@TODO: Look into any of these prints
        return NULL;
    }
    return &level->rooms[x][y];
}

Room* level_get_room_by_index(Level* level, int index)
{
    Vector2i roomxy = level_get_room_coords(index);
    return level_get_room(level, roomxy.x, roomxy.y);
}

int level_get_room_index(int x, int y)
{
    return y*MAX_ROOMS_GRID_X + x;
}

Vector2i level_get_room_coords(int index)
{
    Vector2i p = {0};
    p.x = index % MAX_ROOMS_GRID_X;
    p.y =  index / MAX_ROOMS_GRID_X;
    return p;
}

Vector2i level_get_room_coords_by_pos(float x, float y)
{
    float x0 = floor_area.x - floor_area.w/2.0;
    float y0 = floor_area.y - floor_area.h/2.0;

    float xd = x - x0;
    float yd = y - y0;

    int _x = xd / TILE_SIZE;
    int _y = yd / TILE_SIZE;

    // fix issue with casting float to int and rounding improperly
    if(xd < 0)
        _x--;
    if(yd < 0)
        _y--;

    Vector2i r = {_x,_y};
    return r;
}

Vector2f level_get_pos_by_room_coords(int x, int y)
{
    float x0 = floor_area.x - floor_area.w/2.0;
    float y0 = floor_area.y - floor_area.h/2.0;

    int xd = (x+1) * TILE_SIZE;
    int yd = (y+1) * TILE_SIZE;

    float _x = xd + x0 - (TILE_SIZE/2.0);
    float _y = yd + y0 - (TILE_SIZE/2.0);

    Vector2f r = {_x,_y};
    return r;
}

bool level_is_room_valid(Level* level, int x, int y)
{
    if(x >= MAX_ROOMS_GRID_X || y >= MAX_ROOMS_GRID_Y || x < 0 || y < 0)
        return false;
    return level->rooms[x][y].valid;
}

bool level_is_room_valid_index(Level* level, int index)
{
    Vector2i p = level_get_room_coords(index);
    return level_is_room_valid(level, p.x, p.y);
}

bool level_is_room_discovered(Level* level, int x, int y)
{
    if(!level_is_room_valid(level, x, y))
        return false;
    return level->rooms[x][y].discovered;
}

bool level_is_room_discovered_index(Level* level, int index)
{
    Vector2i p = level_get_room_coords(index);
    return level_is_room_discovered(level, p.x, p.y);
}

Dir angle_to_dir_cardinal(float angle_deg)
{

    if(angle_deg > 45.0 && angle_deg <= 135.0)
    {
        return DIR_UP;
    }
    else if(angle_deg > 315.0 || angle_deg <= 45.0)
    {
        return DIR_RIGHT;
    }
    else if(angle_deg > 225.0 && angle_deg <= 315)
    {
        return DIR_DOWN;
    }
    else
    {
        return DIR_LEFT;
    }
}

float dir_to_angle_deg(Dir dir)
{
    switch(dir)
    {
        case DIR_UP:    return 90.0;
        case DIR_DOWN:  return 270.0;
        case DIR_LEFT:  return 180.0;
        case DIR_RIGHT: return 0.0;
        case DIR_UP_RIGHT:   return 45.0;
        case DIR_DOWN_LEFT:  return 225.0;
        case DIR_DOWN_RIGHT: return 315.0;
        case DIR_UP_LEFT:    return 135.0;
    }
    return 0.0;
}

Dir get_opposite_dir(Dir dir)
{
    switch(dir)
    {
        case DIR_UP:    return DIR_DOWN;
        case DIR_DOWN:  return DIR_UP;
        case DIR_RIGHT: return DIR_LEFT;
        case DIR_LEFT:  return DIR_RIGHT;
        case DIR_UP_RIGHT:   return DIR_DOWN_LEFT;
        case DIR_DOWN_LEFT:  return DIR_UP_RIGHT;
        case DIR_DOWN_RIGHT: return DIR_UP_LEFT;
        case DIR_UP_LEFT:    return DIR_DOWN_RIGHT;
        default: break;
    }
    return DIR_NONE;
}

// grid x,y offsets
Vector2i get_dir_offsets(Dir door)
{
    Vector2i o = {0};
    switch(door)
    {
        case DIR_UP:    o.y = -1; break;
        case DIR_DOWN:  o.y =  1; break;
        case DIR_LEFT:  o.x = -1; break;
        case DIR_RIGHT: o.x =  1; break;
        case DIR_UP_RIGHT:   o.x =  1; o.y = -1; break;
        case DIR_DOWN_LEFT:  o.x = -1; o.y =  1; break;
        case DIR_DOWN_RIGHT: o.x =  1; o.y =  1; break;
        case DIR_UP_LEFT:    o.x = -1; o.y = -1; break;
        default: break;
    }
    return o;
}

// @TODO: Implement this
Dir get_dir_from_vel(float vel_x, float vel_y)
{
    /*
    Vector2i o = {0};

    switch(door)
    {
        case DIR_UP:    o.y = -1; break;
        case DIR_DOWN:  o.y =  1; break;
        case DIR_LEFT:  o.x = -1; break;
        case DIR_RIGHT: o.x =  1; break;
        case DIR_UP_RIGHT:   o.x =  1; o.y = -1; break;
        case DIR_DOWN_LEFT:  o.x = -1; o.y =  1; break;
        case DIR_DOWN_RIGHT: o.x =  1; o.y =  1; break;
        case DIR_UP_LEFT:    o.x = -1; o.y = -1; break;
        default: break;
    }
    return o;
    */
}

const char* get_dir_name(Dir dir)
{
    switch(dir)
    {
        case DIR_UP: return "up";
        case DIR_DOWN: return "down";
        case DIR_LEFT: return "left";
        case DIR_RIGHT: return "right";
        default: return "?";
    }
}

const char* get_tile_name(TileType tt)
{
    switch(tt)
    {
        case TILE_NONE: return "None";
        case TILE_FLOOR: return "Floor";
        case TILE_PIT: return "Pit";
        case TILE_BOULDER: return "Boulder";
        case TILE_MUD: return "Mud";
        case TILE_ICE: return "Ice";
        case TILE_SPIKES: return "Spikes";
        case TILE_TIMED_SPIKES: return "Timed Spikes";
    }
    return "?";
}

const char* get_room_type_name(RoomType rt)
{
    switch(rt)
    {
        case ROOM_TYPE_EMPTY: return "Empty";
        case ROOM_TYPE_MONSTER: return "Monster";
        case ROOM_TYPE_TREASURE: return "Treasure";
        case ROOM_TYPE_BOSS: return "Boss";
    }
    return "?";
}
