#pragma once

#include "audio.h"

#define MAX_GAUDIO  100


typedef struct
{
    uint16_t id;

    int source;
    int buffer1;
    int buffer2;

    bool loop;
    bool dead;

    bool wav;
    WaveStream* stream;
    int chunk_size; // number of samples in a chunk

} Gaudio;

void gaudio_init();
Gaudio* gaudio_add(const char* filepath, bool wav, bool loop);
void gaudio_update(float dt);
void gaudio_remove(uint16_t id);
Gaudio* gaudio_get(uint16_t id);
