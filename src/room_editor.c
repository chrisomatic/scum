#include "headers.h"

#include "core/imgui.h"
#include "core/particles.h"
#include "core/io.h"

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

typedef struct
{
    PlacedType type;
    int subtype;
    int subtype2;
} PlacedObject;


static PlacedObject objects[OBJECTS_MAX_X][OBJECTS_MAX_Y] = {0};
static RoomFileData room_data = {0};
static Room room = {0};

static PlacedObject objects_prior[OBJECTS_MAX_X][OBJECTS_MAX_Y] = {0};

static float ecam_pos_x = 0;
static float ecam_pos_y = 0;
static int ecam_pos_z = 40;

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

        printf("Setting tile names!!\n");
        tile_sel = 1;
        for(int i = 0; i < TILE_MAX; ++i)
        {
            tile_names[i] = (char*)get_tile_name(i);
            printf("  %s (%p)\n",tile_names[i], tile_names[i]);
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

    for(int i = 0;  i < EDITOR_KEY_MAX; ++i)
        memset(&editor_keys[i], 0, sizeof(PlayerInput));

    editor_camera_set(true);

    // load room files
    room_file_get_all();
}

static void draw_room_file_gui()
{
    static int prior_room_file_sel = 0;
    static int room_file_sel = 0;
    static char* filtered_room_files[256] = {0};
    static int filtered_room_files_count = 0;
    static char file_filter_str[32] = {0};
    static char selected_room_name_str[100] = {0};
    static int selected_rank = 0;
    static int selected_room_type = 0;
    static bool are_you_sure = false;

    static Rect room_files_panel_rect = {0};

    imgui_begin_panel("Files",1,1,true);
        
        const float big = 16.0;
        imgui_text_sized(big,"Filter");

        imgui_horizontal_begin();
        char* buttons[] = {"*", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
        selected_rank = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "Rank");
        imgui_horizontal_end();

        imgui_horizontal_begin();
        char* buttons2[] = {"All", "Empty", "Monster", "Treasure", "Boss"};
        selected_room_type = imgui_button_select(IM_ARRAYSIZE(buttons2), buttons2, "Room Type");
        imgui_horizontal_end();

        float opacity_scale = imgui_is_mouse_inside() ? 1.0 : 0.4;
        imgui_set_global_opacity_scale(opacity_scale);
        imgui_horizontal_begin();
        imgui_text_box("##FilterText",file_filter_str,IM_ARRAYSIZE(file_filter_str));
        if(imgui_button("Clear")) memset(file_filter_str,0,32);
        imgui_horizontal_end();
        
        // apply filter to selection list 
        filtered_room_files_count = 0;
        for(int i = 0; i < room_file_count; ++i)
        {
            RoomFileData rfd;
            room_file_load(&rfd, "src/rooms/%s", p_room_files[i]); // @EFFICIENCY: Doing this every frame

            bool match_rank        = selected_rank == 0 ? true : (rfd.rank == selected_rank);
            bool match_room_type   = selected_room_type == 0 ? true : (rfd.type == selected_room_type-1);
            bool match_filter_text = strstr(p_room_files[i],file_filter_str);

            if(match_rank && match_room_type && match_filter_text)
                filtered_room_files[filtered_room_files_count++] = p_room_files[i];
        }

        prior_room_file_sel = room_file_sel;
        imgui_listbox(filtered_room_files, filtered_room_files_count, "##files_listbox", &room_file_sel);

        if(filtered_room_files[room_file_sel])
            snprintf(selected_room_name_str,100,"Selected: %s",filtered_room_files[room_file_sel]);

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
                room_file_get_all();
                are_you_sure = false;
            }

            imgui_horizontal_end();
        }

        if(prior_room_file_sel != room_file_sel)
        {
            clear_all();
            RoomFileData rfd;
            room_file_load(&rfd, "src/rooms/%s", filtered_room_files[room_file_sel]);

            // set properties
            room_type_sel = rfd.type;
            room_rank = rfd.rank;

            // tiles
            for(int i = 0; i < rfd.size_x; ++i)
                for(int j = 0; j < rfd.size_y; ++j)
                    room_data.tiles[i][j] = (TileType)rfd.tiles[i][j];

            for(int i = 0; i < rfd.creature_count; ++i)
            {
                int x = rfd.creature_locations_x[i];
                int y = rfd.creature_locations_y[i];

                objects[x][y].type     = TYPE_CREATURE;
                objects[x][y].subtype  = rfd.creature_types[i];
                objects[x][y].subtype2 = creature_get_image(rfd.creature_types[i]);
            }

            for(int i = 0; i < rfd.item_count; ++i)
            {
                int x = rfd.item_locations_x[i];
                int y = rfd.item_locations_y[i];

                objects[x][y].type     = TYPE_ITEM;
                objects[x][y].subtype  = rfd.item_types[i];
                objects[x][y].subtype2 = creature_get_image(rfd.item_types[i]);
            }

            for(int i = 0; i < 4; ++i)
                room.doors[i] = rfd.doors[i];

            strcpy(room_file_name, filtered_room_files[room_file_sel]);
            remove_extension(room_file_name);
        }

        imgui_text_box("Filename##file_name_room",room_file_name,IM_ARRAYSIZE(room_file_name));

        char file_path[64] = {0};
        snprintf(file_path,63,"src/rooms/%s.room",room_file_name);

        bool file_exists = io_file_exists(file_path);

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
                        rfd.tiles[x][y] = (int)room_data.tiles[x][y];

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
                room_file_get_all();
            }
        }

        if(file_exists)
        {
            imgui_text_colored(0x00CC8800, "File Exists!");
            imgui_horizontal_end();
        }

    room_files_panel_rect = imgui_end();


}

