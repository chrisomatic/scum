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

#define ASCII_NUMS {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"}

void editor_init()
{

}

void editor_draw()
{
    imgui_begin_panel("Editor", view_width - 300, 1, true);

        imgui_newline();
        char* buttons[] = {"Players", "Creatures", "Level"};
        int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        float big = 20.0;

        Room* room = level_get_room_by_index(&level,player->curr_room);

        switch(selection)
        {
            case 0: // players
            {
                imgui_text_sized(big, "Info");
                imgui_text("Pos: %f %f",player->phys.pos.x, player->phys.pos.y);
                imgui_text("Vel: %f %f (%f)",player->phys.vel.x, player->phys.vel.y, player->vel_factor);


                Vector2i tile_coords = level_get_room_coords_by_pos(wmx,wmy);
                TileType tt = level_get_tile_type(room,tile_coords.x,tile_coords.y);
                imgui_text("Mouse Coords: %d %d (Tile %d %d, Type: %s)",wmx,wmy,tile_coords.x,tile_coords.y,level_tile_type_to_str(tt));

                Vector2i ptile_coords = level_get_room_coords_by_pos(player->hitbox.x, player->hitbox.y);
                imgui_text("Tile: %d, %d",ptile_coords.x,ptile_coords.y);

                imgui_text_sized(big,"Physics");
                imgui_slider_float("Speed", 100.0,500.0,&player->phys.speed);

                imgui_text_sized(big, "Gun");
                imgui_slider_float("Damage", 1.0,100.0,&projectile_lookup[0].damage);
                imgui_slider_float("Cooldown", 0.0,1.0,&player->proj_cooldown_max);
                imgui_slider_float("Base Speed", 100.0,1000.0,&projectile_lookup[0].base_speed);
                imgui_slider_float("Min Speed", 50.0,200.0,&projectile_lookup[0].min_speed);
            } break;

            case 1: // creatures
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

            case 2: // level
            {
            } break;
        }
    imgui_end();
}
