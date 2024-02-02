#include "core/headers.h"
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
#include "creature.h"
#include "editor.h"
#include "lighting.h"
#include "net.h"
#include "entity.h"
#include "item.h"
#include "explosion.h"
#include "status_effects.h"
#include "room_editor.h"
#include "text_list.h"
#include "skills.h"
#include "ui.h"
#include "settings.h"

// =========================
// Global Vars
// =========================

GameRole role = ROLE_LOCAL;
GameState game_state = GAME_STATE_MENU;
Timer game_timer = {0};
text_list_t* text_lst = NULL;
bool initialized = false;
bool debug_enabled = false;
bool editor_enabled = false;
bool client_chat_enabled = false;
bool paused = false;
bool show_big_map = false;
bool show_walls = false;
bool show_tile_grid = false;
bool players_invincible = false;

// Settings
uint32_t background_color = COLOR(30,30,30);
uint32_t margin_color = COLOR_BLACK;

Settings menu_settings = {0};

// mouse
float mouse_x=0, mouse_y=0;
float mouse_window_x=0, mouse_window_y=0;

// areas
Rect room_area = {0};
Rect floor_area = {0};
Rect player_area = {0};
Rect margin_left = {0};
Rect margin_right = {0};
Rect margin_top = {0};
Rect margin_bottom = {0};

// decals
Decal decals[MAX_DECALS];
glist* decal_list = NULL;

bool dynamic_zoom = false;
int cam_zoom = 54;
int cam_zoom_temp = 70;
int cam_min_zoom = 54;

Rect camera_limit = {0};
Vector2f aim_camera_offset = {0};
float ascale = 1.0;

Rect send_to_room_rect = {0};

Vector2f transition_offsets = {0};
Vector2f transition_targets = {0};

Level level;
unsigned int level_seed = 0;
int level_rank = 0;
int level_transition = 0;
int level_transition_state = 0;
bool level_generate_triggered = false;

DrawLevelParams minimap_params = {0};
DrawLevelParams bigmap_params = {0};

char* tile_names[TILE_MAX] = {0};
char* creature_names[CREATURE_TYPE_MAX] = {0};
char* item_names[ITEM_MAX] = {0};

// client chat box
char chat_text[128] = {0};
uint32_t chat_box_hash = 0x0;

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
    "Continue Game",
    "New Game",
    "Room Editor",
    "Play Online",
    "Host Local Server",
    "Join Local Server",
    "Settings",
    "Exit"
};

char arg_name[PLAYER_NAME_MAX+1];

// =========================
// Function Prototypes
// =========================
int main(int argc, char* argv[]);
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

    camera_move(CENTER_X, CENTER_Y, (float)cam_zoom/100.0, true, NULL);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);

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

    for(int i = 0;  i < MENU_KEY_MAX; ++i)
        memset(&menu_keys[i], 0, sizeof(PlayerInput));

    menu_keys[MENU_KEY_UP].hold_period = 0.2;
    menu_keys[MENU_KEY_DOWN].hold_period = 0.2;
}

void set_game_state(GameState state)
{
    if(state != game_state)
    {
        game_state = state;
        LOGI("Set game state: %d", game_state);
        switch(game_state)
        {
            case GAME_STATE_EDITOR:
            {
                ambient_light = 0x00FFFFFF;
                room_editor_start();
            } break;
            case GAME_STATE_MENU:
            {
                // menu_selected_option = 0;
                show_tile_grid = false;
                debug_enabled = false;
                bool diff = room_file_load_all(false);
                if(diff)
                {
                    trigger_generate_level(level_seed, level_rank, 0, __LINE__);
                }
                net_client_disconnect();
                set_menu_keys();
            } break;
            case GAME_STATE_SETTINGS:
            {
                // menu_selected_option = 0;
                show_tile_grid = false;
                debug_enabled = false;
                net_client_disconnect();
                set_menu_keys();
            } break;
            case GAME_STATE_PLAYING:
            {
                debug_enabled = false;
                editor_enabled = false;
                paused = false;
                ambient_light = ambient_light_default;
                player = &players[0];
                if(role == ROLE_LOCAL)
                {
                    player_set_active(player, true);
                    memcpy(&player->settings, &menu_settings,sizeof(Settings));
                    player_set_class(player, player->settings.class);
                }
                // player_set_active(&players[1], true);
                player_init_keys();
            } break;
        }
    }
}


