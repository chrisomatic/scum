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
PlayerInput editor_keys[EDITOR_KEY_MAX] = {0};


float ecam_pos_x = 0;
float ecam_pos_y = 0;
int ecam_pos_z = 55;

void editor_camera_set();


void room_editor_init()
{
    ecam_pos_x = CENTER_X;
    ecam_pos_y = CENTER_Y;

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


void room_editor_draw()
{
    gfx_clear_buffer(background_color);

    gfx_draw_rect(&room_area, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, IN_WORLD);







    imgui_begin_panel("Room Editor", view_width - 300, 1, true);

        imgui_number_box("Camera Zoom", 0, 100, &ecam_pos_z);

        imgui_text("Mouse (view): %.1f, %.1f", mx, my);
        imgui_text("Camera Zoom: %.2f", camera_get_zoom());

    imgui_end();

}

void editor_camera_set()
{
    camera_move(ecam_pos_x, ecam_pos_y, (float)ecam_pos_z/100.0, true, NULL);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);
}