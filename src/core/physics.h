#pragma once

#include "math2d.h"

// collision position
#define CPOSX(phys) ((phys).pos.x+(phys).coffset.x)
#define CPOSY(phys) ((phys).pos.y+(phys).coffset.y)

typedef struct
{
    Vector2f pos;
    Vector2f target_pos;
    Vector2f prior_pos;

    Vector2f vel;
    float speed;
    float radius;
    Vector2f coffset;
} Physics;
