#pragma once

#include "audio.h"

#define MAX_GAUDIO  100

//TODO:
// add support for callback when audio ends

typedef void (*gaudio_finished_cb)(uint16_t id);

typedef struct
{
    uint16_t id;

    int source;
    int buffer1;
    int buffer2;

    bool playing;
    bool loop;
    bool destroy;

    bool ending;

    AudioStream stream;
    int chunk_size; // number of samples in a buffer
    gaudio_finished_cb finished_cb;

} Gaudio;

void gaudio_init();
Gaudio* gaudio_add(const char* filepath, bool wav, bool loop, bool destroy, gaudio_finished_cb cb);
void gaudio_update(float dt);
void gaudio_remove(uint16_t id);
Gaudio* gaudio_get(uint16_t id);
void gaudio_pause(uint16_t id);
void gaudio_play(uint16_t id);
bool gaudio_playing(uint16_t id);
void gaudio_rewind(uint16_t id);
void gaudio_set_volume(uint16_t id, float vol);
