#include "headers.h"
#include "core/io.h"
#include "core/gfx.h"
#include "main.h"
#include "creature.h"
#include "item.h"
#include "room_file.h"
#include "entity.h"
#include "physics.h"

RoomFileData room_list[32] = {0};
int room_list_count = 0;
int dungeon_image = -1;

static void generate_rooms(Level* level, int x, int y, Dir came_from, int depth);

static int room_list_monster[32] = {0};
static int room_list_empty[32] = {0};
static int room_list_treasure[32] = {0};
static int room_list_boss[32] = {0};

static int room_count_monster  = 0;
static int room_count_empty    = 0;
static int room_count_treasure = 0;
static int room_count_boss     = 0;

static bool min_depth_reached = false;

static inline bool flip_coin()
{
    return (rand()%2==0);
}

static void branch_room(Level* level, int x, int y, int depth)
{
    //printf("branching room!\n");

    Room* room = &level->rooms[x][y];
    RoomFileData* rfd = &room_list[room->layout];

    bool can_go_up    = rfd->doors[DIR_UP] && (y > 0 && !level->rooms[x][y-1].valid);
    bool can_go_right = rfd->doors[DIR_RIGHT] && (x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid);
    bool can_go_down  = rfd->doors[DIR_DOWN] && (y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid);
    bool can_go_left  = rfd->doors[DIR_LEFT] && (x > 0 && !level->rooms[x-1][y].valid);

    int tries = 0;

try_again:
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

    if(random_doors[DIR_UP])
    {
        room->doors[DIR_UP] = true;
        generate_rooms(level, x,y-1,DIR_UP,depth+1);
    }

    if(random_doors[DIR_RIGHT])
    {
        room->doors[DIR_RIGHT] = true;
        generate_rooms(level, x+1,y,DIR_RIGHT,depth+1);
    }

    if(random_doors[DIR_DOWN])
    {
        room->doors[DIR_DOWN] = true;
        generate_rooms(level, x,y+1,DIR_DOWN,depth+1);
    }

    if(random_doors[DIR_LEFT])
    {
        room->doors[DIR_LEFT] = true;
        generate_rooms(level, x-1,y,DIR_LEFT,depth+1);
    }
}

