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
#include "lighting.h"
#include "camera.h"

#define ASCII_NUMS {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"}

int player_selection = 0;
static char particles_file_name[33] = {0};

static ParticleSpawner* particle_spawner;
static char* effect_options[100] = {0};
static int prior_selected_effect = 0;
static int selected_effect = 0;

static void randomize_effect(ParticleEffect* effect);
static void load_effect_options();
static void particle_editor_gui();

void editor_init()
{
    // create particle effect used for editor
    ParticleEffect effect = {
        .life = {3.0,5.0,1.0},
        .scale = {0.2,0.5,-0.05},
        .velocity_x = {-16.0,16.0,0.0},
        .velocity_y = {-16.0,16.0,0.0},
        .opacity = {0.6,1.0,-0.2},
        .angular_vel = {0.0,0.0,0.0},
        .rotation_init_min = 0.0,
        .rotation_init_max = 0.0,
        .color1 = 0x00FF00FF,
        .color2 = 0x00CC0000,
        .color3 = 0x00202020,
        .spawn_radius_min = 0.0,
        .spawn_radius_max = 1.0,
        .spawn_time_min = 0.2,
        .spawn_time_max = 0.5,
        .burst_count_min = 1,
        .burst_count_max = 3,
        .sprite_index = 57,
        .img_index = particles_image,
        .use_sprite = true,
        .blend_additive = false,
    };

    randomize_effect(&effect);

    particle_spawner = particles_spawn_effect(200, 120, 1, &effect, 0, false, true);
}