// can also checks if the mouse is off the screen
void camera_set(bool immediate)
{
    if(game_state == GAME_STATE_EDITOR)
        return;

    if(role == ROLE_SERVER)
        return;

    // if(paused) return;

    if(player->curr_room != player->transition_room)
    {
        // printf("not updating camera\n");
        return;
    }

    if(false)
    {
        float _vw = view_width;
        float _vh = view_height;

        float r = 0.2;  //should be <= 0.5 to make sense otherwise player will end up off of the screen
        float ox = (mouse_x - _vw/2.0);
        float oy = (mouse_y - _vh/2.0);
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
        if(mouse_x >= view_width || mouse_x <= 0 || mouse_y >= view_height || mouse_y <= 0)
        {
            int new_mx = RANGE(mouse_x, 0, view_width);
            int new_my = RANGE(mouse_y, 0, view_height);
            window_set_mouse_view_coords(new_mx, new_my);
        }
    }


    float cam_pos_x = player->phys.pos.x + aim_camera_offset.x;
    float cam_pos_y = player->phys.pos.y + aim_camera_offset.y;

    camera_limit = room_area;

    // 1.0  ->  zoomed in all the way
    // 0.0  ->  zoomed out

    if(dynamic_zoom)
    {
        Rect cr = calc_camera_rect(cam_pos_x, cam_pos_y, (float)cam_zoom_temp/100.0, view_width, view_height, &camera_limit);

        // check if all players are visible
        bool all_visible = true;
        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            Player* p = &players[i];
            if(!p->active) continue;
            if(p->curr_room == player->curr_room)
            {
                Rect r = RECT(p->phys.pos.x, p->phys.pos.y, 1, 1);
                if(!rectangles_colliding(&cr, &r))
                {
                    all_visible = false;
                    break;
                }
            }
        }

        // float minz = 0.64;
        if(all_visible && cam_zoom_temp == cam_zoom)
        {
            // printf("satisfied\n");
        }
        else
        {
            int zdir = 1;
            if(all_visible)
            {
                // try moving toward configured cam_zoom
                if(cam_zoom_temp > cam_zoom)
                    zdir = -1;
                else
                    zdir = 1;
                // printf("adjusting toward cam_zoom (%d)\n", zdir);
            }
            else
            {
                // need to zoom out
                zdir = -1;
                // printf("adjusting zooming out\n");
            }

            for(;;)
            {
                int cam_zoom_temp_prior = cam_zoom_temp;

                cam_zoom_temp += zdir;

                if(cam_zoom_temp > 100)
                    break;
                if(cam_zoom_temp < cam_min_zoom)
                    break;
                if(cam_zoom_temp < 0)
                    break;

                Rect cr2 = calc_camera_rect(cam_pos_x, cam_pos_y, (float)cam_zoom_temp/100.0, view_width, view_height, &camera_limit);

                bool check = true;
                for(int i = 0; i < MAX_PLAYERS; ++i)
                {
                    Player* p = &players[i];
                    if(!p->active) continue;
                    if(p->curr_room == player->curr_room)
                    {
                        Rect r = RECT(p->phys.pos.x, p->phys.pos.y, 1, 1);
                        if(!rectangles_colliding(&cr2, &r))
                        {
                            check = false;
                            break;
                        }
                    }
                }

                if(zdir > 0 && !check)
                {
                    cam_zoom_temp = cam_zoom_temp_prior;
                    break;
                }

                if(check)
                    break;
            }

            cam_zoom_temp = RANGE(cam_zoom_temp, cam_min_zoom, 100);
        }
    }
    else
    {
        cam_zoom_temp = cam_zoom;
    }


    float cam_pos_z = (float)cam_zoom_temp/100.0;

#if 0
    // camera shake
    static int counter = 0;
    if(player->phys.hp == 1 && debug_enabled)
    {
        counter++;
        if(counter >= 2)
        {
            counter = 0;
            float xyrange = 2.0;
            cam_pos_x += RAND_FLOAT(-xyrange, xyrange);
            cam_pos_y += RAND_FLOAT(-xyrange, xyrange);
            cam_pos_z += RAND_FLOAT(-0.03, 0.03);
            cam_pos_z = MAX((float)cam_min_zoom/100.0, cam_pos_z);
            // immediate = true;
        }
    }
#endif

    bool ret = camera_move(cam_pos_x, cam_pos_y, cam_pos_z, immediate, &camera_limit);
    if(!ret)
    {
        camera_move(CENTER_X, CENTER_Y, cam_pos_z, immediate, NULL);
    }
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);
}

// can see more than the room
bool camera_can_be_limited(float x, float y, float z)
{
    Rect cr = calc_camera_rect(x, y, z, view_width, view_height, NULL);
    if(cr.w > camera_limit.w || cr.h > camera_limit.h)
    {
        return false;
    }
    return true;
}

bool got_bad_seed = false;
void run()
{

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);
    double curr_time = timer_get_time();
    double new_time  = 0.0;
    double accum = 0.0;
    const double dt = 1.0/TARGET_FPS;

    // for(;;)
    // {
    //     trigger_generate_level(rand(), 5);
    //     if(got_bad_seed) break;
    // }

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

void init_areas()
{
    room_area.w = ROOM_W;
    room_area.h = ROOM_H;
    room_area.x = CENTER_X;
    room_area.y = CENTER_Y;

    floor_area.w = ROOM_W - (TILE_SIZE*2);  //minus walls
    floor_area.h = ROOM_H - (TILE_SIZE*2);  //minus walls
    floor_area.x = CENTER_X;
    floor_area.y = CENTER_Y;

    memcpy(&player_area, &room_area, sizeof(Rect));
    player_area.w -= 32;
    player_area.h -= 48;

    // margins scaled with reference to view_width and view_height
    float mscale = view_width/1200.0;

    margin_left.w = 120.0 * mscale;
    margin_left.h = 550.0 * mscale;
    margin_left.x = margin_left.w / 2.0;
    margin_left.y = view_height / 2.0;

    margin_right.w = margin_left.w;
    margin_right.h = margin_left.h;
    margin_right.x = view_width - margin_left.w / 2.0;
    margin_right.y = margin_left.y;

    margin_top.w = view_width;
    margin_top.h = (view_height - margin_left.h) / 2.0;
    margin_top.x = CENTER_X;
    margin_top.y = margin_top.h / 2.0;

    margin_bottom.w = margin_top.w;
    margin_bottom.h = margin_top.h;
    margin_bottom.x = margin_top.x;
    margin_bottom.y = view_height - margin_bottom.h / 2.0;

#if 0
    printf("top:\n");
    print_rect(&margin_top);
    printf("bottom:\n");
    print_rect(&margin_bottom);
    printf("left:\n");
    print_rect(&margin_left);
    printf("right:\n");
    print_rect(&margin_right);
#endif

}

