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

int audio_source_create()
{
    int src;
    alGenSources(1, &src);

    int err = alGetError();
    if(err != AL_NO_ERROR)
    {
        LOGE("Failed to generate sources, error: %d\n", err);
        return -1;
    }

    alSourcef(src, AL_GAIN,  1.0);
    alSourcef(src, AL_PITCH, 1.0);
    alSourcef(src, AL_MAX_DISTANCE, 300.0);
    alSourcef(src, AL_ROLLOFF_FACTOR, 1.0);
    alSourcef(src, AL_REFERENCE_DISTANCE, 150.0);
    alSourcef(src, AL_MIN_GAIN, 0.5);
    alSourcef(src, AL_MAX_GAIN, 3.0);
    alSource3f(src, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(src, AL_VELOCITY, 0.0, 0.0, 0.0);
    
    return src;
}

void audio_source_delete(int source)
{
    if(alIsSource(source))
    {
        alDeleteSources(1, &source);
    }
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

int audio_load_file(char* filepath)
{
    // open test file
    FILE* fp = fopen(filepath,"rb");

    if(!fp)
    {
        LOGE("Failed to load file %s", filepath);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    rewind(fp);

    LOGI("Loaded file: %s, size = %d Bytes", filepath, len);

    char* buf = malloc(len*sizeof(char));
    fread(buf, sizeof(char), len, fp);
    fclose(fp);

    int buffer;
    alGenBuffers(1, &buffer);

    int err = alGetError();
    if(err != AL_NO_ERROR)
    {
        LOGE("Failed to generate buffers, error: %d\n", err);
        return -1;
    }

    alBufferData(buffer, AL_FORMAT_MONO8, buf, len, 44100); 
    free(buf);

    return buffer;
}

bool audio_source_is_playing(int source)
{
    int state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);

    return (state == AL_PLAYING);
}
