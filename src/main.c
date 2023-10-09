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
GameState game_state = GAME_STATE_MENU;
Timer game_timer = {0};
text_list_t* text_lst = NULL;

// Settings
// uint32_t background_color = 0x00303030;
uint32_t background_color = 0x00202020;
// uint32_t background_color = COLOR_BLACK;

int mx=0, my=0;

Rect room_area = {0};
Rect margin_left = {0};
Rect margin_right = {0};
Rect margin_top = {0};
Rect margin_bottom = {0};


Vector2f aim_camera_offset = {0};
float cam_zoom = 0.00;

Level level;
unsigned int seed = 0;

enum
{
    MENU_KEY_UP,
    MENU_KEY_DOWN,
    MENU_KEY_ENTER,
    MENU_KEY_MAX
};
PlayerInput menu_keys[MENU_KEY_MAX] = {0};
int menu_selected_option = 0;
const char* menu_options[] = {
    "Play Local",
    "Play Online",
    "Host Local Server",
    "Join Local Server",
    "Settings",
    "Help",
    "Exit"
};


// =========================
// Function Prototypes
// =========================
int main(int argc, char* argv[]);
void camera_set();
void run();
void parse_args(int argc, char* argv[]);
void init();
void set_menu_keys();
void deinit();
void update(float dt);
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

    camera_move(view_width/2.0, view_height/2.0, true, NULL);

    // camera_move(player->pos.x, player->pos.y, true, &map_view_area);
    // camera_zoom(cam_zoom,false);

    text_lst = text_list_init(50, 10.0, view_height - 10.0, 0.08, COLOR_WHITE, false, TEXT_ALIGN_LEFT);

    run();

    return 0;
}


void update_input_state(PlayerInput* input, float dt)
{
    input->hold = false;

    if(input->state && !input->prior_state)
    {
        input->toggled_on = true;
        input->hold_counter = 0;
    }
    else
    {
        input->toggled_on = false;
    }
    if(!input->state && input->prior_state)
    {
        input->toggled_off = true;
    }
    else
    {
        input->toggled_off = false;
    }
    input->prior_state = input->state;

    if(input->state && !input->toggled_on && !FEQ0(input->hold_period))
    {
        input->hold_counter += dt;
        if(input->hold_counter >= input->hold_period)
        {
            input->hold = true;
            input->hold_counter = 0;
        }
    }
}

