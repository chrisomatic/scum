#pragma once

static float sprite_index_to_angle(Creature* c)
{
    if(c->sprite_index == 0)
        return 0.0;

    if(c->sprite_index == 1)
        return 90.0;

    if(c->sprite_index == 2)
        return 180.0;

    return 270.0;
}

static void creature_set_sprite_index(Creature* c, int sprite_index)
{
    c->sprite_index = sprite_index;
    c->phys.rotation_deg = sprite_index_to_angle(c);
}

void ai_init_action(Creature* c)
{
    c->action_counter = 0.0;
    c->action_counter_max = RAND_FLOAT(0.5, 1.0);
}

void ai_choose_new_action_max(Creature* c)
{
    c->action_counter_max = RAND_FLOAT(c->act_time_min, c->act_time_max);
}
bool ai_update_action(Creature* c, float dt)
{
    if(level_grace_time > 0.0)
        return false;

    c->action_counter += dt;

    if(c->action_counter >= c->action_counter_max)
    {
        c->action_counter = 0;
        ai_choose_new_action_max(c);
        return true;
    }
    return false;
}

static void _update_sprite_index(Creature* c, Dir dir)
{
    bool update = (c->type == CREATURE_TYPE_SLUG || 
           c->type == CREATURE_TYPE_BUZZER ||
           c->type == CREATURE_TYPE_SPIKED_SLUG ||
           c->type == CREATURE_TYPE_INFECTED ||
           c->type == CREATURE_TYPE_SPAWN_SPIDER ||
           c->type == CREATURE_TYPE_PHANTOM || 
           c->type == CREATURE_TYPE_SLUGZILLA ||
           c->type == CREATURE_TYPE_GHOST ||
           c->type == CREATURE_TYPE_BOOMER);
    
    if(!update)
        return;

    switch(dir)
    {
        case DIR_UP: case DIR_UP_RIGHT:
            creature_set_sprite_index(c,DIR_UP);
            break;
        case DIR_RIGHT: case DIR_DOWN_RIGHT:
            creature_set_sprite_index(c,DIR_RIGHT);
            break;
        case DIR_DOWN:  case DIR_DOWN_LEFT:
            creature_set_sprite_index(c,DIR_DOWN);
            break;
        case DIR_LEFT:  case DIR_UP_LEFT:
            creature_set_sprite_index(c,DIR_LEFT);
            break;
    }
}

void ai_stop_imm(Creature* c)
{
    c->h = 0.0;
    c->v = 0.0;

    c->phys.vel.x = 0.0;
    c->phys.vel.y = 0.0;
}

void ai_stop_moving(Creature* c)
{
    c->h = 0.0;
    c->v = 0.0;
}

void ai_move_imm(Creature* c, Dir dir, float speed)
{
    c->h = 0.0;
    c->v = 0.0;

    switch(dir)
    {
        case DIR_UP:
            c->phys.vel.x = 0.0;
            c->phys.vel.y = -speed;
            break;
        case DIR_RIGHT:
            c->phys.vel.x = +speed;
            c->phys.vel.y = 0.0;
            break;
        case DIR_DOWN:
            c->phys.vel.x = 0.0;
            c->phys.vel.y = +speed;
            break;
        case DIR_LEFT:
            c->phys.vel.x = -speed;
            c->phys.vel.y = 0.0;
            break;
        case DIR_UP_RIGHT:
            c->phys.vel.x = +speed;
            c->phys.vel.y = -speed; 
            break;
        case DIR_DOWN_LEFT:
            c->phys.vel.x = -speed;
            c->phys.vel.y = +speed;
            break;
        case DIR_DOWN_RIGHT:
            c->phys.vel.x = +speed;
            c->phys.vel.y = +speed;
            break;
        case DIR_UP_LEFT:
            c->phys.vel.x = -speed;
            c->phys.vel.y = -speed;
            break;
    }

    _update_sprite_index(c, dir);
}

bool ai_move_to_tile(Creature* c, int x, int y)
{
    Rect r = level_get_tile_rect(x, y);

    Vector2f v = {r.x - c->phys.pos.x, r.y - c->phys.pos.y};

    if(ABS(v.x) < 3.0 && ABS(v.y) < 3.0)
    {
        // reached tile
        return true;
    }

    // printf("[%u] %.1f, %.1f moving to %d, %d (%.0f, %.0f)\n", c->id, c->phys.pos.x, c->phys.pos.y, x, y, r.x, r.y);

    normalize(&v);

    // set velocity to move toward tile
    c->phys.vel.x = c->phys.speed*v.x;
    c->phys.vel.y = c->phys.speed*v.y;

    return false;
}

