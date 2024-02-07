#include "headers.h"
#include "main.h"
#include "level.h"
#include "gfx.h"

#include "weapon.h"

static int weapon_image = -1;

static uint16_t id_counter = 1;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

void weapon_init()
{
    if(weapon_image == -1)
        weapon_image = gfx_load_image("src/img/weapon_spear.png", false, false, 48, 32);
}

void weapon_add(WeaponType type, Physics* phys, Weapon* w, bool _new)
{
    if(_new)
        w->id = get_id();

    w->type = type;
    w->state = WEAPON_STATE_NONE;
    w->phys = phys;

    switch(type)
    {
        case WEAPON_TYPE_SPEAR:
            w->image = weapon_image;
            w->damage = 1.0;
            w->windup_time  = 0.2;
            w->release_time = 0.5;
            w->retract_time = 0.2;
            break;
        default:
            w->image = weapon_image;
            w->damage = 1.0;
            w->windup_time  = 0.2;
            w->release_time = 0.5;
            w->retract_time = 0.2;
            break;
    }

    w->time = 0.0;
}

void weapon_update(Weapon* w, float dt)
{
    if(!w) return;
    if(w->state == WEAPON_STATE_NONE) return;

    // update state
    w->time += dt;

    w->color = COLOR_YELLOW;

    if(w->time >= w->windup_time)
    {
        w->state = WEAPON_STATE_RELEASE;
        w->color = COLOR_GREEN;
    }

    if(w->time >= w->windup_time + w->release_time)
    {
        w->state = WEAPON_STATE_RETRACT;
        w->color = COLOR_BLUE;
    }

    if(w->time >= w->windup_time + w->release_time + w->retract_time)
    {
        w->state = WEAPON_STATE_NONE;
        w->time = 0.0;
    }

    // update position offset

    GFXImage* img = &gfx_images[w->image];
    Rect* vr = &img->visible_rects[0];

    Vector2f offset = {0.0, 0.0};

    if(w->phys->rotation_deg == 90.0) // up
    {
        offset.y -= vr->h;
        offset.y -= w->phys->vr.h/2.0;
    }
    else if(w->phys->rotation_deg == 0.0) // right
    {
        offset.x += vr->w/2.0;
    }
    else if(w->phys->rotation_deg == 270.0) // down
    {
        offset.y += vr->h;
        offset.y += w->phys->vr.h/2.0;
    }
    else if(w->phys->rotation_deg == 180.0) // left
    {
        offset.x -= vr->w/2.0;
    }

    w->pos.x = w->phys->pos.x + offset.x;
    w->pos.y = w->phys->pos.y - (w->phys->vr.h + w->phys->pos.z)/2.0 + offset.y;
}

void weapon_draw(Weapon* w)
{
    if(w->state == WEAPON_STATE_NONE)
        return;

    gfx_sprite_batch_add(w->image, 0, w->pos.x, w->pos.y, w->color, false, 1.0, w->phys->rotation_deg, 1.0, false, false, false);

}
