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

    p->hitbox.x = p->phys.pos.x;
    p->hitbox.y = p->phys.pos.y;
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
            // p->active = true;
        }

        p->active = false;

        p->phys.pos.x = CENTER_X;
        p->phys.pos.y = CENTER_Y;

        p->phys.vel.x = 0.0;
        p->phys.vel.y = 0.0;

        p->sprite_index = 4;

        p->radius = 8.0;

        int room_y = (MAX_ROOMS_GRID_X-1)/2;
        int room_x = (MAX_ROOMS_GRID_Y-1)/2;
        p->curr_room = (uint8_t)level_get_room_index(room_x, room_y);
        p->transition_room = p->curr_room;

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
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOOR].state, GLFW_KEY_ENTER);

    for(int i = 0;  i < PLAYER_ACTION_MAX; ++i)
        memset(&player->actions[i], 0, sizeof(PlayerInput));

    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_GENERATE_ROOMS].state, GLFW_KEY_R);
}

void player_set_hit_box_pos(Player* p, float x, float y)
{
    // memcpy(p->hitbox_prior, p->hitbox, sizeof(p->hitbox));

    float dx = p->phys.pos.x - p->hitbox.x;
    float dy = p->phys.pos.y - p->hitbox.y;

    p->hitbox.x = x;
    p->hitbox.y = y;

    p->phys.pos.x = x + dx;
    p->phys.pos.y = y + dy;
}


void player_reset(Player* p)
{
    return;
}

// also does the drawing
void player_draw_room_transition()
{
    Player* p = player;
    if(!p->active) return;

    if(p->curr_room != p->transition_room)
    {
        float dx = transition_targets.x/20.0;
        float dy = transition_targets.y/20.0;

        transition_offsets.x += dx;
        transition_offsets.y += dy;

        if(ABS(transition_offsets.x) >= ABS(transition_targets.x) && ABS(transition_offsets.y) >= ABS(transition_targets.y))
        {
            // printf("room transition complete\n");
            p->transition_room = p->curr_room;
            camera_move(p->phys.pos.x, p->phys.pos.y, true, &camera_limit);
            camera_update(VIEW_WIDTH, VIEW_HEIGHT);
        }
        else
        {
            // float pdx = transition_player_target.x/60.0;
            // float pdy = transition_player_target.y/60.0;
            // player_set_hit_box_pos(player, pdx, pdy);

            float x0 = transition_offsets.x;
            float y0 = transition_offsets.y;

            float xoff = 0.0;
            if(!FEQ0(transition_targets.x))
            {
                if(transition_targets.x > 0)
                    xoff = -room_area.w;
                else
                    xoff = room_area.w;
            }
            float yoff = 0.0;
            if(!FEQ0(transition_targets.y))
            {
                if(transition_targets.y > 0)
                    yoff = -room_area.h;
                else
                    yoff = room_area.h;
            }

            float x1 = transition_offsets.x+xoff;
            float y1 = transition_offsets.y+yoff;

            Vector2i t = level_get_room_coords(p->transition_room);
            level_draw_room(&level.rooms[t.x][t.y], x0, y0);

            Vector2i roomxy = level_get_room_coords((int)p->curr_room);
            level_draw_room(&level.rooms[roomxy.x][roomxy.y], x1, y1);
        }
    }
}


void player_start_room_transition(Player* p)
{
    // printf("start room transition\n");

    Vector2i roomxy = level_get_room_coords((int)p->curr_room);

    // new player positions
    RectXY rxy = {0};
    rect_to_rectxy(&room_area, &rxy);
    float x1 = rxy.x[BL] - (p->hitbox.x - rxy.x[TR]);
    float y1 = rxy.y[BL] - (p->hitbox.y - rxy.y[TL]);

    if(role == ROLE_SERVER || role == ROLE_LOCAL)
    {
        p->transition_room = p->curr_room;

        switch(p->door)
        {
            case DIR_UP:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x, roomxy.y-1);
                player_set_hit_box_pos(p, p->hitbox.x, y1);
                p->sprite_index = SPRITE_UP;
                break;

            case DIR_RIGHT:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x+1, roomxy.y);
                player_set_hit_box_pos(p, x1, p->hitbox.y);
                p->sprite_index = SPRITE_RIGHT;
                break;

            case DIR_DOWN:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x, roomxy.y+1);
                player_set_hit_box_pos(p, p->hitbox.x, y1);
                p->sprite_index = SPRITE_DOWN;
                break;

            case DIR_LEFT:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x-1, roomxy.y);
                player_set_hit_box_pos(p, x1, p->hitbox.y);
                p->sprite_index = SPRITE_LEFT;
                break;

            default:
                break;
        }

        // printf("start room transition: %d -> %d\n", p->transition_room, p->curr_room);
    }

    if(role == ROLE_SERVER)
        return;

    if(p != player)
        return;

    transition_offsets.x = 0;
    transition_offsets.y = 0;
    transition_targets.x = 0;
    transition_targets.y = 0;

    Rect cr = get_camera_rect();
    float zscale = 1.0 - camera_get_zoom();
    float vw = (cr.w - (margin_left.w + margin_right.w)*zscale);
    float vh = (cr.h - (margin_top.h + margin_top.h)*zscale);

    switch(p->door)
    {
        case DIR_UP:
        {
            transition_targets.y = vh;
        } break;

        case DIR_RIGHT:
        {
            transition_targets.x = -vw;
        } break;

        case DIR_DOWN:
        {
            transition_targets.y = -vh;
        } break;

        case DIR_LEFT:
        {
            transition_targets.x = vw;
        } break;

        default:
            break;
    }

    // printf("transition_targets: %.2f, %.2f  door: %d     (%.2f, %.2f)\n", transition_targets.x, transition_targets.y, p->door, vw, vh);
}

