#include "headers.h"
#include "main.h"
#include "window.h"
#include "gfx.h"
#include "level.h"
#include "projectile.h"
#include "creature.h"
#include "camera.h"
#include "lighting.h"
#include "status_effects.h"
#include "player.h"

#define RADIUS_OFFSET_X 0
#define RADIUS_OFFSET_Y 7.5

#define ALWAYS_SHOW_GAUNTLET 1

void player_ai_move_to_target(Player* p, Player* target);

static float sprite_index_to_angle(Player* p);
static void update_player_boxes(Player* p);
static void handle_room_collision(Player* p);

char* player_names[MAX_PLAYERS+1]; // used for name dropdown. +1 for ALL option.
int player_image = -1;
int shadow_image = -1;

Player players[MAX_PLAYERS] = {0};
Player* player = NULL;
Player* player2 = NULL;

void player_init()
{
    if(player_image == -1)
    {
        player_image = gfx_load_image("src/img/spaceman.png", false, true, 32, 32);
        shadow_image = gfx_load_image("src/img/shadow.png", false, true, 32, 32);
    }
for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];

        p->active = false;

        p->phys.dead = false;
        p->phys.pos.x = CENTER_X;
        p->phys.pos.y = CENTER_Y;

        p->phys.speed = 1000.0;
        p->phys.speed_factor = 1.0;
        p->phys.max_velocity = 180.0;
        p->phys.base_friction = 15.0;
        p->phys.vel.x = 0.0;
        p->phys.vel.y = 0.0;
        p->phys.height = gfx_images[player_image].element_height;
        p->phys.mass = 1.0;
        p->phys.elasticity = 0.0;

        p->phys.hp_max = 24;
        p->phys.hp = p->phys.hp_max;

        p->sprite_index = 4;

        p->phys.radius = 8.0;
        p->phys.coffset.x = RADIUS_OFFSET_X;
        p->phys.coffset.y = RADIUS_OFFSET_Y;

        p->proj_cooldown_max = 0.2;
        p->door = DIR_NONE;

#if BOI_SHOOTING
        p->last_shoot_action = PLAYER_ACTION_SHOOT_UP;
#else
        p->last_shoot_action = PLAYER_ACTION_SHOOT;
#endif
        p->shoot_sprite_cooldown = 0.0;

        memset(p->name, PLAYER_NAME_MAX, 0);
        sprintf(p->name, "Player %d", i);

        // animation
        // --------------------------------------------------------
        p->anim.curr_frame = 0;
        p->anim.max_frames = 4;
        p->anim.curr_frame_time = 0.0f;
        p->anim.max_frame_time = 0.07f;
        p->anim.finite = false;
        p->anim.curr_loop = 0;
        p->anim.max_loops = 0;
        p->anim.frame_sequence[0] = 0;
        p->anim.frame_sequence[1] = 1;
        p->anim.frame_sequence[2] = 2;
        p->anim.frame_sequence[3] = 3;

#if ALWAYS_SHOW_GAUNTLET
        p->show_gauntlet = true;
#else
        p->show_gauntlet = false;
#endif
        p->gauntlet_selection = 0;

        p->gauntlet_slots = MIN(4,PLAYER_GAUNTLET_MAX);
        for(int j = 0; j < PLAYER_GAUNTLET_MAX; ++j)
        {
            p->gauntlet[j].type = ITEM_NONE;
            if(j == 0)
                p->gauntlet[j].type = item_get_random_gem();
        }
        p->gauntlet_item.type = ITEM_NONE;
    }
}

uint8_t player_get_gauntlet_count(Player* p)
{
    uint8_t count = 0;
    for(int i = 0; i < p->gauntlet_slots; ++i)
    {
        if(p->gauntlet[i].type != ITEM_NONE)
            count++;
    }
    return count;
}

bool player_gauntlet_full(Player* p)
{
    return (player_get_gauntlet_count(p) == p->gauntlet_slots);
}


void player_set_active(Player* p, bool active)
{
    p->active = active;
}

int player_get_active_count()
{
    int num = 0;
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(players[i].active)
            num++;
    }
    return num;
}

