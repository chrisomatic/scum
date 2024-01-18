#pragma once

typedef enum
{
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER
} text_align_t;

typedef struct
{
    int max;
    int count;
    char** text;
    float* durations;
    uint32_t* colors;
    bool downward;
    text_align_t alignment;
    bool in_world;
    float x;
    float y;
    float scale;
    // uint32_t color;
    float text_height;
    bool fade_out;
} text_list_t;



text_list_t* text_list_init(int max, float x, float y, float scale, bool downward, text_align_t alignment, bool in_world, bool fade_out);
void text_list_add(text_list_t* lst, uint32_t color, float duration, char* fmt, ...);
void text_list_update(text_list_t* lst, float _dt);
void text_list_draw(text_list_t* lst);
