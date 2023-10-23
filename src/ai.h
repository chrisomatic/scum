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
        c->action_counter_max = RAND_FLOAT(0.5, 1.0);
        return true;
    }
    return false;
}

void ai_random_walk_cardinal(Creature* c)
{

}

void ai_random_walk(Creature* c)
{
    int dir = rand() % 8;

    Vector2i o = get_dir_offsets(dir);

    c->h = o.x;
    c->v = o.y;

    if(dir < DIR_NONE)
    {
        c->sprite_index = dir % 4;
    }
}

int ai_rand(int max)
{
    return rand() % max;
}

bool ai_flip_coin()
{
    return (rand() % 2) == 0;
}
