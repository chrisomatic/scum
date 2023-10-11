#include "headers.h"
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "level.h"
#include "projectile.h"
#include "player.h"

int player_image = -1;
Player players[MAX_PLAYERS] = {0};
Player* player = NULL;
int player_image;
int num_players;

static void update_player_boxes(Player* p)
{
    GFXImage* img = &gfx_images[player_image];

    Rect* vr = &img->visible_rects[p->sprite_index];

    int w = img->element_width;
    int h = img->element_height;

    p->hitbox.x = p->pos.x;
    p->hitbox.y = p->pos.y;
    p->hitbox.w = w;
    p->hitbox.h = h;

    p->hitbox.y += 0.3*p->hitbox.h;
    p->hitbox.h *= 0.4;
    p->hitbox.w *= 0.6;
}

static void set_hit_box_pos(Player* p, float x, float y)
{
    // memcpy(p->hitbox_prior, p->hitbox, sizeof(p->hitbox));

    float dx = p->pos.x - p->hitbox.x;
    float dy = p->pos.y - p->hitbox.y;

    p->hitbox.x = x;
    p->hitbox.y = y;

    p->pos.x = x + dx;
    p->pos.y = y + dy;
}


void player_init()
{
    if(player_image == -1)
    {
        player_image = gfx_load_image("src/img/spaceman.png", false, true, 32, 32);
    }

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(p == player)
        {
            player_init_keys();
            p->active = true;
        }

        p->active = false;

        p->pos.x = CENTER_X;
        p->pos.y = CENTER_Y;

        p->vel.x = 0.0;
        p->vel.y = 0.0;

        p->sprite_index = 4;

        p->curr_room.x = (MAX_ROOMS_GRID_X-1)/2;
        p->curr_room.y = (MAX_ROOMS_GRID_Y-1)/2;

        // animation
        // --------------------------------------------------------
        p->anim.curr_frame = 0;
        p->anim.max_frames = 4;
        p->anim.curr_frame_time = 0.0f;
        p->anim.max_frame_time = 0.10f;
        p->anim.finite = false;
        p->anim.curr_loop = 0;
        p->anim.max_loops = 0;
        p->anim.frame_sequence[0] = 0;
        p->anim.frame_sequence[1] = 1;
        p->anim.frame_sequence[2] = 2;
        p->anim.frame_sequence[3] = 3;
    }

}

void player_init_keys()
{
    if(player == NULL) return;

    window_controls_clear_keys();
    window_controls_add_key(&player->actions[PLAYER_ACTION_UP].state, GLFW_KEY_W);
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&player->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_D);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RUN].state, GLFW_KEY_LEFT_SHIFT);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOW_MAP].state, GLFW_KEY_M);
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOOR].state, GLFW_KEY_ENTER);

    // window_controls_add_key(&player->actions[PLAYER_ACTION_SCUM].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_GENERATE_ROOMS].state, GLFW_KEY_R);
}

void player_reset(Player* p)
{
    return;
}

static void handle_room_collision(Player* p)
{
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

    bool go_through_door = p->actions[PLAYER_ACTION_DOOR].toggled_off;
    //text_list_add(text_lst, 3.0, "num_checks: %d" ,num_checks);

    for(int i = 0; i < sizeof(checks)/sizeof(checks[0]); ++i)
    {
        Vector2i c = checks[i];

        Rect rc = {x+w/2.0 + w*c.x, y + h/2.0 + h*c.y, w, h};
        bool collision = rectangles_colliding(&p->hitbox, &rc);

        if(collision)
        {
            if(i == 0)
            {
                if(go_through_door)
                {
                    RectXY rxy = {0};
                    rect_to_rectxy(&room_area, &rxy);

                    if(room->doors[DOOR_LEFT] && c.x == 0 && c.y == 4)
                    {
                        p->curr_room.x--;
                        float x1 = rxy.x[BL] - (p->hitbox.x - rxy.x[TR]);
                        set_hit_box_pos(p, x1, p->hitbox.y);
                    }
                    else if(room->doors[DOOR_RIGHT] && c.x == 14 && c.y == 4)
                    {
                        p->curr_room.x++;
                        float x1 = rxy.x[BL] - (p->hitbox.x - rxy.x[TR]);
                        set_hit_box_pos(p, x1, p->hitbox.y);
                    }
                    else if(room->doors[DOOR_UP] && c.x == 7 && c.y == 0)
                    {
                        p->curr_room.y--;
                        float y1 = rxy.y[BL] - (p->hitbox.y - rxy.y[TL]);
                        set_hit_box_pos(p, p->hitbox.x, y1);
                    }
                    else if(room->doors[DOOR_DOWN] && c.x == 7 && c.y == 8)
                    {
                        p->curr_room.y++;
                        float y1 = rxy.y[BL] - (p->hitbox.y - rxy.y[TL]);
                        set_hit_box_pos(p, p->hitbox.x, y1);
                    }
                }


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
                    case 2: p->pos.x -= p->vel.x > p->vel.y ? x_diff : 0.0; p->pos.y += p->vel.x > p->vel.y ? 0.0 : y_diff; break;
                    case 3: p->pos.x -= x_diff; p->pos.y += 0.0;    break;
                    case 4: p->pos.x -= p->vel.x > p->vel.y ? x_diff : 0.0; p->pos.y -= p->vel.x > p->vel.y ? 0.0 : y_diff; break;
                    case 5: p->pos.x += 0.0;    p->pos.y -= y_diff; break;
                    case 6: p->pos.x += p->vel.x > p->vel.y ? x_diff : 0.0; p->pos.y -= p->vel.x > p->vel.y ? 0.0 : y_diff; break;
                    case 7: p->pos.x += x_diff; p->pos.y += 0.0;    break;
                    case 8: p->pos.x += p->vel.x > p->vel.y ? x_diff : 0.0; p->pos.y += p->vel.x > p->vel.y ? 0.0 : y_diff; break;
                    default: break;
                }

            }
        }
        gfx_draw_rect_xywh(x+w/2.0 + w*c.x,y + h/2.0 + h*c.y,w,h, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }

}