int player_names_build(bool include_all, bool only_active)
{
    int count = 0;

    if(include_all)
    {
        if(player_names[0]) free(player_names[0]);
        player_names[0] = calloc(4, sizeof(char));
        strncpy(player_names[0],"ALL",3);
        count++;
    }

    // fill out useful player_names array
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];
        if(only_active && !p->active) continue;

        if(player_names[count]) free(player_names[count]);

        int namelen  = strlen(p->name);
        player_names[count] = calloc(namelen, sizeof(char));
        strncpy(player_names[count], p->name, namelen);
        count++;
    }

    return count;
}


void player_send_to_room(Player* p, uint8_t room_index)
{
    uint8_t idx = room_index;
    p->curr_room = idx;
    p->transition_room = p->curr_room;

    Room* room = level_get_room_by_index(&level, room_index);

    for(int x = 0; x < ROOM_TILE_SIZE_X; ++x)
    {
        int _x = (ROOM_TILE_SIZE_X-1)/2;
        _x += ((x % 2 == 0) ? -1 : 1) * x/2;

        for(int y = 0; y < ROOM_TILE_SIZE_Y; ++y)
        {
            int _y = (ROOM_TILE_SIZE_Y-1)/2;
            _y += ((y % 2 == 0) ? -1 : 1) * y/2;

            if(level_get_tile_type(room, _x, _y) == TILE_FLOOR)
            {
                // printf("found floor %d,%d\n", _x, _y);
                Rect rp = level_get_tile_rect(_x, _y);
                p->phys.pos.x  = rp.x;
                p->phys.pos.y  = rp.y;
                return;
            }
        }
    }
}

void player_send_to_level_start(Player* p)
{
    uint8_t idx = level_get_room_index(level.start.x, level.start.y);
    player_send_to_room(p, idx);
}

void player_init_keys()
{
    if(player == NULL) return;

    // window_controls_clear_keys();
    window_controls_add_key(&player->actions[PLAYER_ACTION_UP].state, GLFW_KEY_W);
    window_controls_add_key(&player->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_S);
    window_controls_add_key(&player->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_A);
    window_controls_add_key(&player->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_D);
#if BOI_SHOOTING
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_UP].state, GLFW_KEY_I);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_DOWN].state, GLFW_KEY_K);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_LEFT].state, GLFW_KEY_J);
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT_RIGHT].state, GLFW_KEY_L);
#else
    window_controls_add_key(&player->actions[PLAYER_ACTION_SHOOT].state, GLFW_KEY_SPACE);
#endif
    window_controls_add_key(&player->actions[PLAYER_ACTION_GENERATE_ROOMS].state, GLFW_KEY_R);
    window_controls_add_key(&player->actions[PLAYER_ACTION_ACTIVATE].state, GLFW_KEY_E);
    window_controls_add_key(&player->actions[PLAYER_ACTION_JUMP].state, GLFW_KEY_SPACE);
    window_controls_add_key(&player->actions[PLAYER_ACTION_GEM_MENU].state, GLFW_KEY_G);
    window_controls_add_key(&player->actions[PLAYER_ACTION_GEM_MENU_CYCLE].state, GLFW_KEY_TAB);

    for(int i = 0;  i < PLAYER_ACTION_MAX; ++i)
        memset(&player->actions[i], 0, sizeof(PlayerInput));
}

