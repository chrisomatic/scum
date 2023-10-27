#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"

#include "player.h"
#include "gems.h"

int gems_image = -1;
Gem gems[MAX_GEMS];
glist* gem_list = NULL;

static uint16_t id_counter = 0;


void gems_init()
{
    if(gems_image > 0) return;
    gem_list = list_create((void*)gems, MAX_GEMS, sizeof(Gem));
    gems_image = gfx_load_image("src/img/gems.png", false, false, 16, 16);
}

Rect gem_get_rect(GemType type)
{
    GFXImage* img = &gfx_images[gems_image];
    Rect* vr = &img->visible_rects[type];
    Rect r = {0};
    r.w = vr->w;
    r.h = vr->h;
    return r;
}

