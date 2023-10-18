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
        char* buttons[] = {"General", "Level", "Players", "Creatures", "Projectiles"};
        int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        float big = 20.0;

        switch(selection)
        {


            case 0: // general
            {
                imgui_slider_float("Zoom", 0.01,.99, &cam_zoom);

                imgui_text("Mouse (view): %d, %d", 1, 1);
                imgui_text("Mouse (world): %d, %d", 1, 1);

                Rect cr = get_camera_rect();
                imgui_text("Camera: %.1f, %.1f, %.1f, %.1f", cr.x, cr.y, cr.w, cr.h);

                Rect limit = camera_limit;
                imgui_text("Camera Limit: %.1f, %.1f, %.1f, %.1f", limit.x, limit.y, limit.w, limit.h);

                imgui_text("View w/h: %d, %d", view_width, view_height);
                imgui_text("Window w/h: %d, %d", window_width, window_height);

            } break;

            case 1: // level
            {

            } break;

            case 2: // players
            {

                player_selection = imgui_dropdown(player_names, name_count, "Select Player", &player_selection);

                Player* p = &players[player_selection];

                imgui_text_sized(big, "Info");
                imgui_text("Pos: %.2f, %.2f", p->phys.pos.x, p->phys.pos.y);
                imgui_text("Vel: %.2f, %.2f", p->phys.vel.x, p->phys.vel.y);

                Vector2i c;
                c = level_get_room_coords(p->curr_room);
                imgui_text("C Room: %u (%d, %d)", p->curr_room, c.x, c.y);
                c = level_get_room_coords(p->transition_room);
                imgui_text("T Room: %u (%d, %d)", p->transition_room, c.x, c.y);

                if(imgui_button("Send To Start"))
                {
                    player_set_to_level_start(p);
                }

            } break;

            case 3: // creatures
            {
                if(imgui_button("Add Slug"))
                {
                    Room* room = level_get_room_by_index(&level, player->curr_room);
                    creature_add(room, CREATURE_TYPE_SLUG);
                }
                imgui_slider_float("Speed",1.0,100.0,&creatures[0].phys.speed);
                for(int i = 0; i < creature_get_count(); ++i)
                    creatures[i].phys.speed = creatures[0].phys.speed;
            } break;


            case 4: // projectiles
            {
                imgui_text_sized(big, "Gun");
                imgui_slider_float("Damage", 1.0,100.0,&projectile_lookup[0].damage);
                imgui_slider_float("Cooldown", 0.0,1.0,&player->proj_cooldown_max);
                imgui_slider_float("Base Speed", 100.0,1000.0,&projectile_lookup[0].base_speed);
                imgui_slider_float("Min Speed", 50.0,200.0,&projectile_lookup[0].min_speed);
            } break;
        }
    imgui_end();
}