void player2_init_keys()
{
    if(player2 == NULL)
    {
        printf("player2 is null\n");
        return;
    }
    window_controls_add_key(&player2->actions[PLAYER_ACTION_UP].state, GLFW_KEY_UP);
    window_controls_add_key(&player2->actions[PLAYER_ACTION_DOWN].state, GLFW_KEY_DOWN);
    window_controls_add_key(&player2->actions[PLAYER_ACTION_LEFT].state, GLFW_KEY_LEFT);
    window_controls_add_key(&player2->actions[PLAYER_ACTION_RIGHT].state, GLFW_KEY_RIGHT);
    // window_controls_add_key(&player2->actions[PLAYER_ACTION_SHOOT].state, GLFW_KEY_RIGHT_SHIFT);

    for(int i = 0;  i < PLAYER_ACTION_MAX; ++i)
        memset(&player2->actions[i], 0, sizeof(PlayerInput));
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

void player_set_collision_pos(Player* p, float x, float y)
{
    p->phys.pos.x = x - p->phys.coffset.x;
    p->phys.pos.y = y - p->phys.coffset.y;
}

void player_add_hp(Player* p, int hp)
{
    p->phys.hp += hp;
    p->phys.hp = RANGE(p->phys.hp,0,p->phys.hp_max);
}

void player_hurt(Player* p, int damage)
{
    if(players_invincible)
        return;

    if(p->invulnerable)
        return;

    player_add_hp(p,-damage);

    if(p->phys.hp == 0)
    {
        text_list_add(text_lst, 3.0, "%s died", p->name);
        player_die(p);
    }
    else
    {
        p->invulnerable = true;
        p->invulnerable_time = 0.0;
        p->invulnerable_max = 1.0;
    }
}


void player_die(Player* p)
{
    // should reset all?
    p->phys.dead = true;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p2 = &players[i];

        if(!p2->active) continue;
        if(!p2->phys.dead) return;
    }

    // all are dead
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p2 = &players[i];
        if(!p2->active) continue;

        player_reset(p2);
    }
}

void player_reset(Player* p)
{
    p->phys.dead = false;
    p->phys.hp = p->phys.hp_max;
    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p2 = &players[i];
        if(p == p2) continue;
        if(!p2->active) continue;
        player_send_to_room(p, p2->curr_room);
        return;
    }

    player_send_to_level_start(p);
}

void player_draw_room_transition()
{
    Player* p = player;
    if(!p->active) return;

    if(p->curr_room != p->transition_room)
    {
        float dx = transition_targets.x/15.0;
        float dy = transition_targets.y/15.0;

        if(!paused)
        {
            transition_offsets.x += dx;
            transition_offsets.y += dy;
        }

        if(ABS(transition_offsets.x) >= ABS(transition_targets.x) && ABS(transition_offsets.y) >= ABS(transition_targets.y))
        {
            // printf("room transition complete\n");
            p->transition_room = p->curr_room;

            bool immediate = true;
            float cx = p->phys.pos.x;
            float cy = p->phys.pos.y;
            bool check = camera_can_be_limited(cx, cy, (float)cam_zoom/100.0);
            if(!check)
            {
                // zoomed out too far
                immediate = !immediate;
                // cx = CENTER_X;
                // cy = CENTER_Y;
            }

            camera_set(immediate);

        }
        else
        {
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
            level_draw_room(&level.rooms[t.x][t.y], NULL, x0, y0);

            Vector2i roomxy = level_get_room_coords((int)p->curr_room);
            level_draw_room(&level.rooms[roomxy.x][roomxy.y], NULL, x1, y1);
        }
        // paused = true;  //uncommment to step through the transition
    }
}


