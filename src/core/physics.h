#pragma once

#include "math2d.h"

typedef struct
{
    Vector2f pos;
    Vector2f target_pos;
    Vector2f prior_pos;

    Vector2f vel;
    float radius;
} Physics;
