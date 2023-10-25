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

#define ASCII_NUMS {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"}

int player_selection = 0;

void editor_init()
{

}

void editor_draw()
{
    int name_count = player_names_build(false, true);

    imgui_begin_panel("Editor", view_width - 300, 1, true);

        imgui_newline();
        char* buttons[] = {"General", "Level", "Players", "Creatures", "Projectiles", "Theme"};
        int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        float big = 20.0;

        Room* room = level_get_room_by_index(&level,player->curr_room);

        switch(selection)
        {
            case 0: // general
            {
                imgui_slider_float("Zoom", 0.01,.99, &cam_zoom);

                imgui_text("Mouse (view): %.1f, %.1f", mx, my);
                imgui_text("Mouse (world): %.1f, %.1f", wmx, wmy);

                Rect cr = get_camera_rect();
                imgui_text("Camera: %.1f, %.1f, %.1f, %.1f", cr.x, cr.y, cr.w, cr.h);

                Rect limit = camera_limit;
                imgui_text("Camera Limit: %.1f, %.1f, %.1f, %.1f", limit.x, limit.y, limit.w, limit.h);

                imgui_text("View w/h: %d, %d", view_width, view_height);
                imgui_text("Window w/h: %d, %d", window_width, window_height);

            } break;

            case 1: // level
            {

                TileType tt = level_get_tile_type_by_pos(room, wmx, wmy);

                Vector2i tc = level_get_room_coords_by_pos(wmx, wmy);
                imgui_text("Mouse Tile: %d, %d", tc.x, tc.y);
                imgui_text("  Type: %s (%d)", level_tile_type_to_str(tt), tt);

                imgui_text("Doors");
                for(int d = 0; d < 4; ++d)
                {
                    if(room->doors[d])
                        imgui_text(" - %s", get_dir_name(d));
                }

                imgui_toggle_button(&show_walls, "Draw Collision Walls");

            } break;

            case 2: // players
            {

                player_selection = imgui_dropdown(player_names, name_count, "Select Player", &player_selection);
                Player* p = &players[player_selection];

                int hp = p->hp;
                imgui_number_box("HP", 0, p->hp_max, &hp);
                p->hp = (uint8_t)hp;

                imgui_text_sized(big, "Info");
                imgui_text("Pos: %.2f, %.2f", p->phys.pos.x, p->phys.pos.y);
                imgui_text("Vel: %.2f, %.2f", p->phys.vel.x, p->phys.vel.y);

                Vector2i c;
                c = level_get_room_coords(p->curr_room);
                imgui_text("C Room: %u (%d, %d)", p->curr_room, c.x, c.y);
                c = level_get_room_coords(p->transition_room);
                imgui_text("T Room: %u (%d, %d)", p->transition_room, c.x, c.y);


                Vector2i tc = level_get_room_coords_by_pos(CPOSX(p->phys), CPOSY(p->phys));
                imgui_text("Tile: %d, %d", tc.x, tc.y);

                if(imgui_button("Send To Start"))
                {
                    player_send_to_level_start(p);
                }

                if(imgui_button("Hurt"))
                {
                    player_hurt(p, 1);
                }

            } break;

            case 3: // creatures
            {

                static int num_creatures = 1;
                imgui_number_box("Number", 1, 100, &num_creatures);

                Room* room = level_get_room_by_index(&level, player->curr_room);

                if(imgui_button("Add Slug"))
                {
                    for(int i = 0; i < num_creatures; ++i)
                        creature_add(room, CREATURE_TYPE_SLUG, NULL);
                }

                if(imgui_button("Add Clinger"))
                {
                    for(int i = 0; i < num_creatures; ++i)
                        creature_add(room, CREATURE_TYPE_CLINGER, NULL);
                }

                if(imgui_button("Clear"))
                {
                    creature_clear_all();
                }

                static float creature_speed = -1;
                static bool  creature_painful_touch = false;

                if(creature_speed == -1)
                {
                    creature_speed = creatures[0].phys.speed;
                    creature_painful_touch = creatures[0].painful_touch;
                }

                imgui_slider_float("Speed",100.0,500.0,&creature_speed);
                imgui_toggle_button(&creature_painful_touch, "Painful Touch");

                for(int i = 0; i < creature_get_count(); ++i)
                {
                    creatures[i].phys.speed = creature_speed;
                    creatures[i].painful_touch = creature_painful_touch;
                }
            } break;

            case 4: // projectiles
            {
                imgui_text_sized(big, "Gun");
                imgui_slider_float("Damage", 0.0,100.0,&projectile_lookup[0].damage);
                imgui_slider_float("Cooldown", 0.0,1.0,&player->proj_cooldown_max);
                imgui_slider_float("Base Speed", 100.0,1000.0,&projectile_lookup[0].base_speed);
                imgui_slider_float("Min Speed", 50.0,200.0,&projectile_lookup[0].min_speed);
                imgui_toggle_button(&projectile_lookup[0].charge, "Charge");
                int charge_rate = projectile_lookup[0].charge_rate;
                imgui_number_box("Charge Rate", 1, 100, &charge_rate);
                projectile_lookup[0].charge_rate = charge_rate;
            } break;
            case 5: // theme
            {
                imgui_theme_editor();
            }
        }
    imgui_end();
}
