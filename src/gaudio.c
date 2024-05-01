#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "level.h"
#include "log.h"
#include "glist.h"
#include "gaudio.h"

glist* audio_list = NULL;
Gaudio audio_objs[MAX_GAUDIO] = {0};

#define _BUFFER_SIZE    6000
uint8_t stream_data_buffer[_BUFFER_SIZE] = {0};

static uint16_t id_counter = 1;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}


static void _gaudio_start(Gaudio* ga);
static bool _set_next_chunk(Gaudio* ga, int* num_bytes);


void gaudio_init()
{
    if(audio_list)
        return;

    audio_list = list_create((void*)audio_objs, MAX_GAUDIO, sizeof(Gaudio), false);
}

Gaudio* gaudio_add(const char* filepath, bool wav, bool loop, bool destroy, gaudio_finished_cb cb)
{
    if(!audio_list) return NULL;

    if(audio_list->count == audio_list->max_count) return NULL;

    Gaudio ga = {0};
    ga.id = get_id();
    ga.source = audio_source_create(false);
    ga.buffer1 = audio_buffer_create();
    ga.buffer2 = audio_buffer_create();
    ga.loop = loop;
    ga.destroy = destroy;
    ga.ending = false;
    ga.playing = false;
    ga.finished_cb = cb;

    ga.stream = audio_stream_open(filepath, wav ? AUDIO_FILE_WAV : AUDIO_FILE_RAW, NULL, wav ? 0 : 44100);
    if(ga.stream.type == AUDIO_FILE_NONE)
    {
        LOGE("[%s] Failed to open audio stream", filepath);
        return NULL;
    }

    ga.chunk_size = _BUFFER_SIZE / ga.stream.sample_size;

    // float dur = (float)ga.chunk_size / (float)ga.stream.num_samples * ga.stream.duration;
    float dur = (float)ga.chunk_size / (float)ga.stream.sample_rate * 1000;
    LOGI("[%s] id : %u, chunk size: %d (%.4f ms)", filepath, ga.id, ga.chunk_size, dur);

    // frame rate
    if(dur < 16)
    {
        LOGW("Consider increasing stream_data_buffer size");
    }

    bool add = list_add(audio_list, &ga);
    if(!add)
    {
        audio_stream_close(&ga.stream);
        return NULL;
    }

    Gaudio* _ga = &audio_objs[audio_list->count-1];

    // doesn't start playing, just loads initial buffers
    _gaudio_start(_ga);

    return _ga;
}

void gaudio_pause(uint16_t id)
{
    Gaudio* ga = gaudio_get(id);
    if(!ga) return;

    ga->playing = false;
    audio_source_pause(ga->source);
}

void gaudio_play(uint16_t id)
{
    if(role == ROLE_SERVER) return;
    Gaudio* ga = gaudio_get(id);
    if(!ga) return;

    ga->playing = true;
    audio_source_play(ga->source);
}

bool gaudio_playing(uint16_t id)
{
    Gaudio* ga = gaudio_get(id);
    if(!ga) return false;

    return ga->playing;
}

void gaudio_rewind(uint16_t id)
{
    Gaudio* ga = gaudio_get(id);
    if(!ga) return;

    audio_source_stop(ga->source);
    audio_source_unqueue_buffer(ga->source, ga->buffer1);
    audio_source_unqueue_buffer(ga->source, ga->buffer2);

    ga->stream.sample_idx = 0;
    _gaudio_start(ga);
}

void gaudio_set_volume(uint16_t id, float vol)
{
    Gaudio* ga = gaudio_get(id);
    if(!ga) return;

    audio_source_set_volume(ga->source, vol);
}

void gaudio_remove(uint16_t id)
{
    if(!audio_list) return;

    for(int i = 0; i < audio_list->count; ++i)
    {
        if(audio_objs[i].id == id)
        {
            Gaudio* ga = &audio_objs[i];
            audio_source_stop(ga->source);
            audio_stream_close(&ga->stream);
            audio_source_delete(ga->source);
            audio_buffer_delete(ga->buffer1);
            audio_buffer_delete(ga->buffer2);
            list_remove(audio_list, i);
            return;
        }
    }
}

