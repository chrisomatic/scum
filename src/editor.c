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
        char* buttons[] = {"Players", "Creatures", "Projectiles", "Theme"};
        int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");
        imgui_horizontal_line(1);

        float big = 20.0;

        switch(selection)
        {
            case 0: // players
            {
                imgui_text_sized(big, "Physics");
                imgui_text("Pos: %f %f",player->phys.pos.x, player->phys.pos.y);
                imgui_text("Vel: %f %f",player->phys.vel.x, player->phys.vel.y);
            } break;

            case 1: // creatures
            {
                imgui_slider_float("Speed",1.0,50.0,&creatures[0].speed);
            } break;

            case 2: // projectiles
            {
                imgui_slider_float("Damage", 1.0,100.0,&projectile_lookup[0].damage);
                imgui_slider_float("Base Speed", 100.0,1000.0,&projectile_lookup[0].base_speed);
                imgui_slider_float("Min Speed", 50.0,200.0,&projectile_lookup[0].min_speed);
            } break;

            case 3:
            {
                imgui_theme_selector();

            } break;
        }
    imgui_end();
}
