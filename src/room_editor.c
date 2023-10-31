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

enum
{
    EDITOR_KEY_UP,
    EDITOR_KEY_DOWN,
    EDITOR_KEY_LEFT,
    EDITOR_KEY_RIGHT,

    EDITOR_KEY_MAX
};
static PlayerInput editor_keys[EDITOR_KEY_MAX] = {0};


typedef enum
{
    TYPE_NONE,
    TYPE_DOOR,
    TYPE_TILE,
    TYPE_PICKUP,
    TYPE_CREATURE
} PlacedType;

typedef struct
{
    PlacedType type;
    void* ptr;
} PlacedObject;

static float ecam_pos_x = 0;
static float ecam_pos_y = 0;
static int ecam_pos_z = 55;

static int tab_sel = 0;

#define NUM_TILE_TYPES  3
static const char* tile_names[NUM_TILE_TYPES] = {0};
static int tile_sel;


static const char* creature_names[CREATURE_TYPE_MAX+1] = {0};
static int creature_sel = 0;
static Creature creature = {0};

static Room room = {0};
static RoomData room_data = {0};

Vector2i tile_coords = {0}; // mouse tile coords

static void draw_room();
static void editor_camera_set();


void room_editor_init()
{
    show_tile_grid = true;
    debug_enabled = true;

    ecam_pos_x = CENTER_X;
    ecam_pos_y = CENTER_Y;

    creature_sel = 0;
    creature.type = creature_sel;
    creature_init_props(&creature);

    for(int i = 0; i < CREATURE_TYPE_MAX; ++i)
        creature_names[i] = creature_type_name(i);
    creature_names[CREATURE_TYPE_MAX] = "Eraser";

    tile_sel = 0;
    for(int i = 0; i < NUM_TILE_TYPES; ++i)
    {
        TileType tt = i+1;
        if(tt == TILE_FLOOR)
            tile_names[i] = "Floor";
        else if(tt == TILE_PIT)
            tile_names[i] = "Pit";
        else if(tt == TILE_BOULDER)
            tile_names[i] = "Boulder";
    }

    for(int rj = 0; rj < ROOM_TILE_SIZE_Y; ++rj)
    {
        for(int ri = 0; ri < ROOM_TILE_SIZE_X; ++ri)
        {
            room_data.tiles[ri][rj] = TILE_FLOOR;
        }
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

}

void room_editor_update(float dt)
{
    gfx_clear_lines();

    text_list_update(text_lst, dt);

    window_get_mouse_view_coords(&mx, &my);
    window_get_mouse_world_coords(&wmx, &wmy);

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



    editor_camera_set();
}

// static void draw_room()
// {

//     // draw walls
//     float x = room_area.x - room_area.w/2.0;
//     float y = room_area.y - room_area.h/2.0;
//     float w = TILE_SIZE;
//     float h = TILE_SIZE;
//     Rect r = {(x+w/2.0), (y+h/2.0), w,h};   // top left tile of the room

//     for(int i = 1; i < ROOM_TILE_SIZE_X+1; ++i) // top
//         gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_UP, r.x + i*w,r.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
//     for(int i = 1; i < ROOM_TILE_SIZE_Y+1; ++i) // right
//         gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_RIGHT, r.x + (ROOM_TILE_SIZE_X+1)*w,r.y+i*h, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
//     for(int i = 1; i < ROOM_TILE_SIZE_X+1; ++i) // bottom
//         gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_DOWN, r.x + i*w,r.y+(ROOM_TILE_SIZE_Y+1)*h, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
//     for(int i = 1; i < ROOM_TILE_SIZE_Y+1; ++i) // left
//         gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_LEFT, r.x,r.y+i*h, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);

//     // wall corners
//     gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_LU, r.x,r.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
//     gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_UR, r.x+(ROOM_TILE_SIZE_X+1)*w,r.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
//     gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_RD, r.x+(ROOM_TILE_SIZE_X+1)*w,r.y+(ROOM_TILE_SIZE_Y+1)*h, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);
//     gfx_draw_image(dungeon_image, SPRITE_TILE_WALL_CORNER_DL, r.x,r.y+(ROOM_TILE_SIZE_Y+1)*h, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);


//     // draw room
//     for(int _y = 0; _y < ROOM_TILE_SIZE_Y; ++_y)
//     {
//         for(int _x = 0; _x < ROOM_TILE_SIZE_X; ++_x)
//         {

//             TileType tt = room_data.tiles[_x][_y];
//             uint8_t sprite = level_get_tile_sprite(tt);

//             // +1 for walls
//             float draw_x = r.x + (_x+1)*w;
//             float draw_y = r.y + (_y+1)*h;

//             gfx_draw_image(dungeon_image, sprite, draw_x, draw_y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);

//             gfx_draw_rect_xywh(draw_x, draw_y, w, h, COLOR_CYAN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
//         }
//     }

//     if(debug_enabled)
//     {
//         room_draw_walls(&room);
//     }

// }




void room_editor_draw()
{
    gfx_clear_buffer(background_color);



    // draw_room();
    level_draw_room(&room, &room_data, 0, 0);

    tile_coords = level_get_room_coords_by_pos(wmx, wmy);
    Rect tile_rect = level_get_tile_rect(tile_coords.x, tile_coords.y);

    if(tab_sel == 0)
    {
        gfx_draw_rect(&tile_rect, COLOR_CYAN, NOT_SCALED, NO_ROTATION, 0.5, true, true);
        gfx_draw_image(dungeon_image, level_get_tile_sprite(tile_sel+1), tile_rect.x, tile_rect.y, room.color, 1.0, 0.0, 1.0, false, true);
    }
    else if(tab_sel == 1) // creatures
    {
        uint32_t color = COLOR_RED;

        if(creature_sel == CREATURE_TYPE_MAX)   // eraser
        {
            color = COLOR_PINK;
            gfx_draw_rect(&tile_rect, COLOR_PINK, NOT_SCALED, NO_ROTATION, 0.5, true, true);
        }
        else
        {
            gfx_draw_image(creature.image, 0, tile_rect.x, tile_rect.y, COLOR_RED, 1.0, 0.0, 1.0, false, true);
        }

    }
    else if(tab_sel == 2)
    {

    }


    gfx_draw_rect(&room_area, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, IN_WORLD);


    imgui_begin_panel("Room Editor", view_width - 300, 1, true);

        imgui_newline();
        char* buttons[] = {"Room", "Creatures", "Pickups"};
        tab_sel = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        switch(tab_sel)
        {
            // room
            case 0:
            {
                imgui_number_box("Camera Zoom", 0, 99, &ecam_pos_z);

                tile_sel = imgui_dropdown(tile_names, NUM_TILE_TYPES, "Select Tile", &tile_sel);

                imgui_text("Camera Zoom: %.2f", camera_get_zoom());
                imgui_text("Mouse (view): %.1f, %.1f", mx, my);

                imgui_text("Tile Coords: %d, %d", tile_coords.x, tile_coords.y);
            } break;

            case 1:
            {
                int _creature_sel = creature_sel;
                creature_sel = imgui_dropdown(creature_names, CREATURE_TYPE_MAX+1, "Select Creature", &creature_sel);
                if(creature_sel != _creature_sel && creature_sel != CREATURE_TYPE_MAX)
                {
                    creature.type = creature_sel;
                    creature_init_props(&creature);
                }
            } break;

            case 2:
            {

            } break;

        }

    imgui_end();

}

static void editor_camera_set()
{
    camera_move(ecam_pos_x, ecam_pos_y, (float)ecam_pos_z/100.0, false, NULL);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);
}