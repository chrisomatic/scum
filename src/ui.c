#include "main.h"
#include "gfx.h"
#include "window.h"
#include "ui.h"

#define FADE_TIME 0.4

char message_mouse[UI_MESSAGE_MAX];
uint32_t message_mouse_color;

char message_small[UI_MESSAGE_MAX];
float message_small_duration;

char message_title[UI_MESSAGE_MAX];
float message_title_duration;
float message_title_duration_max;
float message_title_scale;
uint32_t message_title_color;

static void ui_message_draw_small();
static void ui_message_draw_title();
static void ui_message_draw_mouse();

void ui_message_set_small(float duration, char* fmt, ...)
{
    static char temp[UI_MESSAGE_MAX] = {0};
    memset(temp, 0, sizeof(char)*UI_MESSAGE_MAX);
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(temp, UI_MESSAGE_MAX, fmt, args);
    va_end(args);

    if(ret < 0 || ret >= UI_MESSAGE_MAX)
    {
        LOGE("Failed to format message_small (%d)", ret);
        return;
    }

    memcpy(message_small, temp, sizeof(char)*UI_MESSAGE_MAX);
    message_small_duration = duration;
}

void ui_message_set_title(float duration, uint32_t color, float scale, char* fmt, ...)
{
    static char temp[UI_MESSAGE_MAX] = {0};
    memset(temp, 0, sizeof(char)*UI_MESSAGE_MAX);
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(temp, UI_MESSAGE_MAX, fmt, args);
    va_end(args);

    if(ret < 0 || ret >= UI_MESSAGE_MAX)
    {
        LOGE("Failed to format message_title (%d)", ret);
        return;
    }

    memcpy(message_title, temp, sizeof(char)*UI_MESSAGE_MAX);

    message_title_duration = duration;
    message_title_duration_max = duration;
    message_title_scale = scale;
    message_title_color = color;
}

void ui_message_set_mouse(uint32_t color, char* fmt, ...)
{
    static char temp[UI_MESSAGE_MAX] = {0};
    memset(temp, 0, sizeof(char)*UI_MESSAGE_MAX);
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(temp, UI_MESSAGE_MAX, fmt, args);
    va_end(args);

    if(ret < 0 || ret >= UI_MESSAGE_MAX)
    {
        LOGE("Failed to format message_mouse (%d)", ret);
        return;
    }

    memcpy(message_mouse, temp, sizeof(char)*UI_MESSAGE_MAX);

    message_mouse_color = color;
}

void ui_message_clear_mouse()
{
    message_mouse[0] = 0;
}


void ui_update(float dt)
{
    if(message_small_duration > 0.0)
        message_small_duration -= dt;

    if(message_title_duration > 0.0)
        message_title_duration -= dt;
}

void ui_draw()
{
    ui_message_draw_small();
    ui_message_draw_title();
    ui_message_draw_mouse();
}

static void ui_message_draw_small()
{
    if(message_small_duration <= 0)
        return;

    if(!message_small)
        return;

    if(strlen(message_small) == 0)
        return;

    float scale = 0.25 * ascale;

    Vector2f size = gfx_string_get_size(scale, message_small);

    float pad = 10.0;

    // bottom right
    Rect bg = {0};
    bg.w = size.x + pad;
    bg.h = size.y + pad;
    // bg.x = bg.w/2.0 + 1.0; // bottom left
    bg.x = view_width - bg.w/2.0 + 1.0; // bottom right
    bg.y = view_height - bg.h/2.0 - 1.0;

    gfx_draw_rect(&bg, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 0.6, true, NOT_IN_WORLD);

    float tx = bg.x - bg.w/2.0 + pad/2.0;
    float ty = bg.y - bg.h/2.0 + pad/4.0;
    gfx_draw_string(tx, ty, COLOR_WHITE, scale, NO_ROTATION, 0.9, NOT_IN_WORLD, NO_DROP_SHADOW, 0, message_small);
}

static void ui_message_draw_title()
{
    if(message_title_duration <= 0)
        return;

    if(!message_title)
        return;

    if(strlen(message_title) == 0)
        return;

    float scale = message_title_scale * ascale;

    Vector2f size = gfx_string_get_size(scale, message_title);

    float pad = 10.0;

    float duration_delta = message_title_duration_max - message_title_duration;

    float offset_y = 0.0;
    float offset_amt = FADE_TIME*20.0;

    if(duration_delta < FADE_TIME)
        offset_y = offset_amt*(1.0 - (duration_delta/FADE_TIME));
    else if(message_title_duration < FADE_TIME)
        offset_y = -offset_amt*(1.0 - (message_title_duration/FADE_TIME));

    Rect bg = {0};
    bg.x = (view_width - bg.w)/2.0;
    bg.y = (view_height - bg.h)/5.0 + offset_y;
    bg.w = size.x + pad;
    bg.h = size.y + pad;

    float tx = bg.x - bg.w/2.0 + pad/2.0;
    float ty = bg.y - bg.h/2.0 + pad/4.0;

    float opacity = 1.0;
    if(message_title_duration < FADE_TIME)
    {
        opacity = (message_title_duration / FADE_TIME);
    }
    else if(message_title_duration > message_title_duration_max - FADE_TIME)
    {
        opacity = duration_delta / FADE_TIME;
    }

    gfx_draw_string(tx, ty, message_title_color, scale, NO_ROTATION, opacity, NOT_IN_WORLD, DROP_SHADOW, 0, message_title);
}

static void ui_message_draw_mouse()
{
    if(!message_mouse)
        return;

    if(strlen(message_mouse) == 0)
        return;

    float scale = 0.09 * ascale;

    Vector2f size = gfx_string_get_size(scale, message_mouse);

    // float pad = 10.0;

    // Rect bg = {0};
    // bg.w = size.x + pad;
    // bg.h = size.y + pad;
    // bg.x = bg.w/2.0 + 1.0;
    // bg.y = view_height - bg.h/2.0 - 1.0;
    // gfx_draw_rect(&bg, COLOR_BLACK, NOT_SCALED, NO_ROTATION, 0.6, true, NOT_IN_WORLD);

    float tx = mouse_world_x+1.0;
    float ty = mouse_world_y+1.0;
    gfx_draw_string(tx, ty, message_mouse_color, scale, NO_ROTATION, 0.7, IN_WORLD, DROP_SHADOW, 0, message_mouse);
}
