#include "headers.h"
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "level.h"
#include "player.h"

int player_image = -1;
Player players[MAX_PLAYERS] = {0};
Player* player = NULL;
int player_image;

static void update_player_boxes(Player* p)
{
    GFXImage* img = &gfx_images[player_image];

    Rect* vr = &img->visible_rects[p->sprite_index];

    p->hitbox.x = vr->x+p->pos.x-img->w/2.0;
    p->hitbox.y = vr->y+p->pos.y-img->h/2.0;
    p->hitbox.w = vr->w;
    p->hitbox.h = vr->h;

    p->hitbox.y += 0.3*p->hitbox.h;
    p->hitbox.h *= 0.4;
}

void player_init()
{

    player = &players[0];
    if(player_image == -1)
    {
        player_image = gfx_load_image("src/img/spaceman.png", false, true, 32, 32);
    }

    window_controls_clear_keys();
    window_controls_add_key(&player->actions[PLAYER_ACTION_UP].state, GLFW_KEY_W);
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&player->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_D);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RUN].state, GLFW_KEY_LEFT_SHIFT);
    // window_controls_add_key(&player->actions[PLAYER_ACTION_SCUM].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_TEST].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_GENERATE_ROOMS].state, GLFW_KEY_R);

    player->pos.x = CENTER_X;
    player->pos.y = CENTER_Y;

    player->sprite_index = 0;

    player->curr_room.x = (MAX_ROOMS_GRID_X-1)/2;
    player->curr_room.y = (MAX_ROOMS_GRID_Y-1)/2;
}

void player_update(Player* p)
{
    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerAction* pa = &p->actions[i];
        if(pa->state && !pa->prior_state)
        {
            pa->toggled_on = true;
        }
        else
        {
            pa->toggled_on = false; }
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

    // bool test = p->actions[PLAYER_ACTION_TEST].toggled_on;
    // if(test)
    // {
    //     if(p->sprite_index == 0)
    //         p->sprite_index = 1;
    //     else
    //         p->sprite_index = 0;
    // }


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

    // update player current tile
    GFXImage* img = &gfx_images[player_image];
    Rect* vr = &img->visible_rects[p->sprite_index];

    int dx = (int)(p->hitbox.x-room_area.x+room_area.w/2.0) / 32.0;
    int dy = (int)(p->hitbox.y-room_area.y+room_area.h/2.0) / 32.0;

    p->curr_tile.x = dx;
    p->curr_tile.y = dy;

    update_player_boxes(p);

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

    // check tiles around player


}

void player_draw(Player* p)
{
    gfx_draw_image(player_image, p->sprite_index, p->pos.x, p->pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);

    Rect r = RECT(player->pos.x, player->pos.y, 2, 2);
    gfx_draw_rect(&r, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);

    gfx_draw_rect(&p->hitbox, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, true);

    int x = room_area.x - room_area.w/2.0;
    int y = room_area.y - room_area.h/2.0;

    int w = 32;
    int h = 32;

    Vector2i checks[] = {
        {p->curr_tile.x, p->curr_tile.y},
        {p->curr_tile.x, p->curr_tile.y-1},
        {p->curr_tile.x+1, p->curr_tile.y-1},
        {p->curr_tile.x+1, p->curr_tile.y},
        {p->curr_tile.x+1, p->curr_tile.y+1},
        {p->curr_tile.x, p->curr_tile.y+1},
        {p->curr_tile.x-1, p->curr_tile.y+1},
        {p->curr_tile.x-1, p->curr_tile.y},
        {p->curr_tile.x-1, p->curr_tile.y-1}
    };

    Room* room = &level.rooms[p->curr_room.x][p->curr_room.y];
    RoomData* rdata = &room_list[room->layout];

    for(int i = 0; i < sizeof(checks)/sizeof(checks[0]); ++i)
    {
        Vector2i c = checks[i];

        Rect rc = {x+w/2.0 + w*c.x, y + h/2.0 + h*c.y, w, h};
        bool collision = rectangles_colliding(&p->hitbox, &rc);

        if(collision)
        {
            if(c.x == 0 && c.y == 4)
            {
                p->curr_room.x--;
            }

            TileType tt = rdata->tiles[checks[i].x-1][checks[i].y-1];

            if(tt == TILE_BOULDER)
            {
                // correct collision
                float x_diff = (rc.w + p->hitbox.w)/2.0 - ABS(rc.x - p->hitbox.x);
                float y_diff = (rc.h + p->hitbox.h)/2.0 - ABS(rc.y - p->hitbox.y);

                switch(i)
                {
                    case 0: p->pos.x += 0.0;    p->pos.y += 0.0;    break;
                    case 1: p->pos.x += 0.0;    p->pos.y += y_diff; break;
                    case 2: p->pos.x -= x_diff; p->pos.y += y_diff; break;
                    case 3: p->pos.x -= x_diff; p->pos.y += 0.0;    break;
                    case 4: p->pos.x -= x_diff; p->pos.y -= y_diff; break;
                    case 5: p->pos.x += 0.0;    p->pos.y -= y_diff; break;
                    case 6: p->pos.x += x_diff; p->pos.y -= y_diff; break;
                    case 7: p->pos.x += x_diff; p->pos.y += 0.0;    break;
                    case 8: p->pos.x += x_diff; p->pos.y += y_diff; break;
                    default: break;
                }

            }
        }
        gfx_draw_rect_xywh(x+w/2.0 + w*c.x,y + h/2.0 + h*c.y,w,h, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
}