void player_start_room_transition(Player* p)
{
    // printf("start room transition\n");

    Vector2i roomxy = level_get_room_coords((int)p->curr_room);

    // new player positions
    RectXY rxy = {0};
    rect_to_rectxy(&room_area, &rxy);
    float x1 = rxy.x[BL] - (CPOSX(p->phys) - rxy.x[TR]);
    float y1 = rxy.y[BL] - (CPOSY(p->phys) - rxy.y[TL]);

    if(role == ROLE_SERVER || role == ROLE_LOCAL)
    {
        p->transition_room = p->curr_room;

        switch(p->door)
        {
            case DIR_UP:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x, roomxy.y-1);
                player_set_collision_pos(p, CPOSX(p->phys), y1);
                p->sprite_index = SPRITE_UP;
                break;

            case DIR_RIGHT:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x+1, roomxy.y);
                player_set_collision_pos(p, x1, CPOSY(p->phys));
                p->sprite_index = SPRITE_RIGHT;
                break;

            case DIR_DOWN:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x, roomxy.y+1);
                player_set_collision_pos(p, CPOSX(p->phys), y1);
                p->sprite_index = SPRITE_DOWN;
                break;

            case DIR_LEFT:
                p->curr_room = (uint8_t)level_get_room_index(roomxy.x-1, roomxy.y);
                player_set_collision_pos(p, x1, CPOSY(p->phys));
                p->sprite_index = SPRITE_LEFT;
                break;

            default:
                break;
        }

        for(int i = 0; i < MAX_PLAYERS; ++i)
        {
            Player* p2 = &players[i];
            if(p == p2) continue;
            if(!p2->active) continue;
            if(p2->curr_room == p->curr_room) continue;
            if(p->transition_room != p2->curr_room) continue;

            p2->curr_room = p->curr_room;
            p2->phys.pos.x = p->phys.pos.x;
            p2->phys.pos.y = p->phys.pos.y;
            p2->sprite_index = p->sprite_index;
            p2->door = p->door;
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
    float vw = cr.w;
    float vh = cr.h;

    bool check = camera_can_be_limited(cr.x, cr.y, camera_get_zoom());
    if(!check)
    {
        vw = room_area.w;
        vh = room_area.h;
    }

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


static float sprite_index_to_angle(Player* p)
{
    if(p->sprite_index >= 0 && p->sprite_index <= 3)
        return 90.0;
    else if(p->sprite_index >= 4 && p->sprite_index <= 7)
        return 270.0;
    else if(p->sprite_index >= 8 && p->sprite_index <= 11)
        return 180.0;
    return 0.0;
}


static void handle_room_collision(Player* p)
{
    Room* room = level_get_room_by_index(&level, p->curr_room);

    // doors
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

        float d = dist(CPOSX(p->phys), CPOSY(p->phys), door_point.x, door_point.y);

        bool colliding_with_door = (d < p->phys.radius);
        if(colliding_with_door)
        {
            bool k = false;
            k |= i == DIR_UP && p->actions[PLAYER_ACTION_UP].state;
            k |= i == DIR_DOWN && p->actions[PLAYER_ACTION_DOWN].state;
            k |= i == DIR_LEFT && p->actions[PLAYER_ACTION_LEFT].state;
            k |= i == DIR_RIGHT && p->actions[PLAYER_ACTION_RIGHT].state;
            if(k)
            {
                p->phys.vel.x = 0;
                p->phys.vel.y = 0;
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

    if(role == ROLE_LOCAL && p == player)
    {
        if(player->curr_room != player->transition_room) return;
    }

    bool ai = false;
    if(p != player && p != player2)
    {
        ai = true;
        player_ai_move_to_target(p, player);
    }

    float prior_x = p->phys.pos.x;
    float prior_y = p->phys.pos.y;

    for(int i = 0; i < PLAYER_ACTION_MAX; ++i)
    {
        PlayerInput* pa = &p->actions[i];
        update_input_state(pa, dt);
    }

    if(p->actions[PLAYER_ACTION_GEM_MENU].toggled_on)
    {
#if ALWAYS_SHOW_GAUNTLET
        if(PLAYER_SWAPPING_GEM(p))
        {
            item_add(p->gauntlet_item.type, player->phys.pos.x, player->phys.pos.y, player->curr_room);
            p->gauntlet_item.type = ITEM_NONE;
        }
#else
        p->show_gauntlet = !p->show_gauntlet;
        if(!p->show_gauntlet && PLAYER_SWAPPING_GEM(p))
        {
            item_add(p->gauntlet_item.type, player->phys.pos.x, player->phys.pos.y, player->curr_room);
            p->gauntlet_item.type = ITEM_NONE;
        }
#endif
    }

    if(p->show_gauntlet)
    {
        if(p->actions[PLAYER_ACTION_GEM_MENU_CYCLE].toggled_on)
        {
            p->gauntlet_selection++;
            if(p->gauntlet_selection >= p->gauntlet_slots)
                p->gauntlet_selection = 0;
        }
    }

    bool up    = p->actions[PLAYER_ACTION_UP].state;
    bool down  = p->actions[PLAYER_ACTION_DOWN].state;
    bool left  = p->actions[PLAYER_ACTION_LEFT].state;
    bool right = p->actions[PLAYER_ACTION_RIGHT].state;

    // update velocity

    Vector2f vel_dir = {0.0,0.0};

    if(up)
    {
        p->sprite_index = SPRITE_UP;
        vel_dir.y = -1.0;
    }

    if(down)
    {
        p->sprite_index = SPRITE_DOWN;
        vel_dir.y = 1.0;
    }

    if(left)
    {
        p->sprite_index = SPRITE_LEFT;
        vel_dir.x = -1.0;
    }

    if(right)
    {
        p->sprite_index = SPRITE_RIGHT;
        vel_dir.x = 1.0;
    }

    if((up && (left || right)) || (down && (left || right)))
    {
        // moving diagonally
        vel_dir.x *= 0.7071f;
        vel_dir.y *= 0.7071f;
    }

    Vector2f vel_max = {
        p->phys.max_velocity*p->phys.speed_factor*vel_dir.x,
        p->phys.max_velocity*p->phys.speed_factor*vel_dir.y
    };

    bool moving = (up || down || left || right);

    float speed = p->phys.speed*p->phys.speed_factor;

    if(moving)
    {
        // trying to move
        p->phys.vel.x += vel_dir.x*speed*dt;
        p->phys.vel.y += vel_dir.y*speed*dt;

        if(ABS(vel_dir.x) > 0.0)
            if(ABS(p->phys.vel.x) > ABS(vel_max.x))
                p->phys.vel.x = vel_max.x;

        if(ABS(vel_dir.y) > 0.0)
            if(ABS(p->phys.vel.y) > ABS(vel_max.y))
                p->phys.vel.y = vel_max.y;
    }

    if(p->phys.pos.z > 0.0)
    {
        // apply gravity
        p->phys.vel.z -= 10.0;
    }

    bool jump = p->actions[PLAYER_ACTION_JUMP].toggled_on;
    if(jump && p->phys.pos.z == 0.0)
    {
        p->phys.vel.z = 200.0;
    }
    
    if(!left && !right)
        phys_apply_friction_x(&p->phys,p->phys.base_friction,dt);

    if(!up && !down)
        phys_apply_friction_y(&p->phys,p->phys.base_friction,dt);

    float m1 = magn2f(p->phys.vel.x,p->phys.vel.y);
    float m2 = magn2f(vel_max.x, vel_max.y);

    p->vel_factor = RANGE(m1/m2,0.4,1.0);

    p->phys.prior_vel.x = p->phys.vel.x;
    p->phys.prior_vel.y = p->phys.vel.y;
    p->phys.prior_vel.z = p->phys.vel.z;
    
    // update position
    p->phys.pos.x += p->phys.vel.x*dt;
    p->phys.pos.y += p->phys.vel.y*dt;
    p->phys.pos.z += p->phys.vel.z*dt;

    p->phys.pos.z = MAX(0.0, p->phys.pos.z);

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

    if(p->proj_cooldown > 0.0)
    {
        p->proj_cooldown -= dt;
        p->proj_cooldown = MAX(p->proj_cooldown,0.0);
    }

    p->shoot_sprite_cooldown = MAX(p->shoot_sprite_cooldown - dt, 0.0);


    ProjectileDef* pd = &projectile_lookup[0];

#if BOI_SHOOTING

    for(int i = 0; i < 4; ++i)
    {
        if(p->actions[PLAYER_ACTION_SHOOT_UP+i].toggled_on)
        {
            p->last_shoot_action = PLAYER_ACTION_SHOOT_UP+i;
            break;
        }
    }

    if(p->last_shoot_action >= PLAYER_ACTION_SHOOT_UP && p->last_shoot_action <= PLAYER_ACTION_SHOOT_RIGHT)
    {

        if(p->actions[p->last_shoot_action].toggled_off)
        {
            for(int i = 0; i < 4; ++i)
            {
                if(p->actions[PLAYER_ACTION_SHOOT_UP+i].state)
                {
                    p->last_shoot_action = PLAYER_ACTION_SHOOT_UP+i;
                    break;
                }
            }
        }


        int sprite_index = 0;
        if(p->last_shoot_action == PLAYER_ACTION_SHOOT_UP)
        {
            sprite_index = SPRITE_UP;
        }
        else if(p->last_shoot_action == PLAYER_ACTION_SHOOT_DOWN)
        {
            sprite_index = SPRITE_DOWN;
        }
        else if(p->last_shoot_action == PLAYER_ACTION_SHOOT_LEFT)
        {
            sprite_index = SPRITE_LEFT;
        }
        else if(p->last_shoot_action == PLAYER_ACTION_SHOOT_RIGHT)
        {
            sprite_index = SPRITE_RIGHT;
        }

        if(p->shoot_sprite_cooldown > 0)
        {
            p->sprite_index = sprite_index;
        }

        if(p->actions[p->last_shoot_action].state && p->proj_cooldown == 0.0)
        {
            p->sprite_index = sprite_index;
            float angle_deg = sprite_index_to_angle(p);

            if(!p->phys.dead)
            {
                projectile_add(&p->phys, p->curr_room, PROJECTILE_TYPE_LASER, angle_deg, 1.0, 1.0, true);
            }
            // text_list_add(text_lst, 5.0, "projectile");
            p->proj_cooldown = p->proj_cooldown_max;
            p->shoot_sprite_cooldown = 1.0;
        }
    }

#else

    PlayerInput* shoot = &p->actions[PLAYER_ACTION_SHOOT];

    if(pd->charge)
    {
        if(shoot->toggled_on)
        {
            // text_list_add(text_lst, 5.0, "start charge");
            p->proj_charge = pd->charge_rate;
        }
        else if(shoot->state)
        {
            if(p->proj_charge > 0)
            {
                p->proj_charge = MIN(p->proj_charge + pd->charge_rate, 100);
                // text_list_add(text_lst, 5.0, "charging: %u", p->proj_charge);
            }
            // else
            // {
            //     text_list_add(text_lst, 5.0, "NOT charging");
            // }
        }

        if(shoot->toggled_off)
        {
            if(p->proj_cooldown == 0.0 && p->proj_charge > 0)
            {
                float scale = (float)p->proj_charge / 50.0;
                float damage = (float)p->proj_charge / 100.0;
                float angle_deg = sprite_index_to_angle(p);

                if(!p->phys.dead)
                {
                    projectile_add(&p->phys, p->curr_room, PROJECTILE_TYPE_LASER, angle_deg, scale, damage, true);
                }

                // text_list_add(text_lst, 5.0, "projectile: %.2f, %.2f", scale, damage);
                p->proj_cooldown = p->proj_cooldown_max;
            }
            else
            {
                // text_list_add(text_lst, 5.0, "[no proj] cooldown: %.2f, charge: %u", p->proj_cooldown, p->proj_charge);
            }
            p->proj_charge = 0;
        }
    }
    else
    {
        if(shoot->state && p->proj_cooldown == 0.0)
        {
            float angle_deg = sprite_index_to_angle(p);

            if(!p->phys.dead)
            {
                projectile_add(&p->phys, p->curr_room, PROJECTILE_TYPE_LASER, angle_deg, 1.0, 1.0, true);
            }
            // text_list_add(text_lst, 5.0, "projectile");
            p->proj_cooldown = p->proj_cooldown_max;
        }
    }

#endif

    if(role == ROLE_LOCAL)
    {
        bool generate = p->actions[PLAYER_ACTION_GENERATE_ROOMS].toggled_on;
        if(generate)
        {
            seed = time(0)+rand()%1000;
            game_generate_level(seed);
        }
    }


    if(PLAYER_SWAPPING_GEM(p))
    {
        message_small_set(0.1, "Press e to swap [%s] for [%s] (press g to cancel)", item_get_name(p->gauntlet[p->gauntlet_selection].type), item_get_name(p->gauntlet_item.type));
    }
    else if(p->highlighted_item)
    {
        message_small_set(0.1, "Item: %s", item_get_name(p->highlighted_item->type));
    }

    bool activate = p->actions[PLAYER_ACTION_ACTIVATE].toggled_on;
    if(activate)
    {
        // @TEMP
        p->phys.vel.z = 100.0;

        if(PLAYER_SWAPPING_GEM(p))
        {

            Item* it = &p->gauntlet[p->gauntlet_selection];

            item_add(it->type, player->phys.pos.x, player->phys.pos.y, player->curr_room);
            
            memcpy(it, &p->gauntlet_item, sizeof(Item));
            p->gauntlet_item.type = ITEM_NONE;

            // message_small_set(0.1, "Sawp Item: %s", item_get_name(p->gauntlet_item.type));
        }
        else if(p->highlighted_item)
        {
            ItemType type = p->highlighted_item->type;

            if(type == ITEM_CHEST)
            {
                if(!p->highlighted_item->opened)
                {
                    p->highlighted_item->opened = true;
                    item_props[type].func(p->highlighted_item,p);
                }
            }
            else
            {
                item_props[type].func(p->highlighted_item,p);
                item_remove(p->highlighted_item);
            }
        }
    }


    // check tiles around player
    handle_room_collision(p);

    if(p == player)
    {
        Room* room = level_get_room_by_index(&level, (int)p->curr_room);
        room->discovered = true;
    }

    // update animation
    if(moving)
    {
        gfx_anim_update(&p->anim, p->vel_factor*dt);
    }
    else
        p->anim.curr_frame = 0;

    if(isnan(p->phys.pos.x) || isnan(p->phys.pos.y))
    {
        text_list_add(text_lst, 5.0, "player pos was nan!");
        p->phys.pos.x = prior_x;
        p->phys.pos.y = prior_y;
        update_player_boxes(p);
    }

    if(p->invulnerable)
    {
        p->invulnerable_time += dt;
        if(p->invulnerable_time >= p->invulnerable_max)
        {
            p->invulnerable = false;
        }
    }
}

void player_ai_move_to_target(Player* p, Player* target)
{
    if(target->curr_room != p->curr_room)
    {
        p->actions[PLAYER_ACTION_LEFT].state = false;
        p->actions[PLAYER_ACTION_RIGHT].state = false;
        p->actions[PLAYER_ACTION_UP].state = false;
        p->actions[PLAYER_ACTION_DOWN].state = false;
        return;
    }

    // const float threshold = 2.0;
    float threshold = p->phys.radius*2.0 + 6.0;

    float tx = target->phys.pos.x;
    float ty = target->phys.pos.y;

    float dx = p->phys.pos.x - tx;
    float dy = p->phys.pos.y - ty;

    if(ABS(dx) > threshold)
    {
        if(dx < 0)
        {
            p->actions[PLAYER_ACTION_LEFT].state = false;
            p->actions[PLAYER_ACTION_RIGHT].state = true;
        }
        else
        {
            p->actions[PLAYER_ACTION_LEFT].state = true;
            p->actions[PLAYER_ACTION_RIGHT].state = false;
        }
    }
    else
    {
        p->actions[PLAYER_ACTION_LEFT].state = false;
        p->actions[PLAYER_ACTION_RIGHT].state = false;
    }

    if(ABS(dy) > threshold)
    {
        if(dy < 0)
        {
            p->actions[PLAYER_ACTION_UP].state = false;
            p->actions[PLAYER_ACTION_DOWN].state = true;
        }
        else
        {
            p->actions[PLAYER_ACTION_UP].state = true;
            p->actions[PLAYER_ACTION_DOWN].state = false;
        }
    }
    else
    {
        p->actions[PLAYER_ACTION_UP].state = false;
        p->actions[PLAYER_ACTION_DOWN].state = false;
    }

}

void draw_gauntlet()
{
    if(!player->show_gauntlet) return;

    float len = 50.0;
    float margin = 5.0;

    int w = gfx_images[items_image].element_width;
    float scale = len / (float)w;

    float total_len = (player->gauntlet_slots * len) + ((player->gauntlet_slots-1) * margin);

    float x = (view_width - total_len) / 2.0;   // left side of first rect
    x += len/2.0;   // centered

    // float y = (float)view_height * 0.75;
    float y = view_height - len/2.0 - margin;

    Rect r = {0};
    r.w = len;
    r.h = len;
    r.y = y;
    r.x = x;

    for(int i = 0; i < player->gauntlet_slots; ++i)
    {
        uint32_t color = COLOR_BLACK;
        if(i == player->gauntlet_selection)
            color = COLOR_WHITE;
        gfx_draw_rect(&r, color, NOT_SCALED, NO_ROTATION, 0.5, true, NOT_IN_WORLD);

        ItemType type = player->gauntlet[i].type;
        if(type != ITEM_NONE)
        {
            ItemProps* props = &item_props[type];
            gfx_draw_image_ignore_light(props->image, props->sprite_index, r.x, r.y, COLOR_TINT_NONE, scale, 0.0, 1.0, false, NOT_IN_WORLD);
        }
        r.x += len;
        r.x += margin;
    }
}

void player_draw(Player* p, bool batch)
{
    if(!p->active) return;
    if(p->curr_room != player->curr_room) return;

    // Vector2i roomxy = level_get_room_coords((int)p->curr_room);
    // Room* room = &level.rooms[roomxy.x][roomxy.y];
    Room* room = level_get_room_by_index(&level, (int)p->curr_room);

    bool blink = p->invulnerable ? ((int)(p->invulnerable_time * 100)) % 2 == 0 : false;
    float opacity = p->phys.dead ? 0.3 : 1.0;

    opacity = blink ? 0.3 : opacity;
    uint32_t color = gfx_blend_colors(COLOR_BLUE, COLOR_TINT_NONE, p->phys.speed_factor);

    float y = p->phys.pos.y-(0.5*p->phys.pos.z);
    float shadow_scale = RANGE(0.5*(1.0 - (p->phys.pos.z / 128.0)),0.1,0.5);

    if(batch)
    {
        gfx_sprite_batch_add(shadow_image, 0, p->phys.pos.x, p->phys.pos.y+12, color, false, shadow_scale, 0.0, 0.5, false, false, false);
        gfx_sprite_batch_add(player_image, p->sprite_index+p->anim.curr_frame, p->phys.pos.x, y, color, false, 1.0, 0.0, opacity, false, false, false);
    }
    else
    {
        gfx_draw_image(shadow_image, 0, p->phys.pos.x, p->phys.pos.y+12, color, shadow_scale, 0.0, 0.5, false, IN_WORLD);
        gfx_draw_image(player_image, p->sprite_index+p->anim.curr_frame, p->phys.pos.x, y, color, 1.0, 0.0, opacity, false, IN_WORLD);
    }

    if(debug_enabled)
    {

        Rect r = RECT(p->phys.pos.x, p->phys.pos.y, 1, 1);
        gfx_draw_rect(&r, COLOR_RED, NOT_SCALED, NO_ROTATION, 1.0, true, true);
        gfx_draw_rect(&p->hitbox, COLOR_GREEN, NOT_SCALED, NO_ROTATION, 1.0, false, true);

        float x0 = p->phys.pos.x;
        float y0 = p->phys.pos.y;

        float x1 = x0 + p->phys.vel.x;
        float y1 = y0 + p->phys.vel.y;
        gfx_add_line(x0, y0, x1, y1, COLOR_RED);
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

    Vector2f lp = lerp2f(&p->server_state_prior.pos, &p->server_state_target.pos, t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    // printf("[lerping player] t: %.2f, x: %.2f -> %.2f = %.2f\n", t, p->server_state_prior.pos.x, p->server_state_target.pos.x, lp.x);

    p->invulnerable_time = lerp(p->server_state_prior.invulnerable_time, p->server_state_target.invulnerable_time, t);


    if(p == player)
    {
        Room* room = level_get_room_by_index(&level, (int)p->curr_room);
        room->discovered = true;
    }
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

void player_handle_collision(Player* p, Entity* e)
{
    if(p->phys.dead)
        return;

    switch(e->type)
    {
        case ENTITY_TYPE_CREATURE:
        {
            Creature* c = (Creature*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&c->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&p->phys, &c->phys,&ci);

                if(c->painful_touch)
                {
                    player_hurt(p,c->damage);
                }
            }
        } break;
        case ENTITY_TYPE_PLAYER:
        {
            Player* p2 = (Player*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&p->phys, &p2->phys,&ci);
            }
        } break;

        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&p->phys,&p2->phys, &ci);

            if(collided)
            {
                if(item_props[p2->type].touch_item)
                    item_props[p2->type].func(p2, p);

                phys_collision_correct(&p->phys, &p2->phys,&ci);
                //phys_collision_correct_no_bounce(&p->phys, &p2->phys, &ci);
            }

        } break;
    }
}

bool is_any_player_room(uint8_t curr_room)
{
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(!players[i].active) continue;
        if(curr_room == players[i].curr_room)
        {
            return true;
        }
    }

    return false;
}

int player_get_count_in_room(uint8_t curr_room)
{
    int count = 0;
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(!players[i].active) continue;
        if(curr_room == players[i].curr_room)
        {
            count++;
        }
    }
    return count;
}
