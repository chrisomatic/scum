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
        char* buttons[] = {"General", "Level", "Players", "Creatures", "Projectiles", "Particles", "Theme Editor"};
        int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        float big = 20.0;

        Room* room = level_get_room_by_index(&level,player->curr_room);

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

                    if(imgui_button("Add Item 'New Level'"))
                    {
                        item_add(ITEM_NEW_LEVEL, player->phys.pos.x, player->phys.pos.y, player->curr_room);
                    }

                    if(imgui_button("Add Random Item"))
                    {
                        item_add(item_rand(true), player->phys.pos.x, player->phys.pos.y, player->curr_room);
                    }

                    if(imgui_button("Add All Items"))
                    {
                        for(int i = 0; i < ITEM_MAX; ++i)
                        {
                            item_add(i, player->phys.pos.x, player->phys.pos.y, player->curr_room);
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
                            p->curr_room = player->curr_room;
                        }
                        p->active = active;
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

                    imgui_slider_float("Jump Velocity", 0.0, 1000.0, &jump_vel_z);
                }

                imgui_text("Level: %d", p->level);
                imgui_text("XP: %d / %d", p->xp, get_xp_req(p->level));

                imgui_text_sized(big, "Info");
                imgui_text("Pos: %.2f, %.2f", p->phys.pos.x, p->phys.pos.y);
                imgui_text("Vel: %.2f, %.2f", p->phys.vel.x, p->phys.vel.y);

                Vector2i c;
                c = level_get_room_coords(p->curr_room);
                imgui_text("C Room: %u (%d, %d)", p->curr_room, c.x, c.y);
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

                imgui_text("Skills: %d", p->skill_count);
                for(int i = 0; i < p->skill_count; ++i)
                {
                    Skill* s = &skill_list[p->skills[i]];
                    imgui_text(" [%d] %s (rank: %d)", i, s->name, s->rank);
                }

            } break;

            case 3: // creatures
            {

                imgui_text("Total Count: %u", creature_get_count());
                imgui_text("Room Count: %u", creature_get_room_count(player->curr_room, true));

                if(role == ROLE_LOCAL)
                {
                    static int num_creatures = 1;
                    imgui_number_box("Spawn", 1, 100, &num_creatures);

                    Room* room = level_get_room_by_index(&level, player->curr_room);

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
                        creature_kill_room(player->curr_room);
                        // creature_clear_room(player->curr_room);
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
                imgui_text("Total Count: %u", plist->count);

                imgui_slider_float("Player Cooldown", 0.0,1.0,&player->proj_cooldown_max);

                if(role == ROLE_LOCAL)
                {

                    char* proj_def_names[PROJECTILE_TYPE_MAX] = {0};
                    for(int i = 0; i < PROJECTILE_TYPE_MAX; ++i)
                    {
                        proj_def_names[i] = (char*)projectile_def_get_name(i);
                    }

                    static int proj_sel = 0;
                    imgui_dropdown(proj_def_names, PROJECTILE_TYPE_MAX, "Projectile Definition", &proj_sel, NULL);


                    ProjectileDef* projd = &projectile_lookup[proj_sel];
                    ProjectileSpawn* projs = &projectile_spawn[proj_sel];

                    if(proj_sel == PROJECTILE_TYPE_PLAYER)
                    {
                        projd = &player->proj_def;
                        projs = &player->proj_spawn;
                    }

                    imgui_slider_float("Damage", 0.0,100.0,&projd->damage);
                    imgui_slider_float("Base Speed", 100.0,1000.0,&projd->speed);
                    imgui_slider_float("Acceleration", -50.0,50.0,&projd->accel);
                    imgui_slider_float("TTL", 0.0,60.0,&projd->ttl);
                    imgui_slider_float("Scale", 0.1, 5.0,&projd->scale);
                    imgui_slider_float("Angle Spread", 0.0, 360.0,&projs->spread);

                    int num = projs->num;
                    imgui_number_box("Num", 1,100, &num);
                    projs->num = num;

                    imgui_text_sized(20.0,"Attributes:");
                    imgui_horizontal_line(1);
                    imgui_checkbox("Explosive", &projd->explosive);
                    imgui_checkbox("Bouncy", &projd->bouncy);
                    imgui_checkbox("Penetrate", &projd->penetrate);
                    imgui_checkbox("Cluster", &projd->cluster);

                    if(projd->cluster)
                    {
                        imgui_number_box("Cluster Stages", 1,3, &projd->cluster_stages);

                        imgui_number_box("Stage 1 Num", 1,10, &projd->cluster_num[0]);
                        imgui_slider_float("Stage 1 Scale", 0.1, 2.0, &projd->cluster_scales[0]);

                        if(projd->cluster_stages >= 2)
                        {
                            imgui_number_box("Stage 2 Num", 1,10, &projd->cluster_num[1]);
                            imgui_slider_float("Stage 2 Scale", 0.1, 2.0, &projd->cluster_scales[1]);
                        }

                        if(projd->cluster_stages >= 3)
                        {
                            imgui_number_box("Stage 3 Num", 1,10, &projd->cluster_num[2]);
                            imgui_slider_float("Stage 3 Scale", 0.1, 2.0, &projd->cluster_scales[2]);
                        }
                    }
                    imgui_slider_float("Homing Chance", 0.0, 1.0, &projs->homing_chance);
                    imgui_slider_float("Ghost Chance", 0.0, 1.0, &projs->ghost_chance);
                    imgui_slider_float("Cold Chance", 0.0, 1.0, &projs->cold_chance);
                    imgui_slider_float("Poison Chance", 0.0, 1.0,&projs->poison_chance);
                }

            } break;

            case 5: // particle editor
            {
                particle_editor_gui();
            } break;
            case 6:
            {
                imgui_theme_editor();
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
