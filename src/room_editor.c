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
#include "camera.h"

#define FORMAT_VERSION 1

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
    TYPE_TILE,
    TYPE_ITEM,
    TYPE_CREATURE,
} PlacedType;

typedef struct
{
    PlacedType type;
    int subtype;
    int subtype2;
} PlacedObject;

// +2 for walls
#define OBJECTS_MAX_X (ROOM_TILE_SIZE_X+2)
#define OBJECTS_MAX_Y (ROOM_TILE_SIZE_Y+2)
PlacedObject objects[OBJECTS_MAX_X][OBJECTS_MAX_Y] = {0};

static float ecam_pos_x = 0;
static float ecam_pos_y = 0;
static int ecam_pos_z = 44;

static int tab_sel = 0;
static int obj_sel = 0;

#define NUM_TILE_TYPES  TILE_MAX
static char* tile_names[NUM_TILE_TYPES] = {0};
static int tile_sel;

#define NUM_DOOR_TYPES 2
static char* door_names[NUM_DOOR_TYPES] = {0};
static int door_sel;

static char* creature_names[CREATURE_TYPE_MAX+1] = {0};
static int creature_sel = 0;

static char* item_names[ITEM_MAX+1] = {0};
static int item_sel = 0;

static char* room_type_names[ROOM_TYPE_MAX] = {0};
static int room_type_sel = 0;

static Room room = {0};
static RoomData room_data = {0};

Vector2i tile_coords = {0}; // mouse tile coords
Vector2i obj_coords = {0}; // tile_coords translated to object grid

Rect gui_size = {0};

static Vector2i get_door_coords(Dir door);
static Vector2i get_in_front_of_door_coords(Dir door);
static Dir match_door_coords(int x, int y);
static Dir match_in_front_of_door_coords(int x, int y);

static void editor_camera_set(bool immediate);
static void clear_all();
static void save_room(char* path, ...);