void start_server()
{
    //init
    view_width = VIEW_WIDTH;
    view_height = VIEW_HEIGHT;
    init_areas();

    gfx_image_init();
    skills_init();
    player_init();
    creature_init();
    effects_load_all();
    decal_init();
    item_init();

    room_editor_init();

    srand(time(0));
    level_seed = rand();

    level_init();
    trigger_generate_level(level_seed, 1, 0, __LINE__);

    projectile_init();
    explosion_init();

    // start
    net_server_start();
}

float level_trans_time = 0.0;
float level_transition_opacity = 0.0;

// 0: no transition
// 1: fade up transition
// 2: fade down, then up transition
void trigger_generate_level(unsigned int _seed, int _rank, int transition, int line)
{
    if(role == ROLE_SERVER) transition = 0;

    level_seed = _seed;
    level_rank = _rank;
    level_transition = transition;

    level_grace_time = ROOM_GRACE_TIME;

    if(level_transition == 0 || level_transition == 1)
    {
        level_generate_triggered = true;
        // printf("set level_generate_triggered = true (line: %d)\n", line);
    }

    if(level_transition == 1)
    {
        level_trans_time = 0.0;
        level_transition_state = 2;
    }
    else if(level_transition == 2)
    {
        level_trans_time = 0.0;
        level_transition_state = 1;
    }
}

void game_generate_level()
{
    if(!level_generate_triggered)
    {
        return;
    }
    level_generate_triggered = false;
    // printf("set level_generate_triggered = false\n");

    projectile_clear_all();
    creature_clear_all();
    item_clear_all();
    decal_clear_all();
    // particles_delete_all_spawners(); //doesn't work properly

    level = level_generate(level_seed, level_rank);
    ui_message_set_title(2.0, 0x00CCCCCC, "Level %d", level_rank);

    if(role == ROLE_CLIENT)
        return;

    LOGI("  Valid Rooms");
    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room* room = &level.rooms[x][y];
            if(room->valid)
            {
                char* room_fname = room_files[room_list[room->layout].file_index];
                // if(strcmp("geizer_3door.room", room_fname) == 0)
                // {
                //     if(room->doors[DIR_RIGHT])
                //     {
                //         LOGE("Door on the right!");
                //         got_bad_seed = true;
                //         // window_set_close(1);
                //     }
                // }
                LOGI("    %2u) %2d, %-2d  %s", room->index, x, y, room_fname);
            }
        }
    }

    uint16_t ccount = creature_get_count();
    LOGI("Total creature count: %u", ccount);
    for(int i = 0; i < ccount; ++i)
    {
        Creature* c = &creatures[i];
        Room* room = level_get_room_by_index(&level, c->curr_room);
        if(!room)
        {
            LOGE("Creature %d has invalid room: %u", c->curr_room);
        }
    }

    uint16_t ccount2 = 0;
    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room* room = &level.rooms[x][y];

            if(room == NULL) LOGE("Room is NULL %d,%d", x, y);
            uint16_t c = creature_get_room_count(room->index);
            ccount2 += c;
            if(c > 0)
            {
                if(!room->valid)
                    LOGE("Room: %-3u (%-2d,%2d) valid: %-5s, creature count: %2u, sum: %2u", room->index, x, y, BOOLSTR(room), c, ccount2);
                else
                    LOGI("Room: %-3u (%-2d,%2d) valid: %-5s, creature count: %2u, sum: %2u", room->index, x, y, BOOLSTR(room), c, ccount2);
            }
        }
    }

    if(ccount != ccount2)
    {
        LOGE("Creature count doesn't add up");
        for(int i = 0; i < ccount; ++i)
        {
            Room* room = level_get_room_by_index(&level, creatures[i].curr_room);
            if(!room->valid)
            {
                LOGE("Creature index %u, room %u is invalid", i, room->index);
            }
        }
    }

    if(level_transition <= 1)
    {
        LOGI("Send to level start");
        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            player_send_to_level_start(&players[i]);
        }
    }

}