static int get_rand_room_index(RoomType type, Dir came_from)
{
    int list_count = 0;
    int* list;
    int index = 0;

    switch(type)
    {
        case ROOM_TYPE_BOSS:
            index = room_list_boss[rand() % room_count_boss];
            list_count = room_count_boss;
            list = room_list_boss;
            break;
        case ROOM_TYPE_TREASURE:
            index = room_list_treasure[rand() % room_count_treasure];
            list_count = room_count_treasure;
            list = room_list_treasure;
            break;
        case ROOM_TYPE_MONSTER:
            index = room_list_monster[rand() % room_count_monster];
            list_count = room_count_monster;
            list = room_list_monster;
            break;
        case ROOM_TYPE_EMPTY:
            index = room_list_empty[rand() % room_count_empty];
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
    Room* room = &level->rooms[x][y];

    if(room->valid)
        goto exit_conditions; // shouldn't happen

    level->num_rooms++;

    room->valid = true;
    room->discovered = false;
    room->color = COLOR((rand() % 159) + 96,(rand() % 159) + 96,(rand() % 159) + 96);
    room->index = level_get_room_index(x,y);

    printf("adding room, depth: %d\n",depth);

    switch(came_from)
    {
        case DIR_UP:    room->doors[DIR_DOWN]  = true; break;
        case DIR_RIGHT: room->doors[DIR_LEFT]  = true; break;
        case DIR_DOWN:  room->doors[DIR_UP]    = true; break;
        case DIR_LEFT:  room->doors[DIR_RIGHT] = true; break;
        default: break;
    }

    bool is_start_room = (x == level->start.x && y == level->start.y);

    if(is_start_room)
    {
        room->type   = ROOM_TYPE_EMPTY;
        room->layout = 0;
        goto exit_conditions;
    }

    bool is_boss_room = !level->has_boss_room && ((level->num_rooms == MAX_ROOMS) || (level->num_rooms > 4 && (rand()%4 == 0)));

    if(is_boss_room)
    {
        room->type = ROOM_TYPE_BOSS;
        room->layout = get_rand_room_index(ROOM_TYPE_BOSS, came_from);
        room->color = COLOR(100,100,200);
        level->has_boss_room = true;
        goto exit_conditions;
    }

    bool is_treasure_room = !level->has_treasure_room && ((level->num_rooms == MAX_ROOMS) || (level->num_rooms > 4 && (rand()%4 == 0)));

    if(is_treasure_room)
    {
        room->type = ROOM_TYPE_TREASURE;
        room->layout = get_rand_room_index(ROOM_TYPE_TREASURE, came_from);
        room->color = COLOR(200,200,100);
        level->has_treasure_room = true;
        goto exit_conditions;
    }

    bool is_monster_room = (rand() % 100 < MONSTER_ROOM_PERCENTAGE);

    if(is_monster_room)
    {
        room->type = ROOM_TYPE_MONSTER;
        room->layout = get_rand_room_index(ROOM_TYPE_MONSTER, came_from);
        room->color = COLOR(200,100,100);

        // add monsters
        RoomFileData* rfd = &room_list[room->layout];
        for(int i = 0; i < rfd->creature_count; ++i)
        {
            Vector2i g = {rfd->creature_locations_x[i], rfd->creature_locations_y[i]};
            g.x--; g.y--;
            Creature* c = creature_add(room, rfd->creature_types[i], &g, NULL);
        }
    }
    else
    {
        // empty room
        room->type = ROOM_TYPE_EMPTY;
        room->layout = get_rand_room_index(ROOM_TYPE_EMPTY, came_from);
        room->color = COLOR(200,200,200);
    }

exit_conditions:

    RoomFileData* rfd = &room_list[room->layout];

    for(int i = 0; i < rfd->item_count; ++i)
    {
        Vector2f pos = level_get_pos_by_room_coords(rfd->item_locations_x[i], rfd->item_locations_y[i]);

        printf("Adding item of type: %s to (%f %f)\n", item_get_name(rfd->item_types[i]), pos.x, pos.y);
        item_add(rfd->item_types[i], pos.x, pos.y, room->index);
    }

    if(level->num_rooms >= MAX_ROOMS)
        return;

    if(room->type == ROOM_TYPE_BOSS)
        return; // boss room is a leaf node

    if(room->type == ROOM_TYPE_TREASURE)
        return; // treasure room is a leaf node

    // branch
    branch_room(level,x,y,depth);
}


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
    wall_top->is_innner_wall = true;
    room->wall_count++;

    Wall* wall_right = &room->walls[room->wall_count];
    wall_right->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+wall_offset;
    wall_right->p0.y = y0-wall_offset;
    wall_right->p1.x = wall_right->p0.x;
    wall_right->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2)+wall_offset;
    wall_right->dir = DIR_LEFT;
    wall_right->is_innner_wall = true;
    room->wall_count++;

    Wall* wall_bottom = &room->walls[room->wall_count];
    wall_bottom->p0.x = x0-wall_offset;
    wall_bottom->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+wall_offset;
    wall_bottom->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2)+wall_offset;
    wall_bottom->p1.y = wall_bottom->p0.y;
    wall_bottom->dir = DIR_UP;
    wall_bottom->is_innner_wall = true;
    room->wall_count++;

    Wall* wall_left = &room->walls[room->wall_count];
    wall_left->p0.x = x0+TILE_SIZE-wall_offset;
    wall_left->p0.y = y0-wall_offset;
    wall_left->p1.x = wall_left->p0.x;
    wall_left->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2)+wall_offset;
    wall_left->dir = DIR_RIGHT;
    wall_left->is_innner_wall = true;
    room->wall_count++;

    // outer walls
    Wall* wall_outer_top = &room->walls[room->wall_count];
    wall_outer_top->p0.x = x0;
    wall_outer_top->p0.y = y0;
    wall_outer_top->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
    wall_outer_top->p1.y = wall_outer_top->p0.y;
    wall_outer_top->dir = DIR_DOWN;
    room->wall_count++;

    Wall* wall_outer_right = &room->walls[room->wall_count];
    wall_outer_right->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
    wall_outer_right->p0.y = y0;
    wall_outer_right->p1.x = wall_outer_right->p0.x;
    wall_outer_right->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
    wall_outer_right->dir = DIR_LEFT;
    room->wall_count++;

    Wall* wall_outer_bottom = &room->walls[room->wall_count];
    wall_outer_bottom->p0.x = x0;
    wall_outer_bottom->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
    wall_outer_bottom->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
    wall_outer_bottom->p1.y = wall_outer_bottom->p0.y;
    wall_outer_bottom->dir = DIR_UP;
    room->wall_count++;

    Wall* wall_outer_left = &room->walls[room->wall_count];
    wall_outer_left->p0.x = x0;
    wall_outer_left->p0.y = y0;
    wall_outer_left->p1.x = wall_outer_left->p0.x;
    wall_outer_left->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
    wall_outer_left->dir = DIR_RIGHT;
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
        room->wall_count++;

        Wall* wall_door_up2 = &room->walls[room->wall_count];
        wall_door_up2->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_up2->p0.y = y0;
        wall_door_up2->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_up2->p1.y = y0+TILE_SIZE-7;
        wall_door_up2->dir = DIR_RIGHT;
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
        room->wall_count++;

        Wall* wall_door_right2 = &room->walls[room->wall_count];
        wall_door_right2->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+1)+7;
        wall_door_right2->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_right2->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X+2);
        wall_door_right2->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_right2->dir = DIR_DOWN;
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
        room->wall_count++;

        Wall* wall_door_down2 = &room->walls[room->wall_count];
        wall_door_down2->p0.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_down2->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+1)+7;
        wall_door_down2->p1.x = x0+TILE_SIZE*(ROOM_TILE_SIZE_X/2+2);
        wall_door_down2->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y+2);
        wall_door_down2->dir = DIR_RIGHT;
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
        room->wall_count++;

        Wall* wall_door_left2 = &room->walls[room->wall_count];
        wall_door_left2->p0.x = x0;
        wall_door_left2->p0.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_left2->p1.x = x0+TILE_SIZE-7;
        wall_door_left2->p1.y = y0+TILE_SIZE*(ROOM_TILE_SIZE_Y/2+2);
        wall_door_left2->dir = DIR_DOWN;
        room->wall_count++;
    }
}