static void handle_room_collision(Player* p)
{
    Vector2i roomxy = level_get_room_coords((int)p->curr_room);

    Room* room = &level.rooms[roomxy.x][roomxy.y];

    level_sort_walls(room->walls,room->wall_count,p->phys.pos.x, p->phys.pos.y+p->radius,p->radius);

    for(int i = 0; i < room->wall_count; ++i)
    {
        Wall* wall = &room->walls[i];

        bool collision = false;
        bool check = false;
        Vector2f check_point;

        float px = p->phys.pos.x;
        float py = p->phys.pos.y + 8;

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
                    case DIR_UP:    p->phys.pos.y -= delta; break;
                    case DIR_DOWN:  p->phys.pos.y += delta; break;
                    case DIR_LEFT:  p->phys.pos.x -= delta; break;
                    case DIR_RIGHT: p->phys.pos.x += delta; break;
                }
            }
        }
    }

    for(int i = 0; i < 4; ++i)
    {
        if(role == ROLE_LOCAL)
        {
            if(player->curr_room != player->transition_room) break;
        }

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

        float d = dist(p->phys.pos.x, p->phys.pos.y+p->radius, door_point.x, door_point.y);

        bool colliding_with_door = (d < 10.0);
        bool go_through_door = p->actions[PLAYER_ACTION_DOOR].toggled_off;

        if(colliding_with_door)
        {
            if(go_through_door)
            {
                p->door = i;
                player_start_room_transition(p);
            }
            break;
        }

    }

}

void player_update(Player* p, float dt)
{
    if(!p->active) return;

    if(role == ROLE_LOCAL)
    {
        if(player->curr_room != player->transition_room) return;
    }

    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerInput* pa = &p->actions[i];
        update_input_state(pa, dt);
    }

    bool up    = p->actions[PLAYER_ACTION_UP].state;
    bool down  = p->actions[PLAYER_ACTION_DOWN].state;
    bool left  = p->actions[PLAYER_ACTION_LEFT].state;
    bool right = p->actions[PLAYER_ACTION_RIGHT].state;
    bool run = p->actions[PLAYER_ACTION_RUN].state;

    float v = run ? 300.0 : 128.0;

    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;

    if(up)
    {
        p->phys.vel.y = -v;
        p->sprite_index = 0;
    }

    if(down)
    {
        p->phys.vel.y = +v;
        p->sprite_index = 4;
    }

    if(left)
    {
        p->phys.vel.x = -v;
        p->sprite_index = 8;
    }

    if(right)
    {
        p->phys.vel.x = +v;
        p->sprite_index = 12;
    }

    p->phys.pos.x += p->phys.vel.x*dt;
    p->phys.pos.y += p->phys.vel.y*dt;

    update_player_boxes(p);

    Vector2f adj = limit_rect_pos(&player_area, &p->hitbox);
    if(!FEQ0(adj.x) || !FEQ0(adj.y))
    {
        p->phys.pos.x += adj.x;
        p->phys.pos.y += adj.y;
        update_player_boxes(p);
    }

    // update player current tile
    GFXImage* img = &gfx_images[player_image];
    Rect* vr = &img->visible_rects[p->sprite_index];

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

    if(role == ROLE_LOCAL)
    {
        bool generate = p->actions[PLAYER_ACTION_GENERATE_ROOMS].toggled_on;
        if(generate)
        {
            seed = time(0)+rand()%1000;
            level = level_generate(seed);
            level_print(&level);
        }
    }

    // check tiles around player
    handle_room_collision(p);

    Vector2i roomxy = level_get_room_coords((int)p->curr_room);
    level.rooms[roomxy.x][roomxy.y].discovered = true;

    // update animation
    if(ABS(p->phys.vel.x) > 0.0 || ABS(p->phys.vel.y) > 0.0)
        gfx_anim_update(&p->anim, dt);
    else
        p->anim.curr_frame = 0;
}

void player_draw(Player* p)
{
    if(!p->active) return;

    Vector2i roomxy = level_get_room_coords((int)p->curr_room);
    Room* room = &level.rooms[roomxy.x][roomxy.y];
    gfx_draw_image(player_image, p->sprite_index+p->anim.curr_frame, p->phys.pos.x, p->phys.pos.y, room->color, 1.0, 0.0, 1.0, false, true);


    if(debug_enabled)
    {
        // @TEMP
        gfx_draw_circle(p->phys.pos.x, p->phys.pos.y+8, p->radius, COLOR_PURPLE, 1.0, false, IN_WORLD);

        Rect r = RECT(p->phys.pos.x, p->phys.pos.y, 1, 1);
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
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    Vector2i roomxy = level_get_room_coords((int)p->curr_room);
    level.rooms[roomxy.x][roomxy.y].discovered = true;
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

    if(p->curr_room != p->transition_room)
    {
        p->input.keys = 0;
    }

    if(p->input.keys != p->input_prior.keys)
    {
        net_client_add_player_input(&p->input);
    }
}