void init()
{
    if(initialized) return;
    initialized = true;

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

    init_areas();

    LOGI(" - Shaders.");
    shader_load_all();

    LOGI(" - Graphics.");
    gfx_init(VIEW_WIDTH, VIEW_HEIGHT);
    ascale = view_width / 1200.0;
    LOGI("   ascale: %.2f", ascale);

    text_lst = text_list_init(50, 2.0, view_height - 40.0, 0.19*ascale, true, TEXT_ALIGN_LEFT, NOT_IN_WORLD, true);

    LOGI(" - Particles.");
    particles_init();

    LOGI(" - Camera.");
    camera_init();

    LOGI(" - Lighting.");
    lighting_init();

    room_editor_init();

    LOGI(" - Status Effects.");
    status_effects_init();

    LOGI(" - Skills.");
    skills_init();

    LOGI(" - Settings.");
    settings_load(DEFAULT_SETTINGS, &menu_settings);
    if(strlen(arg_name) > 0)
    {
        memcpy(menu_settings.name, arg_name, PLAYER_NAME_MAX);
    }

    LOGI(" - Player.");
    player = &players[0];
    player_init();

    LOGI(" - Creatures.");
    creature_init();

    LOGI(" - Effects.");
    effects_load_all();

    LOGI(" - Room Data.");
    level_init();

    LOGI(" - Projectiles.");
    projectile_init();

    LOGI(" - Explosions.");
    explosion_init();

    imgui_load_theme("retro.theme");

    LOGI(" - Editor.");
    editor_init();

    LOGI(" - Decals.");
    decal_init();

    LOGI(" - Items.");
    item_init();

    if(role == ROLE_LOCAL)
    {
        trigger_generate_level(0,5,0,__LINE__);
        // trigger_generate_level(0, 5, 0);
    }

    // camera_zoom(cam_zoom, true);
    camera_move(0,0,cam_zoom,true,NULL);
    camera_update(VIEW_WIDTH, VIEW_HEIGHT);

    DrawLevelParams* params = &minimap_params;
    params->show_all = false;
    params->color_bg = COLOR_BLACK;
    params->opacity_bg = 0.0;
    // params->color_room = COLOR(0x22,0x48,0x70);
    params->color_room = COLOR_BLACK;
    params->opacity_room = 0.8;
    // params->color_uroom = gfx_blend_colors(params->color_room, COLOR_WHITE, 0.3);
    params->color_uroom = COLOR_WHITE;
    params->opacity_uroom = 0.2;
    params->color_player = COLOR_RED;
    params->opacity_player = 0.7;
    params->color_border = COLOR_BLACK;
    params->opacity_border = 0.0;

    params = &bigmap_params;
    params->show_all = true;
    params->color_bg = COLOR_BLACK;
    params->opacity_bg = 0.0;
    params->color_room = COLOR_BLACK;
    params->opacity_room = 0.8;
    params->color_uroom = params->color_room;
    params->opacity_uroom = 0.3;
    params->color_player = COLOR_RED;
    params->opacity_player = 0.3;

    set_menu_keys();

    if(role == ROLE_CLIENT)
    {
        net_client_init();
        set_game_state(GAME_STATE_PLAYING);
    }
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
    // --name <name>

    role = ROLE_LOCAL;

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

                // name
                else if(strncmp(argv[i]+2,"name",6) == 0)
                {
                    if(argc > i+1)
                    {
                        memcpy(arg_name, argv[i+1], MIN(strlen(argv[i+1]), PLAYER_NAME_MAX));
                        i++;
                    }
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



bool client_handle_connection()
{
    if(role != ROLE_CLIENT) return true;

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
                    text_list_add(text_lst, COLOR_RED, 3.0, "Disconnected from Server.");
                    break;
                case SENDING_CONNECTION_REQUEST:
                    memcpy(&player->settings, &menu_settings, sizeof(Settings));
                    text_list_add(text_lst, COLOR_WHITE, 3.0, "Sending Connection Request.");
                    break;
                case SENDING_CHALLENGE_RESPONSE:
                    text_list_add(text_lst, COLOR_WHITE, 3.0, "Sending Challenge Response.");
                    break;
                case CONNECTED:
                {
                    text_list_add(text_lst, COLOR_GREEN, 3.0, "Connected to Server.");
                    int id = net_client_get_id();
                    player = &players[id];

                    // send settings to Server
                    memcpy(&player->settings, &menu_settings, sizeof(Settings));
                    player_set_class(player, player->settings.class);
                    net_client_send_settings();

                    window_controls_clear_keys();
                    player_init_keys();
                } break;
            }
        }

        if(curr_state != CONNECTED)
            return false;
    }

    // Client connected
    net_client_update();

    if(!net_client_received_init_packet())
    {
        // haven't received init packet from server yet
        return false;
    }

    return true;
}

void update_main_menu(float dt)
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

        if(STR_EQUAL(s, "Continue Game"))
        {
            role = ROLE_LOCAL;
            set_game_state(GAME_STATE_PLAYING);
        }
        else if(STR_EQUAL(s, "New Game"))
        {
            player_init();
            trigger_generate_level(rand(), 1, 0, __LINE__);

            role = ROLE_LOCAL;
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
            role = ROLE_SERVER;
            deinit();

            start_server();
        }
        else if(STR_EQUAL(s, "Join Local Server"))
        {
            role = ROLE_CLIENT;
            net_client_init();
            net_client_set_server_ip(LOCAL_SERVER_IP);
            set_game_state(GAME_STATE_PLAYING);
        }
        else if(STR_EQUAL(s, "Room Editor"))
        {
            set_game_state(GAME_STATE_EDITOR);
        }
        else if(STR_EQUAL(s, "Settings"))
        {
            set_game_state(GAME_STATE_SETTINGS);
        }
        else if(STR_EQUAL(s, "Exit"))
        {
            window_set_close(1);
        }
    }
}

void update_level_transition(float dt)
{
    const float transition_duration = 0.6;
    // fade from black to clear
    if(level_transition_state == 2)
    {
        level_trans_time += dt;

        level_transition_opacity = 1.0 - RANGE(level_trans_time / transition_duration, 0.0, 1.0);

        if(level_trans_time >= transition_duration)
        {
            // printf("finished fading to clear\n");
            level_transition_state = 0;
            level_trans_time = 0.0;
        }
    }
    // fade from clear to black
    else if(level_transition_state == 1)
    {
        level_trans_time += dt;

        level_transition_opacity = RANGE(level_trans_time / transition_duration, 0.0, 1.0);

        if(level_trans_time >= transition_duration)
        {
            // printf("finished fading to black\n");
            // level_transition_state = 2;
            // level_trans_time = 0.0;
            trigger_generate_level(level_seed, level_rank, 1, __LINE__);
        }
    }
}