void editor_draw()
{
    if(!editor_enabled)
    {
        if(particle_spawner != NULL) particle_spawner->hidden = true;
        return;
    }

    // int name_count = player_names_build(false, false);

    static Rect gui_size = {0};

    imgui_begin_panel("Editor", view_width - gui_size.w, 1, true);

        imgui_newline();
        char* buttons[] = {"General", "Level", "Players", "Creatures", "Projectiles", "Particles", "Theme Editor","SCUM"};
        int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        float big = 20.0;

        Room* room = level_get_room_by_index(&level,player->phys.curr_room);

        switch(selection)
        {
            case 0: // general
            {
                imgui_text("FPS: %.0f (AVG: %.0f)",game_timer.frame_fps, game_timer.frame_fps_avg);

                if(role == ROLE_CLIENT)
                {
                    imgui_text("Ping: %.2f", net_client_get_rtt());
                    imgui_text("Traffic:");

                    uint32_t sent_bytes = net_client_get_sent_bytes();
                    uint32_t recv_bytes = net_client_get_recv_bytes();

                    uint32_t largest_sent = net_client_get_largest_packet_size_sent();
                    uint32_t largest_recv = net_client_get_largest_packet_size_recv();

                    double connection_time = net_client_get_connected_time();
                    double time_elapsed = timer_get_time() - connection_time;

                    imgui_text("    Sent: %.0f B/s (total: %u B, largest: %u B)", sent_bytes / time_elapsed, sent_bytes, largest_sent);
                    imgui_text("    Recv: %.0f B/s (total: %u B, largest: %u B)", recv_bytes / time_elapsed, recv_bytes, largest_recv);
                }

                imgui_number_box("Camera Zoom", 0, 100, &cam_zoom);
                imgui_number_box("Camera Min Zoom", 0, 100, &cam_min_zoom);

                imgui_text("Current Zoom: %.2f", camera_get_zoom());

                imgui_text("Mouse (view): %.1f, %.1f", mouse_x, mouse_y);
                imgui_text("Mouse (world): %.1f, %.1f", mouse_world_x, mouse_world_y);

                Rect cr = get_camera_rect();
                imgui_text("Camera: %.1f, %.1f, %.1f, %.1f", cr.x, cr.y, cr.w, cr.h);

                imgui_toggle_button(&dynamic_zoom, "Dynamic Zoom");

                Rect limit = camera_limit;
                imgui_text("Camera Limit: %.1f, %.1f, %.1f, %.1f", limit.x, limit.y, limit.w, limit.h);

                imgui_text("View w/h: %d, %d", view_width, view_height);
                imgui_text("Window w/h: %d, %d", window_width, window_height);

            } break;

            case 1: // level
            {

                imgui_color_picker("Ambient Light", &ambient_light);

                static char* dungeon_images[] = {"1", "2", "3"};
                static int first = true;
                if(first)
                {
                    imgui_set_button_select(1, IM_ARRAYSIZE(dungeon_images), dungeon_images, "Dungeon Set");
                    first = false;
                }
                int dungeon_set_select = imgui_button_select(IM_ARRAYSIZE(dungeon_images), dungeon_images, "Dungeon Set");

                switch(dungeon_set_select)
                {
                    case 0: dungeon_image = dungeon_set_image1; break;
                    case 1: dungeon_image = dungeon_set_image2; break;
                    case 2: dungeon_image = dungeon_set_image3; break;
                    default: dungeon_image = dungeon_set_image1; break;
                }

                imgui_text("Current Seed: %u", level_seed);
                imgui_text("Current Rank: %u", level_rank);

                if(role == ROLE_LOCAL)
                {

                    if(imgui_button("Generate Random Level"))
                    {
                        trigger_generate_level(rand(), level_rank, 1, __LINE__);
                    }

                    static char seed_str[32] = {0};
                    imgui_text_box("Input Seed##input seed",seed_str,32);
                    int _seed = atoi(seed_str);

                    static int _rank = 1;
                    imgui_number_box("Input Rank", 1, 10, &_rank);

                    if(imgui_button("Generate New Level"))
                    {
                        trigger_generate_level(_seed, _rank, 2, __LINE__);
                        _rank = level_rank;
                    }

                    static int item_sel = 0;
                    imgui_listbox(item_names, ITEM_MAX, "Item", &item_sel, 4);
                    if(imgui_button("Add Item '%s'", item_names[item_sel]))
                    {
                        item_add(item_sel, player->phys.pos.x, player->phys.pos.y, player->phys.curr_room);
                    }

                    // if(imgui_button("Add Item 'New Level'"))
                    // {
                    //     item_add(ITEM_NEW_LEVEL, player->phys.pos.x, player->phys.pos.y, player->phys.curr_room);
                    // }

                    // if(imgui_button("Add Item 'Chest'"))
                    // {
                    //     item_add(ITEM_CHEST, player->phys.pos.x, player->phys.pos.y, player->phys.curr_room);
                    // }

                    if(imgui_button("Add Random Item"))
                    {
                        item_add(item_rand(true), player->phys.pos.x, player->phys.pos.y, player->phys.curr_room);
                    }

                    if(imgui_button("Add All Items"))
                    {
                        for(int i = 0; i < ITEM_MAX; ++i)
                        {
                            item_add(i, player->phys.pos.x, player->phys.pos.y, player->phys.curr_room);
                        }
                    }
                }

                TileType tt = level_get_tile_type_by_pos(room, mouse_world_x, mouse_world_y);

                Vector2i tc = level_get_room_coords_by_pos(mouse_world_x, mouse_world_y);
                imgui_text("Mouse Tile: %d, %d", tc.x, tc.y);

                Vector2f mc = level_get_pos_by_room_coords(tc.x, tc.y);
                imgui_text("Back to Mouse coords: %.1f, %.1f", mc.x,mc.y);

                Rect tcr = level_get_tile_rect(tc.x, tc.y);
                imgui_text("Back to Mouse coords (rect): %.1f, %.1f", tcr.x,tcr.y);

                // int xd = (tc.x+1) * TILE_SIZE;
                // int yd = (tc.y+1) * TILE_SIZE;
                // float _x = xd - (TILE_SIZE/2.0);
                // float _y = yd - (TILE_SIZE/2.0);
                // imgui_text("MOUSE: %.0f, %.0f",_x,_y);

                imgui_text("  Type: %s (%d)", get_tile_name(tt), tt);

                imgui_text("Doors");
                for(int d = 0; d < 4; ++d)
                {
                    if(room->doors[d])
                        imgui_text(" - %s", get_dir_name(d));
                }

                imgui_toggle_button(&show_walls, "Draw Collision Walls");

                imgui_toggle_button(&show_tile_grid, "Draw Tile Grid");

            } break;

            case 2: // players
            {


                Player* p = player;

                if(role == ROLE_LOCAL)
                {

                    imgui_toggle_button(&players_invincible, "Invincibility");

                    char* pnames[] = ASCII_NUMS;
                    // imgui_dropdown(player_names, name_count, "Select Player", &player_selection, NULL);
                    imgui_dropdown(pnames, MAX_PLAYERS, "Select Player", &player_selection, NULL);
                    Player* p = &players[player_selection];

                    if(p != player)
                    {
                        // imgui_toggle_button(&p->active, "Active");

                        bool active = p->active;
                        imgui_toggle_button(&active, "Active");
                        if(active && !p->active)
                        {
                            p->phys.curr_room = player->phys.curr_room;
                        }
                        p->active = active;
                    }

                    if(p == player)
                    {
                        for(int i = 0; i < MAX_STAT_TYPE; ++i)
                        {
                            int stat_val = p->stats[i];
                            imgui_number_box((char*)stats_get_name(i), 0, 6, &stat_val);
                            p->stats[i] = stat_val;
                        }
                    }

                    int selected_class = p->settings.class;
                    imgui_dropdown(class_strs, 3, "Select Class", &selected_class, NULL);
                    player_set_class(p, (PlayerClass)selected_class);

                    int hp = p->phys.hp;
                    imgui_number_box("HP", 0, p->phys.hp_max, &hp);
                    if(hp == 0 && p->phys.hp != 0)
                    {
                        if(!p->phys.dead && !all_players_dead)
                            player_die(p);
                    }
                    else if(hp > 0 && p->phys.hp == 0)
                    {
                        player_reset(p);
                    }
                    else
                    {
                        p->phys.hp = (uint8_t)hp;
                    }

                    int mp = p->phys.mp;
                    imgui_number_box("MP", 0, p->phys.mp_max, &mp);
                    p->phys.mp = (uint8_t)mp;

                    imgui_slider_float("Jump Velocity", 0.0, 1000.0, &jump_vel_z);
                }

                imgui_text("Level: %d", p->level);
                imgui_text("XP: %d / %d", p->xp, get_xp_req(p->level));

                imgui_text_sized(big, "Info");
                imgui_text("Pos: %.2f, %.2f", p->phys.pos.x, p->phys.pos.y);
                imgui_text("Vel: %.2f, %.2f", p->phys.vel.x, p->phys.vel.y);

                Vector2i c;
                c = level_get_room_coords(p->phys.curr_room);
                imgui_text("C Room: %u (%d, %d)", p->phys.curr_room, c.x, c.y);
                c = level_get_room_coords(p->transition_room);
                imgui_text("T Room: %u (%d, %d)", p->transition_room, c.x, c.y);

                Vector2i tc = level_get_room_coords_by_pos(p->phys.pos.x, p->phys.pos.y);
                imgui_text("Tile: %d, %d", tc.x, tc.y);

                if(role == ROLE_LOCAL)
                {
                    if(imgui_button("Send To Start"))
                    {
                        player_send_to_level_start(p);
                    }

                    if(imgui_button("Hurt"))
                    {
                        player_hurt(p, 1);
                    }

                    static int xp = 100;
                    imgui_number_box("XP to Add", 1, 1000, &xp);

                    if(imgui_button("Add XP"))
                    {
                        player_add_xp(p, xp);
                    }

                    if(imgui_button("Clear Status Effects"))
                    {
                        status_effects_clear(&p->phys);
                    }
                }

                // imgui_text("Skills: %d", p->skill_count);
                // for(int i = 0; i < p->skill_count; ++i)
                // {
                //     Skill* s = &skill_list[p->skills[i]];
                //     imgui_text(" [%d] %s (rank: %d)", i, s->name, s->rank);
                // }

            } break;

            case 3: // creatures
            {

                imgui_text("Total Count: %u", creature_get_count());
                imgui_text("Room Count: %u", creature_get_room_count(player->phys.curr_room, true));

                if(role == ROLE_LOCAL)
                {
                    static int num_creatures = 1;
                    imgui_number_box("Spawn", 1, 100, &num_creatures);

                    Room* room = level_get_room_by_index(&level, player->phys.curr_room);

                    if(imgui_button("Add Slug"))
                    {
                        for(int i = 0; i < num_creatures; ++i)
                            creature_add(room, CREATURE_TYPE_SLUG, NULL, NULL);
                    }

                    if(imgui_button("Add Clinger"))
                    {
                        for(int i = 0; i < num_creatures; ++i)
                            creature_add(room, CREATURE_TYPE_CLINGER, NULL, NULL);
                    }

                    if(imgui_button("Add Geizer"))
                    {
                        for(int i = 0; i < num_creatures; ++i)
                            creature_add(room, CREATURE_TYPE_GEIZER, NULL, NULL);
                    }

                    if(imgui_button("Add Floater"))
                    {
                        for(int i = 0; i < num_creatures; ++i)
                            creature_add(room, CREATURE_TYPE_FLOATER, NULL, NULL);
                    }

                    if(imgui_button("Add Shambler"))
                    {
                        for(int i = 0; i < num_creatures; ++i)
                            creature_add(room, CREATURE_TYPE_SHAMBLER, NULL, NULL);
                    }

                    if(imgui_button("Clear Room"))
                    {
                        creature_kill_room(player->phys.curr_room);
                        // creature_clear_room(player->phys.curr_room);
                    }

                    if(imgui_button("Clear All"))
                    {
                        creature_kill_all();
                        // creature_clear_all();
                    }

                    static float creature_speed = -1;
                    static bool  creature_painful_touch = false;

                    if(creature_speed == -1)
                    {
                        creature_speed = creatures[0].phys.speed;
                        creature_painful_touch = creatures[0].painful_touch;
                    }

                    imgui_slider_float("Speed",10.0,100.0,&creature_speed);

                    imgui_toggle_button(&creatures_can_move, "AI");

                    imgui_toggle_button(&creature_painful_touch, "Painful Touch");

                    for(int i = 0; i < creature_get_count(); ++i)
                    {
                        creatures[i].phys.speed = creature_speed;
                        creatures[i].painful_touch = creature_painful_touch;
                    }
                }

            } break;

            case 4: // projectiles
            {
                imgui_store_theme();

                imgui_set_text_size(9.0);
                imgui_text("Total Count: %u", plist->count);

                if(role == ROLE_LOCAL)
                {
                    char* proj_def_names[PROJECTILE_TYPE_MAX] = {0};
                    for(int i = 0; i < PROJECTILE_TYPE_MAX; ++i)
                    {
                        proj_def_names[i] = (char*)projectile_def_get_name(i);
                    }

                    static int proj_sel = 0;
                    imgui_dropdown(proj_def_names, PROJECTILE_TYPE_MAX, "Projectile Definition", &proj_sel, NULL);

                    static int charge_type_sel = 0;
                    static int spread_type_sel = 0;

                    Gun* gun = &gun_lookup[proj_sel];

                    if(proj_sel == PROJECTILE_TYPE_PLAYER)
                    {
                        gun = &player->gun;
                    }

                    if(imgui_button("Randomize"))
                    {
                        gun->damage_min = RAND_FLOAT(0.0,100.0);
                        gun->damage_max = RAND_FLOAT(0.0,100.0);
                        gun->lifetime   = RAND_FLOAT(0.2,3.0);
                        gun->speed      = RAND_FLOAT(100.0,1000.0);
                        gun->cooldown = RAND_FLOAT(0.1, 1.0);

                        bool wavy = (rand() % 4 == 0);
                        gun->wave_amplitude = wavy ? RAND_FLOAT(0.0,500.0) : 0.0;
                        gun->wave_period = wavy ? RAND_FLOAT(0.0,1.0) : 0.0;

                        bool accels = (rand() % 4 == 0);
                        gun->directional_accel = accels ? RAND_FLOAT(-1.0,1.0) : 0.0;

                        bool air_friction = (rand() % 4 == 0);
                        gun->air_friction = air_friction ? RAND_FLOAT(0.0,1.0) : 0.0;

                        gun->gravity_factor = RAND_FLOAT(0.0,2.0);
                        gun->scale1 = RAND_FLOAT(0.1,2.5);
                        gun->scale2 = RAND_FLOAT(0.1,2.5);
                        gun->color1 = rand();
                        gun->color2 = rand();
                        gun->knockback_factor = RAND_FLOAT(0.0,1.0);
                        gun->critical_hit_chance = RAND_FLOAT(0.0,1.0);

                        bool spinning = (rand() % 4 == 0);
                        gun->spin_factor = spinning ? RAND_FLOAT(-1.0,1.0) : 0.0;

                        bool random_spread = (rand() % 4 == 0);
                        gun->spread_type = random_spread ? SPREAD_TYPE_RANDOM : SPREAD_TYPE_UNIFORM;
                        spread_type_sel = gun->spread_type;
                        gun->spread = RAND_FLOAT(0.0, 360.0);

                        gun->sprite_index = rand() % 10;

                        bool more_than_one = (rand() % 4 == 0);
                        gun->num = more_than_one ? rand() % 9 + 1 : 1;

                        bool explosive = (rand() % 4 == 0);
                        gun->explosion_chance = explosive ? RAND_FLOAT(0.0, 1.0) : 0.0;

                        bool bouncy = (rand() % 4 == 0);
                        gun->bounce_chance = bouncy ? RAND_FLOAT(0.0, 1.0) : 0.0;

                        bool penetrative = (rand() % 4 == 0);
                        gun->penetration_chance = penetrative ? RAND_FLOAT(0.0, 1.0) : 0.0;

                        bool homing = (rand() % 4 == 0);
                        gun->homing_chance = homing ? RAND_FLOAT(0.0, 1.0) : 0.0;

                        bool ghost = (rand() % 4 == 0);
                        gun->ghost_chance = ghost ? RAND_FLOAT(0.0, 1.0) : 0.0;

                        bool burst = (rand() % 4 == 0);
                        gun->burst_count = burst ? rand() % 4 + 1 : 0;
                        gun->burst_rate  = burst ? RAND_FLOAT(0.1, 1.0) : 0.0;

                        bool elemental = (rand() % 4 == 0);
                        if(elemental)
                        {
                            gun->fire_chance = RAND_FLOAT(0.0, 1.0);
                            gun->cold_chance = RAND_FLOAT(0.0, 1.0);
                            gun->poison_chance = RAND_FLOAT(0.0, 1.0);
                            gun->lightning_chance = RAND_FLOAT(0.0, 1.0);
                        }
                        else
                        {
                            gun->fire_chance = 0.0;
                            gun->cold_chance = 0.0;
                            gun->poison_chance = 0.0;
                            gun->lightning_chance = 0.0;
                        }

                        gun->cluster = (rand() % 4 == 0);
                        if(gun->cluster)
                        {
                            gun->cluster_stages = rand() % 2 + 1;
                            for(int i = 0; i < gun->cluster_stages; ++i)
                            {
                                gun->cluster_num[i] = rand() % 4 + 1;
                                gun->cluster_scales[i] = RAND_FLOAT(0.1,2.0);
                            }
                        }

                        gun->chargeable = (rand() % 4 == 0);
                        if(gun->chargeable)
                        {
                            gun->charge_type = rand() % CHARGE_TYPE_COUNT;
                            charge_type_sel = gun->charge_type;
                            gun->charge_time = 0.0;
                            gun->charge_time_max = RAND_FLOAT(0.3, 2.0);
                        }

                        //gun->orbital = rand() % 2;
                        //gun->orbital_distance = RAND_FLOAT(1.0, 100.0);
                    }

                    imgui_horizontal_begin();
                        imgui_slider_float("Damage Min", 0.0,100.0,&gun->damage_min);
                        if(gun->damage_min > gun->damage_max)
                            gun->damage_max = gun->damage_min;
                        imgui_slider_float("Damage Max", 0.0,100.0,&gun->damage_max);
                    imgui_horizontal_end();

                    imgui_slider_float("Lifetime", 0.2,3.0,&gun->lifetime);
                    imgui_slider_float("Base Speed", 100.0,1000.0,&gun->speed);

                    imgui_slider_float("Cooldown", 0.1,1.0,&gun->cooldown);

                    imgui_horizontal_begin();
                        imgui_slider_float("Wave Amplitude", 0.0,500.0,&gun->wave_amplitude);
                        imgui_slider_float("Wave Period", 0.0,1.0,&gun->wave_period);
                    imgui_horizontal_end();

                    imgui_slider_float("Directional Accel", -1.0,1.0,&gun->directional_accel);

                    imgui_horizontal_begin();
                        imgui_slider_float("Gravity Factor", 0.0,2.0,&gun->gravity_factor);
                        imgui_slider_float("Air Friction", 0.0,1.0,&gun->air_friction);
                    imgui_horizontal_end();

                    imgui_horizontal_begin();
                        imgui_number_box("Burst Count", 0, 4,&gun->burst_count);
                        imgui_slider_float("Burst Rate", 0.1, 1.0,&gun->burst_rate);
                    imgui_horizontal_end();

                    imgui_checkbox("Chargeable", &gun->chargeable);

                    if(gun->chargeable)
                    {
                        char* charge_type_names[CHARGE_TYPE_COUNT] = {0};
                        for(int i = 0; i < CHARGE_TYPE_COUNT; ++i)
                            charge_type_names[i] = (char*)projectile_charge_type_get_name(i);

                        imgui_horizontal_begin();
                            imgui_dropdown(charge_type_names, CHARGE_TYPE_COUNT, "Charge Type", &charge_type_sel, NULL);
                            gun->charge_type = charge_type_sel;
                            imgui_slider_float("Charge Time Max", 0.5, 5.0,&gun->charge_time_max);
                        imgui_horizontal_end();

                    }

                    imgui_horizontal_begin();
                        imgui_slider_float("Scale1", 0.1, 2.5,&gun->scale1);
                        imgui_slider_float("Scale2", 0.1, 2.5,&gun->scale2);
                    imgui_horizontal_end();

                    imgui_horizontal_begin();
                        imgui_color_picker("Color1", &gun->color1);
                        imgui_color_picker("Color2", &gun->color2);
                    imgui_horizontal_end();

                    imgui_slider_float("Spin Factor", -1.0, 1.0, &gun->spin_factor);

                    imgui_horizontal_begin();
                        imgui_slider_float("Knockback Factor", 0.0, 1.0, &gun->knockback_factor);
                        imgui_slider_float("Critical Hit Chance", 0.0, 1.0, &gun->critical_hit_chance);
                    imgui_horizontal_end();

                    char* spread_type_names[SPREAD_TYPE_COUNT] = {0};
                    for(int i = 0; i < SPREAD_TYPE_COUNT; ++i)
                        spread_type_names[i] = (char*)projectile_spread_type_get_name(i);

                    imgui_horizontal_begin();
                        imgui_dropdown(spread_type_names, SPREAD_TYPE_COUNT, "Spread Type", &spread_type_sel, NULL);
                        gun->spread_type = spread_type_sel;
                        imgui_slider_float("Angle Spread", 0.0, 360.0,&gun->spread);
                    imgui_horizontal_end();

                    imgui_number_box("Sprite Index", 0,9, &gun->sprite_index);

                    int num = gun->num;
                    imgui_number_box("Num", 1,10, &num);
                    gun->num = num;

                    imgui_text_sized(20.0,"Attributes:");
                    imgui_horizontal_line(1);
                    imgui_slider_float("Explosive Chance",0.0, 1.0, &gun->explosion_chance);
                    imgui_slider_float("Bounce Chance", 0.0, 1.0, &gun->bounce_chance);
                    imgui_slider_float("Penetration Chance", 0.0, 1.0, &gun->penetration_chance);

                    imgui_checkbox("Orbital", &gun->orbital);
                    if(gun->orbital)
                    {
                        imgui_slider_float("Orbital Distance", 1.0,100.0, &gun->orbital_distance);
                    }

                    imgui_checkbox("Cluster", &gun->cluster);

                    if(gun->cluster)
                    {
                        imgui_number_box("Cluster Stages", 1, MAX_CLUSTER_STAGES, &gun->cluster_stages);

                        imgui_number_box("Stage 1 Num", 1,5, &gun->cluster_num[0]);
                        imgui_slider_float("Stage 1 Scale", 0.1, 2.0, &gun->cluster_scales[0]);

                        if(gun->cluster_stages >= 2)
                        {
                            imgui_number_box("Stage 2 Num", 1,10, &gun->cluster_num[1]);
                            imgui_slider_float("Stage 2 Scale", 0.1, 2.0, &gun->cluster_scales[1]);
                        }
                    }
                    imgui_slider_float("Homing Chance", 0.0, 1.0, &gun->homing_chance);
                    imgui_slider_float("Ghost Chance", 0.0, 1.0, &gun->ghost_chance);
                    imgui_slider_float("Cold Chance", 0.0, 1.0, &gun->cold_chance);
                    imgui_slider_float("Poison Chance", 0.0, 1.0,&gun->poison_chance);
                }

                imgui_restore_theme();

            } break;

            case 5: // particle editor
            {
                particle_editor_gui();
            } break;

            case 6:
            {
                imgui_theme_editor();
            } break;

            case 7: //SCUM
            {
                static bool searching_for_seed = false;
                static int search_seed = 0;
                static int search_rank = 1;
                static bool search_success = false;
                static Room search_room = {0};

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


                if(searching_for_seed)
                {
                    imgui_text("Searching for '%s' (seed: %d, rank: %d)", selected_room_name, search_seed, search_rank);
                    if(imgui_button("Stop Search"))
                    {
                        searching_for_seed = false;
                    }
                    else
                    {
                        search_seed++;
                        Level search_level = level_generate(search_seed, search_rank);

                        for(int y = 0; y < MAX_ROOMS_GRID_Y; ++y)
                        {
                            for(int x = 0; x < MAX_ROOMS_GRID_X; ++x)
                            {
                                Room* room = &search_level.rooms[x][y];
                                if(!room->valid) continue;

                                char* room_fname = room_files[room_list[room->layout].file_index];
                                if(STR_EQUAL(room_fname, selected_room_name))
                                {
                                    search_success = true;
                                    searching_for_seed = false;
                                    memcpy(&search_room, room, sizeof(Room));
                                    trigger_generate_level(search_seed, search_rank, 1, __LINE__);
                                    break;
                                }

                            }
                        }

                    }

                }
                else
                {

                    if(search_success)
                    {
                        imgui_text("Seed: %d, Rank: %d", search_seed, search_rank);


                        if(role == ROLE_CLIENT)
                        {
                            search_success = false;
                            net_client_send_message("$level %d %d", search_seed, search_rank);
                        }

                        if(imgui_button("Goto Room") || role == ROLE_CLIENT)
                        {
                            if(role == ROLE_CLIENT)
                            {
                                net_client_send_message("$level %d %d", search_seed, search_rank);
                                search_success = false;
                            }

                            if(role != ROLE_CLIENT)
                            {
                                for(int i = 0; i < 4; ++i)
                                {
                                    if(search_room.doors[i])
                                    {
                                        player_send_to_room(player, search_room.index, true, level_get_door_tile_coords(i));
                                        break;
                                    }
                                }
                            }

                        }

                    }

                    if(imgui_button("Start Search"))
                    {
                        search_seed = -1;
                        search_rank = 1;
                        searching_for_seed = true;
                        search_success = false;
                    }


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
                    if(_filtered_room_files_count != filtered_room_files_count || memcmp(_room_file_sel_index_map, room_file_sel_index_map, sizeof(int)*MAX_ROOM_LIST_COUNT) != 0)
                    {
                        filtered_room_files_count = _filtered_room_files_count;
                        memcpy(room_file_sel_index_map, _room_file_sel_index_map, sizeof(int)*MAX_ROOM_LIST_COUNT);
                        for(int i = 0; i < filtered_room_files_count; ++i)
                        {
                            filtered_room_files[i] = p_room_files[room_file_sel_index_map[i]];
                        }

                        room_file_sel = 0;
                        for(int i = 0; i < filtered_room_files_count; ++i)
                        {
                            if(strcmp(selected_room_name, filtered_room_files[i]) == 0)
                            {
                                room_file_sel = i;
                                break;
                            }
                        }
                    }

                    prior_room_file_sel = room_file_sel;
                    imgui_listbox(filtered_room_files, filtered_room_files_count, "##files_listbox", &room_file_sel, 15);

                    if(filtered_room_files[room_file_sel])
                        snprintf(selected_room_name_str,100,"Selected: %s",filtered_room_files[room_file_sel]);

                    strcpy(selected_room_name, filtered_room_files[room_file_sel]);
                    imgui_text(selected_room_name_str);

                    // search_rank = room_list[ _room_file_sel_index_map[room_file_sel] ].rank;

                    int layout = _room_file_sel_index_map[room_file_sel];
                    RoomFileData* rfd = &room_list[layout];
                    search_rank = rfd->rank;

                    Room room = {0};
                    room.valid = true;
                    memcpy(room.doors, rfd->doors, 4);
                    room.type = rfd->type;
                    room.layout = layout;
                    room.doors_locked = false;

                    float scale = 0.3;
                    float xo = room_area.w/4.0 - room_area.w/2.0*scale;
                    float yo = room_area.h/2.0 - room_area.h/2.0*scale;

                    level_draw_room(&room, rfd, xo, yo, 0.3, true);
                }



            } break;

        }

    if(selection != 5)
    {
        particle_spawner->hidden = true;
    }
    gui_size = imgui_end();
}

