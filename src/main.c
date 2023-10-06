#include "headers.h"
#include "main.h"
#include "window.h"
#include "shader.h"
#include "timer.h"
#include "gfx.h"
#include "math2d.h"
#include "imgui.h"
#include "log.h"
#include "particles.h"
#include "effects.h"
#include "camera.h"
#include "level.h"
#include "player.h"
#include "text_list.h"

// =========================
// Global Vars
// =========================

bool initialized = false;
bool debug_enabled = false;
Timer game_timer = {0};
text_list_t* text_lst = NULL;

// Settings
uint32_t background_color = 0x00303030;
// uint32_t background_color = COLOR_BLACK;


Rect room_area = {0};
Rect margin_left = {0};
Rect margin_right = {0};
Rect margin_top = {0};
Rect margin_bottom = {0};


Vector2f aim_camera_offset = {0};
float cam_zoom = 0.00;

Level level;
unsigned int seed = 0;

// =========================
// Function Prototypes
// =========================
int main(int argc, char* argv[]);
void camera_set();
void run();
void parse_args(int argc, char* argv[]);
void init();
void deinit();
void update(float _dt);
void draw();
void key_cb(GLFWwindow* window, int key, int scan_code, int action, int mods);


// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{

    init_timer();
    log_init(0);

    parse_args(argc, argv);

    time_t t;
    srand((unsigned) time(&t));

    init();

    room_area.w = ROOM_W;
    room_area.h = ROOM_H;
    room_area.x = CENTER_X;
    room_area.y = CENTER_Y;

    // margins
    float margin_left_w = room_area.x - room_area.w/2.0;
    float margin_right_w = view_width - (room_area.x + room_area.w/2.0);

    float margin_top_h = room_area.y - room_area.h/2.0;
    float margin_bottom_h = view_height - (room_area.y + room_area.h/2.0);

    margin_left.w = margin_left_w;
    margin_left.h = view_height - margin_top_h - margin_bottom_h;
    margin_left.x = margin_left.w/2.0;
    margin_left.y = CENTER_Y;

    margin_right.w = margin_right_w;
    margin_right.h = view_height - margin_top_h - margin_bottom_h;
    margin_right.x = view_width - margin_right.w/2.0;
    margin_right.y = CENTER_Y;

    margin_top.w = view_width;
    margin_top.h = margin_top_h;
    margin_top.x = CENTER_X;
    margin_top.y = margin_top_h/2.0;

    margin_bottom.w = view_width;
    margin_bottom.h = margin_bottom_h;
    margin_bottom.x = CENTER_X;
    margin_bottom.y = view_height - margin_bottom_h/2.0;


    level = level_generate(seed);

    camera_move(view_width/2.0, view_height/2.0, true, NULL);

    // camera_move(player->pos.x, player->pos.y, true, &map_view_area);
    // camera_zoom(cam_zoom,false);

    text_lst = text_list_init(50, 10.0, view_height - 10.0, 0.08, COLOR_WHITE, false, TEXT_ALIGN_LEFT);

    run();

    return 0;
}

// also checks if the mouse is off the screen
void camera_set()
{
    int mx, my;
    window_get_mouse_view_coords(&mx, &my);

    // if(player->item.mouse_aim)
    if(false)
    {
        float _vw = view_width;
        float _vh = view_height;

        float r = 0.2;  //should be <= 0.5 to make sense otherwise player will end up off of the screen
        float ox = (mx - _vw/2.0);
        float oy = (my - _vh/2.0);
        float xr = _vw*r;
        float yr = _vh*r;
        ox = 2.0*xr*(ox/_vw);
        oy = 2.0*yr*(oy/_vh);
        ox = RANGE(ox, -1.0*xr, xr);
        oy = RANGE(oy, -1.0*yr, yr);
        aim_camera_offset.x = ox;
        aim_camera_offset.y = oy;
    }
    else
    {
        aim_camera_offset.x = 0.0;
        aim_camera_offset.y = 0.0;
    }

    if(!window_is_cursor_enabled())
    {
        if(mx >= view_width || mx <= 0 || my >= view_height || my <= 0)
        {
            int new_mx = RANGE(mx, 0, view_width);
            int new_my = RANGE(my, 0, view_height);
            window_set_mouse_view_coords(new_mx, new_my);
        }
    }

    float cam_pos_x = player->pos.x + aim_camera_offset.x;
    float cam_pos_y = player->pos.y + aim_camera_offset.y;

    camera_zoom(cam_zoom,false);
    // camera_move(cam_pos_x, cam_pos_y, false, &map_view_area);
    camera_move(cam_pos_x, cam_pos_y, false, NULL);
}