void update(float dt)
{
    if(game_state == GAME_STATE_EDITOR)
    {
        bool res = room_editor_update(dt);
        if(!res) return;
    }

    game_generate_level();

    // mouse stuff
    // ------------------------------
    window_get_mouse_view_coords(&mouse_x, &mouse_y);
    window_get_mouse_world_coords(&mouse_window_x, &mouse_window_y);

    if(role == ROLE_LOCAL)
    {
        // if(debug_enabled && window_mouse_left_went_up())
        if(window_mouse_left_went_up())
        {
            send_to_room_rect.x = mouse_x;
            send_to_room_rect.y = mouse_y;
            send_to_room_rect.w = 1;
            send_to_room_rect.h = 1;
        }
        else
        {
            memset(&send_to_room_rect, 0, sizeof(Rect));
        }
    }

    gfx_clear_lines();

    text_list_update(text_lst, dt);

    if(game_state == GAME_STATE_SETTINGS)
        return;

    if(game_state == GAME_STATE_MENU)
    {
        update_main_menu(dt);
        return;
    }

    ui_update(dt);

    bool conn = client_handle_connection();
    if(!conn)
    {
        creature_clear_all();
        item_clear_all();
        decal_clear_all();
        // particles_delete_all_spawners(); //doesn't work properly
        return;
    }

    player_handle_net_inputs(player, dt);

    if(!paused)
    {
        lighting_point_light_clear_all();
        level_update(dt);
        player_update_all(dt);
        projectile_update_all(dt);
        creature_update_all(dt);
        item_update_all(dt);
        explosion_update_all(dt);
        decal_update_all(dt);
        particles_update(dt);

        entity_build_all();
        entity_handle_collisions();
        entity_handle_status_effects(dt);

        update_level_transition(dt);
    }

    camera_set(false);
}

// void handle_room_completion(int room_index)
void handle_room_completion(Room* room)
{
    if(role == ROLE_CLIENT) return;

    // Room* room = level_get_room_by_index(&level, room_index);
    // if(!room) return;

    uint8_t room_index = room->index;

    bool prior_locked = room->doors_locked;
    room->doors_locked = (creature_get_room_count(room_index) != 0);

    if(!room->doors_locked && prior_locked)
    {

        if(room->xp > 0)
        {
            for(int j = 0; j < MAX_CLIENTS; ++j)
            {
                Player* p = &players[j];
                if(p->active && p->curr_room == room->index && !p->phys.dead)
                {
                    player_add_xp(p, room->xp);
                }
            }
        }
        room->xp = 0;

        if(room->type == ROOM_TYPE_BOSS)
        {
            Vector2f pos = {0};
            Vector2i start = {.x = ROOM_TILE_SIZE_X/2, .y = ROOM_TILE_SIZE_Y/2};

            level_get_safe_floor_tile(room, start, NULL, &pos);
            item_add(ITEM_CHEST, pos.x, pos.y, room_index);

            level_get_safe_floor_tile(room, start, NULL, &pos);
            item_add(ITEM_NEW_LEVEL, pos.x, pos.y, room_index);
        }
        else
        {
            if(rand() % 5 == 0) //TODO: probability
            {
                Vector2f pos = {0};
                Vector2i start = {.x = ROOM_TILE_SIZE_X/2, .y = ROOM_TILE_SIZE_Y/2};
                level_get_safe_floor_tile(room, start, NULL, &pos);
                item_add(item_get_random_heart(), pos.x, pos.y, room_index);
            }
        }
    }
}

