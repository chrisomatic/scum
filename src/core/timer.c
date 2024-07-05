#include "headers.h"
#if _WIN32
#include <profileapi.h>
#else
#include <sys/time.h>
#endif

#include <float.h>
#include "timer.h"

static struct
{
    bool monotonic;
    uint64_t  frequency;
    uint64_t  offset;
} _timer;

static double _fps_hist[60] = {0};
static int _fps_hist_count = 0;
static int _fps_hist_max_count = 0;

// used for profiling
double _stopwatch_start = 0.0;
double _stopwatch_time = 0.0;
double _stopwatch_time_prior  = 0.0;

#if _WIN32
void usleep(__int64 usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

static uint64_t get_timer_value(void)
{
#if _WIN32
    uint64_t counter;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);
    return counter;
#else
#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    if (_timer.monotonic)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * (uint64_t)1000000000 + (uint64_t)ts.tv_nsec;
    }
    else
#endif

    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;

    }
#endif
}

void init_timer(void)
{
#if _WIN32
    uint64_t freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    _timer.monotonic = false;
    _timer.frequency = freq;
#else

#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        _timer.monotonic = true;
        _timer.frequency = 1000000000;
    }
    else
#endif
    {
        _timer.monotonic = false;
        _timer.frequency = 1000000;
    }
#endif
    _timer.offset = get_timer_value();

}


static double get_time()
{
    return (double) (get_timer_value() - _timer.offset) / (double)_timer.frequency;
}

void timer_begin(Timer* timer)
{
    timer->time_start = get_time();
    timer->time_last = timer->time_start;
    timer->frame_fps = 0.0f;
    timer->frame_fps_avg = 0.0f;
}

double timer_get_time()
{
    return get_time();
}

void timer_set_fps(Timer* timer, float fps)
{
    timer->fps = fps;
    timer->spf = 1.0f / fps;
}

void timer_wait_for_frame(Timer* timer)
{
    double now;
    for(;;)
    {
        now = get_time();
        if(now >= timer->time_last + timer->spf)
            break;
    }

    timer->frame_fps = 1.0f / (now - timer->time_last);
    timer->time_last = now;

    // calculate average FPS
    _fps_hist[_fps_hist_count++] = timer->frame_fps;

    if(_fps_hist_count >= 60)
        _fps_hist_count = 0;

    if(_fps_hist_max_count < 60)
        _fps_hist_max_count++;

    double fps_sum = 0.0;
    for(int i = 0; i < _fps_hist_max_count; ++i)
        fps_sum += _fps_hist[i];

    timer->frame_fps_avg = (fps_sum / _fps_hist_max_count);
}

double timer_get_elapsed(Timer* timer)
{
    double time_curr = get_time();
    return time_curr - timer->time_start;
}

double timer_get_prior_frame_fps(Timer* timer)
{
    return timer->frame_fps;
}

void timer_delay_us(int us)
{
    usleep(us);
}

void stopwatch_start()
{
    printf("[STOPWATCH] 00:00.000 (BEGIN)\n");
    _stopwatch_start = get_time();
    _stopwatch_time = 0.0;
}

void stopwatch_capture(char* str)
{
    _stopwatch_time_prior = _stopwatch_time;
    _stopwatch_time = (get_time() - _stopwatch_start);

    double time_left = _stopwatch_time;

    int min = (int)(time_left/60.0f); time_left -= (min*60.0);
    int sec = (int)(time_left);       time_left -= (sec);
    int ms = (int)(time_left*1000.0); 

    float delta_time = _stopwatch_time - _stopwatch_time_prior;

    //printf("[STOPWATCH] %02d:%02d.%03d [delta: %6.4f ms] (%s)\n", min, sec, ms, delta_time*1000.0, str);
    printf("[STOPWATCH] %08.4f ms [delta: %08.4f ms] (%s)\n", _stopwatch_time*1000.0, delta_time*1000.0, str);
}

void stopwatch_reset()
{
    _stopwatch_time = 0.0;
}