static void particle_editor_gui()
{

    ParticleEffect* effect = &particle_spawner->effect;
    particle_spawner->hidden = false;

    gfx_draw_string(view_width-300, 100, 0xAAAAAAAA, 0.2, 0.0, 1.0, false, false, 0.0, "Preview");
    gfx_draw_rect_xywh(view_width-200, 200, 200, 200, 0x00000000, 0.0, 1.0, 0.5, true,false);

    int big = 12;
    imgui_set_text_size(10);

    imgui_newline();

    imgui_horizontal_begin();

    if(imgui_button("Randomize##particle_spawner"))
    {
        randomize_effect(effect);
    }
    if(imgui_button("Reload Effects##particle_spawner"))
    {
        load_effect_options();
    }

    if(!effect_options[0])
        load_effect_options();

    imgui_horizontal_end();

    imgui_dropdown(effect_options, num_effects, "Effects", &selected_effect, NULL);
    if(selected_effect > 0 && prior_selected_effect != selected_effect)
    {
        prior_selected_effect = selected_effect;
        memcpy(effect,&particle_effects[selected_effect-1],sizeof(ParticleEffect));
        
        strncpy(particles_file_name,effect_options[selected_effect],32);
        remove_extension(particles_file_name);
    }

    imgui_horizontal_line(1);

    //imgui_set_slider_width(60);
    imgui_text_sized(8,"Particle Count: %d",particle_spawner->particle_list->count);
    imgui_text_sized(big,"Particle Life");
    imgui_horizontal_begin();
        imgui_slider_float("Min##life", 0.1,5.0,&effect->life.init_min);
        imgui_slider_float("Max##life", 0.1,5.0,&effect->life.init_max);
        effect->life.init_max = (effect->life.init_min > effect->life.init_max ? effect->life.init_min : effect->life.init_max);
        imgui_slider_float("Rate##life", 0.1,5.0,&effect->life.rate);
    imgui_horizontal_end();
    imgui_text_sized(big,"Rotation");
    imgui_horizontal_begin();
        imgui_slider_float("Min##rotation", -360.0,360.0,&effect->rotation_init_min);
        imgui_slider_float("Max##rotation", -360.0,360.0,&effect->rotation_init_max);
        effect->rotation_init_max = (effect->rotation_init_min > effect->rotation_init_max ? effect->rotation_init_min : effect->rotation_init_max);
    imgui_horizontal_end();
    imgui_text_sized(big,"Scale");
    imgui_horizontal_begin();
        imgui_slider_float("Min##scale", 0.01,3.00,&effect->scale.init_min);
        imgui_slider_float("Max##scale", 0.01,3.00,&effect->scale.init_max);
        effect->scale.init_max = (effect->scale.init_min > effect->scale.init_max ? effect->scale.init_min : effect->scale.init_max);
        imgui_slider_float("Rate##scale", -1.0,1.0,&effect->scale.rate);
    imgui_horizontal_end();
    imgui_text_sized(big,"Velocity X");
    imgui_horizontal_begin();
        imgui_slider_float("Min##velx", -300.0,300.0,&effect->velocity_x.init_min);
        imgui_slider_float("Max##velx", -300.0,300.0,&effect->velocity_x.init_max);
        effect->velocity_x.init_max = (effect->velocity_x.init_min > effect->velocity_x.init_max ? effect->velocity_x.init_min : effect->velocity_x.init_max);
        imgui_slider_float("Rate##velx", -300.0,300.0,&effect->velocity_x.rate);
    imgui_horizontal_end();
    imgui_text_sized(big,"Velocity Y");
    imgui_horizontal_begin();
        imgui_slider_float("Min##vely", -300.0,300.0,&effect->velocity_y.init_min);
        imgui_slider_float("Max##vely", -300.0,300.0,&effect->velocity_y.init_max);
        effect->velocity_y.init_max = (effect->velocity_y.init_min > effect->velocity_y.init_max ? effect->velocity_y.init_min : effect->velocity_y.init_max);
        imgui_slider_float("Rate##vely", -300.0,300.0,&effect->velocity_y.rate);
    imgui_horizontal_end();
    imgui_text_sized(big,"Opacity");
    imgui_horizontal_begin();
        imgui_slider_float("Min##opacity", 0.01,1.0,&effect->opacity.init_min);
        imgui_slider_float("Max##opacity", 0.01,1.0,&effect->opacity.init_max);
        effect->opacity.init_max = (effect->opacity.init_min > effect->opacity.init_max ? effect->opacity.init_min : effect->opacity.init_max);
        imgui_slider_float("Rate##opacity", -1.0,1.0,&effect->opacity.rate);
    imgui_horizontal_end();
    imgui_text_sized(big,"Angular Velocity");
    imgui_horizontal_begin();
        imgui_slider_float("Min##angular_vel", -360.0,360.0,&effect->angular_vel.init_min);
        imgui_slider_float("Max##angular_vel", -360.0,360.0,&effect->angular_vel.init_max);
        effect->angular_vel.init_max = (effect->angular_vel.init_min > effect->angular_vel.init_max ? effect->angular_vel.init_min : effect->angular_vel.init_max);
        imgui_slider_float("Rate##angular_vel", -360.0,360.0,&effect->angular_vel.rate);
    imgui_horizontal_end();
    imgui_text_sized(big,"Spawn Radius");
    imgui_horizontal_begin();
        imgui_slider_float("Min##spawn_radius", 0.0,64.0,&effect->spawn_radius_min);
        imgui_slider_float("Max##spawn_radius", 0.0,64.0,&effect->spawn_radius_max);
        effect->spawn_radius_max = (effect->spawn_radius_min > effect->spawn_radius_max ? effect->spawn_radius_min : effect->spawn_radius_max);
    imgui_horizontal_end();
    imgui_text_sized(big,"Spawn Time");
    imgui_horizontal_begin();
        imgui_slider_float("Min##spawn_time", 0.01,2.0,&effect->spawn_time_min);
        imgui_slider_float("Max##spawn_time", 0.01,2.0,&effect->spawn_time_max);
        effect->spawn_time_max = (effect->spawn_time_min > effect->spawn_time_max ? effect->spawn_time_min : effect->spawn_time_max);
    imgui_horizontal_end();
    imgui_text_sized(big,"Burst Count");
    imgui_horizontal_begin();
        imgui_number_box("Min##burst_count", 1, 20, &effect->burst_count_min);
        imgui_number_box("Max##burst_count", 1, 20, &effect->burst_count_max);
        effect->burst_count_max = (effect->burst_count_min > effect->burst_count_max ? effect->burst_count_min : effect->burst_count_max);
    imgui_horizontal_end();
    imgui_checkbox("Use Sprite",&effect->use_sprite);
    if(effect->use_sprite)
    {
        imgui_horizontal_begin();
        imgui_number_box("Img Index##img_index", 0, MAX_GFX_IMAGES-1, &effect->img_index);
        imgui_number_box("Sprite Index##sprite_index", 0, MAX_GFX_IMAGES-1, &effect->sprite_index);
        imgui_horizontal_end();
    }
    imgui_text_sized(big,"Colors");
    imgui_horizontal_begin();
        imgui_color_picker("1##colors", &effect->color1);
        imgui_color_picker("2##colors", &effect->color2);
        imgui_color_picker("3##colors", &effect->color3);
    imgui_horizontal_end();
    imgui_checkbox("Blend Addtive",&effect->blend_additive);
    imgui_newline();

    imgui_text_box("Filename##file_name_particles",particles_file_name,IM_ARRAYSIZE(particles_file_name));

    char file_path[64]= {0};
    snprintf(file_path,63,"src/effects/%s.effect",particles_file_name);

    if(!STR_EMPTY(particles_file_name))
    {
        if(imgui_button("Save##particles"))
        {
            effects_save(file_path, effect);
        }
    }

    if(io_file_exists(file_path))
    {
        imgui_text_colored(0x00CC8800, "File Exists!");
    }

    // show preview
    particles_draw_spawner(particle_spawner, true, false);


}

