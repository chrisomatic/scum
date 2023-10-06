#include "headers.h"
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "level.h"
#include "player.h"

Player players[MAX_PLAYERS] = {0};
Player* player = NULL;

void player_init()
{
    player = &players[0];

    window_controls_clear_keys();
    window_controls_add_key(&player->actions[PLAYER_ACTION_UP].state, GLFW_KEY_W);
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&player->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_D);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RUN].state, GLFW_KEY_LEFT_SHIFT);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SCUM].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_GENERATE_ROOMS].state, GLFW_KEY_R);

    player->pos.x = CENTER_X;
    player->pos.y = CENTER_Y;
}

void player_update()
{
    
    Player* p = player;
    
    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerAction* pa = &p->actions[i];
        if(pa->state && !pa->prior_state)
        {
            pa->toggled_on = true;
        }
        else
        {
            pa->toggled_on = false;
        }
        if(!pa->state && pa->prior_state)
        {
            pa->toggled_off = true;
        }
        else
        {
            pa->toggled_off = false;
        }
        pa->prior_state = pa->state;
    }

    bool up    = p->actions[PLAYER_ACTION_UP].state;
    bool down  = p->actions[PLAYER_ACTION_DOWN].state;
    bool left  = p->actions[PLAYER_ACTION_LEFT].state;
    bool right = p->actions[PLAYER_ACTION_RIGHT].state;
    bool run = p->actions[PLAYER_ACTION_RUN].state;
    float v = 2.0;
    if(run) v = 8.0;
    if(up) p->pos.y -= v;
    if(down) p->pos.y += v;
    if(left) p->pos.x -= v;
    if(right) p->pos.x += v;

    bool scum = p->actions[PLAYER_ACTION_SCUM].toggled_on;
    if(scum)
    {
        const char* text[] = {
            "Scum is fun",
            "Never stop playing Scum",
            "Scummer 4 life",
            "Scumeron waz here",
            "Sup scummy",
            "Dedicate your life to Scum",
            "Scum over family",
            "No Scum for Aaron",
            "Endulge yourself with Scum",
            "I want more Scum",
            "Time for more Scum",
            "SCUM",
            "I dedicate my life to Scum",
            "Scum is all I care about",
            "WANT MORE SCUM"
        };

        text_list_add(text_lst, 3.0, (char*)text[rand()%sizeof(text)/sizeof(text[0])]);
    }

    bool generate = p->actions[PLAYER_ACTION_GENERATE_ROOMS].toggled_on;
    if(generate)
    {
        seed = time(0)+rand()%1000;
        level = level_generate(seed);
        level_print(&level);
    }
}

void player_draw()
{
    Rect p = RECT(player->pos.x, player->pos.y, 3, 3);
    gfx_draw_rect(&p, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);
}
