#pragma once

bool audio_init();
void audio_deinit();

void audio_set_listener_pos(float x, float y, float z);

// Audio Buffer
int audio_load_file(char* filepath);
int audio_load_music(char* filepath);

// Source
int audio_source_create(bool looping);
void audio_source_delete(int source);
void audio_source_update_position(int src, float x, float y, float z);
void audio_source_assign_buffer(int source, int buffer);
void audio_source_play(int source);
void audio_source_set_volume(int src, float vol);

bool audio_source_is_playing(int source);


// WAV
typedef struct
{
    char riff[5];
    uint32_t total_size;
    char wave[5];
    char fmt_chunk_marker[5];
    uint32_t fmt_length;
    uint16_t fmt_type;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_chunk_header[5];
    uint32_t data_size;
} WaveHeader;


typedef struct
{
    WaveHeader header;

    FILE* fp;

    const char* fmt_str;

    int sample_idx;

    uint64_t num_samples;
    uint64_t sample_size;
    float duration;
    int num_channel_bytes;

} WaveStream;

void wav_print_metadata(WaveStream* stream);
int wav_stream_get_al_format(WaveStream* stream);
WaveStream wav_stream_open(const char* fpath);
void wav_stream_close(WaveStream* stream);
uint64_t wav_stream_get_chunk(WaveStream* stream, uint64_t num_samples, uint8_t** buf);

//TEMP
int audio_load_wav_file(char* filepath);
