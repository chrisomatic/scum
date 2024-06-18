#include "headers.h"

#include "core/imgui.h"
#include "core/particles.h"
#include "core/files.h"

#include "window.h"
#include "gfx.h"
#include "log.h"
#include "player.h"
#include "creature.h"
#include "projectile.h"
#include "effects.h"
#include "editor.h"
#include "main.h"
#include "level.h"
#include "room_file.h"
#include "camera.h"

enum
{
    EDITOR_KEY_UP,
    EDITOR_KEY_DOWN,
    EDITOR_KEY_LEFT,
    EDITOR_KEY_RIGHT,
    EDITOR_KEY_ROTATE,

    EDITOR_KEY_MAX
};
static PlayerInput editor_keys[EDITOR_KEY_MAX] = {0};

static bool lmouse_state = false;
static bool rmouse_state = false;

typedef enum
{
    TYPE_NONE,
    TYPE_ITEM,
    TYPE_CREATURE,
} PlacedType;

static int orientation = 0;
static float orientation_deg = 0.0;

// orientation
// 0: 0.0
// 1: 90.0
// 2: 180.0
// 3: 270.0

typedef struct
{
    PlacedType type;
    int subtype;
    int subtype2;
    int orientation;
} PlacedObject;


static PlacedObject objects[OBJECTS_MAX_X][OBJECTS_MAX_Y] = {0};
RoomFileData room_data_editor = {0};
static Room room = {0};

static PlacedObject objects_prior[OBJECTS_MAX_X][OBJECTS_MAX_Y] = {0};

static float ecam_pos_x = 0;
static float ecam_pos_y = 0;
static int ecam_pos_z = 23;

static int tab_sel = 0;
static int obj_sel = 0;

static int tile_sel = 1;

#define NUM_DOOR_TYPES 2
static char* door_names[NUM_DOOR_TYPES] = {0};
static int door_sel;

static int creature_sel = 0;
static int item_sel = 0;

static int room_rank = 0;
static char room_file_name[32] = {0};

static char* room_type_names[ROOM_TYPE_MAX] = {0};
static int room_type_sel = 0;

Vector2i tile_coords = {0}; // mouse tile coords
Vector2i obj_coords = {0}; // tile_coords translated to object grid

Rect gui_size = {0};

static float get_orientation_deg(int _orientation);
static bool check_no_door_placement();
static Vector2i get_door_coords(Dir door);
static Vector2i get_in_front_of_door_coords(Dir door);
static Dir match_door_coords(int x, int y);
static Dir match_in_front_of_door_coords(int x, int y);

static void editor_camera_set(bool immediate);
static void clear_all();

void room_editor_init()
{
    static bool _init = false;

    if(!_init)
    {
        item_sel = 0;
        for(int i = 0; i < ITEM_MAX; ++i)
        {
            item_names[i] = (char*)item_get_name(i);
        }

        creature_sel = 0;
        for(int i = 0; i < CREATURE_TYPE_MAX; ++i)
            creature_names[i] = (char*)creature_type_name(i);

        tile_sel = 1;
        for(int i = 0; i < TILE_MAX; ++i)
        {
            tile_names[i] = (char*)get_tile_name(i);
        }

        door_sel = 0;
        door_names[0] = "Possible Door";
        door_names[1] = "No Door";

        room_type_sel = 0;
        for(int i = 0; i < ROOM_TYPE_MAX; ++i)
            room_type_names[i] = (char*)get_room_type_name(i);

        clear_all();

        _init = true;
    }
}