void set_menu_keys()
{
    window_controls_clear_keys();
    window_controls_add_key(&menu_keys[MENU_KEY_UP].state, GLFW_KEY_W);
    window_controls_add_key(&menu_keys[MENU_KEY_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&menu_keys[MENU_KEY_ENTER].state, GLFW_KEY_ENTER);

    menu_keys[MENU_KEY_UP].hold_period = 0.3;
    menu_keys[MENU_KEY_DOWN].hold_period = 0.3;
}

void set_game_state(GameState state)
{
    if(state != game_state)
    {
        game_state = state;
        LOGI("Set game state: %d", game_state);
        switch(game_state)
        {
            case GAME_STATE_MENU:
            {
                set_menu_keys();
            } break;
            case GAME_STATE_PLAYING:
            {
                player_init_keys();
            } break;
        }
    }
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

    level = level_generate(seed);

    room_area.w = ROOM_W;
    room_area.h = ROOM_H;
    room_area.x = CENTER_X;
    room_area.y = CENTER_Y;

    RectXY rxy = {0};
    rect_to_rectxy(&room_area, &rxy);

    float margin_left_w = rxy.x[TL];
    float margin_right_w = view_width - rxy.x[TR];

    float margin_top_h = rxy.y[TL];
    float margin_bottom_h = view_height - rxy.y[BL];

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

    set_menu_keys();
}

void deinit()
{
    if(!initialized) return;
    initialized = false;
    shader_deinit();
    window_deinit();
}


void update(float dt)
{
    gfx_clear_lines();

    window_get_mouse_view_coords(&mx, &my);

    if(game_state == GAME_STATE_MENU)
    {


        for(int i = 0; i < MENU_KEY_MAX; ++i)
        {
            update_input_state(&menu_keys[i], dt);
        }

        int num_opts = sizeof(menu_options)/sizeof(menu_options[0]);

        if(menu_keys[MENU_KEY_UP].toggled_on || menu_keys[MENU_KEY_UP].hold)
        {
            menu_selected_option--;
            if(menu_selected_option < 0) menu_selected_option = num_opts-1;
        }

        if(menu_keys[MENU_KEY_DOWN].toggled_on || menu_keys[MENU_KEY_DOWN].hold)
        {
            menu_selected_option++;
            if(menu_selected_option >= num_opts) menu_selected_option = 0;
        }

        if(menu_keys[MENU_KEY_ENTER].toggled_on)
        {
            //TODO
            if(STR_EQUAL(menu_options[menu_selected_option], "Play Local"))
            {
                set_game_state(GAME_STATE_PLAYING);
            }
            else if(STR_EQUAL(menu_options[menu_selected_option], "Play Online"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", menu_options[menu_selected_option]);
            }
            else if(STR_EQUAL(menu_options[menu_selected_option], "Host Local Server"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", menu_options[menu_selected_option]);
            }
            else if(STR_EQUAL(menu_options[menu_selected_option], "Join Local Server"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", menu_options[menu_selected_option]);
            }
            else if(STR_EQUAL(menu_options[menu_selected_option], "Settings"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", menu_options[menu_selected_option]);
            }
            else if(STR_EQUAL(menu_options[menu_selected_option], "Help"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", menu_options[menu_selected_option]);
            }
            else if(STR_EQUAL(menu_options[menu_selected_option], "Exit"))
            {
                exit(0);
            }

        }

    }
    else if(game_state == GAME_STATE_PLAYING)
    {
        player_update(player, dt);
    }


    text_list_update(text_lst, dt);

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

    float tlx = area->x - area->w/2.0;
    float tly = area->y - area->h/2.0;

    // room width/height
    float rw = area->w/MAX_ROOMS_GRID_X;
    float rh = area->h/MAX_ROOMS_GRID_Y;
    float rwh = MIN(rw, rh);
    area->w = rwh*MAX_ROOMS_GRID_X;
    area->h = rwh*MAX_ROOMS_GRID_Y;

    float margin = rwh / 10.0;
    float room_wh = rwh-margin;
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

                float draw_x = tlx + x*rwh + rwh/2.0;
                float draw_y = tly + y*rwh + rwh/2.0;
                Rect r = RECT(draw_x, draw_y, room_wh, room_wh);
                gfx_draw_rect(&r, color_room, NOT_SCALED, NO_ROTATION, 1.0, true, true);

                if(room.doors[DOOR_UP])
                {
                    Rect door = RECT(draw_x-door_offset, draw_y-rwh/2.0, door_w, door_h);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

                if(room.doors[DOOR_DOWN])
                {
                    Rect door = RECT(draw_x+door_offset, draw_y+rwh/2.0, door_w, door_h);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

                if(room.doors[DOOR_RIGHT])
                {
                    Rect door = RECT(draw_x+rwh/2.0, draw_y-door_offset, door_h, door_w);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

                if(room.doors[DOOR_LEFT])
                {
                    Rect door = RECT(draw_x-rwh/2.0, draw_y+door_offset, door_h, door_w);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, 1.0, true, true);
                }

            }
        }
    }

    float px = player->pos.x - rect_tlx(&room_area);
    float py = player->pos.y - rect_tly(&room_area);
    px *= (room_wh/room_area.w);
    py *= (room_wh/room_area.h);
    Rect pr = RECT(px, py, margin, margin);

    // translate to minimap
    float draw_x = tlx + player->curr_room.x*rwh + rwh/2.0;
    float draw_y = tly + player->curr_room.y*rwh + rwh/2.0;

    Rect r = RECT(draw_x, draw_y, room_wh, room_wh);
    gfx_get_absolute_coords(&pr, ALIGN_CENTER, &r, ALIGN_CENTER);

    gfx_draw_rect(&pr, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);
}

void draw_minimap()
{
    float w = margin_left.w;
    // define the minimap location and size
    Rect minimap_area = RECT(w/2.0, w/2.0, w, w);
    // translate the location
    gfx_get_absolute_coords(&minimap_area, ALIGN_CENTER, &margin_left, ALIGN_CENTER);
    draw_level(&minimap_area);

    // solve for size
    float s = 0.01;
    for(int i = 0; i < 100; ++i)
    {
        Vector2f seed_size = gfx_string_get_size(s, "Level Seed: %u", seed);
        if(ABS(seed_size.y - minimap_area.h/14.0) <= 0.2)
        {
            break;
        }
        s += 0.01;
    }

    float seed_scale = s;
    Vector2f seed_size = gfx_string_get_size(seed_scale, "Level Seed: %u", seed);

    Rect seed_rect = minimap_area;
    seed_rect.h = seed_size.y + 2.0;
    seed_rect.y += minimap_area.h/2.0 + seed_rect.h/2.0;
    gfx_draw_rect(&seed_rect, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 0.5, true, true);

    gfx_draw_string(rect_tlx(&seed_rect), rect_tly(&seed_rect), COLOR_WHITE, seed_scale, NO_ROTATION, FULL_OPACITY, IN_WORLD, DROP_SHADOW, "Level Seed: %u", seed);
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
    if(game_state == GAME_STATE_PLAYING)
    {
        level_draw_room(&level.rooms[player->curr_room.x][player->curr_room.y]);
        gfx_draw_rect(&room_area, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
    else if(game_state == GAME_STATE_MENU)
    {
        //TODO
        level_draw_room(&level.rooms[player->curr_room.x][player->curr_room.y]);
        gfx_draw_rect(&room_area, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }


    if(game_state == GAME_STATE_MENU)
    {

        // draw menu
        float menu_item_scale = 0.3;

        // get text height first
        Vector2f text_size = gfx_string_get_size(menu_item_scale, "A");
        float margin = 1.0;

        int num_opts = sizeof(menu_options)/sizeof(menu_options[0]);

        float total_height = num_opts * (text_size.y+margin);

        float x = view_width*0.35;
        float y = CENTER_Y - total_height / 2.0;

        for(int i = 0; i < num_opts; ++i)
        {
            uint32_t color = COLOR_WHITE;
            if(i == menu_selected_option) color = COLOR_BLUE;
            gfx_draw_string(x,y, color, menu_item_scale, NO_ROTATION, FULL_OPACITY, NOT_IN_WORLD, DROP_SHADOW, (char*)menu_options[i]);
            y += (text_size.y+margin);
        }

    }


    // draw map
    draw_minimap();

    if(game_state == GAME_STATE_PLAYING)
    {
        // draw player
        player_draw(player);
    }

    if(debug_enabled)
    {
        gfx_draw_string(10, 10, COLOR_WHITE, 0.08, NO_ROTATION, FULL_OPACITY, IN_WORLD, NO_DROP_SHADOW, "%d, %d", mx, my);
        gfx_draw_rect(&margin_left, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        gfx_draw_rect(&margin_right, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        gfx_draw_rect(&margin_top, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
        gfx_draw_rect(&margin_bottom, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }

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
                else if(game_state != GAME_STATE_MENU)
                {
                    set_game_state(GAME_STATE_MENU);
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