static void generate_walls(Level* level)
{
    const float wall_offset = 7.0;
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

                                    if(tt == TILE_PIT)
                                    {
                                        w->is_pit = true;
                                    }

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

    RoomFileData* room_data = &room_list[0];

    int tile_x = 0;
    int tile_y = 0;

    int pc1 = 0, pc2 = 0;
    int c = 0;

    room_list_count = 0;
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

        TileType* tile = (TileType*)&room_data->tiles[tile_x++][tile_y];

        switch(c)
        {
            case '.': *tile = TILE_FLOOR; break;
            case 'p': *tile = TILE_PIT; break;
            case 'b': *tile = TILE_BOULDER; break;
            case 'm': *tile = TILE_MUD; break;
            default: break;
        }
    }

    return true;
}

void level_print_room(Room* room)
{
    RoomFileData* room_data = &room_list[room->layout];

    for(int j = 0; j < ROOM_TILE_SIZE_Y; ++j)
    {
        for(int i = 0; i < ROOM_TILE_SIZE_X; ++i)
        {
            TileType* tile = (TileType*)&room_data->tiles[i][j];
            switch(*tile)
            {
                case TILE_FLOOR:   printf("."); break;
                case TILE_PIT:     printf("p"); break;
                case TILE_BOULDER: printf("b"); break;
                case TILE_MUD:     printf("m"); break;
                default: printf("%c", *tile+65);break;
            }
        }
        printf("\n");
    }
    printf("\n");
}