void room_editor_init()
{
    static bool _init = false;

    show_tile_grid = true;
    debug_enabled = true;

    ecam_pos_x = CENTER_X;
    ecam_pos_y = CENTER_Y;

    if(!_init)
    {
        item_sel = 0;
        for(int i = 0; i < ITEM_MAX; ++i)
        {
            item_names[i] = (char*)item_get_name(i);
        }
        item_names[ITEM_MAX] = "Eraser";

        creature_sel = 0;
        for(int i = 0; i < CREATURE_TYPE_MAX; ++i)
            creature_names[i] = (char*)creature_type_name(i);
        creature_names[CREATURE_TYPE_MAX] = "Eraser";

        tile_sel = 0;
        for(int i = 0; i < NUM_TILE_TYPES; ++i)
            tile_names[i] = (char*)get_tile_name(i);

        door_sel = 0;
        door_names[0] = "Possible Door";
        door_names[1] = "No Door";

        room_type_sel = 0;
        for(int i = 0; i < ROOM_TYPE_MAX; ++i)
            room_type_names[i] = (char*)get_room_type_name(i);

        clear_all();
    }

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

    _init = true;
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
            if(o->type == TYPE_TILE) continue;

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

        obj.type = TYPE_TILE;
        obj.subtype = tt;
    }
    else if(obj_sel == 1) // doors
    {
        error |= !on_door;

        int sprite = SPRITE_TILE_DOOR_UP;

        if(door_sel == 1)
        {
            sprite = SPRITE_TILE_WALL_UP;

            int count = 0;
            for(int i = 0; i < 4; ++i)
            {
                if(room.doors[i]) count++;
            }
            if(count <= 1)
                error = true;
        }

        if(error)
            status_color = COLOR_RED;


        gfx_draw_image(dungeon_image, sprite, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
        gfx_draw_rect(&tile_rect, status_color, NOT_SCALED, NO_ROTATION, 0.2, true, true);

    }
    else if(obj_sel == 2) // creatures
    {

        error |= out_of_area;

        if(creature_sel == CREATURE_TYPE_MAX)   // eraser
        {
            eraser = true;
        }
        else
        {

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

    }
    else if(obj_sel == 3)
    {

        if(item_sel == ITEM_MAX)   // eraser
        {
            eraser = true;
            error |= out_of_area;
        }
        else
        {
            error |= out_of_room;
            if(error)
                status_color = COLOR_RED;

            gfx_draw_image(item_props[item_sel].image, item_props[item_sel].sprite_index, tile_rect.x, tile_rect.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
            gfx_draw_rect(&tile_rect, status_color, NOT_SCALED, NO_ROTATION, 0.2, true, true);

            obj.type = TYPE_ITEM;
            obj.subtype = item_sel;
        }
    }

    if(eraser)
    {
        gfx_draw_rect(&tile_rect, COLOR_PINK, NOT_SCALED, NO_ROTATION, 0.5, true, true);
    }

    imgui_begin_panel("Room Editor", view_width - gui_size.w, 1, true);

        imgui_newline();
        char* buttons[] = {"Info", "Editor"};
        tab_sel = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        switch(tab_sel)
        {
            case 0:
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

            case 1:
            {
                if(imgui_button("Clear All"))
                {
                    clear_all();
                }

                imgui_dropdown(room_type_names, ROOM_TYPE_MAX, "Room Type", &room_type_sel);

                int _tile_sel = tile_sel;
                imgui_dropdown(tile_names, NUM_TILE_TYPES, "Select Tile", &tile_sel);
                if(tile_sel != _tile_sel)
                {
                    obj_sel = 0;
                }

                int _door_sel = door_sel;
                imgui_dropdown(door_names, NUM_DOOR_TYPES, "Select Door", &door_sel);
                if(door_sel != _door_sel)
                {
                    obj_sel = 1;
                }

                int _creature_sel = creature_sel;
                imgui_dropdown(creature_names, CREATURE_TYPE_MAX+1, "Select Creature", &creature_sel);
                if(creature_sel != _creature_sel)
                {
                    obj_sel = 2;
                }

                int _item_sel = item_sel;
                imgui_dropdown(item_names, ITEM_MAX+1, "Select Item", &item_sel);
                if(item_sel != _item_sel)
                {
                    obj_sel = 3;
                }

                if(imgui_button("Save"))
                {
                    save_room("test.room");
                }

            }
        }

    gui_size = imgui_end();
    // gfx_draw_rect(&gui_size, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, false);

    if(error || !lmouse_state)
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

    if(obj_sel == 1) //doors
    {
        Dir door = match_door_coords(obj_coords.x, obj_coords.y);
        if(door != DIR_NONE)
        {
            room.doors[door] = (door_sel == 0);

            // check if anything is placed on the door or in front of the door
            if(room.doors[door])
            {
                PlacedObject* o = &objects[obj_coords.x][obj_coords.y];
                o->type = TYPE_NONE;

                Vector2i front = get_in_front_of_door_coords(door);
                PlacedObject* fronto = &objects[front.x][front.y];
                if(fronto->type == TYPE_TILE)
                {
                    if(fronto->subtype == TILE_PIT || fronto->subtype == TILE_BOULDER)
                    {
                        fronto->subtype = TILE_FLOOR;
                    }
                }
            }
        }
    }
    else
    {

        PlacedObject* o = &objects[obj_coords.x][obj_coords.y];
        if(eraser)
        {
            o->type = TYPE_NONE;
        }
        else
        {
            o->type = obj.type;
            o->subtype = obj.subtype;
            o->subtype2 = obj.subtype2;
        }

    }

    // rebuild the room data
    for(int rj = 0; rj < ROOM_TILE_SIZE_Y; ++rj)
    {
        for(int ri = 0; ri < ROOM_TILE_SIZE_X; ++ri)
        {
            PlacedObject* o = &objects[ri+1][rj+1];
            if(o->type == TYPE_TILE)
                room_data.tiles[ri][rj] = o->subtype;
            else
                room_data.tiles[ri][rj] = TILE_FLOOR;
        }
    }

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

static void save_room(char* path, ...)
{
    va_list args;
    va_start(args, path);
    char fpath[256] = {0};
    vsprintf(fpath, path, args);
    va_end(args);

    FILE* fp = fopen(fpath, "w");
    if(fp)
    {

        fprintf(fp, "[%d] # version\n\n", FORMAT_VERSION);

        fputs("; Room Dimensions\n", fp);
        fprintf(fp, "%d,%d\n\n", ROOM_TILE_SIZE_X, ROOM_TILE_SIZE_Y);

        fputs("; Room Type\n", fp);
        fprintf(fp, "%d\n\n", room_type_sel);

        fputs("; Tile Mapping\n", fp);
        for(int i = 0; i < NUM_TILE_TYPES; ++i)
        {
            fprintf(fp, "%s\n", tile_names[i]);
        }
        fputs("\n", fp);

        fputs("; Tile Data\n", fp);
        for(int y = 0; y < ROOM_TILE_SIZE_Y; ++y)
        {
            for(int x = 0; x < ROOM_TILE_SIZE_X; ++x)
            {
                PlacedObject* o = &objects[x+1][y+1];
                int tt = TILE_FLOOR;
                if(o->type == TYPE_TILE) tt = o->subtype;

                fprintf(fp, "%d", tt);

                if(x == ROOM_TILE_SIZE_X-1)
                    fputs("\n", fp);
                else
                    fputs(",", fp);
            }
        }
        fputs("\n", fp);

        fputs("; Creature Mapping\n", fp);
        for(int i = 0; i < CREATURE_TYPE_MAX; ++i)
        {
            fprintf(fp, "%s\n", creature_names[i]);
        }
        fputs("\n", fp);

        fputs("; Creatures\n", fp);
        for(int y = 0; y < OBJECTS_MAX_Y; ++y)
        {
            for(int x = 0; x < OBJECTS_MAX_X; ++x)
            {
                PlacedObject* o = &objects[x][y];
                if(o->type != TYPE_CREATURE) continue;
                fprintf(fp, "%d,%d,%d\n", o->subtype, x, y);
            }
        }
        fputs("\n", fp);

        fputs("; Item Mapping\n", fp);
        for(int i = 0; i < ITEM_MAX; ++i)
        {
            fprintf(fp, "%s\n", item_names[i]);
        }
        fputs("\n", fp);

        fputs("; Items\n", fp);
        for(int y = 0; y < OBJECTS_MAX_Y; ++y)
        {
            for(int x = 0; x < OBJECTS_MAX_X; ++x)
            {
                PlacedObject* o = &objects[x][y];
                if(o->type != TYPE_ITEM) continue;
                fprintf(fp, "%d,%d,%d\n", o->subtype, x, y);
            }
        }
        fputs("\n", fp);

        fputs("; Doors\n", fp);
        for(int i = 0; i < 4; ++i)
        {
            fprintf(fp, "%d", room.doors[i] ? 1 : 0);
            if(i == 3)
                fputs("\n", fp);
            else
                fputs(",", fp);
        }
        fputs("\n", fp);


        fclose(fp);
    }


}