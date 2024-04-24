#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "log.h"
#include "audio.h"

#define SOURCE_MAX 1024

bool audio_init()
{
    // Initialize Open AL

    ALCdevice* device = alcOpenDevice(NULL);
    if (!device)
    {
        LOGE("Failed to create device");
        return false;
    }

    ALCcontext* context = alcCreateContext(device, NULL);
    if(!context)
    {
        LOGE("Failed to create context");
        return false;
    }

    alcMakeContextCurrent(context);

    alGetError(); // clear previous errors
}

void audio_set_listener_pos(float x, float y, float z)
{
    alListener3f(AL_POSITION, x, y, z);
    alListener3f(AL_VELOCITY, 0.0, 0.0, 0.0);
}


int audio_source_create(bool looping)
{
    alGetError();

    int src;
    alGenSources(1, &src);

    int err = alGetError();
    if(err != AL_NO_ERROR)
    {
        LOGE("Failed to generate sources, error: %d\n", err);
        return -1;
    }

    alSourcef(src, AL_GAIN,  0.0);
    alSourcef(src, AL_PITCH, 1.0);
    alSourcef(src, AL_MAX_DISTANCE, 300.0);
    alSourcef(src, AL_ROLLOFF_FACTOR, 1.0);
    alSourcef(src, AL_REFERENCE_DISTANCE, 150.0);
    alSourcef(src, AL_MIN_GAIN, 0.5);
    alSourcef(src, AL_MAX_GAIN, 3.0);
    alSource3f(src, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(src, AL_VELOCITY, 0.0, 0.0, 0.0);
    alSourcei(src, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);

    alGetError(); // clear previous errors
    return src;
}

int audio_source_get_processed_buffers(int source)
{
    int val = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
    alGetError(); // clear previous errors
    return val;
}

int audio_source_get_buffer(int source)
{
    int val = 0;
    alGetSourcei(source, AL_BUFFER, &val);
    alGetError(); // clear previous errors
    return val;
}

void audio_source_set_volume(int src, float vol)
{
    alSourcef(src, AL_GAIN,  vol);
    alGetError(); // clear previous errors
}

void audio_source_delete(int source)
{
    if(alIsSource(source))
    {
        alDeleteSources(1, &source);
    }
    alGetError(); // clear previous errors
}

void audio_source_update_position(int src, float x, float y, float z)
{
    alSource3f(src,AL_POSITION, x, y, z);
}

void audio_source_assign_buffer(int source, int buffer)
{
    alSourcei(source, AL_BUFFER, buffer);
}

void audio_source_play(int source)
{
    alSourcePlay(source);
}

void audio_source_queue_buffer(int source, int buffer)
{
    // alSourceUnqueueBuffers(source, 1, &buffer);
    alSourceQueueBuffers(source, 1, &buffer);
    alGetError(); // clear previous errors
}

void audio_source_unqueue_buffer(int source, int buffer)
{
    alSourceUnqueueBuffers(source, 1, &buffer);
    alGetError(); // clear previous errors
}


static int _load_file_to_buffer(char* filepath, char** return_buf)
{
    // open test file
    FILE* fp = fopen(filepath,"rb");

    if(!fp)
    {
        LOGE("Failed to load file %s", filepath);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    rewind(fp);

    LOGI("Loaded file: %s, size = %d Bytes", filepath, len);

    *return_buf = malloc(len*sizeof(char));
    fread(*return_buf, sizeof(char), len, fp);
    fclose(fp);

    return len;

}

int audio_buffer_create()
{
    int buffer = -1;
    alGenBuffers(1, &buffer);
    alGetError(); // clear previous errors
    return buffer;
}

void audio_buffer_delete(int buffer)
{
    alDeleteBuffers(1, &buffer);
    alGetError(); // clear previous errors
}

void audio_buffer_set(int buffer, int format, uint8_t* audio_data, int len, int sample_rate)
{
    alBufferData(buffer, format, audio_data, len, sample_rate);
    alGetError(); // clear previous errors
}

int audio_load_file(char* filepath)
{
    char* buf;
    int len = _load_file_to_buffer(filepath, &buf);
    if(len == 0) return -1;

    int buffer;
    alGenBuffers(1, &buffer);

    int err = alGetError();
    if(err != AL_NO_ERROR)
    {
        LOGE("Failed to generate buffers, error: %d\n", err);
        free(buf);
        return -1;
    }

    alBufferData(buffer, AL_FORMAT_MONO8, buf, len, 44100);
    free(buf);

    return buffer;
}

int audio_load_music(char* filepath)
{
    char* buf;
    int len = _load_file_to_buffer(filepath, &buf);
    if(len == 0) return -1;

    int buffer;
    alGenBuffers(1, &buffer);

    int err = alGetError();
    if(err != AL_NO_ERROR)
    {
        LOGE("Failed to generate buffers, error: %d\n", err);
        free(buf);
        return -1;
    }

    alBufferData(buffer, AL_FORMAT_MONO16, buf, len, 44100); 
    free(buf);

    return buffer;

}

bool audio_source_is_playing(int source)
{
    int state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);

    return (state == AL_PLAYING);
}




// WAV

static uint32_t convert_to_big_endian_u32(uint8_t buf[4])
{
    uint32_t val = buf[0];
    val |= buf[1] << 8;
    val |= buf[2] << 16;
    val |= buf[3] << 24;
    return val;
}

static uint16_t convert_to_big_endian_u16(uint8_t buf[2])
{
    uint16_t val = buf[0];
    val |= buf[1] << 8;
    return val;
}

void wav_print_header(WaveHeader* h)
{
    const char* fmt_str;

    if(h->fmt_type == 1) // PCM (not compressed)
        fmt_str = "PCM";
    if(h->fmt_type == 3) // IEEE float
        fmt_str = "whatever";
    if(h->fmt_type == 6) // 8bit A law
        fmt_str = "A-law";
    if(h->fmt_type == 7) // 8bit mu law
        fmt_str = "Mu-law";

    printf("Header:\n");
    printf("  riff: '%s'\n", h->riff);
    printf("  total size: %u\n", h->total_size);
    printf("  wave: '%s'\n", h->wave);
    printf("  fmt: '%s'\n", h->fmt_chunk_marker);
    printf("  fmt length: %u\n", h->fmt_length);
    printf("  fmt type: %u  (%s)\n", h->fmt_type, fmt_str);
    printf("  channels: %u\n", h->channels);
    printf("  sample rate: %u\n", h->sample_rate);
    printf("  byte rate: %u\n", h->byte_rate);
    printf("  block align: %u\n", h->block_align);
    printf("  bits/sample: %u\n", h->bits_per_sample);
    printf("  data header: '%s'\n", h->data_chunk_header);
    printf("  data size: %u\n", h->data_size);
}

static int _get_al_format(AudioStream* stream)
{
    int ch = stream->num_channels;
    int num_channel_bytes = stream->sample_size / ch;

    if(num_channel_bytes == 1)
    {
        if(ch == 1)
            return AL_FORMAT_MONO8;
        else if(ch == 2)
            return AL_FORMAT_STEREO8;
    }
    else if(num_channel_bytes == 2)
    {
        if(ch == 1)
            return AL_FORMAT_MONO16;
        else if(ch == 2)
            return AL_FORMAT_STEREO16;
    }

    return -1;
}

AudioStream audio_stream_open(const char* fpath, AudioFileType type, WaveHeader* wave_header, uint64_t raw_sample_rate)
{
    AudioStream s = {0};
    s.type = AUDIO_FILE_NONE;

    s.fp = fopen(fpath, "r");
    if(!s.fp) return s;

    s.type = type;

    if(type == AUDIO_FILE_WAV)
    {
        WaveHeader header = {0};

        char buffer[4] = {0};

        int read = 0;
        read = fread(header.riff, 4, 1, s.fp);

        read = fread(buffer, 4, 1, s.fp);
        header.total_size = convert_to_big_endian_u32(buffer);

        read = fread(header.wave, 4, 1, s.fp);

        read = fread(header.fmt_chunk_marker, 4, 1, s.fp);

        read = fread(buffer, 4, 1, s.fp);
        header.fmt_length = convert_to_big_endian_u32(buffer);

        read = fread(buffer, 2, 1, s.fp);
        header.fmt_type = convert_to_big_endian_u16(buffer);

        read = fread(buffer, 2, 1, s.fp);
        header.channels = convert_to_big_endian_u16(buffer);

        read = fread(buffer, 4, 1, s.fp);
        header.sample_rate = convert_to_big_endian_u32(buffer);

        read = fread(buffer, 4, 1, s.fp);
        header.byte_rate = convert_to_big_endian_u32(buffer);

        read = fread(buffer, 2, 1, s.fp);
        header.block_align = convert_to_big_endian_u16(buffer);

        read = fread(buffer, 2, 1, s.fp);
        header.bits_per_sample = convert_to_big_endian_u16(buffer);

        read = fread(header.data_chunk_header, 4, 1, s.fp);

        read = fread(buffer, 4, 1, s.fp);
        header.data_size = convert_to_big_endian_u32(buffer);

        // int num_channel_bytes = s.sample_size / s.header.channels;

        s.num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
        s.sample_size = (header.channels * header.bits_per_sample) / 8;  // size of each sample
        s.duration = (float)header.data_size / header.byte_rate;    // seconds
        s.sample_rate = header.sample_rate;
        s.num_channels = header.channels;

        s.data_offset = 44;

        if(wave_header)
        {
            memcpy(wave_header, &header, sizeof(WaveHeader));
        }

    }
    else
    {
        fseek(s.fp, 0, SEEK_END);
        size_t sz = ftell(s.fp);
        rewind(s.fp);

        s.data_offset = 0;
        s.num_samples = sz;
        s.sample_rate = raw_sample_rate;
        s.sample_size = 1;
        s.duration = (float)s.num_samples / (float)s.sample_rate;
        s.num_channels = 1;
    }

    s.al_format = _get_al_format(&s);
    s.sample_idx = 0;

    return s;
}


void audio_stream_close(AudioStream* stream)
{
    if(stream->fp) fclose(stream->fp);
}



uint64_t audio_stream_get_chunk(AudioStream* stream, uint64_t num_samples, uint8_t* buf)
{
    if(num_samples == 0) num_samples = stream->num_samples;

    fseek(stream->fp, stream->data_offset+(stream->sample_idx*stream->sample_size), SEEK_SET);

    if(stream->sample_idx + num_samples > stream->num_samples)
    {
        num_samples = stream->num_samples - stream->sample_idx;
    }

    int nbytes = stream->sample_size*num_samples;
    int read = fread(buf, nbytes, 1, stream->fp);

    if(read != 1)
    {
        printf("read failed\n");
        return 0;
    }

    stream->sample_idx += num_samples;

    if(stream->sample_idx >= stream->num_samples)
    {
        // printf("back to beginning\n");
        // fseek(stream->fp, 44, SEEK_SET);
        stream->sample_idx = 0;
    }

    return nbytes;
}
