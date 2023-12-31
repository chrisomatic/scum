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
bool paused = false;
bool show_big_map = false;
bool show_walls = false;
bool show_tile_grid = false;
bool players_invincible = false;

// Settings
uint32_t background_color = COLOR_BLACK;
uint32_t margin_color = COLOR_BLACK;

// mouse
float mx=0, my=0;
float wmx=0, wmy=0;

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
int cam_zoom = 70;
int cam_zoom_temp = 70;
int cam_min_zoom = 65;

Rect camera_limit = {0};
Vector2f aim_camera_offset = {0};
float ascale = 1.0;

Rect send_to_room_rect = {0};

Vector2f transition_offsets = {0};
Vector2f transition_targets = {0};

Level level;
unsigned int level_seed = 0;
int level_rank = 0;

DrawLevelParams minimap_params = {0};
DrawLevelParams bigmap_params = {0};

char* tile_names[TILE_MAX] = {0};
char* creature_names[CREATURE_TYPE_MAX] = {0};
char* item_names[ITEM_MAX] = {0};

#define MESSAGE_SMALL_MAX 256
char message_small[MESSAGE_SMALL_MAX];
float message_small_duration;


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
    "Room Editor",
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
void run();
void parse_args(int argc, char* argv[]);
void init();
void set_menu_keys();
void deinit();
void update(float dt);
void draw();
void key_cb(GLFWwindow* window, int key, int scan_code, int action, int mods);
void start_server();

void message_small_update(float dt);
void message_small_draw();

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
                show_tile_grid = false;
                debug_enabled = false;
                bool diff = room_file_load_all(false);
                if(diff)
                {
                    game_generate_level(level_seed, level_rank);
                }
                net_client_disconnect();
                set_menu_keys();
            } break;
            case GAME_STATE_PLAYING:
            {
                ambient_light = ambient_light_default;
                if(role == ROLE_LOCAL)
                {
                    player = &players[0];
                    player2 = &players[1];
                    player_set_active(player, true);
                    // player_set_active(player2, true);
                    player2_init_keys();
                }
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
    //     game_generate_level(rand(), 5);
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
    decal_init();
    item_init();

    srand(time(0));
    level_seed = rand();

    level_init();
    game_generate_level(level_seed, 1);

    projectile_init();
    explosion_init();

    // start
    net_server_start();
}