void level_handle_room_collision(Room* room, Physics* phys, int entity_type)
{
    if(!room)
        return;

    level_sort_walls(room->walls,room->wall_count, CPOSX(*phys),CPOSY(*phys), phys->radius);

    for(int i = 0; i < room->wall_count; ++i)
    {
        Wall* wall = &room->walls[i];

        if(entity_type == ENTITY_TYPE_PROJECTILE && (wall->is_pit || wall->is_innner_wall))
            continue;

        if(entity_type == ENTITY_TYPE_PLAYER && wall->is_pit && phys->pos.z > 0)
            continue;

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
                if(phys->amorphous)
                {
                    phys->dead = true;
                    return;
                }

                //printf("Collision! player: %f %f. Wall point: %f %f. Dist: %f\n", px, py, check_point.x, check_point.y, d);
                float delta = phys->radius - d + 0.1;
                switch(wall->dir)
                {
                    case DIR_UP:    phys->pos.y -= delta; phys->vel.y *= -phys->elasticity; break;
                    case DIR_DOWN:  phys->pos.y += delta; phys->vel.y *= -phys->elasticity; break;
                    case DIR_LEFT:  phys->pos.x -= delta; phys->vel.x *= -phys->elasticity; break;
                    case DIR_RIGHT: phys->pos.x += delta; phys->vel.x *= -phys->elasticity; break;
                }

                // add convenient sliding to get around walls

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
            }
        }
    }
}

