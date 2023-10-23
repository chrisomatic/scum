#include "headers.h"
#include "core/gfx.h"
#include "main.h"
#include "creature.h"
#include "physics.h"

RoomData room_list[32] = {0};
int room_list_count = 0;
int dungeon_image;

static inline bool flip_coin()
{
    return (rand()%2==0);
}

static void generate_rooms(Level* level, int x, int y, Dir came_from, int depth)
{
    Room* room = &level->rooms[x][y];
    room->valid = true;
    room->discovered = false;
    room->color = rand();
    room->index = level_get_room_index(x,y);

    if(x == (MAX_ROOMS_GRID_X / 2) && y == (MAX_ROOMS_GRID_Y / 2))
    {
        room->layout = 0;
    }
    else
    {
        room->layout = rand() % room_list_count;
    }


    int n = rand() % 16;
    for(int i = 0; i < n; ++i)
    {
        // if(level->start.x == x && level->start.y == y)
        //     printf("%d) spawning creature in start room\n", i + 1);
        creature_add(room,rand() % CREATURE_TYPE_MAX);
    }

    switch(came_from)
    {
        case DIR_UP:    room->doors[DIR_DOWN]  = true; break;
        case DIR_RIGHT: room->doors[DIR_LEFT]  = true; break;
        case DIR_DOWN:  room->doors[DIR_UP]    = true; break;
        case DIR_LEFT:  room->doors[DIR_RIGHT] = true; break;
        default: break;
    }

    if(depth >= MAX_DEPTH)
        return;

    if(y > 0 && !level->rooms[x][y-1].valid) // check up
    {
        bool door = flip_coin();
        room->doors[DIR_UP] = door;
        if(door)
            generate_rooms(level, x,y-1,DIR_UP,depth+1);
    }

    if(x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid) // check right
    {
        bool door = flip_coin();
        room->doors[DIR_RIGHT] = door;
        if(door)
            generate_rooms(level, x+1,y,DIR_RIGHT,depth+1);
    }

    if(y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid) // check down
    {
        bool door = flip_coin();
        room->doors[DIR_DOWN] = door;
        if(door)
            generate_rooms(level, x,y+1,DIR_DOWN,depth+1);
    }

    if(x > 0 && !level->rooms[x-1][y].valid) // check left
    {
        bool door = flip_coin();
        room->doors[DIR_LEFT] = door;
        if(door)
            generate_rooms(level, x-1,y,DIR_LEFT,depth+1);
    }

    if(depth < MIN_DEPTH)
    {
        if( room->doors[DIR_UP]    == false &&
            room->doors[DIR_RIGHT] == false &&
            room->doors[DIR_DOWN]  == false &&
            room->doors[DIR_LEFT]  == false )
        {
            if(y > 0 && !level->rooms[x][y-1].valid) // check up
            {
                room->doors[DIR_UP] = true;
                generate_rooms(level, x,y-1,DIR_UP,depth+1);
            }
            else if(x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid) // check right
            {
                room->doors[DIR_RIGHT] = true;
                generate_rooms(level, x+1,y,DIR_RIGHT,depth+1);
            }
            else if(y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid) // check down
            {
                room->doors[DIR_DOWN] = true;
                generate_rooms(level, x,y+1,DIR_DOWN,depth+1);
            }
            else if(x > 0 && !level->rooms[x-1][y].valid) // check left
            {
                room->doors[DIR_LEFT] = true;
                generate_rooms(level, x-1,y,DIR_LEFT,depth+1);
            }
        }
    }
}

