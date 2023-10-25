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

void ai_stop_moving(Creature* c)
{
    c->h = 0.0;
    c->v = 0.0;
}
void ai_walk_dir(Creature* c, Dir dir)
{
    Vector2i o = get_dir_offsets(dir);

    c->h = o.x;
    c->v = o.y;

    if(dir < DIR_NONE)
    {
        if(c->type == CREATURE_TYPE_SLUG) // do this for any standard crawler creature
            c->sprite_index = dir % 4;
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

int ai_rand(int max)
{
    return rand() % max;
}

bool ai_flip_coin()
{
    return (rand() % 2) == 0;
}