void level_init()
{
    if(dungeon_image > 0) return;

    //level_load_room_list();
    dungeon_image = gfx_load_image("src/img/dungeon_set.png", false, true, TILE_SIZE, TILE_SIZE);

    room_file_get_all();

    for(int i = 0; i < room_file_count; ++i)
    {
        printf("i: %d, Loading room %s\n",i, p_room_files[i]);

        bool success = room_file_load(&room_list[room_list_count], "src/rooms/%s", p_room_files[i]);
        if(!success)
            continue;

        room_list_count++;
    }
    printf("room list count: %d\n",room_list_count);
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

uint8_t level_get_tile_sprite(TileType tt)
{
    SpriteTileType sprite = SPRITE_TILE_MAX;
    switch(tt)
    {
        case TILE_FLOOR:   sprite = SPRITE_TILE_FLOOR; break;
        case TILE_PIT:     sprite = SPRITE_TILE_PIT;   break;
        case TILE_BOULDER: sprite = SPRITE_TILE_BLOCK; break;
        case TILE_MUD:     sprite = SPRITE_TILE_MUD;   break;
        default: break;
    }
    return (uint8_t)sprite;
}

TileType level_get_tile_type(Room* room, int x, int y)
{
    if(x < 0 || x >= ROOM_TILE_SIZE_X) return TILE_NONE;
    if(y < 0 || y >= ROOM_TILE_SIZE_Y) return TILE_NONE;

    RoomFileData* rdata = &room_list[room->layout];
    return rdata->tiles[x][y];
}

TileType level_get_tile_type_by_pos(Room* room, float x, float y)
{
    Vector2i tile_coords = level_get_room_coords_by_pos(x, y);
    return level_get_tile_type(room, tile_coords.x, tile_coords.y);
}


void level_get_center_floor_tile(Room* room, Vector2i* tile_coords, Vector2f* tile_pos)
{
    for(int x = 0; x < ROOM_TILE_SIZE_X; ++x)
    {
        int _x = (ROOM_TILE_SIZE_X-1)/2;
        _x += ((x % 2 == 0) ? -1 : 1) * x/2;

        for(int y = 0; y < ROOM_TILE_SIZE_Y; ++y)
        {
            int _y = (ROOM_TILE_SIZE_Y-1)/2;
            _y += ((y % 2 == 0) ? -1 : 1) * y/2;

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
                    // printf("setting tile_pos: %.2f, %.2f\n", tile_pos->x, tile_pos->y);
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

    uint8_t sprite_door_up = SPRITE_TILE_DOOR_UP;
    uint8_t sprite_door_right = SPRITE_TILE_DOOR_RIGHT;
    uint8_t sprite_door_down = SPRITE_TILE_DOOR_DOWN;
    uint8_t sprite_door_left = SPRITE_TILE_DOOR_LEFT;
    if(room->doors_locked)
    {
        sprite_door_up = SPRITE_TILE_DOOR_UP_CLOSED;
        sprite_door_right = SPRITE_TILE_DOOR_RIGHT_CLOSED;
        sprite_door_down = SPRITE_TILE_DOOR_DOWN_CLOSED;
        sprite_door_left = SPRITE_TILE_DOOR_LEFT_CLOSED;
    }

    // draw doors
    if(room->doors[DIR_UP])
        gfx_draw_image(dungeon_image, sprite_door_up, r.x+w*(ROOM_TILE_SIZE_X+1)/2.0,r.y, color, 1.0, 0.0, 1.0, false, true);
    if(room->doors[DIR_RIGHT])
        gfx_draw_image(dungeon_image, sprite_door_right, r.x+w*(ROOM_TILE_SIZE_X+1),r.y+h*(ROOM_TILE_SIZE_Y+1)/2.0, color, 1.0, 0.0, 1.0, false, true);
    if(room->doors[DIR_DOWN])
        gfx_draw_image(dungeon_image, sprite_door_down, r.x+w*(ROOM_TILE_SIZE_X+1)/2.0,r.y+h*(ROOM_TILE_SIZE_Y+1), color, 1.0, 0.0, 1.0, false, true);
    if(room->doors[DIR_LEFT])
        gfx_draw_image(dungeon_image, sprite_door_left, r.x,r.y+h*(ROOM_TILE_SIZE_Y+1)/2.0, color, 1.0, 0.0, 1.0, false, true);

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

            gfx_draw_image(dungeon_image, sprite, draw_x, draw_y, color, 1.0, 0.0, 1.0, false, true);

            if(debug_enabled && show_tile_grid)
            {
                gfx_draw_rect_xywh(draw_x, draw_y, TILE_SIZE, TILE_SIZE, COLOR_CYAN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
            }
        }
    }
}

void room_draw_walls(Room* room)
{
    for(int i = 0; i < room->wall_count; ++i)
    {
        Wall* wall = &room->walls[i];

        float x = wall->p0.x;
        float y = wall->p0.y;
        float w = ABS(wall->p0.x - wall->p1.x)+1;
        float h = ABS(wall->p0.y - wall->p1.y)+1;

        gfx_draw_rect_xywh_tl(x, y, w, h, COLOR_WHITE, NOT_SCALED, NO_ROTATION, FULL_OPACITY, true, IN_WORLD);
    }
}

void level_generate(Level* level, unsigned int seed, int rank)
{
    // seed PRNG
    srand(seed);

    LOGI("Generating level, seed: %u", seed);

    // start in center of grid
    level->start.x = floor(MAX_ROOMS_GRID_X/2); //RAND_RANGE(1,MAX_ROOMS_GRID_X-2);
    level->start.y = floor(MAX_ROOMS_GRID_Y/2); //RAND_RANGE(1,MAX_ROOMS_GRID_Y-2);

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

    printf("Total Rooms: %d\n", room_list_count);
    printf("   # monster rooms: %d\n",room_count_monster);
    printf("   # empty rooms: %d\n",room_count_empty);
    printf("   # treasure rooms: %d\n",room_count_treasure);
    printf("   # boss rooms: %d\n",room_count_boss);

    item_clear_all();
    creature_clear_all();
    min_depth_reached = false;

    LOGI("Generating rooms, seed: %u", seed);

    generate_rooms(level, level->start.x, level->start.y, DIR_NONE, 0);

    for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
    {
        for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
        {
            uint8_t index = level_get_room_index(x,y);
            level->rooms[x][y].index = index;
            level->rooms[x][y].doors_locked = (creature_get_room_count(index) != 0);
            level->rooms[x][y].xp = 0;
        }
    }

    generate_walls(level);
    level_print(level);
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


Room* level_get_room(Level* level, int x, int y)
{
    if(!level_is_room_valid(level, x, y))
    {
        //LOGE("Room not valid, %d %d",x,y);
        return NULL;
    }
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
    float x0 = room_area.x - room_area.w/2.0;
    float y0 = room_area.y - room_area.h/2.0;

    float xd = x - x0;
    float yd = y - y0;

    int _x = xd / TILE_SIZE;
    int _y = yd / TILE_SIZE;

    // fix issue with casting float to int and rounding improperly
    if(xd < 0)
        _x--;
    if(yd < 0)
        _y--;

    Vector2i r = {_x-1,_y-1};
    return r;
}

Vector2f level_get_pos_by_room_coords(int x, int y)
{
    float x0 = room_area.x - room_area.w/2.0;
    float y0 = room_area.y - room_area.h/2.0;

    int xd = (x+1) * TILE_SIZE;
    int yd = (y+1) * TILE_SIZE;

    float _x = xd + x0 - (TILE_SIZE/2.0);
    float _y = yd + y0 - (TILE_SIZE/2.0);

    Vector2f r = {_x,_y};
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
