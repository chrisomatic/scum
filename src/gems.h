#pragma once

#include "player.h"
#include "glist.h"
#include "physics.h"
#include "net.h"

#define MAX_GEMS 256
typedef enum
{
    GEM_TYPE_WHITE,
    GEM_TYPE_RED,
    GEM_TYPE_GREEN,
    GEM_TYPE_BLUE,

    GEM_TYPE_NONE,
    GEM_TYPE_MAX
} GemType;

typedef struct
{
    uint16_t id;

    GemType type;
    Physics phys;

    Rect hit_box;
    Rect hit_box_prior;
    float radius;
    float angle_deg;
    uint8_t curr_room;

} Gem;

extern int gems_image;
extern Gem gems[MAX_GEMS];
extern glist* gem_list;

void gems_init();
Rect gem_get_rect(GemType type);