void game_generate_level(unsigned int _seed, int _rank)
{
    level_seed = _seed;
    level_rank = _rank;

    creature_clear_all();
    item_clear_all();

    level = level_generate(level_seed, level_rank);

    LOGI("  Valid Rooms");
    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room* room = &level.rooms[x][y];
            if(room->valid)
            {
                char* room_fname = room_files[room_list[room->layout].file_index];
                if(strcmp("geizer_3door.room", room_fname) == 0)
                {
                    if(room->doors[DIR_RIGHT])
                    {
                        LOGE("Door on the right!");
                        got_bad_seed = true;
                        // window_set_close(1);
                    }
                }
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

            if(room == NULL) printf("Room is NULL %d,%d\n", x, y);
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

    LOGI("Send to level start");
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        player_send_to_level_start(&players[i]);
    }

    // Room* room = level_get_room_by_index(&level, player->curr_room);
    // Vector2f pos = {0};
    // for(int i = 0; i < 5; ++i)
    // {
    //     level_get_rand_floor_tile(room, NULL, &pos);
    //     item_add(ITEM_CHEST, pos.x, pos.y, player->curr_room);
    // }
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

    text_lst = text_list_init(50, 10.0, view_height - 40.0, 0.12*ascale, COLOR_WHITE, false, TEXT_ALIGN_LEFT, NOT_IN_WORLD, true);

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

    //imgui_load_theme("nord_deep.theme");

    LOGI(" - Editor.");
    editor_init();

    LOGI(" - Decals.");
    decal_init();

    LOGI(" - Items.");
    item_init();

    if(role == ROLE_LOCAL)
    {
        game_generate_level(0, 5);
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

    if(game_state == GAME_STATE_EDITOR)
    {
        bool res = room_editor_update(dt);
        if(!res)
            return;
    }

    gfx_clear_lines();

    text_list_update(text_lst, dt);

    message_small_update(dt);

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
                        window_controls_clear_keys();
                        player_init_keys();

                    } break;
                }
            }

            if(curr_state != CONNECTED)
                return;
        }

        // Client connected
        net_client_update();

        if(!net_client_received_init_packet())
        {
            // haven't received init packet from server yet
            return;
        }

        player_handle_net_inputs(player, dt);

        for(int i = 0; i < plist->count; ++i)
        {
            projectile_lerp(&projectiles[i], dt);
        }

        for(int i = 0; i < creature_get_count(); ++i)
        {
            creature_lerp(&creatures[i], dt);
        }

        decal_update_all(dt);

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            Player* p = &players[i];
            if(p->active)
            {
                player_lerp(p, dt);
            }
        }

        entity_build_all();

        camera_set(true);

        return;
    }

    // Update Local
    // ------------------------------

    window_get_mouse_view_coords(&mx, &my);
    window_get_mouse_world_coords(&wmx, &wmy);

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

            if(STR_EQUAL(s, "Play Local"))
            {
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
            if(role == ROLE_LOCAL)
            {
                // if(debug_enabled && window_mouse_left_went_up())
                if(window_mouse_left_went_up())
                {
                    send_to_room_rect.x = mx;
                    send_to_room_rect.y = my;
                    send_to_room_rect.w = 1;
                    send_to_room_rect.h = 1;
                }
                else
                {
                    memset(&send_to_room_rect, 0, sizeof(Rect));
                }
            }

            lighting_point_light_clear_all();
            for(int i = 0; i < MAX_PLAYERS; ++i)
            {
                Player* p = &players[i];
                player_update(p, dt);

                if(p->active && p->curr_room == player->curr_room)
                {
                    p->light_index = lighting_point_light_add(p->phys.pos.x, p->phys.pos.y, 1.0, 1.0, 1.0, p->light_radius,0.0);
                }
            }

#if 0
            if(debug_enabled)
            {
                static float ptimer = 1.0;
                ptimer -= dt;
                if(ptimer <= 0)
                {
                    ptimer = 1.0;
                    Physics phys = player->phys;
                    phys.pos.x = room_area.x-room_area.w/2.0 + room_area.w*0.80;
                    phys.pos.y = CENTER_Y;
                    phys.vel.x = 0;
                    phys.vel.y = 0;
                    projectile_add_type(&phys, player->curr_room, PROJECTILE_TYPE_PLAYER, 180.0, 1.0, 1.0,false);
                }
            }
#endif

            // update point lights
            projectile_update(dt);
            creature_update_all(dt);
            item_update_all(dt);
            explosion_update_all(dt);
            decal_update_all(dt);
            particles_update(dt);

            entity_build_all();
            entity_handle_collisions();
            entity_handle_status_effects(dt);

            Room* room = level_get_room_by_index(&level, (int)player->curr_room);
            bool prior_locked = room->doors_locked;
            room->doors_locked = (creature_get_room_count(player->curr_room) != 0);
            if(!room->doors_locked && prior_locked)
            {
                // if(player->new_levels == 0)
                //     randomize_skill_choices(player);

                if(room->xp > 0)
                {
                    for(int i = 0; i < MAX_PLAYERS; ++i)
                    {
                        Player* p = &players[i];
                        if(p->active && p->curr_room == room->index)
                        {
                            player_add_xp(p, room->xp);
                        }
                    }
                    room->xp = 0;
                }

                if(room->type == ROOM_TYPE_BOSS)
                {
                    item_add(ITEM_CHEST, CENTER_X, CENTER_Y, player->curr_room);
                    item_add(ITEM_NEW_LEVEL, CENTER_X, CENTER_Y-32, player->curr_room);
                }
            }

        }
    }

    camera_set(false);
}

