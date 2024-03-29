#pragma once

#include <stdint.h>
#include "math2d.h"
#include "glist.h"

#define MAX_PARTICLE_SPAWNERS 200
#define MAX_PARTICLES_PER_SPAWNER 500

typedef struct
{
    float init_min;
    float init_max;
    float rate;
} ParticleParam;

typedef struct
{
    uint8_t version;

    ParticleParam life;
    ParticleParam scale;
    ParticleParam velocity_x;
    ParticleParam velocity_y;
    ParticleParam opacity;
    ParticleParam angular_vel;

    uint32_t color1;
    uint32_t color2;
    uint32_t color3;

    float spawn_radius_min;
    float spawn_radius_max;
    float rotation_init_min;
    float rotation_init_max;
    float spawn_time_max;
    float spawn_time_min;
    int burst_count_min;
    int burst_count_max;
    int sprite_index;
    int img_index;
    bool use_sprite;
    bool blend_additive;

    char name[100];

} ParticleEffect;

typedef struct
{
    Vector2f pos;
    Vector2f vel;
    uint32_t color;
    float angular_vel;
    float rotation;
    float scale;
    float opacity;
    float life;
    float life_max;
} Particle;

typedef struct
{
    int id;
    Vector2f pos;
    int z;
    ParticleEffect effect;
    float life;
    float life_max;
    float spawn_time;
    float spawn_time_max;
    Particle particles[MAX_PARTICLES_PER_SPAWNER];
    glist* particle_list;
    bool mortal;
    bool in_world;
    bool dead;
    bool hidden;
    int  userdata;
} ParticleSpawner;

extern int particles_image;
extern ParticleSpawner spawners[MAX_PARTICLE_SPAWNERS];
extern glist* spawner_list;

void particles_init();
ParticleSpawner* particles_spawn_effect(float x, float y, int z, ParticleEffect* effect, float lifetime, bool in_world, bool hidden);
ParticleSpawner* get_spawner_by_id(int id);
void particles_respawn_effect(ParticleSpawner* spawner, float x, float y, float lifetime, bool in_world, bool hidden);
void particles_clear(ParticleSpawner* spawner);
void particles_update(double delta_t);
void particles_show_spawner(int id, bool show);
void particles_draw_spawners_all();
void particles_draw_layer(int z);
void particles_draw_spawner(ParticleSpawner* spawner, bool ignore_light, bool add_to_existing_batch);
void particles_delete_spawner_by_id(int id);
void particles_delete_spawner(ParticleSpawner* ps);
void particles_delete_all_spawners();

void print_particle(Particle* p);
void print_particle_effect(ParticleEffect* e);