void draw_map(DrawLevelParams* params)
{
    // could also add this bool to Room struct
    bool _near[MAX_ROOMS_GRID_X][MAX_ROOMS_GRID_Y] = {0};
    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room* room = &level.rooms[x][y];
            if(room->discovered) _near[x][y] = false;
            if(_near[x][y]) continue;

            if(room->discovered)
            {
                for(int d = 0; d < MAX_DOORS; ++d)
                {
                    if(!room->doors[d]) continue;
                    Vector2i o = get_dir_offsets(d);
                    Room* nroom = level_get_room(&level, x+o.x, y+o.y);
                    if(nroom != NULL) _near[x+o.x][y+o.y] = true;
                }
            }
        }
    }

    bool dbg = params->show_all && debug_enabled;

    gfx_draw_rect(&params->area, params->color_bg, NOT_SCALED, NO_ROTATION, params->opacity_bg, true, NOT_IN_WORLD);
    gfx_draw_rect(&params->area, params->color_border, NOT_SCALED, NO_ROTATION, params->opacity_border, false, NOT_IN_WORLD);

    float tlx = params->area.x - params->area.w/2.0;
    float tly = params->area.y - params->area.h/2.0;

    // room width/height
    float rw = params->area.w/MAX_ROOMS_GRID_X;
    float rh = params->area.h/MAX_ROOMS_GRID_Y;
    float len = MIN(rw, rh);
    // printf("len: %.2f\n", len);
    params->area.w = len*MAX_ROOMS_GRID_X;
    params->area.h = len*MAX_ROOMS_GRID_Y;

    float tscale = 0.12 * 57.0/len;

    float margin = len / 12.0;
    float room_wh = len-margin;
    const float door_w = rw/12.0;
    const float door_h = margin;

    float r = len/2.0;

    // if(dbg)
    // {
    //     for(int i = 0; i < 100; ++i)
    //     {
    //         tscale += 0.01;
    //         Vector2f size = gfx_string_get_size(tscale, "|");
    //         if(size.y >= len/5.0)
    //         {
    //             printf("tscale: %.2f\n", tscale);
    //             break;
    //         }
    //     }
    // }

    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room* room = &level.rooms[x][y];

            float draw_x = tlx + x*len + r;
            float draw_y = tly + y*len + r;
            Rect room_rect = RECT(draw_x, draw_y, room_wh, room_wh);

            if(!room->valid)
            {
                if(debug_enabled)
                {
                    gfx_draw_rect(&room_rect, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 1.0, false, NOT_IN_WORLD);

                    room_rect.w -= 1.0;
                    room_rect.h -= 1.0;
                    gfx_draw_rect(&room_rect, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 1.0, false, NOT_IN_WORLD);
                }
                continue;
            }

            bool discovered = room->discovered;
            bool is_near = _near[x][y];

            if(!params->show_all && !discovered && !is_near)
            {
                continue;
            }

            uint32_t _color = params->color_room;
            float _opacity = params->opacity_room;
            if(!discovered)
            {
                _color = params->color_uroom;
                _opacity = params->opacity_uroom;
            }

            if(dbg)
            {
                _color = room->color;
            }

            gfx_draw_rect(&room_rect, _color, NOT_SCALED, NO_ROTATION, _opacity, true, NOT_IN_WORLD);

            if(dbg)
            {
                float tlx = room_rect.x - room_rect.w/2.0 + 1.0;
                float tly = room_rect.y - room_rect.h/2.0;
                gfx_draw_string(tlx, tly, COLOR_BLACK, tscale, NO_ROTATION, 1.0, NOT_IN_WORLD, NO_DROP_SHADOW, 0, "creatures: %d", creature_get_room_count(room->index));

                Vector2f size = gfx_string_get_size(tscale, "|");
                gfx_draw_string(tlx, tly+size.y, COLOR_BLACK, tscale, NO_ROTATION, 1.0, NOT_IN_WORLD, NO_DROP_SHADOW, room_rect.w, "%s", room_files[room_list[room->layout].file_index]);
            }

            if(!IS_RECT_EMPTY(&send_to_room_rect) && params->show_all && room->index != player->curr_room)
            {
                if(rectangles_colliding(&room_rect, &send_to_room_rect))
                {
                    // printf("room->index: %u\n", room->index);
                    // printf("player->curr_room: %u\n", player->curr_room);
                    player_send_to_room(player, room->index, true, level_get_door_tile_coords(DIR_NONE));

                }
            }

            // draw the doors
            for(int d = 0; d < MAX_DOORS; ++d)
            {
                if(!room->doors[d]) continue;

                Vector2i o = get_dir_offsets(d);
                int _x = x+o.x;
                int _y = y+o.y;
                Room* nroom = level_get_room(&level, _x, _y);

                //both rooms discovered
                if(room->discovered && nroom->discovered && (d == DIR_DOWN || d == DIR_LEFT))
                    continue;
                if(!room->discovered && nroom->discovered)
                    continue;
                if(!params->show_all && !room->discovered && !nroom->discovered)
                    continue;

                float _w = door_w;
                float _h = door_h;
                if(d == DIR_LEFT || d == DIR_RIGHT)
                {
                    _w = door_h;
                    _h = door_w;
                }

                Rect door = RECT(draw_x+(r*o.x), draw_y+(r*o.y), _w, _h);
                gfx_draw_rect(&door, _color, NOT_SCALED, NO_ROTATION, _opacity, true, NOT_IN_WORLD);

            }
        }
    }

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(!p->active) continue;

        float px = p->phys.pos.x - rect_tlx(&room_area);
        float py = p->phys.pos.y - rect_tly(&room_area);
        px *= (room_wh/room_area.w);
        py *= (room_wh/room_area.h);
        Rect pr = RECT(px, py, margin, margin);

        // translate to minimap
        Vector2i roomxy = level_get_room_coords(p->curr_room);
        float draw_x = tlx + roomxy.x*len + r;
        float draw_y = tly + roomxy.y*len + r;

        Rect r = RECT(draw_x, draw_y, room_wh, room_wh);
        gfx_get_absolute_coords(&pr, ALIGN_CENTER, &r, ALIGN_CENTER);

        gfx_draw_rect(&pr, params->color_player, NOT_SCALED, NO_ROTATION, params->opacity_player, true, NOT_IN_WORLD);
    }
}

void draw_minimap()
{
    float h = margin_top.h;
    float x = view_width - h/2.0;
    float y = h/2.0;
    Rect area = RECT(x, y, h, h);
    gfx_get_absolute_coords(&area, ALIGN_CENTER, &margin_top, ALIGN_CENTER);

    minimap_params.area = area;
    draw_map(&minimap_params);
}

void draw_bigmap()
{
    if(!show_big_map) return;
    float len = MIN(view_width, view_height) * 0.5;
    // float len = MIN(room_area.w, room_area.h);
    // // len /= (camera_get_zoom()/2.0);
    Rect area = RECT(room_area.x, room_area.y, len, len);
    bigmap_params.area = area;
    draw_map(&bigmap_params);
}

void draw_chat_box()
{
    if(client_chat_enabled)
    {
        // draw chat box
        static Rect gui_size = {321,28};
        imgui_begin("##Chat", (view_width - gui_size.w)/ 2.0, view_height - gui_size.h - 100);
        imgui_horizontal_begin();
            imgui_text("Chat:");
            uint32_t hash = imgui_text_box_sized("##chat_text",chat_text,IM_ARRAYSIZE(chat_text), 250, 24);
            if(chat_box_hash == 0x0)
            {
                chat_box_hash = hash;
                imgui_focus_text_box(hash);
            }

            if(imgui_text_get_enter())
            {
                if(role == ROLE_CLIENT)
                {
                    net_client_send_message(chat_text);
                }
                else if(role == ROLE_LOCAL)
                {

                    if(memcmp("cmd ", chat_text, 4) == 0)
                    {
                        char* argv[20] = {0};
                        int argc = 0;

                        for(int i = 0; i < 20; ++i)
                        {
                            char* s = string_split_index_copy(chat_text+4, " ", i, true);
                            if(!s) break;
                            argv[argc++] = s;
                        }

                        bool ret = server_process_command(argv, argc, player->index);

                        if(!ret)
                        {
                            server_send_message(player->index, FROM_SERVER, "Invalid command or command syntax");
                        }

                        // free
                        for(int i = 0; i < argc; ++i)
                        {
                            free(argv[i]);
                        }

                    }
                    else
                    {
                        text_list_add(text_lst, player->settings.color, 10.0, "%s: %s", player->settings.name, chat_text);
                    }


                }
                memset(chat_text, 0, 128);
                client_chat_enabled = false;
            }
            else if(imgui_text_get_ctrl_enter())
            {
                client_chat_enabled = false;
            }

        imgui_horizontal_end();
        gui_size = imgui_end();
        //printf("w: %f, h: %f\n", gui_size.w, gui_size.h);

        if(!client_chat_enabled)
        {
            player_ignore_input = 2;
            window_controls_set_key_mode(KEY_MODE_NORMAL);
        }
    }
}

