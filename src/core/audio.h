#pragma once

bool audio_init();
void audio_deinit();

void audio_set_listener_pos(float x, float y, float z);

// Audio Buffer
int audio_buffer_create();
void audio_buffer_delete(int buffer);
void audio_buffer_set(int buffer, int format, uint8_t* audio_data, int len, int sample_rate);
int audio_load_file(char* filepath);
int audio_load_music(char* filepath);

// Source
int audio_source_create(bool looping);
void audio_source_delete(int source);
void audio_source_update_position(int src, float x, float y, float z);
void audio_source_assign_buffer(int source, int buffer);
void audio_source_play(int source);
void audio_source_queue_buffer(int source, int buffer);
void audio_source_unqueue_buffer(int source, int buffer);
void audio_source_set_volume(int src, float vol);
int audio_source_get_processed_buffers(int source);
int audio_source_get_buffer(int source);
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

typedef enum
{
    AUDIO_FILE_NONE,
    AUDIO_FILE_RAW,
    AUDIO_FILE_WAV,
} AudioFileType;

typedef struct
{
    AudioFileType type;
    FILE* fp;
    int al_format;
    int data_offset;
    int sample_idx;
    uint64_t num_samples;
    uint16_t sample_size;
    uint8_t num_channels;
    uint32_t sample_rate;
    float duration;
} AudioStream;

void wav_print_header(WaveHeader* h);

AudioStream audio_stream_open(const char* fpath, AudioFileType type, WaveHeader* wave_header, uint64_t raw_sample_rate);
void audio_stream_close(AudioStream* stream);
uint64_t audio_stream_get_chunk(AudioStream* stream, uint64_t num_samples, uint8_t* buf);
