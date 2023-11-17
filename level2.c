
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
    Room* room = &level->rooms[x][y];

    bool can_go_up    = (y > 0 && !level->rooms[x][y-1].valid);
    bool can_go_right = (x < MAX_ROOMS_GRID_X-1 && !level->rooms[x+1][y].valid);
    bool can_go_down  = (y < MAX_ROOMS_GRID_Y-1 && !level->rooms[x][y+1].valid);
    bool can_go_left  = (x > 0 && !level->rooms[x-1][y].valid);

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
        if(door_count == 0)
            goto try_again;
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

static void generate_rooms(Level* level, int x, int y, Dir came_from, int depth)
{
    Room* room = &level->rooms[x][y];

    if(!room->valid)
    {
        level->num_rooms++;

        room->valid = true;
        room->discovered = false;
        room->color = COLOR((rand() % 159) + 96,(rand() % 159) + 96,(rand() % 159) + 96);
        room->index = level_get_room_index(x,y);

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

        bool is_boss_room     = !level->has_boss_room && ((level->num_rooms == MAX_ROOMS) || (level->num_rooms > 4 && (rand()%4 == 0)));

        if(is_boss_room)
        {
            room->type = ROOM_TYPE_BOSS;
            room->layout = room_list_boss[rand() % room_count_boss];
            room->color = COLOR(100,100,200);
            level->has_boss_room = true;
            goto exit_conditions;
        }

        bool is_treasure_room = !level->has_treasure_room && ((level->num_rooms == MAX_ROOMS) || (level->num_rooms > 4 && (rand()%4 == 0)));

        if(is_treasure_room)
        {
            room->type = ROOM_TYPE_TREASURE;
            room->layout = room_list_treasure[rand() % room_count_treasure];
            room->color = COLOR(200,200,100);
            level->has_treasure_room = true;
            goto exit_conditions;
        }

        bool is_monster_room = (rand() % 100 < MONSTER_ROOM_PERCENTAGE);

        if(is_monster_room)
        {
            room->type = ROOM_TYPE_MONSTER;
            room->layout = room_list_monster[rand() % room_count_monster];
            room->color = COLOR(200,200,200);

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
            room->layout = room_list_empty[rand() % room_count_empty];
            room->color = COLOR(200,200,200);
        }
    }

exit_conditions:

    if(level->num_rooms >= MAX_ROOMS)
        return;

    if(room->type == ROOM_TYPE_BOSS)
        return; // boss room is a leaf node

    if(room->type == ROOM_TYPE_TREASURE)
        return; // treasure room is a leaf node

    // branch
    branch_room(level,x,y,depth);
}

Level level_generate(unsigned int seed, int rank)
{
    // seed PRNG
    srand(seed);

    LOGI("Generating level, seed: %u", seed);

    Level level = {0};

    // start in center of grid
    level.start.x = floor(MAX_ROOMS_GRID_X/2); //RAND_RANGE(1,MAX_ROOMS_GRID_X-2);
    level.start.y = floor(MAX_ROOMS_GRID_y/2); //RAND_RANGE(1,MAX_ROOMS_GRID_Y-2);

    // fill out helpful room information
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

    creature_clear_all();
    min_depth_reached = false;

    generate_rooms(&level, level.start.x, level.start.y, DIR_NONE, 0);
    generate_walls(&level);

    level_print(&level);

    return level;
}