void particles_draw_all()
{
    // draw particles
    gfx_sprite_batch_begin(true);
        for(int i = 0; i < spawner_list->count; ++i)
        {
            if(spawners[i].userdata == player->curr_room)
                particles_draw_spawner(&spawners[i], true, true);
        }
    gfx_sprite_batch_draw();
}

void draw_main_menu()
{
    gfx_clear_buffer(background_color);

    float menu_item_scale = 0.4 * ascale;

    // get text height first
    Vector2f text_size = gfx_string_get_size(menu_item_scale, "A");
    float margin = 1.0*ascale;

    int num_opts = sizeof(menu_options)/sizeof(menu_options[0]);

    float total_height = num_opts * (text_size.y+margin);

    float x = (view_width - 200)/2.0;
    float y = CENTER_Y - total_height / 2.0 + 50;

    for(int i = 0; i < num_opts; ++i)
    {
        uint32_t color = COLOR_WHITE;
        if(i == menu_selected_option) color = COLOR_BLUE;
        gfx_draw_string(x,y, color, menu_item_scale, NO_ROTATION, FULL_OPACITY, NOT_IN_WORLD, DROP_SHADOW, 0, (char*)menu_options[i]);
        y += (text_size.y+margin);
    }

    char* title = "SCUM";
    float title_scale = 2.5*ascale;
    Vector2f title_size = gfx_string_get_size(title_scale, title);
    Rect title_r = RECT(margin_top.w/2.0, margin_top.h/2.0, title_size.x, title_size.y);
    gfx_get_absolute_coords(&title_r, ALIGN_CENTER, &margin_top, ALIGN_TOP_LEFT);
    gfx_draw_string(title_r.x, title_r.y+150, COLOR_WHITE, title_scale, NO_ROTATION, FULL_OPACITY, NOT_IN_WORLD, DROP_SHADOW, 0, title);

    //gfx_draw_image(d->image, d->sprite_index, d->pos.x, d->pos.y, d->tint, d->scale, d->rotation, d->opacity*op, false, true);


    text_list_draw(text_lst);
}

void draw_settings()
{
    gfx_clear_buffer(background_color);

    int x = (view_width-200)/2.0;
    int y = (view_height-200)/2.0 - 100;

    imgui_begin_panel("Settings", x, y, false);
        //imgui_set_text_size(menu_item_scale);
        imgui_text_box("Name", menu_settings.name, PLAYER_NAME_MAX);
        imgui_color_picker("Color", &menu_settings.color);
        int class = (int)menu_settings.class;
        imgui_dropdown(class_strs, PLAYER_CLASS_MAX, "Class", &class, NULL);
        menu_settings.class = class;
        imgui_newline();
        if(imgui_button("Return"))
        {
            set_game_state(GAME_STATE_MENU);
            settings_save(DEFAULT_SETTINGS, &menu_settings);

            player->anim.curr_frame = 0;
            player->anim.curr_frame_time = 0.0;
            player->anim.curr_loop = 0;
        }
    imgui_end();

    player_set_class(player, menu_settings.class);

    gfx_draw_image_color_mask(player->image, SPRITE_DOWN + player->anim.curr_frame, (view_width-32)/2.0, (view_height-32)/2.0 + 20, menu_settings.color, 3.0, 0.0, 1.0, false, NOT_IN_WORLD);

    gfx_anim_update(&player->anim, 0.010);
    text_list_draw(text_lst);
}