void run()
{

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);
    double curr_time = timer_get_time();
    double new_time  = 0.0;
    double accum = 0.0;
    const double dt = 1.0/TARGET_FPS;

    // loop
    for(;;)
    {
        new_time = timer_get_time();
        double frame_time = new_time - curr_time;
        curr_time = new_time;
        accum += frame_time;

        window_poll_events();
        if(window_should_close())
            break;

        while(accum >= dt)
        {
            update(dt);
            accum -= dt;
        }

        draw();

        timer_wait_for_frame(&game_timer);

        window_swap_buffers();
        window_mouse_update_actions();
    }
}


void parse_args(int argc, char* argv[])
{
    // if(argc > 1)
    // {
    //     for(int i = 1; i < argc; ++i)
    //     {
    //         if(argv[i][0] == '-' && argv[i][1] == '-')
    //         {
    //         }
    //     }
    // }
}

void init()
{
    if(initialized) return;
    initialized = true;

    LOGI("Resolution: %d %d",VIEW_WIDTH, VIEW_HEIGHT);
    bool success = window_init(VIEW_WIDTH, VIEW_HEIGHT);

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    window_controls_set_cb(key_cb);
    window_controls_set_key_mode(KEY_MODE_NORMAL);

    LOGI("Initializing...");

    LOGI(" - Shaders.");
    shader_load_all();

    LOGI(" - Graphics.");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);

    LOGI(" - Camera.");
    camera_init();

    LOGI(" - Effects.");
    effects_load_all();

    LOGI(" - Room Data.");
    level_init();

    LOGI(" - Player.");
    player_init();

    imgui_load_theme("retro.theme");
}


void deinit()
{
    if(!initialized) return;
    initialized = false;
    shader_deinit();
    window_deinit();
}


void update(float _dt)
{
    gfx_clear_lines();

    player_update();

    text_list_update(text_lst, _dt);

    // camera_set();
    camera_zoom(cam_zoom,false);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);
}

