#pragma once

typedef struct
{
    float  fps;
    float  spf;
    double time_start;
    double time_last;
    double frame_fps;
    double frame_fps_hist[60];
    double frame_fps_avg;
} Timer;


void init_timer(void);

void timer_begin(Timer* timer);
void timer_set_fps(Timer* timer, float fps);
void timer_wait_for_frame(Timer* timer);
double timer_get_prior_frame_fps(Timer* timer);

double timer_get_elapsed(Timer* timer);
void timer_delay_us(int us);
double timer_get_time();

void stopwatch_start();
void stopwatch_capture(char* str);
void stopwatch_reset();
