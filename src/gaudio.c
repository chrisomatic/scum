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
uint8_t* _buffer = NULL;

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

    _buffer = calloc(_BUFFER_SIZE, sizeof(uint8_t));

    audio_list = list_create((void*)audio_objs, MAX_GAUDIO, sizeof(Gaudio), false);

}

Gaudio* gaudio_add(const char* filepath, bool wav, bool loop)
{
    if(audio_list->count == audio_list->max_count) return NULL;

    Gaudio ga = {0};
    ga.id = get_id();
    ga.source = audio_source_create(false);
    ga.buffer1 = audio_buffer_create();
    ga.buffer2 = audio_buffer_create();
    ga.wav = wav;
    ga.loop = loop;
    ga.dead = false;

    ga.stream = audio_stream_open(filepath, wav ? AUDIO_FILE_WAV : AUDIO_FILE_RAW, NULL, wav ? 0 : 44100);
    if(ga.stream.type == AUDIO_FILE_NONE)
    {
        return NULL;
    }

    ga.chunk_size = _BUFFER_SIZE / ga.stream.sample_size;
    printf("chunk size: %d\n", ga.chunk_size);

    // audio_source_set_volume(ga.source, 5.0);

    bool add = list_add(audio_list, &ga);
    if(!add)
    {
        audio_stream_close(&ga.stream);
        return NULL;
    }

    _gaudio_start(&ga);

    return &audio_objs[audio_list->count-1];
}

static void _gaudio_start(Gaudio* ga)
{
    int n = 0;
    bool ret = _set_next_chunk(ga, &n);
    audio_buffer_set(ga->buffer1, ga->stream.al_format, _buffer, n, ga->stream.sample_rate);
    audio_source_queue_buffer(ga->source, ga->buffer1);

    // set up next buffer
    if(!ret)
    {
        n = 0;
        _set_next_chunk(ga, &n);
        audio_buffer_set(ga->buffer2, ga->stream.al_format, _buffer, n, ga->stream.sample_rate);
        audio_source_queue_buffer(ga->source, ga->buffer2);
    }

    audio_source_play(ga->source);
}


// fills the _buffer completely if able to
// returns true if ended
static bool _set_next_chunk(Gaudio* ga, int* num_bytes)
{
    memset(_buffer, 0, _BUFFER_SIZE);

    *num_bytes = 0;

    int n_chunks = 0;
    int rem_chunks = ga->chunk_size - n_chunks;


    for(;;)
    {
        if(rem_chunks == 0) break;

        uint64_t n = audio_stream_get_chunk(&ga->stream, rem_chunks, (_buffer + *num_bytes));

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
            // printf("%d / %d  ended\n", n_chunks, ga->stream->num_samples);
            ga->dead = true;
            return true;
        }
    }


    // printf("%d / %d  \n", n_chunks, ga->stream->num_samples);

    return false;
}

void gaudio_update(float dt)
{
    for(int i = audio_list->count-1; i >= 0; --i)
    {
        Gaudio* ga = &audio_objs[i];

        if(ga->dead) continue;

        // if(ga->wav)
        {


            int nump = audio_source_get_processed_buffers(ga->source);
            int cbuf = audio_source_get_buffer(ga->source);

            // printf("buffers: %d, %d   processed: %d, current buf: %d\n", ga->buffer1, ga->buffer2, nump, cbuf);

            int* b1 = &ga->buffer1;
            int* b2 = &ga->buffer2;

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
                audio_buffer_set(*b1, ga->stream.al_format, _buffer, n, ga->stream.sample_rate);
                audio_source_queue_buffer(ga->source, *b1);

            }
            else if(nump == 2)
            {

                audio_source_unqueue_buffer(ga->source, *b1);
                audio_source_unqueue_buffer(ga->source, *b2);

                int n = 0;
                bool ret = _set_next_chunk(ga, &n);
                audio_buffer_set(*b1, ga->stream.al_format, _buffer, n, ga->stream.sample_rate);
                audio_source_queue_buffer(ga->source, *b1);

                // set up next buffer
                if(!ret)
                {
                    n = 0;
                    _set_next_chunk(ga, &n);
                    audio_buffer_set(*b2, ga->stream.al_format, _buffer, n, ga->stream.sample_rate);
                    audio_source_queue_buffer(ga->source, *b2);
                }

                audio_source_play(ga->source);

            }


        }


    }
}

//TODO
void gaudio_remove(uint16_t id)
{
    for(int i = 0; i < audio_list->count; ++i)
    {
        if(audio_objs[i].id == id)
        {
            list_remove(audio_list, i);
            return;
        }
    }
}

Gaudio* gaudio_get(uint16_t id)
{
    for(int i = 0; i < audio_list->count; ++i)
    {
        if(audio_objs[i].id == id)
        {
            return &audio_objs[i];
        }
    }

    return NULL;
}