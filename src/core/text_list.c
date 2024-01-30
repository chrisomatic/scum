#include "headers.h"
#include "log.h"
#include "gfx.h"
#include "text_list.h"

#define FADEOUT_DURATION 1.0
#define SHIFT_DURATION 0.3

text_list_t* text_list_init(int max, float x, float y, float scale, bool downward, text_align_t alignment, bool in_world, bool fade_out)
{
    text_list_t* lst = calloc(1,sizeof(text_list_t));
    lst->max = MAX(max,1);
    lst->count = 0;
    lst->downward = downward;
    lst->alignment = alignment;
    lst->in_world = in_world;
    lst->x = x;
    lst->y = y;
    lst->y_shift = 0.0;
    lst->scale = scale;
    // lst->color = color;
    lst->fade_out = fade_out;
    lst->text = calloc(lst->max, sizeof(char*));
    lst->durations = calloc(lst->max, sizeof(float));
    lst->colors = calloc(lst->max, sizeof(uint32_t));
    Vector2f size = gfx_string_get_size(lst->scale, "P");
    lst->text_height = size.y;
    return lst;
}

void text_list_add(text_list_t* lst, uint32_t color, float duration, char* fmt, ...)
{
    if(lst == NULL) return;
    if(lst->count == lst->max) return;

    char** d = &lst->text[lst->count];

    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);

    int size = vsnprintf(NULL, 0, fmt, args);

    *d = calloc(size+1, sizeof(char));
    if(!*d) return;

    vsnprintf(*d, size+1, fmt, args2);

    va_end(args);
    va_end(args2);

    /*
    va_list args;
    va_start(args, fmt);
    vasprintf(d, fmt, args);
    va_end(args);
    */

    lst->durations[lst->count] = duration;
    lst->colors[lst->count] = color;
    lst->count++;

    if(lst->downward)
    {
        lst->y_shift = SHIFT_DURATION;
    }
}

void text_list_update(text_list_t* lst, float _dt)
{
    if(lst == NULL) return;
    if(lst->count == 0) return;

    for(int i = 0; i < lst->count; ++i)
    {
        lst->durations[i] -= _dt;
        if(lst->durations[i] <= 0.0)
        {
            if(!lst->downward)
                lst->y_shift += SHIFT_DURATION;

            if(lst->text[i]) free(lst->text[i]);
            lst->text[i] = NULL;
            for(int j = i; j < lst->count; j++)
            {
                lst->text[j] = lst->text[j+1];
                lst->durations[j] = lst->durations[j+1];
                lst->colors[j] = lst->colors[j+1];
                lst->text[j+1] = NULL;
            }
            lst->count--;
        }
    }

    if(lst->y_shift > 0.0)
    {
        lst->y_shift -= (lst->y_shift*_dt*10.0);

        if(lst->y_shift < 0.0)
            lst->y_shift = 0.0;
    }
}

void text_list_draw(text_list_t* lst)
{
    if(lst == NULL) return;
    if(lst->count == 0) return;

    float _y = lst->y;
    float yadj = (lst->text_height + 1.0);

    if(lst->downward)
    {
        _y -= ((lst->count-1) * yadj);
    }

    _y += (lst->downward ? 1 : -1) * (yadj * (lst->y_shift/SHIFT_DURATION));

    for(int i = 0; i < lst->count; ++i)
    {
        if(lst->text[i] == NULL) continue;

        float opacity = 1.0;
        if(lst->fade_out)
            opacity = MIN(lst->durations[i]/FADEOUT_DURATION, 1.0);

        float _x = lst->x;
        if(lst->alignment == TEXT_ALIGN_RIGHT)
        {
            Vector2f _size = gfx_string_get_size(lst->scale, lst->text[i]);
            _x -= _size.x;
        }
        else if(lst->alignment == TEXT_ALIGN_CENTER)
        {
            Vector2f _size = gfx_string_get_size(lst->scale, lst->text[i]);
            _x -= _size.x/2.0;
        }

        gfx_draw_string(_x, _y, lst->colors[i], lst->scale, 0.0, opacity, lst->in_world, NO_DROP_SHADOW, 0, lst->text[i]);

        _y += lst->downward ? yadj : -yadj;
    }
}