static void load_effect_options()
{
    effects_load_all();

    for(int i = 0; i < num_effects+1; ++i)
    {
        if(effect_options[i])
            free(effect_options[i]);
    }
    effect_options[0] = calloc(33,sizeof(char));
    strncpy(effect_options[0], "*New*", 5);

    for(int i = 0; i < num_effects; ++i)
    {
        effect_options[i+1] = calloc(33,sizeof(char));
        strncpy(effect_options[i+1],particle_effects[i].name, 32);
    }

}
static void randomize_effect(ParticleEffect* effect)
{
    effect->life.init_min = RAND_FLOAT(0.1,5.0);
    effect->life.init_max = RAND_FLOAT(0.1,5.0);
    effect->life.rate = 1.0;

    effect->rotation_init_min = RAND_FLOAT(-360.0,360.0);
    effect->rotation_init_max = RAND_FLOAT(-360.0,360.0);

    effect->scale.init_min = RAND_FLOAT(0.01,1.0);
    effect->scale.init_max = RAND_FLOAT(0.01,1.0);
    effect->scale.rate     = RAND_FLOAT(-0.5,0.5);

    effect->velocity_x.init_min = RAND_FLOAT(-32.0,32.0);
    effect->velocity_x.init_max = RAND_FLOAT(-32.0,32.0);
    effect->velocity_x.rate     = RAND_FLOAT(-32.0,32.0);

    effect->velocity_y.init_min = RAND_FLOAT(-32.0,32.0);
    effect->velocity_y.init_max = RAND_FLOAT(-32.0,32.0);
    effect->velocity_y.rate     = RAND_FLOAT(-32.0,32.0);

    effect->opacity.init_min = RAND_FLOAT(0.01,1.0);
    effect->opacity.init_max = RAND_FLOAT(0.01,1.0);
    effect->opacity.rate     = RAND_FLOAT(-1.0,1.0);

    effect->angular_vel.init_min = RAND_FLOAT(-360.0, 360.0);
    effect->angular_vel.init_max = RAND_FLOAT(-360.0, 360.0);
    effect->angular_vel.rate     = RAND_FLOAT(-360.0, 360.0);

    effect->spawn_radius_min  = RAND_FLOAT(0.0, 64.0);
    effect->spawn_radius_max  = RAND_FLOAT(0.0, 64.0);

    effect->spawn_time_min  = RAND_FLOAT(0.01, 2.0);
    effect->spawn_time_max  = RAND_FLOAT(0.01, 2.0);

    effect->burst_count_min  = RAND_FLOAT(1, 20);
    effect->burst_count_max  = RAND_FLOAT(1, 20);

    effect->img_index = particles_image;
    effect->sprite_index = RAND_RANGE(0,79);

    effect->color1 = RAND_RANGE(0x0,0x00FFFFFF);
    effect->color2 = RAND_RANGE(0x0,0x00FFFFFF);
    effect->color3 = RAND_RANGE(0x0,0x00FFFFFF);

    effect->blend_additive = RAND_RANGE(0,1) == 1 ? true : false;
}