void draw_map(DrawLevelParams* params)
{
    // could also add this bool to Room struct
    bool near[MAX_ROOMS_GRID_X][MAX_ROOMS_GRID_Y] = {0};
    for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
    {
        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
        {
            Room* room = &level.rooms[x][y];
            if(room->discovered) near[x][y] = false;
            if(near[x][y]) continue;

            if(room->discovered)
            {
                for(int d = 0; d < MAX_DOORS; ++d)
                {
                    if(!room->doors[d]) continue;
                    Vector2i o = get_dir_offsets(d);
                    Room* nroom = level_get_room(&level, x+o.x, y+o.y);
                    if(nroom != NULL) near[x+o.x][y+o.y] = true;
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
            bool is_near = near[x][y];

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

            //TEMP
            if(!IS_RECT_EMPTY(&send_to_room_rect) && params->show_all && room->index != player->curr_room)
            {
                if(rectangles_colliding(&room_rect, &send_to_room_rect))
                {
                    // printf("room->index: %u\n", room->index);
                    // printf("player->curr_room: %u\n", player->curr_room);
                    player_send_to_room(player, room->index);
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

void draw()
{

    if(game_state == GAME_STATE_EDITOR)
    {
        room_editor_draw();
        return;
    }

    if(debug_enabled && game_state == GAME_STATE_PLAYING)
    {
        gfx_clear_buffer(COLOR_YELLOW);
    }
    else
    {
        gfx_clear_buffer(background_color);
    }

    // draw room
    Room* room = level_get_room_by_index(&level, player->curr_room);
    if(game_state == GAME_STATE_PLAYING)
    {
        player_draw_room_transition();

        if(player->curr_room == player->transition_room)
        {
            Room* room = level_get_room_by_index(&level,player->curr_room);
            level_draw_room(room, NULL, 0, 0);
        }

        // gfx_draw_rect(&room_area, COLOR_WHITE, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
    else if(game_state == GAME_STATE_MENU)
    {
        // //TODO
        // Room* room = &level.rooms[roomxy.x][roomxy.y];
        // level_draw_room(room, NULL, 0, 0);
        gfx_draw_rect(&room_area, COLOR(30,30,30), NOT_SCALED, NO_ROTATION, 1.0, true, true);
    }


    if(game_state == GAME_STATE_MENU)
    {

        // draw menu
        float menu_item_scale = 0.6 * ascale;

        // get text height first
        Vector2f text_size = gfx_string_get_size(menu_item_scale, "A");
        float margin = 1.0*ascale;

        int num_opts = sizeof(menu_options)/sizeof(menu_options[0]);

        float total_height = num_opts * (text_size.y+margin);

        float x = view_width*0.35;
        float y = CENTER_Y - total_height / 2.0;

        for(int i = 0; i < num_opts; ++i)
        {
            uint32_t color = COLOR_WHITE;
            if(i == menu_selected_option) color = COLOR_BLUE;
            gfx_draw_string(x,y, color, menu_item_scale, NO_ROTATION, FULL_OPACITY, NOT_IN_WORLD, DROP_SHADOW, 0, (char*)menu_options[i]);
            y += (text_size.y+margin);
        }

    }

    if(game_state == GAME_STATE_PLAYING)
    {
        if(player->curr_room == player->transition_room)
        {
            decal_draw_all();
            entity_draw_all();
            explosion_draw_all();

            // draw particles
            gfx_sprite_batch_begin(true);
                for(int i = 0; i < spawner_list->count; ++i)
                {
                    if(spawners[i].userdata == player->curr_room)
                        particles_draw_spawner(&spawners[i], true, true);
                }
            gfx_sprite_batch_draw();
        }

        // @TEST
        // {
        //     float x = 200;
        //     float y = 200;
        //     float w = 200;
        //     float h = 200;
        //     gfx_draw_rect_xywh_tl(x, y, w, h, COLOR_GRAY, 1.0, 0.0, 0.5,true,false);
        //     float scale = 0.32 * ascale;
        //     // gfx_draw_string2(x, y, COLOR_WHITE, scale, NO_ROTATION, 1.0, NOT_IN_WORLD, DROP_SHADOW, w, "A B C D E F G H I J K123 L M N O P Q R S T U V W X Y Z 1 2 3 4 5 6 7 8 9");
        //     gfx_draw_string2(x, y, COLOR_WHITE, scale, NO_ROTATION, 1.0, NOT_IN_WORLD, DROP_SHADOW, w, "A B C D E F G H I J K A B C D E F end");
        // }

        draw_bigmap();
    }

    // draw walls
    if(debug_enabled && show_walls && game_state == GAME_STATE_PLAYING)
    {
        room_draw_walls(room);
    }

    // // margins
    // gfx_draw_rect(&margin_left, margin_color, NOT_SCALED, NO_ROTATION, 1.0, true, false);
    // gfx_draw_rect(&margin_right, margin_color, NOT_SCALED, NO_ROTATION, 1.0, true, false);
    // gfx_draw_rect(&margin_top, margin_color, NOT_SCALED, NO_ROTATION, 1.0, true, false);
    // gfx_draw_rect(&margin_bottom, margin_color, NOT_SCALED, NO_ROTATION, 1.0, true, false);

    if(game_state == GAME_STATE_MENU)
    {
        char* title = "SCUM";
        float title_scale = 1.1*ascale;
        Vector2f title_size = gfx_string_get_size(title_scale, title);
        Rect title_r = RECT(margin_top.w/2.0, margin_top.h/2.0, title_size.x, title_size.y);
        gfx_get_absolute_coords(&title_r, ALIGN_CENTER, &margin_top, ALIGN_TOP_LEFT);
        gfx_draw_string(title_r.x, title_r.y, COLOR_WHITE, title_scale, NO_ROTATION, FULL_OPACITY, NOT_IN_WORLD, DROP_SHADOW, 0, title);
    }
    else if(game_state == GAME_STATE_PLAYING)
    {
        draw_minimap();
        draw_hearts();
        draw_xp_bar();
        draw_gauntlet();
        draw_skill_selection();
    }

    if(debug_enabled)
    {
        float zscale = 1.0 - camera_get_zoom();

        Rect mr = RECT(mx, my, 10, 10);
        gfx_draw_rect(&mr, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, NOT_IN_WORLD);
        // mr.x = wmx;
        // mr.y = wmy;
        // mr.w *= zscale;
        // mr.h *= zscale;
        // gfx_draw_rect(&mr, COLOR_BLUE, NOT_SCALED, NO_ROTATION, 1.0, false, IN_WORLD);

        // room border
        // gfx_draw_rect(&room_area, COLOR_WHITE, NOT_SCALED, NO_ROTATION, 1.0, false, true);

        // gfx_draw_rect(&margin_left, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, false);
        // gfx_draw_rect(&margin_right, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, false);
        // gfx_draw_rect(&margin_top, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, false);
        // gfx_draw_rect(&margin_bottom, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, false);

        // Rect xaxis = RECT(0,0,1000,1);
        // Rect yaxis = RECT(0,0,1,1000);
        // gfx_draw_rect(&xaxis, COLOR_PURPLE, NOT_SCALED, NO_ROTATION, 1.0, true, true);
        // gfx_draw_rect(&yaxis, COLOR_PURPLE, NOT_SCALED, NO_ROTATION, 1.0, true, true);

        // //TEST
        // Rect l = margin_left;
        // l.w *= zscale;
        // l.h *= zscale;
        // l.x = cr.x-cr.w/2.0 + l.w/2.0;
        // l.y = cr.y;
        // gfx_draw_rect(&l, COLOR_CYAN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }

    if(editor_enabled)
    {
        editor_draw();
    }

    //imgui_draw_demo(10,10);

    text_list_draw(text_lst);
    message_small_draw();
    gfx_draw_lines();
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
                editor_enabled = !editor_enabled;
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
                if(game_state == GAME_STATE_PLAYING)
                    show_big_map = !show_big_map;
            }

        }
    }
}

// ==================================================================================
// SMALL MESSAGE
// ==================================================================================

void message_small_set(float duration, char* fmt, ...)
{
    static char temp[MESSAGE_SMALL_MAX] = {0};
    memset(temp, 0, sizeof(char)*MESSAGE_SMALL_MAX);
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(temp, MESSAGE_SMALL_MAX, fmt, args);
    va_end(args);

    if(ret < 0 || ret >= MESSAGE_SMALL_MAX)
    {
        LOGE("Failed to format message_small (%d)", ret);
        return;
    }

    memcpy(message_small, temp, sizeof(char)*MESSAGE_SMALL_MAX);
    message_small_duration = duration;
}

void message_small_update(float dt)
{
    message_small_duration -= dt;
}


void message_small_draw()
{
    if(message_small_duration <= 0)
        return;

    if(!message_small)
        return;

    if(strlen(message_small) == 0)
        return;

    float scale = 0.25 * ascale;

    Vector2f size = gfx_string_get_size(scale, message_small);

    float pad = 10.0;

    Rect bg = {0};
    bg.w = size.x + pad;
    bg.h = size.y + pad;
    bg.x = bg.w/2.0 + 1.0;
    bg.y = view_height - bg.h/2.0 - 1.0;

    gfx_draw_rect(&bg, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 0.6, true, NOT_IN_WORLD);

    float tx = bg.x - bg.w/2.0 + pad/2.0;
    float ty = bg.y - bg.h/2.0 + pad/4.0;
    gfx_draw_string(tx, ty, COLOR_WHITE, scale, NO_ROTATION, 0.9, NOT_IN_WORLD, NO_DROP_SHADOW, 0, message_small);
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