void room_editor_update(float dt)
{
    gfx_clear_lines();

    text_list_update(text_lst, dt);

    window_get_mouse_view_coords(&mx, &my);
    window_get_mouse_world_coords(&wmx, &wmy);

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
}

void room_editor_draw()
{
    gfx_clear_buffer(background_color);

    level_draw_room(&room, &room_data, 0, 0);

    for(int _y = 0; _y < OBJECTS_MAX_Y; ++_y)
    {
        for(int _x = 0; _x < OBJECTS_MAX_X; ++_x)
        {
            PlacedObject* o = &objects[_x][_y];
            if(o->type == TYPE_NONE) continue;

            Rect tile_rect = level_get_tile_rect(_x-1, _y-1);

            if(o->type == TYPE_CREATURE)
            {
                gfx_draw_image(o->subtype2, 0, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
            }
            else if(o->type == TYPE_ITEM)
            {
                gfx_draw_image(item_props[o->subtype].image, item_props[o->subtype].sprite_index, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
            }

        }
    }

    // room_draw_walls(&room);

    gfx_draw_rect(&room_area, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, IN_WORLD);

    tile_coords = level_get_room_coords_by_pos(wmx, wmy);
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

        gfx_draw_image(obj.subtype2, 0, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
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
                imgui_number_box("Camera Zoom", 0, 99, &ecam_pos_z);

                imgui_text("Camera Zoom: %.2f", camera_get_zoom());
                imgui_text("Mouse (view): %.1f, %.1f", mx, my);
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
                imgui_dropdown(item_names, ITEM_MAX, "Select Item", &item_sel, &interacted);
                if(item_sel != _item_sel || interacted)
                {
                    obj_sel = 3;
                }
            }
        }

    gui_size = imgui_end();
    // gfx_draw_rect(&gui_size, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, false);

    if(!lmouse_state && !rmouse_state)
        return;

    Rect mr = RECT(mx, my, 1, 1);
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
            error = false;
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
    memcpy(&room_data_temp, &room_data, sizeof(RoomFileData));
    memcpy(&room_temp, &room, sizeof(Room));

    PlacedObject* o = &objects[obj_coords.x][obj_coords.y];

    if(eraser)
    {
        if(obj_coords.x > 0 && obj_coords.y > 0 && (obj_coords.x-1) < ROOM_TILE_SIZE_X && (obj_coords.y-1) < ROOM_TILE_SIZE_Y)
            room_data.tiles[obj_coords.x-1][obj_coords.y-1] = TILE_FLOOR;
        o->type = TYPE_NONE;
    }
    else if(obj_sel == 0) //tiles
    {
        TileType tt = tile_sel;
        if(tt == TILE_PIT || tt == TILE_BOULDER)
        {
            o->type = TYPE_NONE;
        }
        room_data.tiles[obj_coords.x-1][obj_coords.y-1] = tt;
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
                TileType* ttf = (TileType*)&room_data.tiles[front.x-1][front.y-1];
                if(*ttf == TILE_PIT || *ttf == TILE_BOULDER)
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
        if(obj_coords.x > 0 && obj_coords.y > 0 && (obj_coords.x+1) < ROOM_TILE_SIZE_X && (obj_coords.y+1) < ROOM_TILE_SIZE_Y)
        {
            TileType* tt = (TileType*)&room_data.tiles[obj_coords.x-1][obj_coords.y-1];
            if(*tt == TILE_PIT || *tt == TILE_BOULDER)
                *tt = TILE_FLOOR;
        }

        o->type = obj.type;
        o->subtype = obj.subtype;
        o->subtype2 = obj.subtype2;
    }

    obj_sel = prior_obj_sel;
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
            room_data.tiles[ri][rj] = TILE_FLOOR;
        }
    }
}

