#pragma once

// fade_pattern
//   0: fade out
//   1: fade in
//   2: no fade
typedef struct
{
    int image;
    uint8_t sprite_index;
    uint32_t tint;
    float scale;
    float rotation;
    float opacity;
    uint8_t fade_pattern;
    float ttl;
    float ttl_start;
    Vector2f pos;
    uint8_t room;
} Decal;

#define MAX_DECALS  100
extern Decal decals[MAX_DECALS];
extern glist* decal_list;

void decal_init();
void decal_add(Decal d);
void decal_draw_all();
void decal_update_all(float dt);
void decal_clear_all();