void ai_walk_dir(Creature* c, Dir dir)
{
    Vector2i o = get_dir_offsets(dir);

    c->h = o.x;
    c->v = o.y;

    _update_sprite_index(c, dir);
}

void ai_random_walk(Creature* c)
{
    int dir = rand() % 8;
    ai_walk_dir(c,dir);
}

void ai_random_walk_cardinal(Creature* c)
{
    int dir = rand() % 4;
    ai_walk_dir(c,dir);
}

void ai_set_target(Creature* c, int tile_x, int tile_y)
{
    c->target_tile.x = tile_x;
    c->target_tile.y = tile_y;
}

bool ai_has_target(Creature* c)
{
    return (c->target_tile.x >= 0 && c->target_tile.y >= 0);
}

bool ai_has_subtarget(Creature* c)
{
    return (c->subtarget_tile.x >= 0 && c->subtarget_tile.y >= 0);
}

bool ai_on_target(Creature* c)
{
    Vector2f target_pos = level_get_pos_by_room_coords(c->target_tile.x, c->target_tile.y);
    Vector2f v = {target_pos.x - c->phys.pos.x, target_pos.y - c->phys.pos.y};

    return (ABS(v.x) < 3.0 && ABS(v.y) < 3.0);
}

bool ai_move_to_target(Creature* c,float dt)
{
    Vector2f target_pos = level_get_pos_by_room_coords(c->target_tile.x, c->target_tile.y);
    Vector2f v = {target_pos.x - c->phys.pos.x, target_pos.y - c->phys.pos.y};

    bool at_target =  (ABS(v.x) < 3.0 && ABS(v.y) < 3.0);

    if(at_target)
    {
        c->phys.vel.x = 0.0;
        c->phys.vel.y = 0.0;
        c->h = 0.0;
        c->v = 0.0;
    }
    else
    {
        // stop momentum
        normalize(&v);
        c->h = v.x;
        c->v = v.y;
    }

    return at_target;

}

void ai_clear_target(Creature* c)
{
    c->target_tile.x = -1;
    c->target_tile.y = -1;
}

int ai_rand(int max)
{
    return rand() % max;
}

void ai_target_rand_tile(Creature* c)
{
    Room* room = level_get_room_by_index(&level, c->phys.curr_room);

    Vector2i tilec = {0};
    Vector2f tilep = {0};

    level_get_rand_floor_tile(room, &tilec, &tilep);

    c->target_tile.x = tilec.x;
    c->target_tile.y = tilec.y;
}

void ai_shoot_nearest_player(Creature* c)
{
    Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
    float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y) + RAND_FLOAT(-10.0,10.0);

    Vector3f pos = {c->phys.pos.x, c->phys.pos.y, c->phys.height/2.0 + c->phys.pos.z};
    Vector3f vel = {c->phys.vel.x, c->phys.vel.y, 0.0};

    projectile_add(pos, &vel, c->phys.curr_room, c->room_gun_index, angle, false, &c->phys, c->id);
}

bool ai_on_target_tile(Creature* c)
{
    if(c->phys.curr_tile.x == c->target_tile.x && c->phys.curr_tile.y == c->target_tile.y)
    {
        return true;
    }
    return false;
}

bool ai_path_find_to_target_tile(Creature* c)
{
    if(!ai_on_target_tile(c))
    {
        if(c->phys.underground)
        {
            astar_set_traversable_func(&level.asd, level_every_tile_traversable_func);
        }
        bool traversable = astar_traverse(&level.asd, c->phys.curr_tile.x, c->phys.curr_tile.y, c->target_tile.x, c->target_tile.y);
        astar_set_traversable_func(&level.asd, level_tile_traversable_func);//set back to default

        if(!traversable)
        {
            printf("%u not traversable\n", c->id);
            return false;
        }

        if(level.asd.pathlen == 1)
        {
            return true;
        }

        AStarNode_t* n = &level.asd.path[1];
        return ai_move_to_tile(c, n->x, n->y);
    }

    return ai_move_to_tile(c, c->phys.curr_tile.x, c->phys.curr_tile.y);
}

bool ai_flip_coin()
{
    return (rand() % 2) == 0;
}
