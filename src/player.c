#include "headers.h"
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "level.h"
#include "projectile.h"
#include "player.h"
#include "camera.h"

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

void player_init()
{
    if(player_image == -1)
    {
        if(role != ROLE_SERVER)
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

        p->radius = 8.0;
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

void player_set_hit_box_pos(Player* p, float x, float y)
{
    // memcpy(p->hitbox_prior, p->hitbox, sizeof(p->hitbox));

    float dx = p->pos.x - p->hitbox.x;
    float dy = p->pos.y - p->hitbox.y;

    p->hitbox.x = x;
    p->hitbox.y = y;

    p->pos.x = x + dx;
    p->pos.y = y + dy;
}



void player_reset(Player* p)
{
    return;
}

static void handle_room_collision(Player* p)
{
    Room* room = &level.rooms[p->curr_room.x][p->curr_room.y];

    for(int i = 0; i < room->wall_count; ++i)
    {
        Wall* wall = &room->walls[i];

        bool collision = false;
        bool check = false;
        Vector2f check_point;

        float px = p->pos.x;
        float py = p->pos.y + 8;

        switch(wall->dir)
        {
            case DIR_UP: case DIR_DOWN:
                if(px+p->radius >= wall->p0.x && px-p->radius <= wall->p1.x)
                {
                    check_point.x = px;
                    check_point.y = wall->p0.y;
                    check = true;
                }
                break;
            case DIR_LEFT: case DIR_RIGHT:
                if(py+p->radius >= wall->p0.y && py-p->radius <= wall->p1.y)
                {
                    check_point.x = wall->p0.x;
                    check_point.y = py;
                    check = true;
                }
                break;
        }

        if(check)
        {
            float d = dist(px, py, check_point.x, check_point.y);
            bool collision = (d < p->radius);

            if(collision)
            {
                //printf("Collision! player: %f %f. Wall point: %f %f. Dist: %f\n", px, py, check_point.x, check_point.y, d);

                float delta = p->radius - d + 1.0;
                switch(wall->dir)
                {
                    case DIR_UP:    p->pos.y -= delta; break;
                    case DIR_DOWN:  p->pos.y += delta; break;
                    case DIR_LEFT:  p->pos.x -= delta; break;
                    case DIR_RIGHT: p->pos.x += delta; break;
                }
            }
        }
    }

    for(int i = 0; i < 4; ++i)
    {
        bool is_door = room->doors[i];

        if(!is_door)
            continue;

        Vector2f door_point;

        int x0 = room_area.x - room_area.w/2.0;
        int y0 = room_area.y - room_area.h/2.0;

        switch(i)
        {
            case DIR_UP:
                door_point.x = x0 + TILE_SIZE*((ROOM_TILE_SIZE_X+2)/2.0);
                door_point.y = y0 + TILE_SIZE*(1);
                break;
            case DIR_RIGHT:
                door_point.x = x0 + TILE_SIZE*(ROOM_TILE_SIZE_X+1);
                door_point.y = y0 + TILE_SIZE*((ROOM_TILE_SIZE_Y+2)/2.0);
                break;
            case DIR_DOWN:
                door_point.x = x0 + TILE_SIZE*((ROOM_TILE_SIZE_X+2)/2.0);
                door_point.y = y0 + TILE_SIZE*(ROOM_TILE_SIZE_Y+1);
                break;
            case DIR_LEFT:
                door_point.x = x0 + TILE_SIZE*(1);
                door_point.y = y0 + TILE_SIZE*((ROOM_TILE_SIZE_Y+2)/2.0);
                break;
        }

        float d = dist(p->pos.x, p->pos.y+p->radius, door_point.x, door_point.y);

        bool colliding_with_door = (d < 10.0);
        bool go_through_door = p->actions[PLAYER_ACTION_DOOR].toggled_off;

        if(colliding_with_door && go_through_door)
        {
            // through door
            RectXY rxy = {0};
            rect_to_rectxy(&room_area, &rxy);

            switch(i)
            {
                case DIR_UP:
                {
                    transition_room = true;
                    transition_offsets.x = 0;
                    transition_offsets.y = 0;

                    Rect cr = get_camera_rect();
                    float zscale = 1.0 - camera_get_zoom();
                    float h = (cr.h - (margin_top.h + margin_top.h)*zscale);

                    transition_targets.x = 0;
                    transition_targets.y = h;

                    transition_next_room.x = p->curr_room.x;
                    transition_next_room.y = p->curr_room.y-1;

                    float y1 = rxy.y[BL] - (p->hitbox.y - rxy.y[TL]);
                    transition_player_target.x = p->hitbox.x;
                    transition_player_target.y = y1;
                    player_set_hit_box_pos(player, transition_player_target.x, transition_player_target.y);
                    p->sprite_index = SPRITE_UP;

                }   break;
                case DIR_RIGHT:
                {
                    transition_room = true;
                    transition_offsets.x = 0;
                    transition_offsets.y = 0;

                    Rect cr = get_camera_rect();
                    float zscale = 1.0 - camera_get_zoom();
                    float w = -(cr.w - (margin_left.w + margin_right.w)*zscale);

                    transition_targets.x = w;
                    transition_targets.y = 0;

                    transition_next_room.x = p->curr_room.x + 1;
                    transition_next_room.y = p->curr_room.y;

                    float x1 = rxy.x[BL] - (p->hitbox.x - rxy.x[TR]);
                    transition_player_target.x = x1;
                    transition_player_target.y = p->hitbox.y;
                    player_set_hit_box_pos(player, transition_player_target.x, transition_player_target.y);
                    p->sprite_index = SPRITE_RIGHT;
                }   break;
                case DIR_DOWN:
                {
                    transition_room = true;
                    transition_offsets.x = 0;
                    transition_offsets.y = 0;

                    Rect cr = get_camera_rect();
                    float zscale = 1.0 - camera_get_zoom();
                    float h = -(cr.h - (margin_top.h + margin_top.h)*zscale);

                    transition_targets.x = 0;
                    transition_targets.y = h;

                    transition_next_room.x = p->curr_room.x;
                    transition_next_room.y = p->curr_room.y+1;

                    float y1 = rxy.y[BL] - (p->hitbox.y - rxy.y[TL]);
                    transition_player_target.x = p->hitbox.x;
                    transition_player_target.y = y1;
                    player_set_hit_box_pos(player, transition_player_target.x, transition_player_target.y);
                    p->sprite_index = SPRITE_DOWN;
                }   break;
                case DIR_LEFT:
                {
                    transition_room = true;
                    transition_offsets.x = 0;
                    transition_offsets.y = 0;

                    Rect cr = get_camera_rect();
                    float zscale = 1.0 - camera_get_zoom();
                    float w = (cr.w - (margin_left.w + margin_right.w)*zscale);

                    transition_targets.x = w;
                    transition_targets.y = 0;

                    transition_next_room.x = p->curr_room.x - 1;
                    transition_next_room.y = p->curr_room.y;

                    float x1 = rxy.x[BL] - (p->hitbox.x - rxy.x[TR]);
                    transition_player_target.x = x1;
                    transition_player_target.y = p->hitbox.y;
                    player_set_hit_box_pos(player, transition_player_target.x, transition_player_target.y);
                    p->sprite_index = SPRITE_LEFT;
                }   break;
            }
            //printf("colliding with door, dist: %f\n", d);
        }
    }

}

void player_update(Player* p, float dt)
{
    if(!p->active) return;
    if(transition_room) return;

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
    level.rooms[p->curr_room.x][p->curr_room.y].discovered = true;


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

    // @TEMP
    gfx_draw_circle(p->pos.x, p->pos.y+8, p->radius, COLOR_PURPLE, 1.0, false, IN_WORLD);

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