static void generate_walls(Level* level)
{
    const float wall_offset = 7.0;
    for(int j = 0; j < MAX_ROOMS_GRID_Y; ++j)
    {
        for(int i = 0; i < MAX_ROOMS_GRID_X; ++i)
        {
            Room* room = &level->rooms[i][j];

            if(room->valid)
            {
                int x0 = room_area.x - room_area.w/2.0;
                int y0 = room_area.y - room_area.h/2.0;

                Wall* wall_top = &room->walls[room->wall_count];
                wall_top->p0.x = x0+TILE_SIZE-wall_offset;
                wall_top->p0.y = y0+TILE_SIZE-wall_offset;
                wall_top->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+wall_offset;
                wall_top->p1.y = wall_top->p0.y;
                wall_top->dir = DIR_DOWN;
                room->wall_count++;

                Wall* wall_right = &room->walls[room->wall_count];
                wall_right->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+wall_offset;
                wall_right->p0.y = y0+TILE_SIZE-wall_offset;
                wall_right->p1.x = wall_right->p0.x;
                wall_right->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+wall_offset;
                wall_right->dir = DIR_LEFT;
                room->wall_count++;

                Wall* wall_bottom = &room->walls[room->wall_count];
                wall_bottom->p0.x = x0+TILE_SIZE-wall_offset;
                wall_bottom->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+wall_offset;
                wall_bottom->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+wall_offset;
                wall_bottom->p1.y = wall_bottom->p0.y;
                wall_bottom->dir = DIR_UP;
                room->wall_count++;

                Wall* wall_left = &room->walls[room->wall_count];
                wall_left->p0.x = x0+TILE_SIZE-wall_offset;
                wall_left->p0.y = y0+TILE_SIZE-wall_offset;
                wall_left->p1.x = wall_left->p0.x;
                wall_left->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+wall_offset;
                wall_left->dir = DIR_RIGHT;
                room->wall_count++;

                RoomData* rdata = &room_list[room->layout];

                for(int dir = 0; dir < 4; ++dir)
                {
                    for(int rj = 0; rj < ROOM_TILE_SIZE_Y; ++rj)
                    {
                        for(int ri = 0; ri < ROOM_TILE_SIZE_X; ++ri)
                        {
                            TileType tt = rdata->tiles[ri][rj];

                            if(tt == TILE_PIT || tt == TILE_BOULDER)
                            {
                                TileType check_tile = rdata->tiles[ri][rj];
                                bool is_vertical = false;

                                switch(dir)
                                {
                                    case DIR_UP:    check_tile = rdata->tiles[ri][rj-1]; break;
                                    case DIR_RIGHT: check_tile = rdata->tiles[ri+1][rj]; is_vertical = true; break;
                                    case DIR_DOWN:  check_tile = rdata->tiles[ri][rj+1]; break;
                                    case DIR_LEFT:  check_tile = rdata->tiles[ri-1][rj]; is_vertical = true; break;
                                    default: break;
                                }

                                if(check_tile != TILE_PIT && check_tile != TILE_BOULDER)
                                {
                                    Wall* w = &room->walls[room->wall_count++];

                                    // start at top left
                                    w->p0.x = x0+TILE_SIZE*(ri+1);
                                    w->p0.y = y0+TILE_SIZE*(rj+1);

                                    w->p1.x = x0+TILE_SIZE*(ri+1);
                                    w->p1.y = y0+TILE_SIZE*(rj+1);

                                    switch(dir)
                                    {
                                        case DIR_UP: // top
                                            w->p1.x += TILE_SIZE;
                                            w->dir = DIR_UP;
                                        break;
                                        case DIR_RIGHT: // right
                                            w->p0.x += TILE_SIZE;
                                            w->p1.x += TILE_SIZE;
                                            w->p1.y += TILE_SIZE;
                                            w->dir = DIR_RIGHT;
                                        break;
                                        case DIR_DOWN: // bottom
                                            w->p0.y += TILE_SIZE;
                                            w->p1.x += TILE_SIZE;
                                            w->p1.y += TILE_SIZE;
                                            w->dir = DIR_DOWN;
                                        break;
                                        case DIR_LEFT: // left
                                            w->p1.y += TILE_SIZE;
                                            w->dir = DIR_LEFT;
                                        break;
                                    }

                                }
                            }
                        }
                    }
                }
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
            case 'p': *tile = TILE_PIT; break;
            case 'b': *tile = TILE_BOULDER; break;
            default: break;
        }
    }

    return true;
}

void level_print_room(Room* room)
{
    RoomData* room_data = &room_list[room->layout];
    for(int j = 0; j < ROOM_TILE_SIZE_Y; ++j)
    {
        for(int i = 0; i < ROOM_TILE_SIZE_X; ++i)
        {
            TileType* tile = &room_data->tiles[i][j];
            switch(*tile)
            {
                case TILE_FLOOR:   printf("."); break;
                case TILE_PIT:     printf("p"); break;
                case TILE_BOULDER: printf("b"); break;
                default: printf("%c", *tile+65);break;
            }
        }
        printf("\n");
    }
    printf("\n");
}

void level_handle_room_collision(Room* room, Physics* phys)
{
    if(!room)
        return;

    level_sort_walls(room->walls,room->wall_count, CPOSX(*phys),CPOSY(*phys), phys->radius);
    for(int i = 0; i < room->wall_count; ++i)
    {
        Wall* wall = &room->walls[i];

        bool collision = false;
        bool check = false;
        Vector2f check_point;

        float px = CPOSX(*phys);
        float py = CPOSY(*phys);

        switch(wall->dir)
        {
            case DIR_UP: case DIR_DOWN:
                if(px+phys->radius >= wall->p0.x && px-phys->radius <= wall->p1.x)
                {
                    check_point.x = px;
                    check_point.y = wall->p0.y;
                    check = true;
                }
                break;
            case DIR_LEFT: case DIR_RIGHT:
                if(py+phys->radius >= wall->p0.y && py-phys->radius <= wall->p1.y)
                {
                    check_point.x = wall->p0.x;
                    check_point.y = py;
                    check = true;
                }
                break;
        }

        if(check)
        {
            float d = dist(px, py, check_point.x, check_point.y);
            bool collision = (d < phys->radius);

            if(collision)
            {
                //printf("Collision! player: %f %f. Wall point: %f %f. Dist: %f\n", px, py, check_point.x, check_point.y, d);
                float delta = phys->radius - d + 1.0;
                switch(wall->dir)
                {
                    case DIR_UP:    phys->pos.y -= delta; phys->vel.y = 0.0; break;
                    case DIR_DOWN:  phys->pos.y += delta; phys->vel.y = 0.0; break;
                    case DIR_LEFT:  phys->pos.x -= delta; phys->vel.x = 0.0; break;
                    case DIR_RIGHT: phys->pos.x += delta; phys->vel.x = 0.0; break;
                }

                // add convenient sliding to get around walls
                if(wall->dir == DIR_UP || wall->dir == DIR_DOWN)
                {
                    bool can_slide_left   = px-3 < wall->p0.x;
                    bool can_slide_right  = px+3 > wall->p1.x;

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
                    bool can_slide_up   = py-3 < wall->p0.y;
                    bool can_slide_down = py+3 > wall->p1.y;

                    if(can_slide_up || can_slide_down)
                    {
                        Vector2i tile_coords = level_get_room_coords_by_pos(px, py);

                        tile_coords.x += wall->dir == DIR_LEFT ? +1 : -1;
                        //tile_coords.y += can_slide_up ? -1 : +1;

                        TileType tt = level_get_tile_type(room, tile_coords.x, tile_coords.y);

                        if(tt == TILE_FLOOR)
                        {
                            phys->pos.y += can_slide_up ? -1 : +1;
                        }
                    } 
                }

                memcpy(&phys->target_pos, &phys->pos, sizeof(Vector2f));
            }
        }
    }

}

void level_init()
{
    level_load_room_list();
    dungeon_image = gfx_load_image("src/img/dungeon_set.png", false, true, TILE_SIZE, TILE_SIZE);
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

Rect level_get_tile_rect(int x, int y)
{
    float _x = room_area.x - room_area.w/2.0;
    float _y = room_area.y - room_area.h/2.0;

    _x += (x+1)*TILE_SIZE;
    _y += (y+1)*TILE_SIZE;

    Rect r = RECT((_x+TILE_SIZE/2.0), (_y+TILE_SIZE/2.0), TILE_SIZE, TILE_SIZE);

    return r;
}

TileType level_get_tile_type(Room* room, int x, int y)
{
    if(x < 0 || x >= ROOM_TILE_SIZE_X) return TILE_NONE;
    if(y < 0 || y >= ROOM_TILE_SIZE_Y) return TILE_NONE;

    RoomData* rdata = &room_list[room->layout];
    return rdata->tiles[x][y];
}

TileType level_get_tile_type_by_pos(Room* room, float x, float y)
{
    Vector2i tile_coords = level_get_room_coords_by_pos(x, y);
    return level_get_tile_type(room,tile_coords.x,tile_coords.y);
}

void level_draw_room(Room* room, float xoffset, float yoffset)
{
    if(!room->valid)
        return;

    float x = room_area.x - room_area.w/2.0 + xoffset;
    float y = room_area.y - room_area.h/2.0 + yoffset;
    float w = TILE_SIZE;
    float h = TILE_SIZE;

    Rect r = {(x+w/2.0), (y+h/2.0), w,h};   // top left tile of the room
    uint32_t color = room->color;

    // draw walls
    for(int i = 1; i < ROOM_TILE_SIZE_X+1; ++i) // top
        gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_UP, r.x + i*w,r.y, color, 1.0, 0.0, 1.0, false, true);
    for(int i = 1; i < ROOM_TILE_SIZE_Y+1; ++i) // right
        gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_RIGHT, r.x + (ROOM_TILE_SIZE_X+1)*w,r.y+i*h, color, 1.0, 0.0, 1.0, false, true);
    for(int i = 1; i < ROOM_TILE_SIZE_X+1; ++i) // bottom
        gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_DOWN, r.x + i*w,r.y+(ROOM_TILE_SIZE_Y+1)*h, color, 1.0, 0.0, 1.0, false, true);
    for(int i = 1; i < ROOM_TILE_SIZE_Y+1; ++i) // left
        gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_LEFT, r.x,r.y+i*h, color, 1.0, 0.0, 1.0, false, true);

    // wall corners
    gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_LU, r.x,r.y, color, 1.0, 0.0, 1.0, false, true);
    gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_UR, r.x+(ROOM_TILE_SIZE_X+1)*w,r.y, color, 1.0, 0.0, 1.0, false, true);
    gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_RD, r.x+(ROOM_TILE_SIZE_X+1)*w,r.y+(ROOM_TILE_SIZE_Y+1)*h, color, 1.0, 0.0, 1.0, false, true);
    gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_DL, r.x,r.y+(ROOM_TILE_SIZE_Y+1)*h, color, 1.0, 0.0, 1.0, false, true);

    // draw doors
    if(room->doors[DIR_UP])
        gfx_draw_image(dungeon_image, SPRITE_TILE_DOOR_UP, r.x+w*(ROOM_TILE_SIZE_X+1)/2.0,r.y, color, 1.0, 0.0, 1.0, false, true);
    if(room->doors[DIR_RIGHT])
        gfx_draw_image(dungeon_image, SPRITE_TILE_DOOR_RIGHT, r.x+w*(ROOM_TILE_SIZE_X+1),r.y+h*(ROOM_TILE_SIZE_Y+1)/2.0, color, 1.0, 0.0, 1.0, false, true);
    if(room->doors[DIR_DOWN])
        gfx_draw_image(dungeon_image, SPRITE_TILE_DOOR_DOWN, r.x+w*(ROOM_TILE_SIZE_X+1)/2.0,r.y+h*(ROOM_TILE_SIZE_Y+1), color, 1.0, 0.0, 1.0, false, true);
    if(room->doors[DIR_LEFT])
        gfx_draw_image(dungeon_image, SPRITE_TILE_DOOR_LEFT, r.x,r.y+h*(ROOM_TILE_SIZE_Y+1)/2.0, color, 1.0, 0.0, 1.0, false, true);

    RoomData* rdata = &room_list[room->layout];


    // draw room
    for(int _y = 0; _y < ROOM_TILE_SIZE_Y; ++_y)
    {
        for(int _x = 0; _x < ROOM_TILE_SIZE_X; ++_x)
        {
            TileType tt = rdata->tiles[_x][_y];
            SpriteTileType sprite = SPRITE_TILE_MAX;

            switch(tt)
            {
                case TILE_FLOOR:   sprite = SPRITE_TILE_FLOOR; break;
                case TILE_PIT:     sprite = SPRITE_TILE_PIT; break;
                case TILE_BOULDER: sprite = SPRITE_TILE_BLOCK; break;
                default: break;
            }

            // +1 for walls
            float draw_x = r.x + (_x+1)*w;
            float draw_y = r.y + (_y+1)*h;

            gfx_draw_image(dungeon_image, sprite, draw_x, draw_y, color, 1.0, 0.0, 1.0, false, true);

            if(debug_enabled)
            {
                gfx_draw_rect_xywh(draw_x, draw_y, TILE_SIZE, TILE_SIZE, COLOR_CYAN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
            }
        }
    }
}

