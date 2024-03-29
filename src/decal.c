#include "headers.h"
#include "math2d.h"
#include "glist.h"
#include "player.h"
#include "gfx.h"
#include "decal.h"

Decal decals[MAX_DECALS];
glist* decal_list = NULL;

void decal_init()
{
    decal_list = list_create((void*)decals, MAX_DECALS, sizeof(Decal), false);
}

void decal_add(Decal d)
{
    d.ttl_start = d.ttl;
    list_add(decal_list, (void*)&d);
}

void decal_draw_all()
{
    if(decal_list == NULL) return;

    for(int i = decal_list->count-1; i >= 0; --i)
    {
        Decal* d = &decals[i];
        if(d->room != player->curr_room) continue;
        // printf("room: %u == %u\n", d->room, player->curr_room);
        // printf("%d\n", d->image);
        // printf("%u\n", d->sprite_index);
        // printf("%.1f, %.1f\n", d->pos.x, d->pos.y);
        // printf("%u\n", d->tint);
        // printf("%.1f\n", d->scale);
        // printf("%.1f\n", d->rotation);
        // printf("%.1f\n", d->opacity);
        float op = 1.0;

        if(d->fade_pattern == 0)
        {
            if(d->ttl <= 1.0)
            {
                op = MAX(0.0, d->ttl);
            }
        }
        else if(d->fade_pattern == 1)
        {
            float dt = d->ttl_start - d->ttl;
            if(dt <= 1.0)
            {
                op = dt;
            }
        }
        gfx_draw_image(d->image, d->sprite_index, d->pos.x, d->pos.y, d->tint, d->scale, d->rotation, d->opacity*op, false, true);
    }
}

void decal_update_all(float dt)
{
    for(int i = decal_list->count-1; i >= 0; --i)
    {
        Decal* d = &decals[i];
        d->ttl -= dt;
        if(d->ttl <= 0.0)
        {
            list_remove(decal_list, i);
        }
    }
}

void decal_clear_all()
{
    list_clear(decal_list);

}

