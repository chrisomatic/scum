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
#include "projectile.h"
#include "player.h"
#include "net.h"
#include "text_list.h"

// =========================
// Global Vars
// =========================

bool initialized = false;
bool debug_enabled = false;
bool paused = false;
GameState game_state = GAME_STATE_MENU;
Timer game_timer = {0};
text_list_t* text_lst = NULL;
bool show_big_map = false;
GameRole role = ROLE_UNKNOWN;

// Settings
// uint32_t background_color = 0x00303030;
uint32_t background_color = 0x00202020;
// uint32_t background_color = COLOR_BLACK;

int mx=0, my=0;

Rect room_area = {0};
Rect player_area = {0};
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

void start_server();


// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{
    init_timer();
    log_init(0);

    time_t t;
    srand((unsigned) time(&t));

    parse_args(argc, argv);

    if(role == ROLE_SERVER)
    {
        start_server();
    }

    init();

    camera_move(view_width/2.0, view_height/2.0, true, NULL);

    // camera_move(player->pos.x, player->pos.y, true, &map_view_area);
    // camera_zoom(cam_zoom,false);


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
                player->active = true;
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

void start_server()
{
    //init
    gfx_image_init();
    player_init();

    level_init();
    level = level_generate(seed);
    level_print(&level);

    projectile_init();

    view_width = VIEW_WIDTH;
    view_height = VIEW_HEIGHT;

    room_area.w = ROOM_W;
    room_area.h = ROOM_H;
    room_area.x = CENTER_X;
    room_area.y = CENTER_Y;

    memcpy(&player_area, &room_area, sizeof(Rect));
    player_area.w -= 32;
    player_area.h -= 48;

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

    // start
    net_server_start();
}

void init()
{
    if(initialized) return;
    initialized = true;

    text_lst = text_list_init(50, 10.0, view_height - 10.0, 0.08, COLOR_WHITE, false, TEXT_ALIGN_LEFT);

    LOGI("Initializing...");

    LOGI("Resolution: %d %d",VIEW_WIDTH, VIEW_HEIGHT);
    bool success = window_init(VIEW_WIDTH, VIEW_HEIGHT);

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    window_controls_set_cb(key_cb);
    window_controls_set_key_mode(KEY_MODE_NORMAL);

    LOGI(" - Shaders.");
    shader_load_all();

    LOGI(" - Graphics.");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);

    LOGI(" - Camera.");
    camera_init();

    LOGI(" - Player.");
    player = &players[0];
    player_init();

    LOGI(" - Effects.");
    effects_load_all();

    LOGI(" - Room Data.");
    level_init();

    LOGI(" - Projectiles.");
    projectile_init();

    imgui_load_theme("retro.theme");

    level = level_generate(seed);

    room_area.w = ROOM_W;
    room_area.h = ROOM_H;
    room_area.x = CENTER_X;
    room_area.y = CENTER_Y;

    memcpy(&player_area, &room_area, sizeof(Rect));
    player_area.w -= 32;
    player_area.h -= 48;

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

void parse_args(int argc, char* argv[])
{
    // --local
    // --server
    // --client <ip-addr>

    role = ROLE_UNKNOWN;

    if(argc > 1)
    {
        for(int i = 1; i < argc; ++i)
        {
            if(argv[i][0] == '-' && argv[i][1] == '-')
            {
                // local
                if(strncmp(argv[i]+2,"local",5) == 0)
                {
                    role = ROLE_LOCAL;
                }

                // server
                else if(strncmp(argv[i]+2,"server",6) == 0)
                {
                    role = ROLE_SERVER;
                }

                // client
                else if(strncmp(argv[i]+2,"client",6) == 0)
                {
                    role = ROLE_CLIENT;
                }
            }
            else
            {
                if(role == ROLE_CLIENT)
                    net_client_set_server_ip(argv[i]);
            }
        }
    }
}