void draw()
{

    if(game_state == GAME_STATE_EDITOR)
    {
        room_editor_draw();
        return;
    }
    else if(game_state == GAME_STATE_MENU)
    {
        draw_main_menu();
        return;
    }
    else if(game_state == GAME_STATE_SETTINGS)
    {
        draw_settings();
        return;
    }


    // draw game

    gfx_clear_buffer(background_color);

    if(player->curr_room != player->transition_room)
    {
        lighting_point_light_clear_all();
        player_draw_room_transition();
    }
    else
    {

        Room* room = level_get_room_by_index(&level, player->curr_room);
        if(!room) LOGW("room is null");
        level_draw_room(room, NULL, 0, 0);

        // Vector2i c = level_get_room_coords(player->curr_room);
        // for(int d = 0; d < 4; ++d)
        // {
        //     if(!room->doors[d]) continue;
        //     Vector2i o = get_dir_offsets(d);
        //     Room* r = level_get_room(&level, c.x+o.x, c.y+o.y);
        //     if(!r) continue;
        //     if(!r->valid) continue;
        //     level_draw_room(r, NULL, room_area.w*o.x, room_area.h*o.y);
        // }

        decal_draw_all();
        entity_draw_all();
        explosion_draw_all();
        particles_draw_all();


        if(debug_enabled)
        {
            if(show_walls) room_draw_walls(room);
        }

    }

    draw_all_other_player_info();
    draw_minimap();
    draw_hearts();
    draw_xp_bar();
    draw_gauntlet();
    draw_timed_items();
    draw_skill_selection();

    if(debug_enabled)
    {
        Rect mr = RECT(mouse_x, mouse_y, 10, 10);
        gfx_draw_rect(&mr, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, NOT_IN_WORLD);
    }

    draw_bigmap();
    editor_draw();

    text_list_draw(text_lst);
    ui_draw();
    gfx_draw_lines();

    draw_chat_box();


    if(level_transition_state != 0)
    {
        Rect cr = get_camera_rect();
        gfx_draw_rect(&cr, COLOR_BLACK, NOT_SCALED, NO_ROTATION, level_transition_opacity, true, IN_WORLD);
    }

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

            if(ctrl && key == GLFW_KEY_ENTER)
            {
                client_chat_enabled = !client_chat_enabled;
                if(client_chat_enabled)
                {
                    // printf("client chat enabled\n");
                    player->actions[PLAYER_ACTION_ACTIVATE].state = false;  //must manually set the state to false since key mode gets changed
                    player_ignore_input = 2;
                    window_controls_set_text_buf(chat_text,128);
                    window_controls_set_key_mode(KEY_MODE_TEXT);
                }
                else
                {
                    // printf("client chat disabled\n");
                    player_ignore_input = 2;
                }
            }

            if(key == GLFW_KEY_ESCAPE)
            {
                if(editor_enabled)
                {
                    editor_enabled = false;
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
            else if(key == GLFW_KEY_F3)
            {
                // if(role == ROLE_LOCAL)
                {
                    editor_enabled = !editor_enabled;
                }
            }
            else if(key == GLFW_KEY_P)
            {
                if(role == ROLE_LOCAL && game_state == GAME_STATE_PLAYING)
                {
                    paused = !paused;
                }
            }
            else if(key == GLFW_KEY_M)
            {
                if(role == ROLE_LOCAL && game_state == GAME_STATE_PLAYING)
                {
                    show_big_map = !show_big_map;
                }
            }

        }
    }
    else if(kmode == KEY_MODE_TEXT)
    {
        //TODO
        // if(key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
        // {
        //     // printf("console_text_hist_selection: %d  -> ", console_text_hist_selection);
        //     console_text_hist_selection = console_text_hist_get(key == GLFW_KEY_UP ? -1: 1);
        //     // printf("%d\n", console_text_hist_selection);
        //     if(console_text_hist_selection != -1)
        //     {
        //         // memset(console_text, 0, CONSOLE_TEXT_MAX*sizeof(console_text[0]));
        //         memcpy(console_text, console_text_hist[console_text_hist_selection], CONSOLE_TEXT_MAX);
        //     }
        // }
        // else
        // {
        //     // reset selection on any other key press
        //     console_text_hist_selection = -1;
        // }
    }
}


// ==================================================================================
// DECALS
// ==================================================================================

void decal_init()
{
    decal_list = list_create((void*)decals, MAX_DECALS, sizeof(Decal));
}

void decal_add(Decal d)
{
    list_add(decal_list, (void*)&d);
}

void decal_draw_all()
{
    if(decal_list == NULL) return;

    for(int i = decal_list->count-1; i >= 0; --i)
    {
        Decal* d = &decals[i];
        if(d->room != player->curr_room) continue;
        // printf("room: %u == %u\n", d->room, player->curr_room);
        // printf("%d\n", d->image);
        // printf("%u\n", d->sprite_index);
        // printf("%.1f, %.1f\n", d->pos.x, d->pos.y);
        // printf("%u\n", d->tint);
        // printf("%.1f\n", d->scale);
        // printf("%.1f\n", d->rotation);
        // printf("%.1f\n", d->opacity);
        float op = 1.0;
        if(d->ttl <= 1.0)
        {
            op = MAX(0.0, d->ttl);
        }
        gfx_draw_image(d->image, d->sprite_index, d->pos.x, d->pos.y, d->tint, d->scale, d->rotation, d->opacity*op, false, true);
    }
}

void decal_update_all(float dt)
{
    for(int i = decal_list->count-1; i >= 0; --i)
    {
        Decal* d = &decals[i];
        d->ttl -= dt;
        if(d->ttl <= 0.0)
        {
            list_remove(decal_list, i);
        }
    }
}

void decal_clear_all()
{
    list_clear(decal_list);

}

// ==================================================================================
// string parsing
// ==================================================================================

char* string_split_index(char* str, const char* delim, int index, int* ret_len, bool split_past_index)
{
    char* s = str;
    char* s_end = str+strlen(str);

    for(int i = 0; i < (index+1); ++i)
    {
        char* end = strstr(s, delim);

        if(end == NULL)
            end = s_end;

        int len = end - s;

        if(len == 0)
        {
            *ret_len = 0;
            return NULL;
        }

        // printf("%d]  '%.*s' \n", i, len, s);

        if(i == index)
        {
            if(split_past_index)
                *ret_len = len;
            else
                *ret_len = s_end-s;
            return s;
        }

        if(end == s_end)
        {
            *ret_len = 0;
            return NULL;
        }

        s += len+strlen(delim);
    }

    *ret_len = 0;
    return NULL;
}

char* string_split_index_copy(char* str, const char* delim, int index, bool split_past_index)
{
    int len = 0;
    char* s = string_split_index(str, delim, index, &len, split_past_index);

    if(s == NULL || len == 0)
        return NULL;

    char* ret = calloc(len+1,sizeof(char));
    memcpy(ret, s, len);
    return ret;
}