Level level_generate(unsigned int seed)
{
    int start_x, start_y;

    Level level = {0};

    srand(seed);

    start_x = RAND_RANGE(1,MAX_ROOMS_GRID_X-2);
    start_y = RAND_RANGE(1,MAX_ROOMS_GRID_Y-2);

    level.start.x = start_x;
    level.start.y = start_y;

    // start_x = MAX_ROOMS_GRID_X / 2; //rand() % MAX_ROOMS_GRID_X;
    // start_y = MAX_ROOMS_GRID_Y / 2; //rand() % MAX_ROOMS_GRID_Y;

    creature_clear_all();

    LOGI("Generating rooms, seed: %u", seed);

    // memset(level->rooms,0, sizeof(level->rooms));
    generate_rooms(&level, level.start.x, level.start.y, DIR_NONE, 0);
    generate_walls(&level);

    return level;
}

void level_sort_walls(Wall* walls, int wall_count, float x, float y, float radius)
{
    if(!walls)
        return;

    // calculate and store distance to player
    for(int i = 0; i < wall_count; ++i)
    {
        Wall* wall = &walls[i];

        if(wall->dir == DIR_UP || wall->dir == DIR_DOWN)
        {
            if(x+radius < wall->p0.x)
                wall->distance_to_player = dist(x,y, wall->p0.x, wall->p0.y);
            else if(x-radius > wall->p1.x)
                wall->distance_to_player = dist(x,y, wall->p1.x, wall->p1.y);
            else
                wall->distance_to_player = ABS(y - wall->p0.y);
        }
        else if(wall->dir == DIR_LEFT || wall->dir == DIR_RIGHT)
        {
            if(y+radius < wall->p0.y)
                wall->distance_to_player = dist(x,y, wall->p0.x, wall->p0.y);
            else if(y-radius > wall->p1.y)
                wall->distance_to_player = dist(x,y, wall->p1.x, wall->p1.y);
            else
                wall->distance_to_player = ABS(x - wall->p0.x);
        }
    }

    // insertion sort
    int i, j;
    Wall key;

    for (i = 1; i < wall_count; ++i) 
    {
        memcpy(&key, &walls[i], sizeof(Wall));
        j = i - 1;

        while (j >= 0 && walls[j].distance_to_player < key.distance_to_player)
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

char* level_tile_type_to_str(TileType tt)
{
    switch(tt)
    {
        case TILE_NONE: return "None";
        case TILE_FLOOR: return "Floor";
        case TILE_BOULDER: return "Block";
        case TILE_PIT: return "Pit";
    }
    return "Unknown";
}

Room* level_get_room(Level* level, int x, int y)
{
    if(!level_is_room_valid(level, x, y))
        return NULL;
    return &level->rooms[x][y];
}

Room* level_get_room_by_index(Level* level, int index)
{
    Vector2i roomxy = level_get_room_coords(index);
    return level_get_room(level, roomxy.x, roomxy.y);
}

uint8_t level_get_room_index(int x, int y)
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
    int x0 = room_area.x - room_area.w/2.0;
    int y0 = room_area.y - room_area.h/2.0;

    int _x = (x - x0) / TILE_SIZE;
    int _y = (y - y0) / TILE_SIZE;

    Vector2i r = {_x-1,_y-1};
    return r;
}

bool level_is_room_valid(Level* level, int x, int y)
{
    if(x >= MAX_ROOMS_GRID_X || y >= MAX_ROOMS_GRID_Y)
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

// grid x,y offsets
Vector2i get_dir_offsets(Dir door)
{
    Vector2i o = {0};
    switch(door)
    {
        case DIR_UP:
            o.y = -1;
            break;
        case DIR_DOWN:
            o.y = 1;
            break;
        case DIR_LEFT:
            o.x = -1;
            break;
        case DIR_RIGHT:
            o.x = 1;
            break;
        default:
            break;
    }
    return o;
}

const char* get_dir_name(Dir door)
{
    switch(door)
    {
        case DIR_UP: return "up";
        case DIR_DOWN: return "down";
        case DIR_LEFT: return "left";
        case DIR_RIGHT: return "right";
        default: return "?";
    }
}