void update(float dt)
{
    gfx_clear_lines();

    // Update Client
    // ------------------------------
    if(role == ROLE_CLIENT)
    {
        ConnectionState check_state = net_client_get_state();

        if(check_state != CONNECTED)
        {
            net_client_connect_update(); // progress through connection routine

            ConnectionState curr_state = net_client_get_state();

            if(curr_state != check_state)
            {
                // connection state changed
                switch(curr_state)
                {
                    case DISCONNECTED:
                        text_list_add(text_lst, 3.0, "Disconnected from Server.");
                        break;
                    case SENDING_CONNECTION_REQUEST:
                        text_list_add(text_lst, 3.0, "Sending Connection Request.");
                        break;
                    case SENDING_CHALLENGE_RESPONSE:
                        text_list_add(text_lst, 3.0, "Sending Challenge Response.");
                        break;
                    case CONNECTED:
                    {
                        text_list_add(text_lst, 3.0, "Connected to Server.");
                        int id = net_client_get_id();
                        player = &players[id];
                        player_init_keys();

                    } break;
                }
            }

            if(curr_state != CONNECTED)
                return;
        }

        // Client connected
        net_client_update();

        player_handle_net_inputs(player, dt);

        for(int i = 0; i < plist->count; ++i)
        {
            projectile_lerp(&projectiles[i], dt);
            projectile_update_hit_box(&projectiles[i]);
        }

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            Player* p = &players[i];
            if(p->active)
            {
                memcpy(&p->hitbox_prior, &p->hitbox, sizeof(Rect));
                player_lerp(p, dt);
            }
        }

        text_list_update(text_lst, dt);

        return;
    }

    // Update Local
    // ------------------------------

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
            const char* s = menu_options[menu_selected_option];
            //TODO
            if(STR_EQUAL(s, "Play Local"))
            {
                set_game_state(GAME_STATE_PLAYING);
            }
            else if(STR_EQUAL(s, "Play Online"))
            {
                role = ROLE_CLIENT;
                net_client_init();
                net_client_set_server_ip(ONLINE_SERVER_IP);
                set_game_state(GAME_STATE_PLAYING);
            }
            else if(STR_EQUAL(s, "Host Local Server"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", s);
                role = ROLE_SERVER;
                deinit();

                // init
                gfx_image_init();
                player_init();
                projectile_init();

                net_server_start();
            }
            else if(STR_EQUAL(s, "Join Local Server"))
            {
                role = ROLE_CLIENT;
                net_client_init();
                net_client_set_server_ip(LOCAL_SERVER_IP);
                set_game_state(GAME_STATE_PLAYING);
            }
            else if(STR_EQUAL(s, "Settings"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", s);
            }
            else if(STR_EQUAL(s, "Help"))
            {
                text_list_add(text_lst, 2.0, "'%s' not supported", s);
            }
            else if(STR_EQUAL(s, "Exit"))
            {
                window_set_close(1);
            }
        }
    }
    else if(game_state == GAME_STATE_PLAYING)
    {
        if(!paused)
        {
            player_update(player, dt);
            projectile_update(dt);
        }
    }


    text_list_update(text_lst, dt);

    // camera_set();
    camera_zoom(cam_zoom,false);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);
}

void draw_level(Rect* area, uint32_t color_bg, float opacity_bg, uint32_t color_room, float opacity_room, uint32_t color_player, float opacity_player)
{
    // float opacity_bg = 0.3;
    // uint32_t color_bg = COLOR_BLACK;

    // float opacity_room = 0.3;
    // uint32_t color_room = COLOR(0x22,0x48,0x70);

    float opacity_door = opacity_room;
    uint32_t color_door = color_room;

    gfx_draw_rect(area, color_bg, NOT_SCALED, NO_ROTATION, opacity_bg, true, true);

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
                gfx_draw_rect(&r, color_room, NOT_SCALED, NO_ROTATION, opacity_room, true, true);

                if(room.doors[DOOR_UP])
                {
                    Rect door = RECT(draw_x-door_offset, draw_y-rwh/2.0, door_w, door_h);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, opacity_door, true, true);
                }

                if(room.doors[DOOR_DOWN])
                {
                    Rect door = RECT(draw_x+door_offset, draw_y+rwh/2.0, door_w, door_h);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, opacity_door, true, true);
                }

                if(room.doors[DOOR_RIGHT])
                {
                    Rect door = RECT(draw_x+rwh/2.0, draw_y-door_offset, door_h, door_w);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, opacity_door, true, true);
                }

                if(room.doors[DOOR_LEFT])
                {
                    Rect door = RECT(draw_x-rwh/2.0, draw_y+door_offset, door_h, door_w);
                    gfx_draw_rect(&door, color_door, NOT_SCALED, NO_ROTATION, opacity_door, true, true);
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

    gfx_draw_rect(&pr, color_player, NOT_SCALED, NO_ROTATION, opacity_player, true, true);
}

void draw_minimap()
{
    float w = margin_left.w;
    // define the minimap location and size
    Rect minimap_area = RECT(w/2.0, w/2.0, w, w);
    // translate the location
    gfx_get_absolute_coords(&minimap_area, ALIGN_CENTER, &margin_left, ALIGN_CENTER);
    draw_level(&minimap_area, COLOR_BLACK,0.3,  COLOR(0x22,0x48,0x70),0.4,  COLOR_RED,0.5);

#if 1
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
#endif
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

        if(show_big_map)
        {
            float len = MIN(room_area.w, room_area.h);
            Rect minimap_area = RECT(room_area.x, room_area.y, len, len);
            draw_level(&minimap_area, COLOR_BLACK,0.0,  COLOR_BLACK,0.3,  COLOR_RED,0.2);
        }

        // draw player
        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            Player* p = &players[i];
            if(p->active)
            {
                if(   p->curr_room.x == player->curr_room.x && 
                      p->curr_room.y == player->curr_room.y
                )
                {
                    // draw players if they are in the same room as you
                    player_draw(p);
                }
            }
        }

        for(int i = 0; i < plist->count; ++i)
        {
            projectile_draw(&projectiles[i]);
        }
    }

    if(debug_enabled)
    {
        gfx_draw_string(1, 0, COLOR_WHITE, 0.07, NO_ROTATION, FULL_OPACITY, IN_WORLD, NO_DROP_SHADOW, "mouse: %d, %d", mx, my);
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
            else if(key == GLFW_KEY_P)
            {
                // if(role == ROLE_LOCAL && game_state == GAME_STATE_PLAYING)
                // TODO
                if(game_state == GAME_STATE_PLAYING)
                {
                    paused = !paused;
                }
            }

        }
    }
}