void player_update(Player* p, float dt)
{
    if(!p->active) return;
    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerInput* pa = &p->actions[i];
        update_input_state(pa, dt);
    }

    bool map = p->actions[PLAYER_ACTION_SHOW_MAP].toggled_on;
    if(map)
    {
        show_big_map = !show_big_map;
    }

    bool up    = p->actions[PLAYER_ACTION_UP].state;
    bool down  = p->actions[PLAYER_ACTION_DOWN].state;
    bool left  = p->actions[PLAYER_ACTION_LEFT].state;
    bool right = p->actions[PLAYER_ACTION_RIGHT].state;
    bool run = p->actions[PLAYER_ACTION_RUN].state;

    float v = run ? 300.0 : 128.0;

    p->vel.x = 0.0;
    p->vel.y = 0.0;

    if(up)
    {
        p->vel.y = -v;
        p->sprite_index = 0;
    }

    if(down)
    {
        p->vel.y = +v;
        p->sprite_index = 4;
    }

    if(left)
    {
        p->vel.x = -v;
        p->sprite_index = 8;
    }

    if(right)
    {
        p->vel.x = +v;
        p->sprite_index = 12;
    }

    p->pos.x += p->vel.x*dt;
    p->pos.y += p->vel.y*dt;

    update_player_boxes(p);

    Vector2f adj = limit_rect_pos(&player_area, &p->hitbox);
    if(!FEQ0(adj.x) || !FEQ0(adj.y))
    {
        p->pos.x += adj.x;
        p->pos.y += adj.y;
        update_player_boxes(p);
    }

    // update player current tile
    GFXImage* img = &gfx_images[player_image];
    Rect* vr = &img->visible_rects[p->sprite_index];

    int dx = (int)(p->hitbox.x-room_area.x+room_area.w/2.0) / 32.0;
    int dy = (int)(p->hitbox.y-room_area.y+room_area.h/2.0) / 32.0;

    p->curr_tile.x = dx;
    p->curr_tile.y = dy;

    const float pcooldown = 0.1; //seconds

    if(p->actions[PLAYER_ACTION_SHOOT].state)
    {
        p->proj_cooldown -= dt;
        if(p->proj_cooldown <= 0.0)
        {
            if(p->sprite_index >= 0 && p->sprite_index <= 3)
                projectile_add(p, 90.0);
            else if(p->sprite_index >= 4 && p->sprite_index <= 7)
                projectile_add(p, 270.0);
            else if(p->sprite_index >= 8 && p->sprite_index <= 11)
                projectile_add(p, 180.0);
            else if(p->sprite_index >= 12)
                projectile_add(p, 0.0);
            p->proj_cooldown = pcooldown;
        }
    }
    bool generate = p->actions[PLAYER_ACTION_GENERATE_ROOMS].toggled_on;
    if(generate)
    {
        seed = time(0)+rand()%1000;
        level = level_generate(seed);
        level_print(&level);
    }
    
    // check tiles around player
    handle_room_collision(p);

    // update animation
    if(ABS(p->vel.x) > 0.0 || ABS(p->vel.y) > 0.0)
        gfx_anim_update(&p->anim, dt);
    else
        p->anim.curr_frame = 0;
}

void player_draw(Player* p)
{
    if(!p->active) return;
    gfx_draw_image(player_image, p->sprite_index+p->anim.curr_frame, p->pos.x, p->pos.y, COLOR_TINT_NONE, 1.0, 0.0, 1.0, false, true);

    if(debug_enabled)
    {
        Rect r = RECT(p->pos.x, p->pos.y, 1, 1);
        gfx_draw_rect(&r, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);
        gfx_draw_rect(&p->hitbox, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, true);
    }
}

void player_lerp(Player* p, float dt)
{
    if(!p->active) return;

    p->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    // printf("[lerp prior]  %.2f, %.2f\n", p->server_state_prior.pos.x, p->server_state_prior.pos.y);
    // printf("[lerp target] %.2f, %.2f\n", p->server_state_target.pos.x, p->server_state_target.pos.y);

    Vector2f lp = lerp2f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->pos.x = lp.x;
    p->pos.y = lp.y;
}

void player_handle_net_inputs(Player* p, double dt)
{
    // handle input
    memcpy(&p->input_prior, &p->input, sizeof(NetPlayerInput));

    p->input.delta_t = dt;

    p->input.keys = 0;

    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        if(p->actions[i].state)
        {
            p->input.keys |= (1<<i);
        }
    }

    if(p->input.keys != p->input_prior.keys)
    {
        net_client_add_player_input(&p->input);
    }
}