void draw_level(Rect* area)
{
    uint32_t color_room = COLOR(0x22,0x48,0x70);
    // uint32_t color_bg = COLOR(0x12,0x2c,0x34);
    uint32_t color_bg = COLOR_BLACK;
    // uint32_t color_door = COLOR(0x4e,0xa5,0xd9);
    uint32_t color_door = color_room;

    gfx_draw_rect(area, color_bg, NOT_SCALED, NO_ROTATION, 0.3, true, true);

    //TODO
    int px_index = 3;
    int py_index = 3;

    float wh = MIN(area->w, area->h);


    float rw = area->w/MAX_ROOMS_GRID_X;
    float rh = area->h/MAX_ROOMS_GRID_Y;
    float rwh = MIN(rw, rh);
    float tlx = area->x - area->w/2.0;
    float tly = area->y - area->h/2.0;
    float margin = rw / 10.0;

    const float door_w = rw/14.0;
    const float door_h = margin;

    const float door_offset = door_w/2.0;

    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room room = level.rooms[x][y];
            if(room.valid)
            {

                float draw_x = tlx + x*rw + rw/2.0;
                float draw_y = tly + y*rh + rh/2.0;
                Rect r = RECT(draw_x, draw_y, rw-margin, rh-margin);
                gfx_draw_rect(&r, color_room, NOT_SCALED, NO_ROTATION, 1.0, true, true);

                if(room.doors[DOOR_UP])
                {
                    Rect door = RECT(draw_x-door_offset, draw_y-rh/2.0, door_w, door_h);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

                if(room.doors[DOOR_DOWN])
                {
                    Rect door = RECT(draw_x+door_offset, draw_y+rh/2.0, door_w, door_h);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

                if(room.doors[DOOR_RIGHT])
                {
                    Rect door = RECT(draw_x+rw/2.0, draw_y-door_offset, door_h, door_w);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

                if(room.doors[DOOR_LEFT])
                {
                    Rect door = RECT(draw_x-rw/2.0, draw_y+door_offset, door_h, door_w);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

            }

        }
    }
    
    // @TEMP
    
    level_draw_room(&level.rooms[player->curr_room_x][player->curr_room_y]);

<<<<<<< Updated upstream
    float px = player->pos.x - (room_area.x - room_area.w/2.0);
    float py = player->pos.y - (room_area.y - room_area.h/2.0);
    px /= (rw-margin);
    py /= (rh-margin);
    Rect pr = RECT(px, py, 1.0, 1.0);

    // translate to minimap
    float draw_x = tlx + px_index*rw + rw/2.0;
    float draw_y = tly + py_index*rh + rh/2.0;
    Rect r = RECT(draw_x, draw_y, rw-margin, rh-margin);

    gfx_get_absolute_coords(&pr, ALIGN_CENTER, &r, ALIGN_CENTER);

    gfx_draw_rect(&pr, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);
    // print_rect(&pr);
}

void draw()
{
    gfx_clear_buffer(background_color);

    float title_scale = 0.5;
    Vector2f title_size = gfx_string_get_size(title_scale, "SCUM");
    Rect title_r = RECT(margin_top.w/2.0, margin_top.h/2.0, title_size.x, title_size.y);
    gfx_get_absolute_coords(&title_r, ALIGN_CENTER, &margin_top, ALIGN_TOP_LEFT);
    gfx_draw_string(title_r.x, title_r.y, COLOR_WHITE, title_scale, NO_ROTATION, FULL_OPACITY, IN_WORLD, NO_DROP_SHADOW, "SCUM");


    // draw room
    uint32_t color_bg = COLOR(0x12,0x2c,0x34);
    gfx_draw_rect(&room_area, color_bg, NOT_SCALED, NO_ROTATION, 1.0, true, true);
    level_draw_room(&level.rooms[3][3]);

    // draw map

    float w = margin_left.w;
    Rect minimap_area = RECT(w/2.0, w/2.0, w, w);

    float seed_scale = 0.08;
    Vector2f seed_size = gfx_string_get_size(seed_scale, "Level Seed: %u", seed);
    seed_size.y += 2.0;
    Rect seed_rect = RECT(minimap_area.x, minimap_area.y+minimap_area.h/2.0+seed_size.y/2.0, minimap_area.w, seed_size.y);
    Rect seed_rect2 = seed_rect;
    seed_rect2.x = 0.0;
    seed_rect2.y = 0.0;

    gfx_get_absolute_coords(&minimap_area, ALIGN_CENTER, &margin_left, ALIGN_CENTER);
    draw_level(&minimap_area);


    gfx_get_absolute_coords(&seed_rect, ALIGN_CENTER, &margin_left, ALIGN_CENTER);
    gfx_draw_rect(&seed_rect, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 0.5, true, true);

    gfx_get_absolute_coords(&seed_rect2, ALIGN_TOP_LEFT, &seed_rect, ALIGN_TOP_LEFT);
    gfx_draw_string(seed_rect2.x, seed_rect2.y, COLOR_WHITE, seed_scale, NO_ROTATION, FULL_OPACITY, IN_WORLD, DROP_SHADOW, "Level Seed: %u", seed);


    // draw player
    player_draw(player);

    if(debug_enabled)
    {
        gfx_draw_rect(&margin_left, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        gfx_draw_rect(&margin_right, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        gfx_draw_rect(&margin_top, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        gfx_draw_rect(&margin_bottom, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
=======
    player_draw();

    gfx_draw_rect(&margin_left, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    gfx_draw_rect(&margin_right, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    gfx_draw_rect(&margin_top, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    gfx_draw_rect(&margin_bottom, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
>>>>>>> Stashed changes

#if 0
    {
        Rect r = RECT(50, 50, 5,10);
        gfx_get_absolute_coords(&r, ALIGN_CENTER, &margin_right, ALIGN_CENTER);
        gfx_draw_rect(&r, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        r.w = 1.0;
        r.h = 1.0;
        gfx_draw_rect(&r, COLOR_BLUE, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
    {
        float scale = 0.1;
        Vector2f size = gfx_string_get_size(scale, "SCUM");
        Rect r = RECT(0,0, size.x, size.y);
        gfx_get_absolute_coords(&r, ALIGN_BOTTOM_RIGHT, &margin_right, ALIGN_TOP_LEFT);
        gfx_draw_string(r.x, r.y, COLOR_WHITE, scale, NO_ROTATION, FULL_OPACITY, IN_WORLD, NO_DROP_SHADOW, "SCUM");
    }
#endif

    text_list_draw(text_lst);
}


void key_cb(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    // printf("key: %d, action: %d\n", key, action);
    bool ctrl = (mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL;

    KeyMode kmode = window_controls_get_key_mode();
    if(kmode == KEY_MODE_NORMAL)
    {
        if(action == GLFW_PRESS)
        {
            if(ctrl && key == GLFW_KEY_C)
            {
                window_set_close(1);
            }
            if(key == GLFW_KEY_ESCAPE)
            {

                if(debug_enabled)
                {
                    debug_enabled = false;
                }
            }
            else if(key == GLFW_KEY_F2)
            {
                debug_enabled = !debug_enabled;
            }
            else if(key == GLFW_KEY_ENTER)
            {

            }

        }
    }
}
