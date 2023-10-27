#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"

#include "player.h"
#include "pickup.h"

glist* pickup_list = NULL;
Pickup pickups[MAX_PICKUPS];

int gems_image = -1;

static Rect gem_get_rect(GemType type)
{
    GFXImage* img = &gfx_images[gems_image];
    Rect* vr = &img->visible_rects[type];
    Rect r = {0};
    r.w = vr->w;
    r.h = vr->h;
    return r;
}

void pickup_init()
{
    pickup_list = list_create((void*)pickups, MAX_PICKUPS, sizeof(Pickup));

    if(gems_image > 0) return;
    pickup_list = list_create((void*)pickups, MAX_PICKUPS, sizeof(Pickup));
    gems_image = gfx_load_image("src/img/gems.png", false, false, 16, 16);

}

void pickup_add(PickupType type, int subtype, float x, float y, uint8_t curr_room)
{
    Pickup pu = {0};

    pu.type = type;
    pu.subtype = subtype;
    pu.sprite_index = type;
    pu.phys.pos.x = x;
    pu.phys.pos.y = y;
    pu.phys.mass = 1.0;
    pu.curr_room = curr_room;

    list_add(pickup_list,&pu);
}

void pickup_draw(Pickup* pu)
{
    switch(pu->type)
    {
        case PICKUP_TYPE_GEM:
            //gem_draw((Pickup*)pu);
            break;

    }
}

