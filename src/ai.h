#pragma once

void ai_init_action(Creature* c)
{
    c->action_counter = 0.0;
    c->action_counter_max = RAND_FLOAT(0.5, 1.0);
}

bool ai_update_action(Creature* c, float dt)
{
    c->action_counter += dt;

    if(c->action_counter >= c->action_counter_max)
    {
        c->action_counter -= c->action_counter_max;
        c->action_counter_max = RAND_FLOAT(c->act_time_min, c->act_time_max);
        return true;
    }
    return false;
}

static void update_creature_sprite_index(Creature* c)
{
    /*
    Dir dir = get_dir_from_offsets(c);
    if(c->type == CREATURE_TYPE_SLUG) // do this for any standard crawler creature
        c->sprite_index = dir % 4;
        */
}

static void _update_sprite_index(Creature* c, Dir dir)
{
    switch(dir)
    {
        case DIR_UP: case DIR_UP_RIGHT:
            c->sprite_index = DIR_UP;
            break;
        case DIR_RIGHT: case DIR_DOWN_RIGHT:
            c->sprite_index = DIR_RIGHT;
            break;
        case DIR_DOWN:  case DIR_DOWN_LEFT:
            c->sprite_index = DIR_DOWN;
            break;
        case DIR_LEFT:  case DIR_UP_LEFT:
            c->sprite_index = DIR_LEFT;
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
}

void ai_walk_dir(Creature* c, Dir dir)
{
    Vector2i o = get_dir_offsets(dir);

    c->h = o.x;
    c->v = o.y;

    if(dir < DIR_NONE)
    {
        if(c->type == CREATURE_TYPE_SLUG) // do this for any standard crawler creature
            _update_sprite_index(c, dir);
    }
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

bool ai_move_to_target(Creature* c,float dt)
{
    Vector2f target_pos = level_get_pos_by_room_coords(c->target_tile.x, c->target_tile.y);
    Vector2f v = {target_pos.x - c->phys.pos.x, target_pos.y - c->phys.pos.y};

    bool at_target = (ABS(v.x) < 3.0 && ABS(v.y) < 3.0);

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

bool ai_flip_coin()
{
    return (rand() % 2) == 0;
}