static void _gaudio_start(Gaudio* ga)
{
    int n = 0;
    bool ret = _set_next_chunk(ga, &n);
    audio_buffer_set(ga->buffer1, ga->stream.al_format, stream_data_buffer, n, ga->stream.sample_rate);
    // printf("[%u] setting buffer %d: %d\n", ga->id, ga->buffer1, n);
    audio_source_queue_buffer(ga->source, ga->buffer1);

    // set up next buffer
    if(!ret)
    {
        n = 0;
        _set_next_chunk(ga, &n);
        audio_buffer_set(ga->buffer2, ga->stream.al_format, stream_data_buffer, n, ga->stream.sample_rate);
        // printf("[%u] setting buffer %d: %d\n", ga->id, ga->buffer2, n);
        audio_source_queue_buffer(ga->source, ga->buffer2);
    }

    // audio_source_play(ga->source);
}


// fills the stream_data_buffer completely if able to
// returns true if ended
static bool _set_next_chunk(Gaudio* ga, int* num_bytes)
{
    memset(stream_data_buffer, 0, _BUFFER_SIZE);

    *num_bytes = 0;

    int n_chunks = 0;
    int rem_chunks = ga->chunk_size - n_chunks;


    for(;;)
    {
        if(rem_chunks == 0) break;

        uint64_t n = audio_stream_get_chunk(&ga->stream, rem_chunks, (stream_data_buffer + *num_bytes));

        if(n == 0)
        {
            printf("failed to read file\n");
            break;
        }

        *num_bytes += n;
        n_chunks = *num_bytes / ga->stream.sample_size;
        rem_chunks = ga->chunk_size - n_chunks;

        bool end = (ga->stream.sample_idx == 0);
        if(end && !ga->loop)
        {
            // printf("[%u] (%d) %d / %d  ended\n", ga->id, ga->stream.sample_idx, n_chunks, ga->stream.num_samples);
            ga->ending = true;
            return true;
        }
    }


    // printf("[%u] (%d) %d / %d  \n", ga->id, ga->stream.sample_idx, n_chunks, ga->stream.num_samples);

    return false;
}

void gaudio_update(float dt)
{
    if(!audio_list) return;

    for(int i = audio_list->count-1; i >= 0; --i)
    {
        Gaudio* ga = &audio_objs[i];

        if(!ga->playing) continue;


        int nump = audio_source_get_processed_buffers(ga->source);
        int cbuf = audio_source_get_buffer(ga->source);

        // printf("[%u] buffers: %d, %d   processed: %d, current buf: %d\n", ga->id, ga->buffer1, ga->buffer2, nump, cbuf);

        int* b1 = &ga->buffer1;
        int* b2 = &ga->buffer2;

        if(ga->ending)
        {
            if(cbuf == 0)
            {
                if(ga->finished_cb) ga->finished_cb(ga->id);

                if(ga->destroy)
                {
                    gaudio_remove(ga->id);
                }
                else
                {
                    ga->playing = false;
                    gaudio_rewind(ga->id);
                }
                // printf("removing\n");
            }
            continue;
        }

        if(nump == 1)
        {
            if(cbuf == ga->buffer1)
            {
                b1 = &ga->buffer2;
                b2 = &ga->buffer1;
            }

            audio_source_unqueue_buffer(ga->source, *b1);
            int n = 0;
            bool ret = _set_next_chunk(ga, &n);
            audio_buffer_set(*b1, ga->stream.al_format, stream_data_buffer, n, ga->stream.sample_rate);
            // printf("[%u] setting buffer %d: %d\n", ga->id, *b1, n);
            audio_source_queue_buffer(ga->source, *b1);

        }
        else if(nump == 2)
        {
            audio_source_unqueue_buffer(ga->source, *b1);
            audio_source_unqueue_buffer(ga->source, *b2);

            int n = 0;
            bool ret = _set_next_chunk(ga, &n);
            audio_buffer_set(*b1, ga->stream.al_format, stream_data_buffer, n, ga->stream.sample_rate);
            // printf("[%u] setting buffer %d: %d\n", ga->id, *b1, n);
            audio_source_queue_buffer(ga->source, *b1);

            // set up next buffer
            if(!ret)
            {
                n = 0;
                _set_next_chunk(ga, &n);
                audio_buffer_set(*b2, ga->stream.al_format, stream_data_buffer, n, ga->stream.sample_rate);
                // printf("[%u] setting buffer %d: %d\n", ga->id, *b2, n);
                audio_source_queue_buffer(ga->source, *b2);
            }

            audio_source_play(ga->source);

        }

    }
}



Gaudio* gaudio_get(uint16_t id)
{
    if(!audio_list) return NULL;

    for(int i = 0; i < audio_list->count; ++i)
    {
        if(audio_objs[i].id == id)
        {
            return &audio_objs[i];
        }
    }

    return NULL;
}
