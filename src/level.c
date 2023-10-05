#include "headers.h"
#include "core/gfx.h"
#include "level.h"

RoomData room_list[32] = {0};
int room_list_count = 0;
int dungeon_image;

static inline bool flip_coin()
{
    return (rand()%2==0);
}

static void generate_rooms(Level* level, int x, int y, Door came_from, int depth)
{
    Room* room = &level->rooms[x][y];
    room->valid = true;

    room->layout = rand() % room_list_count;

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

    if(x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid) // check right
    {
        bool door = flip_coin();
        room->doors[DOOR_RIGHT] = door;
        if(door)
            generate_rooms(level, x+1,y,DOOR_RIGHT,depth+1);
    }

    if(y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid) // check down
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
            else if(x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid) // check right
            {
                room->doors[DOOR_RIGHT] = true;
                generate_rooms(level, x+1,y,DOOR_RIGHT,depth+1);
            }
            else if(y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid) // check down
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

static bool level_load_room_list()
{
    FILE* fp = fopen("src/rooms/rooms.txt", "r");

    if(!fp)
        return false;

    RoomData* room_data = &room_list[0];

    int tile_x = 0;
    int tile_y = 0;

    int pc1 = 0, pc2 = 0;
    int c = 0;

    for(;;)
    {
        pc2 = pc1;
        pc1 = c;
        c = fgetc(fp);

        if(c == EOF)
            break;

        if(c == '#')
        {
            // ignore
            continue;
        }

        if(c == '\n')
        {
            if(pc2 != '#')
                tile_y++;

            tile_x = 0;

            if(tile_y >= ROOM_TILE_SIZE_Y)
            {
                room_data++;
                room_list_count++;
                tile_y = 0;
            }
            continue;
        }

        TileType* tile = &room_data->tiles[tile_x++][tile_y];

        switch(c)
        {
            case '.': *tile = TILE_FLOOR; break;
            case 'o': *tile = TILE_BOULDER; break;
            default: break;
        }
    }

#if 0

    // print rooms
    printf("Room list Count: %d\n", room_list_count);
    for(int n = 0; n < room_list_count; ++n)
    {
        RoomData* room_data = &room_list[n];
        for(int j = 0; j < ROOM_TILE_SIZE_Y; ++j)
        {
            for(int i = 0; i < ROOM_TILE_SIZE_X; ++i)
            {
                TileType* tile = &room_data->tiles[i][j];
                switch(*tile)
                {
                    case TILE_FLOOR:   printf("."); break;
                    case TILE_BOULDER: printf("o"); break;
                    default: printf("%c", *tile+65);break;
                }
            }
            printf("\n");
        }
        printf("\n");
    }
    printf("\n");
#endif

    return true;
}

void level_init()
{
    level_load_room_list();
    dungeon_image = gfx_load_image("src/img/dungeon_set.png", false, true, 32, 32);
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

void level_draw_room(Room* room)
{
    gfx_draw_image(dungeon_image, 0, 100,100, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
}

Level level_generate(unsigned int seed)
{
    int start_x, start_y;

    Level level = {0};

    srand(seed);

    start_x = MAX_ROOMS_GRID_X / 2; //rand() % MAX_ROOMS_GRID_X;
    start_y = MAX_ROOMS_GRID_Y / 2; //rand() % MAX_ROOMS_GRID_Y;

    // memset(level->rooms,0, sizeof(level->rooms));
    generate_rooms(&level, start_x, start_y, DOOR_NONE, 0);

    return level;
}