void room_editor_start()
{
    show_tile_grid = true;
    debug_enabled = true;

    ecam_pos_x = CENTER_X;
    ecam_pos_y = CENTER_Y;

    room.valid = true;
    room.color = COLOR_TINT_NONE;
    room.wall_count = 0;
    level_generate_room_outer_walls(&room);

    window_controls_clear_keys();
    window_controls_add_key(&editor_keys[EDITOR_KEY_UP].state, GLFW_KEY_W);
    window_controls_add_key(&editor_keys[EDITOR_KEY_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&editor_keys[EDITOR_KEY_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&editor_keys[EDITOR_KEY_RIGHT].state, GLFW_KEY_D);
    window_controls_add_key(&editor_keys[EDITOR_KEY_ROTATE].state, GLFW_KEY_R);

    for(int i = 0;  i < EDITOR_KEY_MAX; ++i)
        memset(&editor_keys[i], 0, sizeof(PlayerInput));

    editor_camera_set(true);

    // load room files
    // room_file_get_all();
    room_file_load_all(true);
}

static RoomFileData loaded_rfd = {0};


static void draw_room_file_gui()
{
    static int prior_room_file_sel = 0;
    static int room_file_sel = 0;
    static int room_file_sel_index_map[MAX_ROOM_LIST_COUNT] = {0}; // filtered list mapped to room_list index
    static char* filtered_room_files[256] = {0};
    static int filtered_room_files_count = 0;
    static char file_filter_str[32] = {0};
    static char selected_room_name_str[100] = {0};
    static char selected_room_name[32] = {0};
    static int selected_rank = 0;
    static int selected_room_type = 0;

    static bool force_reload = false;
    static bool should_load_file = true;
    static bool are_you_sure = false;
    static bool file_exists = false;

    static Rect room_files_panel_rect = {0};

    imgui_begin_panel("Files",1,1,true);

        const float big = 16.0;
        imgui_text_sized(big,"Filter");

        imgui_horizontal_begin();
        char* buttons[] = {"*", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
        selected_rank = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "Rank");
        imgui_horizontal_end();

        imgui_horizontal_begin();
        char* buttons2[] = {"All", "Empty", "Monster", "Treasure", "Boss", "Shrine", "Library"};
        selected_room_type = imgui_button_select(IM_ARRAYSIZE(buttons2), buttons2, "Room Type");
        imgui_horizontal_end();

        float opacity_scale = imgui_is_mouse_inside() ? 1.0 : 0.4;
        imgui_set_global_opacity_scale(opacity_scale);
        imgui_horizontal_begin();
        imgui_text_box("##FilterText",file_filter_str,IM_ARRAYSIZE(file_filter_str));
        if(imgui_button("Clear")) memset(file_filter_str,0,32);
        imgui_horizontal_end();

        bool file_diff = room_file_load_all(force_reload);
        force_reload = false;

        int _filtered_room_files_count = 0;
        int _room_file_sel_index_map[MAX_ROOM_LIST_COUNT] = {0};
        for(int i = 0; i < room_list_count; ++i)
        {
            RoomFileData* rfd = &room_list[i];
            bool match_rank        = selected_rank == 0 ? true : (rfd->rank == selected_rank);
            bool match_room_type   = selected_room_type == 0 ? true : (rfd->type == selected_room_type-1);
            bool match_filter_text = strstr(p_room_files[i],file_filter_str);

            if(match_rank && match_room_type && match_filter_text)
            {
                _room_file_sel_index_map[_filtered_room_files_count] = i;
                _filtered_room_files_count++;
            }
        }

        // rebuild the filtered list
        if(file_diff || _filtered_room_files_count != filtered_room_files_count || memcmp(_room_file_sel_index_map, room_file_sel_index_map, sizeof(int)*MAX_ROOM_LIST_COUNT) != 0)
        {
            filtered_room_files_count = _filtered_room_files_count;
            memcpy(room_file_sel_index_map, _room_file_sel_index_map, sizeof(int)*MAX_ROOM_LIST_COUNT);
            for(int i = 0; i < filtered_room_files_count; ++i)
            {
                filtered_room_files[i] = p_room_files[room_file_sel_index_map[i]];
            }

            room_file_sel = 0;
            if(file_exists)
            {
                should_load_file = true;
                for(int i = 0; i < filtered_room_files_count; ++i)
                {
                    if(strcmp(selected_room_name, filtered_room_files[i]) == 0)
                    {
                        // no need to reload
                        should_load_file = false;
                        room_file_sel = i;
                        break;
                    }
                }
            }
        }

        prior_room_file_sel = room_file_sel;
        imgui_listbox(filtered_room_files, filtered_room_files_count, "##files_listbox", &room_file_sel, 15);

        if(filtered_room_files[room_file_sel])
            snprintf(selected_room_name_str,100,"Selected: %s",filtered_room_files[room_file_sel]);

        strcpy(selected_room_name, filtered_room_files[room_file_sel]);

        imgui_text(selected_room_name_str);

        if(imgui_button("Delete"))
            are_you_sure = true;

        if(are_you_sure)
        {
            imgui_text("Are you sure?");

            imgui_horizontal_begin();

            if(imgui_button("No"))
                are_you_sure = false;

            if(imgui_button("Yes"))
            {
                // delete file
                char file_name[100] = {0};
                snprintf(file_name,100,"src/rooms/%s",filtered_room_files[room_file_sel]);
                remove(file_name);
                force_reload = true;
                are_you_sure = false;
            }

            imgui_horizontal_end();
        }

        if(prior_room_file_sel != room_file_sel || should_load_file)
        {
            should_load_file = false;

            clear_all();
            // memcpy(loaded_rfd, )
            room_file_load(&loaded_rfd, true, true, "src/rooms/%s", filtered_room_files[room_file_sel]);
            // printf("1  %s\n", filtered_room_files[room_file_sel]);
            // printf("2  %s\n", p_room_files[room_file_sel_index_map[room_file_sel]]);

            // p_room_files
            // loaded_rfd.file_index

            // set properties
            room_type_sel = loaded_rfd.type;
            room_rank = loaded_rfd.rank;

            // tiles
            for(int i = 0; i < loaded_rfd.size_x; ++i)
                for(int j = 0; j < loaded_rfd.size_y; ++j)
                    room_data_editor.tiles[i][j] = (TileType)loaded_rfd.tiles[i][j];

            const int adj = 0;
            for(int i = 0; i < loaded_rfd.creature_count; ++i)
            {
                int x = loaded_rfd.creature_locations_x[i];
                int y = loaded_rfd.creature_locations_y[i];
                int ori = loaded_rfd.creature_orientations[i];

                objects[x][y].type     = TYPE_CREATURE;
                objects[x][y].subtype  = loaded_rfd.creature_types[i];
                objects[x][y].subtype2 = creature_get_image(loaded_rfd.creature_types[i]);
                objects[x][y].orientation = ori;
            }

            for(int i = 0; i < loaded_rfd.item_count; ++i)
            {
                int x = loaded_rfd.item_locations_x[i];
                int y = loaded_rfd.item_locations_y[i];

                objects[x][y].type     = TYPE_ITEM;
                objects[x][y].subtype  = loaded_rfd.item_types[i];
                // objects[x][y].subtype2 = creature_get_image(loaded_rfd.item_types[i]);
            }

            for(int i = 0; i < 4; ++i)
                room.doors[i] = loaded_rfd.doors[i];

            strcpy(room_file_name, filtered_room_files[room_file_sel]);
            remove_extension(room_file_name);
        }

        imgui_text_box("Filename##file_name_room",room_file_name,IM_ARRAYSIZE(room_file_name));

        imgui_horizontal_line(1);
        imgui_text("Commentary");
        static char* star_labels[] = {" *", "**", "***", "****", "*****"};
        imgui_newline();
        imgui_horizontal_begin();
            int selected_rating = imgui_button_select(5, star_labels, "Rating");
            imgui_text("(%d Stars)", selected_rating+1);
        imgui_horizontal_end();

        static char comment[100] = {0};
        imgui_text_box("Comment", comment, IM_ARRAYSIZE(comment));

        imgui_horizontal_line(2);

        char file_path[64] = {0};
        snprintf(file_path,63,"src/rooms/%s.room",room_file_name);

        file_exists = io_file_exists(file_path);

        if(!STR_EMPTY(room_file_name))
        {
            if(file_exists) imgui_horizontal_begin();


            if(imgui_button("Save##room"))
            {
                // fill out room file data structure
                RoomFileData rfd = {
                    .version = FORMAT_VERSION,
                    .size_x = ROOM_TILE_SIZE_X,
                    .size_y = ROOM_TILE_SIZE_Y,
                    .type = room_type_sel,
                    .rank = room_rank
                };

                for(int y = 0; y < ROOM_TILE_SIZE_Y; ++y)
                    for(int x = 0; x < ROOM_TILE_SIZE_X; ++x)
                        rfd.tiles[x][y] = (int)room_data_editor.tiles[x][y];

                for(int y = 0; y < OBJECTS_MAX_Y; ++y)
                {
                    for(int x = 0; x < OBJECTS_MAX_X; ++x)
                    {
                        PlacedObject* o = &objects[x][y];

                        if(o->type == TYPE_CREATURE)
                        {
                            rfd.creature_types[rfd.creature_count] = o->subtype;
                            rfd.creature_locations_x[rfd.creature_count] = x;
                            rfd.creature_locations_y[rfd.creature_count] = y;
                            rfd.creature_orientations[rfd.creature_count] = o->orientation;
                            // printf("creature (%d) %.1f,%.1f   %d,%d\n", rfd.creature_count, rfd.creature_locations_x[rfd.creature_count], rfd.creature_locations_y[rfd.creature_count],x,y);
                            rfd.creature_count++;

                        }
                        else if(o->type == TYPE_ITEM)
                        {
                            rfd.item_types[rfd.item_count] = o->subtype;
                            rfd.item_locations_x[rfd.item_count] = x;
                            rfd.item_locations_y[rfd.item_count] = y;
                            rfd.item_count++;
                        }
                    }
                }

                for(int i = 0; i < 4; ++i)
                    rfd.doors[i] = room.doors[i];

                room_file_save(&rfd, file_path);
                // room_file_get_all();

                force_reload = true;
            }
        }

        if(file_exists)
        {
            imgui_text_colored(0x00CC8800, "File Exists!");
            imgui_horizontal_end();
        }

    room_files_panel_rect = imgui_end();
}

static bool _play_room = false;

bool room_editor_update(float dt)
{
    if(_play_room)
    {
        _play_room = false;
        role = ROLE_LOCAL;

        creature_clear_all();
        item_clear_all();

        // set up room to play
        memset(&level,0,sizeof(Level));
        memcpy(&room_list[0],&loaded_rfd,sizeof(RoomFileData));
        room_list_count = 1;
        level_set_room_pointers(&level);

        level.num_rooms = 1;
        level.start.x = floor(MAX_ROOMS_GRID_X/2);
        level.start.y = floor(MAX_ROOMS_GRID_Y/2);

        LOGI("level start: %d %d",level.start.x, level.start.y);

        astar_create(&level.asd, ROOM_TILE_SIZE_X, ROOM_TILE_SIZE_Y);
        astar_set_traversable_func(&level.asd, level_tile_traversable_func);

        Room* room = &level.rooms[level.start.x][level.start.y];

        room->valid = true;
        room->type  = room_list[0].type;
        room->doors[0] = false;
        room->doors[1] = false;
        room->doors[2] = false;
        room->doors[3] = false;
        room->color = COLOR_TINT_NONE;
        room->layout = 0;
        room->index = level_get_room_index(level.start.x, level.start.y);
        room->discovered = true;

        generate_walls(&level);
        level_place_entities(&level);

        player->phys.curr_room = room->index;
        player->transition_room = player->phys.curr_room;

        for(int i = 0; i < 4; ++i)
        {
            if(loaded_rfd.doors[i])
            {
                player_send_to_room(player, room->index, true, level_get_door_tile_coords(i));
                break;
            }
        }

        // // add monsters
        // for(int i = 0; i < loaded_rfd.creature_count; ++i)
        // {
        //     Vector2i g = {loaded_rfd.creature_locations_x[i], loaded_rfd.creature_locations_y[i]};
        //     g.x--; g.y--; // @convert room objects to tile grid coordinates
        //     Creature* c = creature_add(room, loaded_rfd.creature_types[i], &g, NULL);
        // }

        // // add items
        // for(int i = 0; i < loaded_rfd.item_count; ++i)
        // {
        //     Vector2f pos = level_get_pos_by_room_coords(loaded_rfd.item_locations_x[i]-1, loaded_rfd.item_locations_y[i]-1); // @convert room objects to tile grid coordinates
        //     item_add(loaded_rfd.item_types[i], pos.x, pos.y, room->index);
        // }

        set_game_state(GAME_STATE_PLAYING);
        return true;
    }

    gfx_clear_lines();

    text_list_update(text_lst, dt);

    window_get_mouse_view_coords(&mouse_x, &mouse_y);
    window_get_mouse_world_coords(&mouse_world_x, &mouse_world_y);

    if(window_mouse_left_went_down())
        lmouse_state = true;
    else if(window_mouse_left_went_up())
        lmouse_state = false;

    if(window_mouse_right_went_down())
        rmouse_state = true;
    else if(window_mouse_right_went_up())
        rmouse_state = false;

    for(int i = 0; i < EDITOR_KEY_MAX; ++i)
    {
        update_input_state(&editor_keys[i], dt);
    }

    bool up    = editor_keys[EDITOR_KEY_UP].state;
    bool down  = editor_keys[EDITOR_KEY_DOWN].state;
    bool left  = editor_keys[EDITOR_KEY_LEFT].state;
    bool right = editor_keys[EDITOR_KEY_RIGHT].state;
    bool rotate = editor_keys[EDITOR_KEY_ROTATE].toggled_on;

    if(rotate)
    {
        if(orientation == 3) orientation = 0;
        else orientation++;

        if(orientation == 0) orientation_deg = 0.0;
        else if(orientation == 1) orientation_deg = 90.0;
        else if(orientation == 2) orientation_deg = 180.0;
        else if(orientation == 3) orientation_deg = 270.0;
    }

    Vector2f vel_dir = {0.0,0.0};
    if(up)
    {
        vel_dir.y = -1.0;
    }
    if(down)
    {
        vel_dir.y = 1.0;
    }
    if(left)
    {
        vel_dir.x = -1.0;
    }
    if(right)
    {
        vel_dir.x = 1.0;
    }
    if((up && (left || right)) || (down && (left || right)))
    {
        // moving diagonally
        vel_dir.x *= 0.7071f;
        vel_dir.y *= 0.7071f;
    }
    ecam_pos_x += vel_dir.x*200.0*dt;
    ecam_pos_y += vel_dir.y*200.0*dt;


    editor_camera_set(false);

    return false;
}

void room_editor_draw()
{
    gfx_clear_buffer(background_color);

    room.layout = -1;
    level_draw_room(&room, &room_data_editor, 0, 0, 1.0, false);

    for(int _y = 0; _y < OBJECTS_MAX_Y; ++_y)
    {
        for(int _x = 0; _x < OBJECTS_MAX_X; ++_x)
        {
            PlacedObject* o = &objects[_x][_y];
            if(o->type == TYPE_NONE) continue;

            Rect tile_rect = level_get_tile_rect(_x-1, _y-1);

            if(o->type == TYPE_CREATURE)
            {
                // printf("%.2f\n", get_orientation_deg(o->orientation));
                gfx_draw_image(o->subtype2, 0, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, get_orientation_deg(o->orientation), 1.0, false, true);
            }
            else if(o->type == TYPE_ITEM)
            {
                gfx_draw_image(item_props[o->subtype].image, item_props[o->subtype].sprite_index, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
            }

        }
    }

    // room_draw_walls(&room);

    gfx_draw_rect(&room_area, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, IN_WORLD);

    tile_coords = level_get_room_coords_by_pos(mouse_world_x, mouse_world_y);
    Rect tile_rect = level_get_tile_rect(tile_coords.x, tile_coords.y);

    obj_coords.x = tile_coords.x +1;
    obj_coords.y = tile_coords.y +1;

    bool out_of_area = false;
    if(obj_coords.x < 0 || obj_coords.x >= OBJECTS_MAX_X)
        out_of_area = true;
    else if(obj_coords.y < 0 || obj_coords.y >= OBJECTS_MAX_Y)
        out_of_area = true;

    bool out_of_room = false;
    if(obj_coords.x < 1 || obj_coords.x >= OBJECTS_MAX_X-1)
        out_of_room = true;
    else if(obj_coords.y < 1 || obj_coords.y >= OBJECTS_MAX_Y-1)
        out_of_room = true;

    bool on_door = false;
    if(obj_coords.x == OBJECTS_MAX_X/2 && (obj_coords.y == 0 || obj_coords.y == OBJECTS_MAX_Y-1))
        on_door = true;
    else if(obj_coords.y == OBJECTS_MAX_Y/2 && (obj_coords.x == 0 || obj_coords.x == OBJECTS_MAX_X-1))
        on_door = true;

    bool in_front_of_door = false;
    if(obj_coords.x == OBJECTS_MAX_X/2 && (obj_coords.y == 1 || obj_coords.y == OBJECTS_MAX_Y-2))
        in_front_of_door = true;
    else if(obj_coords.y == OBJECTS_MAX_Y/2 && (obj_coords.x == 1 || obj_coords.x == OBJECTS_MAX_X-2))
        in_front_of_door = true;

    bool on_wall = false; // excludes corner
    if(obj_coords.x == 0 && obj_coords.y > 0 && obj_coords.y < OBJECTS_MAX_Y-1) //left
        on_wall = true;
    else if(obj_coords.x == OBJECTS_MAX_X-1 && obj_coords.y > 0 && obj_coords.y < OBJECTS_MAX_Y-1) //right
        on_wall = true;
    else if(obj_coords.y == 0 && obj_coords.x > 0 && obj_coords.x < OBJECTS_MAX_X-1) //top
        on_wall = true;
    else if(obj_coords.y == OBJECTS_MAX_Y-1 && obj_coords.x > 0 && obj_coords.x < OBJECTS_MAX_X-1) //bottom
        on_wall = true;

    PlacedObject obj = {0};
    obj.type = TYPE_NONE;

    bool eraser = false;
    bool error = false;
    uint32_t status_color = COLOR_GREEN;

    if(obj_sel == 0) // tiles
    {
        error |= out_of_room;

        TileType tt = tile_sel;
        if(tt == TILE_PIT || tt == TILE_BOULDER)
        {
            Dir door = match_in_front_of_door_coords(obj_coords.x, obj_coords.y);
            if(door != DIR_NONE)
            {
                if(room.doors[door])
                    error = true;
            }
        }

        if(error)
            status_color = COLOR_RED;

        gfx_draw_image(dungeon_image, level_get_tile_sprite(tt), tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
        gfx_draw_rect(&tile_rect, status_color, NOT_SCALED, NO_ROTATION, 0.2, true, true);
    }
    else if(obj_sel == 1) // doors
    {
        error |= !on_door;

        int sprite = SPRITE_TILE_DOOR_UP;

        if(door_sel == 1)
        {
            sprite = SPRITE_TILE_WALL_UP;
            error = check_no_door_placement();
        }

        if(error)
            status_color = COLOR_RED;

        gfx_draw_image(dungeon_image, sprite, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
        gfx_draw_rect(&tile_rect, status_color, NOT_SCALED, NO_ROTATION, 0.2, true, true);

    }
    else if(obj_sel == 2) // creatures
    {

        error |= out_of_area;

        if(creature_sel == CREATURE_TYPE_CLINGER)
        {
            error |= !on_wall;
            // error |= on_door;
            Dir door = match_door_coords(obj_coords.x, obj_coords.y);
            if(door != DIR_NONE)
            {
                if(room.doors[door])
                    error = true;
            }
        }
        else
        {
            error |= out_of_room;
        }

        if(error)
            status_color = COLOR_RED;


        obj.type = TYPE_CREATURE;
        obj.subtype = creature_sel;
        obj.subtype2 = creature_get_image(obj.subtype);
        obj.orientation = orientation;

        gfx_draw_image(obj.subtype2, 0, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, orientation_deg, 1.0, false, true);
        gfx_draw_rect(&tile_rect, status_color, NOT_SCALED, NO_ROTATION, 0.2, true, true);

    }
    else if(obj_sel == 3)
    {

        error |= out_of_room;
        if(error)
            status_color = COLOR_RED;

        gfx_draw_image(item_props[item_sel].image, item_props[item_sel].sprite_index, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
        gfx_draw_rect(&tile_rect, status_color, NOT_SCALED, NO_ROTATION, 0.2, true, true);

        obj.type = TYPE_ITEM;
        obj.subtype = item_sel;
    }

    // if(eraser)
    // {
    //     gfx_draw_rect(&tile_rect, COLOR_PINK, NOT_SCALED, NO_ROTATION, 0.5, true, true);
    // }

    draw_room_file_gui();

    imgui_begin_panel("Room Editor", view_width - gui_size.w, 1, true);

        imgui_newline();
        char* buttons[] = {"Editor", "Info"};
        tab_sel = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        switch(tab_sel)
        {
            case 1:
            {

                if(imgui_button("Reset Camera"))
                {
                    ecam_pos_x = CENTER_X;
                    ecam_pos_y = CENTER_Y;
                    ecam_pos_z = 23;
                }

                imgui_number_box("Camera Zoom", 0, 99, &ecam_pos_z);

                imgui_text("Camera Zoom: %.2f", camera_get_zoom());
                imgui_text("Camera Pos: %.2f, %.2f", ecam_pos_x, ecam_pos_y);
                imgui_text("Mouse (view): %.1f, %.1f", mouse_x, mouse_y);
                imgui_text("Mouse States: %d, %d", lmouse_state, rmouse_state);

                imgui_text("Object Coords: %d, %d", obj_coords.x, obj_coords.y);

                imgui_text("Out of area: %s", BOOLSTR(out_of_area));
                imgui_text("Out of room: %s", BOOLSTR(out_of_room));
                imgui_text("On wall: %s", BOOLSTR(on_wall));
                imgui_text("On door: %s", BOOLSTR(on_door));
                imgui_text("In front of door: %s", BOOLSTR(in_front_of_door));

            } break;

            case 0:
            {

                if(imgui_button("Clear All"))
                {
                    clear_all();
                }

                bool interacted = false;

                bool is_monster_room = false;
                for(int y = 0; y < OBJECTS_MAX_Y; ++y)
                {
                    for(int x = 0; x < OBJECTS_MAX_X; ++x)
                    {
                        PlacedObject* o = &objects[x][y];
                        if(o->type == TYPE_CREATURE)
                        {
                            is_monster_room = true;
                            break;
                        }
                    }
                    if(is_monster_room) break;
                }
                if(is_monster_room)
                {
                    if(room_type_sel != ROOM_TYPE_MONSTER && room_type_sel != ROOM_TYPE_BOSS)
                        room_type_sel = ROOM_TYPE_MONSTER;
                }
                else if(room_type_sel == ROOM_TYPE_MONSTER)
                {
                    room_type_sel = ROOM_TYPE_EMPTY;
                }

                const float big = 16.0;

                imgui_text_sized(big, "Settings");

                imgui_dropdown(room_type_names, ROOM_TYPE_MAX, "Room Type", &room_type_sel, NULL);
                imgui_number_box("Room Rank", 0, 10, &room_rank);

                imgui_newline();
                imgui_text_sized(big, "Placeables");

                int _tile_sel = tile_sel;
                imgui_dropdown(tile_names, TILE_MAX, "Select Tile", &tile_sel, &interacted);
                if(tile_sel != _tile_sel || interacted)
                {
                    obj_sel = 0;
                }

                int _door_sel = door_sel;
                imgui_dropdown(door_names, NUM_DOOR_TYPES, "Select Door", &door_sel, &interacted);
                if(door_sel != _door_sel || interacted)
                {
                    obj_sel = 1;
                }

                int _creature_sel = creature_sel;
                imgui_dropdown(creature_names, CREATURE_TYPE_MAX, "Select Creature", &creature_sel, &interacted);
                if(creature_sel != _creature_sel || interacted)
                {
                    obj_sel = 2;
                }

                int _item_sel = item_sel;
                imgui_listbox(item_names, ITEM_MAX, "Selected Item", &item_sel, 10);
                // imgui_dropdown(item_names, ITEM_MAX, "Select Item", &item_sel, &interacted);
                if(item_sel != _item_sel)
                {
                    obj_sel = 3;
                }


                imgui_horizontal_line(2);

                if(imgui_button("Play Room"))
                {
                    _play_room = true;
                }
            }
        }

    gui_size = imgui_end();
    // gfx_draw_rect(&gui_size, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, false);


    if(!lmouse_state && !rmouse_state)
        return;

    Rect mr = RECT(mouse_x, mouse_y, 1, 1);
    if(rectangles_colliding(&mr, &gui_size))
        return;

    static bool imgui_click = false;
    if(window_mouse_left_went_down())
    {
        // the mouse click was on an imgui object
        if(imgui_clicked() || imgui_active())
        {
            imgui_click = true;
        }
        else
        {
            imgui_click = false;
        }
    }


    if(imgui_click)
        return;

    int prior_obj_sel = obj_sel;
    int prior_door_sel = door_sel;

    if(rmouse_state)
    {
        if(on_door)
        {
            error = check_no_door_placement();
            if(error)
            {
                if(!room.doors[match_door_coords(obj_coords.x, obj_coords.y)])
                {
                    // 1 door remaining, but not the door we're currently modifying
                    error = false;
                }
            }
            obj_sel = 1;
            door_sel = 1;
        }
        else
        {
            eraser = true;
            error = out_of_area;
        }
    }

    if(error)
    {
        obj_sel = prior_obj_sel;
        door_sel = prior_door_sel;
        return;
    }

    PlacedObject objects_temp[OBJECTS_MAX_X][OBJECTS_MAX_Y] = {0};
    RoomFileData room_data_temp = {0};
    Room room_temp = {0};
    memcpy(objects_temp, objects, sizeof(PlacedObject)*OBJECTS_MAX_X*OBJECTS_MAX_Y);
    memcpy(&room_data_temp, &room_data_editor, sizeof(RoomFileData));
    memcpy(&room_temp, &room, sizeof(Room));

    if(out_of_area)
    {
        // printf("return! (out of area)\n");
        obj_sel = prior_obj_sel;
        return;
    }

    PlacedObject* o = &objects[obj_coords.x][obj_coords.y];

    if(eraser)
    {
        // if(obj_coords.x > 0 && obj_coords.y > 0 && (obj_coords.x-1) < ROOM_TILE_SIZE_X && (obj_coords.y-1) < ROOM_TILE_SIZE_Y)
        if(!out_of_room)
        {
            room_data_editor.tiles[obj_coords.x-1][obj_coords.y-1] = TILE_FLOOR;
        }
        o->type = TYPE_NONE;
    }
    else if(obj_sel == 0) //tiles
    {
        TileType tt = tile_sel;
        if(tt == TILE_PIT || tt == TILE_BOULDER || tt == TILE_BREAKABLE_FLOOR)
        {
            o->type = TYPE_NONE;
        }
        room_data_editor.tiles[obj_coords.x-1][obj_coords.y-1] = tt;
    }
    else if(obj_sel == 1) //doors
    {
        Dir door = match_door_coords(obj_coords.x, obj_coords.y);
        if(door != DIR_NONE)
        {
            room.doors[door] = (door_sel == 0);

            // check if anything is placed on the door or in front of the door
            if(room.doors[door])
            {
                o->type = TYPE_NONE;

                Vector2i front = get_in_front_of_door_coords(door);
                TileType* ttf = (TileType*)&room_data_editor.tiles[front.x-1][front.y-1];
                if(*ttf == TILE_PIT || *ttf == TILE_BOULDER || *ttf == TILE_BREAKABLE_FLOOR)
                {
                    *ttf = TILE_FLOOR;
                }
            }
            else
            {
                o->type = TYPE_NONE;
            }
        }
        door_sel = prior_door_sel;
    }
    else
    {
        // if(obj_coords.x > 0 && obj_coords.y > 0 && (obj_coords.x+1) < ROOM_TILE_SIZE_X && (obj_coords.y+1) < ROOM_TILE_SIZE_Y)
        if(!out_of_room)
        {
            TileType* tt = (TileType*)&room_data_editor.tiles[obj_coords.x-1][obj_coords.y-1];
            if(*tt == TILE_PIT || *tt == TILE_BOULDER || *tt == TILE_BREAKABLE_FLOOR)
                *tt = TILE_FLOOR;
        }

        o->type = obj.type;
        o->subtype = obj.subtype;
        o->subtype2 = obj.subtype2;
        o->orientation = obj.orientation;
    }

    obj_sel = prior_obj_sel;
}

static float get_orientation_deg(int _orientation)
{
    if(_orientation == 0) return 0.0;
    else if(_orientation == 1) return 90.0;
    else if(_orientation == 2) return 180.0;
    return 270.0;
}
// returns true is number of doors is 0 or 1
static bool check_no_door_placement(int x, int y)
{
    bool error = false;
    int count = 0;
    for(int i = 0; i < 4; ++i)
    {
        if(room.doors[i]) count++;
    }
    if(count <= 1)
        error = true;
    return error;
}

static Vector2i get_door_coords(Dir door)
{
    Vector2i ret = {.x=0,.y=0};

    if(door == DIR_LEFT)
    {
        ret.x = 0;
        ret.y = OBJECTS_MAX_Y/2;
    }
    else if(door == DIR_RIGHT)
    {
        ret.x = OBJECTS_MAX_X-1;
        ret.y = OBJECTS_MAX_Y/2;
    }
    else if(door == DIR_UP)
    {
        ret.x = OBJECTS_MAX_X/2;
        ret.y = 0;
    }
    else if(door == DIR_DOWN)
    {
        ret.x = OBJECTS_MAX_X/2;
        ret.y = OBJECTS_MAX_Y-1;
    }

    // printf("door %s -> %d, %d\n", get_dir_name(door), ret.x, ret.y);

    return ret;
}

static Vector2i get_in_front_of_door_coords(Dir door)
{
    Vector2i ret = get_door_coords(door);
    if(door == DIR_LEFT)
    {
        ret.x++;
    }
    else if(door == DIR_RIGHT)
    {
        ret.x--;
    }
    else if(door == DIR_UP)
    {
        ret.y++;
    }
    else if(door == DIR_DOWN)
    {
        ret.y--;
    }
    return ret;
}

static Dir match_door_coords(int x, int y)
{
    for(int i = 0; i < 4; ++i)
    {
        Vector2i ret = get_door_coords(i);
        if(x == ret.x && y == ret.y)
            return (Dir)i;
    }
    return DIR_NONE;
}

static Dir match_in_front_of_door_coords(int x, int y)
{
    for(int i = 0; i < 4; ++i)
    {
        Vector2i ret = get_in_front_of_door_coords(i);
        if(x == ret.x && y == ret.y)
            return (Dir)i;
    }
    return DIR_NONE;
}


static void editor_camera_set(bool immediate)
{
    camera_move(ecam_pos_x, ecam_pos_y, (float)ecam_pos_z/100.0, immediate, NULL);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);
}

static void clear_all()
{
    for(int _y = 0; _y < OBJECTS_MAX_Y; ++_y)
    {
        for(int _x = 0; _x < OBJECTS_MAX_X; ++_x)
        {
            objects[_x][_y].type = TYPE_NONE;
        }
    }
    for(int i = 0; i < 4; ++i)
    {
        room.doors[i] = true;
    }
    for(int rj = 0; rj < ROOM_TILE_SIZE_Y; ++rj)
    {
        for(int ri = 0; ri < ROOM_TILE_SIZE_X; ++ri)
        {
            room_data_editor.tiles[ri][rj] = TILE_FLOOR;
        }
    }
}

