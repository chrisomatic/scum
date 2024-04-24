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

void wav_print_metadata(WaveStream* stream)
{
    printf("Header:\n");
    printf("  riff: '%s'\n", stream->header.riff);
    printf("  total size: %u\n", stream->header.total_size);
    printf("  wave: '%s'\n", stream->header.wave);
    printf("  fmt: '%s'\n", stream->header.fmt_chunk_marker);
    printf("  fmt length: %u\n", stream->header.fmt_length);
    printf("  fmt type: %u  (%s)\n", stream->header.fmt_type, stream->fmt_str);
    printf("  channels: %u\n", stream->header.channels);
    printf("  sample rate: %u\n", stream->header.sample_rate);
    printf("  byte rate: %u\n", stream->header.byte_rate);
    printf("  block align: %u\n", stream->header.block_align);
    printf("  bits/sample: %u\n", stream->header.bits_per_sample);
    printf("  data header: '%s'\n", stream->header.data_chunk_header);
    printf("  data size: %u\n", stream->header.data_size);

    printf("Num Samples: %u\n", stream->num_samples);
    printf("Sample Size: %u bytes\n", stream->sample_size);
    printf("Duration: %.4f\n", stream->duration);
}


int wav_stream_get_al_format(WaveStream* stream)
{
    int ch = stream->header.channels;

    if(stream->num_channel_bytes == 1)
    {
        if(ch == 1)
            return AL_FORMAT_MONO8;
        else if(ch == 2)
            return AL_FORMAT_STEREO8;
    }
    else if(stream->num_channel_bytes == 2)
    {
        if(ch == 1)
            return AL_FORMAT_MONO16;
        else if(ch == 2)
            return AL_FORMAT_STEREO16;
    }

    return -1;
}


WaveStream wav_stream_open(const char* fpath)
{
    WaveStream s = {0};

    s.fp = fopen(fpath, "r");
    if(!s.fp) return s;


    uint8_t buffer[4] = {0};

    fseek(s.fp, 0, SEEK_END);
    size_t sz = ftell(s.fp);
    rewind(s.fp);

    // printf("file size: %d\n", sz);

    WaveHeader header = {0};

    int read = 0;
    read = fread(s.header.riff, 4, 1, s.fp);

    read = fread(buffer, 4, 1, s.fp);
    s.header.total_size = convert_to_big_endian_u32(buffer);

    read = fread(s.header.wave, 4, 1, s.fp);

    read = fread(s.header.fmt_chunk_marker, 4, 1, s.fp);

    read = fread(buffer, 4, 1, s.fp);
    s.header.fmt_length = convert_to_big_endian_u32(buffer);

    read = fread(buffer, 2, 1, s.fp);
    s.header.fmt_type = convert_to_big_endian_u16(buffer);

    if(s.header.fmt_type == 1) // PCM (not compressed)
        s.fmt_str = "PCM";
    if(s.header.fmt_type == 3) // IEEE float
        s.fmt_str = "whatever";
    if(s.header.fmt_type == 6) // 8bit A law
        s.fmt_str = "A-law";
    if(s.header.fmt_type == 7) // 8bit mu law
        s.fmt_str = "Mu-law";

    read = fread(buffer, 2, 1, s.fp);
    s.header.channels = convert_to_big_endian_u16(buffer);

    read = fread(buffer, 4, 1, s.fp);
    s.header.sample_rate = convert_to_big_endian_u32(buffer);

    read = fread(buffer, 4, 1, s.fp);
    s.header.byte_rate = convert_to_big_endian_u32(buffer);

    read = fread(buffer, 2, 1, s.fp);
    s.header.block_align = convert_to_big_endian_u16(buffer);

    read = fread(buffer, 2, 1, s.fp);
    s.header.bits_per_sample = convert_to_big_endian_u16(buffer);

    read = fread(s.header.data_chunk_header, 4, 1, s.fp);

    read = fread(buffer, 4, 1, s.fp);
    s.header.data_size = convert_to_big_endian_u32(buffer);

    s.num_samples = (8 * s.header.data_size) / (s.header.channels * s.header.bits_per_sample);
    s.sample_size = (s.header.channels * s.header.bits_per_sample) / 8;  // size of each sample
    s.duration = (float)s.header.data_size / s.header.byte_rate;    // seconds

    s.num_channel_bytes = s.sample_size / s.header.channels;

    s.sample_idx = 0;

    return s;
}

void wav_stream_close(WaveStream* stream)
{
    fclose(stream->fp);
}


uint64_t wav_stream_get_chunk(WaveStream* stream, uint64_t num_samples, uint8_t* buf)
{
    if(num_samples == 0) num_samples = stream->num_samples;

    fseek(stream->fp, 44+(stream->sample_idx*stream->sample_size), SEEK_SET);

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
        fseek(stream->fp, 44, SEEK_SET);
        stream->sample_idx = 0;
    }

    return nbytes;


    //TODO
    // int bufi = 0;
    // char data_buffer[64];
    // for(int i = 0; i < num_samples; ++i)
    // {
    //     read = fread(data_buffer, stream->sample_size, 1, fp);
    //     if(read != 1)
    //     {
    //         printf("fread error\n");
    //         fclose(fp);
    //         return 1;
    //     }
    //     int channel_data = 0;
    //     int idx = 0;
    //     for(int c = 0; c < stream->header.channels; ++c)
    //     {
    //         if(stream->num_channel_bytes == 2)
    //         {
    //             data_buffer[idx];
    //             data_buffer[idx+1];
    //         }
    //         else if(num_channel_bytes == 1)
    //         {
    //             channel_data = data_buffer[idx];
    //         }
    //     }
    //     idx += stream->num_channel_bytes;
    //     stream->sample_idx++;
    //     if(stream->sample_idx >= stream->num_samples)
    //         break;
    // }



}


//TEMP
int audio_load_wav_file(char* filepath)
{

    WaveStream s = wav_stream_open(filepath);
    size_t size = s.num_samples * s.sample_size;
    uint8_t* buf = calloc(size, 1);

    uint64_t len = wav_stream_get_chunk(&s, s.num_samples, buf);

    int buffer;
    alGenBuffers(1, &buffer);

    int err = alGetError();
    if(err != AL_NO_ERROR)
    {
        LOGE("Failed to generate buffers, error: %d\n", err);
        free(buf);
        return -1;
    }

    alBufferData(buffer, wav_stream_get_al_format(&s), buf, len, s.header.sample_rate);
    free(buf);

    return buffer;
}
