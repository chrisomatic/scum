#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "gfx.h"
#include "core/text_list.h"
#include "log.h"
#include "entity.h"
#include "player.h"
#include "creature.h"
#include "explosion.h"
#include "lighting.h"
#include "status_effects.h"
#include "projectile.h"
#include "effects.h"
#include "decal.h"
#include "audio.h"
#include "str.h"
#include "core/files.h"

Projectile projectiles[MAX_PROJECTILES];
Projectile prior_projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static ProjectileOrbital orbitals[MAX_ORBITALS] = {0};
static int orbital_count = 0;

static int audio_buffer_explode = -1;

// @TEMP
static const uint32_t orbital_colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_ORANGE,
    COLOR_CYAN,
    COLOR_PURPLE,
    COLOR_PINK,
    COLOR_YELLOW,
    COLOR_WHITE,
    COLOR_BLACK
};

static float perk_drop_table[] =
{
12.6024, 9.8255, 7.7696,
 1.1972, 3.5372, 4.6618,
 0.0236, 0.1474, 0.9712,
 8.8217, 6.8778, 5.4387,
 0.8979, 2.6529, 3.4963,
 0.0126, 0.0786, 0.5180,
10.0819, 7.8604, 6.2157,
 1.1972, 3.5372, 4.6618,
 0.0158, 0.0983, 0.6475,
 5.0410, 3.9302, 3.1078,
 0.4490, 1.3264, 1.7482,
 0.0047, 0.0295, 0.1942,
 8.1916, 6.3866, 5.0502,
 1.0476, 3.0950, 4.0790,
 0.0236, 0.1474, 0.9712,
 6.3012, 4.9127, 3.8848,
 0.7483, 2.2107, 2.9136,
 0.0142, 0.0884, 0.5827,
 8.8217, 6.8778, 5.4387,
 1.1972, 3.5372, 4.6618,
 0.0189, 0.1179, 0.7770,
 6.9313, 5.4040, 4.2733,
 0.8979, 2.6529, 3.4963,
 0.0095, 0.0590, 0.3885,
 3.1506, 2.4564, 1.9424,
 0.2993, 0.8843, 1.1654,
 0.0047, 0.0295, 0.1942,
 5.0410, 3.9302, 3.1078,
 0.4490, 1.3264, 1.7482,
 0.0079, 0.0491, 0.3237,
 5.0410, 3.9302, 3.1078,
 0.4490, 1.3264, 1.7482,
 0.0079, 0.0491, 0.3237,
 5.0410, 3.9302, 3.1078,
 0.4490, 1.3264, 1.7482,
 0.0079, 0.0491, 0.3237,
 5.0410, 3.9302, 3.1078,
 0.4490, 1.3264, 1.7482,
 0.0079, 0.0491, 0.3237,
 0.0016, 0.0039, 0.0078,
 0.0016, 0.0039, 0.0078,
 0.0016, 0.0039, 0.0078,
 0.0016, 0.0039, 0.0078
};

static int projectile_image;

Gun gun_list[MAX_GUNS];
int gun_list_count = 0;

Gun room_gun_list[MAX_ROOM_GUNS];
int room_gun_count = 0;

static uint16_t id_counter = 1;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

static void projectile_remove(int index)
{
    list_remove(plist, index);
}

void projectile_init()
{
    plist = list_create((void*)projectiles, MAX_PROJECTILES, sizeof(Projectile), false);
    projectile_image = gfx_load_image("src/img/projectiles.png", false, false, 16, 16);

    // Audio Buffers
    audio_buffer_explode = audio_load_file("src/audio/splat1.wav");

    gun_refresh_list();

}

void projectile_clear_all()
{
    list_clear(plist);
}

static float calc_orbital_target(Projectile* proj)
{
    float delta_angle = (2*PI) / proj->orbital->count;
    float target_angle = (proj->orbital->base_angle*proj->orbital->speed_factor) + (proj->orbital_index * delta_angle);
    target_angle = fmod(target_angle,2*PI);

    float actual_orb_distance = proj->orbital->distance;

    switch(proj->orbital->evolution)
    {
        case PROJ_ORB_EVOLUTION_NONE:
            break;
        case PROJ_ORB_EVOLUTION_GROW_SHRINK:
        {
            float dist_delta = proj->orbital->base_angle;
            if(proj->orbital->base_angle >= PI)
            {
                dist_delta = (PI - (proj->orbital->base_angle - PI));
            }
            actual_orb_distance += 10*dist_delta;
        }
        break;
    }

    proj->orbital_pos_target.x =  (cosf(target_angle)*actual_orb_distance);
    proj->orbital_pos_target.y = -(sinf(target_angle)*actual_orb_distance);
}

void projectile_add(Vector3f pos, Vector3f* vel, uint8_t curr_room, uint8_t room_gun_index, float angle_deg, bool from_player, Physics* phys, uint16_t from_id)
{
    if(role == ROLE_CLIENT)
        return;

    Projectile proj = {0};

    proj.room_gun_index = room_gun_index; 
    Gun* proj_gun = &room_gun_list[proj.room_gun_index];

    proj.from_id = from_id;

    if(proj_gun->charging)
    {
        proj_gun->charging = false;
        float factor  = (proj_gun->charge_time / proj_gun->charge_time_max);

        switch(proj_gun->charge_type)
        {
            case CHARGE_TYPE_SCALE_DAMAGE:
            {
                proj_gun->damage_min *= (1.0 + factor);
                proj_gun->damage_max *= (1.0 + factor);
                proj_gun->scale1 *= (1.0 + factor);
                proj_gun->scale2 *= (1.0 + factor);
            }   break;
            case CHARGE_TYPE_SCALE_NUM:
            {
                int num = (int)(factor*proj_gun->num);
                proj_gun->num = MAX(1,num);
            } break;
            case CHARGE_TYPE_SCALE_BURST_COUNT:
            {
                int burst_count = (int)(factor*proj_gun->burst_count);
                proj_gun->burst_count = burst_count;
            } break;
        }
    }

    Rect vr = gfx_images[projectile_image].visible_rects[proj_gun->sprite_index];

    vr.w *= proj_gun->scale1;
    vr.h *= proj_gun->scale1;

    proj.color = proj_gun->color1;
    proj.sprite_index = proj_gun->sprite_index;
    proj.phys.height = vr.h;
    proj.phys.width =  vr.w;
    proj.phys.length = vr.h;
    proj.phys.scale = proj_gun->scale1;
    proj.phys.vr = vr;
    proj.phys.pos.x = pos.x;
    proj.phys.pos.y = pos.y;
    proj.phys.pos.z = pos.z;
    proj.phys.mass = 1.0;
    proj.phys.base_friction = proj_gun->air_friction;
    proj.phys.speed = proj_gun->speed;
    proj.phys.max_velocity = 2.0*proj_gun->speed;
    proj.phys.radius = (MAX(proj.phys.length, proj.phys.width) / 2.0) * proj_gun->scale1;
    proj.phys.rotation_deg = angle_deg;

    bool bouncy = RAND_FLOAT(0.0,1.0) <= proj_gun->bounce_chance;
    proj.phys.amorphous = bouncy ? false : true;
    proj.phys.elasticity = bouncy ? 1.0 : 0.1;

    proj.phys.curr_room = curr_room;
    proj.from_player = from_player;
    proj.angle_deg = angle_deg;
    proj.ttl = proj_gun->lifetime;
    proj.wave_time = proj_gun->wave_period / 2.0;
    proj.wave_dir = 1;

    // TODO: Move bursting code to this function

    if(proj_gun->orbital)
    {
        // assign projectile to orbital
        ProjectileOrbital* orbital = NULL;

        bool new_orbital = true;

        for(int o = 0; o < MAX_ORBITALS; ++o)
        {
            ProjectileOrbital* orb = &orbitals[o];

            if(!orb->body && !orbital)
            {
                orbital = orb;
                continue;
            }

            if(phys == orb->body && proj_gun->orbital_distance == orb->distance)
            {
                new_orbital = false;
                orbital = orb;
                break;
            }
        }

        if(!orbital)
        {
            LOGW("Hit max orbitals! Failed to add projectile! (Max: %d)\n", MAX_ORBITALS);
            return;
        }

        if(new_orbital)
        {
            orbital_count++;
            printf("New orbital! Count: %d\n", orbital_count);
            orbital->body = phys;
            orbital->distance = proj_gun->orbital_distance;
            orbital->speed_factor = proj_gun->orbital_speed_factor;
            orbital->max_count = proj_gun->orbital_max_count;
            orbital->base_angle = 0.0;
            orbital->evolution = PROJ_ORB_EVOLUTION_NONE;
        }

        if(orbital->count >= orbital->max_count)
            return;

        orbital->count++;
        orbital->lerp_t = 0.0;
        orbital->lerp_factor = 5.0;

        proj.orbital = orbital;
        proj.orbital_index = orbital->count-1;

        calc_orbital_target(&proj);

        proj.orbital_pos_prior.x = 0.0;
        proj.orbital_pos_prior.y = 0.0;

        proj.orbital_pos.x = proj.orbital_pos_target.x;
        proj.orbital_pos.y = proj.orbital_pos_target.y;

        // override color
        proj.color = orbital_colors[proj.orbital_index % 10];

        // update all orbital projectiles
        for(int i = plist->count-1; i >= 0; --i)
        {
            Projectile* proj2 = &projectiles[i];

            if(proj2->orbital == NULL)
                continue;

            if(proj2->orbital->body == NULL)
                continue;

            if(proj2->orbital->body != proj.orbital->body)
                continue;

            if(proj2->orbital->distance != proj.orbital->distance)
                continue;

            proj2->orbital_pos_prior.x = proj2->orbital_pos.x;
            proj2->orbital_pos_prior.y = proj2->orbital_pos.y;
        }
    }

    proj.cluster_stage = proj_gun->cluster_stage;

    float spread = proj_gun->spread_type == SPREAD_TYPE_RANDOM ? proj_gun->spread/2.0 : proj_gun->spread / proj_gun->num;
    float spread_angle_start = angle_deg - (proj_gun->spread/2.0);

    uint16_t target_ids[32] = {0};
    int target_count = 0;

    for(int i = 0; i < proj_gun->num; ++i)
    {

        for(int j = 0; j < proj_gun->burst_count+1; ++j)
        {
            Projectile p = {0};
            memcpy(&p, &proj, sizeof(Projectile));
            p.id = get_id();

            if(j > 0)   // burst
            {
                p.tts = 1/60.0*((j)*(1.0/proj_gun->burst_rate));
                p.shooter = phys;
            }

            p.phys.ethereal = RAND_FLOAT(0.0,1.0) <= proj_gun->ghost_chance;
            p.fire = RAND_FLOAT(0.0,1.0) <= proj_gun->fire_damage;
            p.cold = RAND_FLOAT(0.0,1.0) <= proj_gun->cold_damage;
            p.lightning = RAND_FLOAT(0.0,1.0) <= proj_gun->lightning_damage;
            p.poison = RAND_FLOAT(0.0,1.0) <= proj_gun->poison_damage;
            bool homing = RAND_FLOAT(0.0,1.0) <= proj_gun->homing_chance;

            if(!FEQ0(spread))
            {
                if(proj_gun->spread_type == SPREAD_TYPE_RANDOM)
                {
                    p.angle_deg += RAND_FLOAT(-spread, spread);
                }
                else if(i > 0 && proj_gun->spread_type == SPREAD_TYPE_UNIFORM)
                {
                    p.angle_deg = angle_deg + (i*spread);
                    if(p.angle_deg > angle_deg + proj_gun->spread/2.0)
                        p.angle_deg += (360 - proj_gun->spread);
                }
            }

            float angle = RAD(p.angle_deg);

            p.phys.vel.x = +(proj_gun->speed)*cosf(angle) + vel->x;
            p.phys.vel.y = -(proj_gun->speed)*sinf(angle) + vel->y;
            p.phys.vel.z = vel->z;

            if(homing)
            {
                uint16_t id = 0;
                Physics* target = NULL;
                if(p.from_player)
                    target = entity_get_closest_to(&p.phys, p.phys.curr_room, ENTITY_TYPE_CREATURE, target_ids, target_count, &id);
                else
                    target = entity_get_closest_to(&p.phys, p.phys.curr_room, ENTITY_TYPE_PLAYER, target_ids, target_count, &id);

                if(target)
                {
                    float tx = target->pos.x;
                    float ty = target->pos.y;

                    Vector2f v = {tx - p.phys.pos.x, ty - p.phys.pos.y};
                    normalize(&v);
                    p.angle_deg = calc_angle_deg(p.phys.pos.x, p.phys.pos.y, tx, ty);
                    p.phys.vel.x = v.x * proj_gun->speed;
                    p.phys.vel.y = v.y * proj_gun->speed;

                    target_ids[target_count++] = id;
                }
            }

            // p.source_explode = audio_source_create(false);
            // audio_source_assign_buffer(p.source_explode, audio_buffer_explode);

            list_add(plist, (void*)&p);
        }
    }
}

void projectile_kill(Projectile* proj)
{
    if(role == ROLE_CLIENT)
        return;

    proj->phys.dead = true;

    //audio_source_play(proj->source_explode);
    //audio_source_delete(proj->source_explode);

    Gun* proj_gun = &room_gun_list[proj->room_gun_index];

    Gun gun;
    memcpy(&gun,proj_gun,sizeof(Gun));

    bool more_cluster = proj_gun->cluster && (proj->cluster_stage < (MAX_CLUSTER_STAGES) && proj->cluster_stage < (gun.cluster_stages));

    const float scale_particle_thresh = 0.25;
    float pscale = MIN(1.0, proj_gun->scale2);

    if(pscale > scale_particle_thresh && !more_cluster)
    {
        ParticleEffect splash = {0};
        int effect_index = EFFECT_SPLASH;

        if(!proj->phys.collided)
        {
            effect_index = EFFECT_DISSIPATE;
        }

        memcpy(&splash, &particle_effects[effect_index], sizeof(ParticleEffect));

        uint32_t color = proj->color;
        float r,g,b;
        gfx_color2floats(color, &r, &g, &b);

        uint32_t c1 = COLOR2(r,g,b);
        uint32_t c2 = COLOR2(0.66*r, 0.66*g, 0.66*b);
        uint32_t c3 = COLOR2(0.33*r, 0.33*g, 0.33*b);

        float lifetime = 0.4;

        float projy = proj->phys.pos.y - (proj->phys.vr.h + proj->phys.pos.z)/2.0;

        if(role == ROLE_SERVER)
        {
            NetEvent ev = {
                .type = EVENT_TYPE_PARTICLES,
                .data.particles.effect_index = effect_index,
                .data.particles.pos = { proj->phys.pos.x, projy },
                .data.particles.scale = pscale,
                .data.particles.color1 = c1,
                .data.particles.color2 = c2,
                .data.particles.color3 = c3,
                .data.particles.lifetime = lifetime,
                .data.particles.room_index = proj->phys.curr_room,
            };

            net_server_add_event(&ev);

        }
        else
        {
            splash.color1 = c1;
            splash.color2 = c2;
            splash.color3 = c3;
            splash.scale.init_min *= pscale;
            splash.scale.init_max *= pscale;
            ParticleSpawner* ps = particles_spawn_effect(proj->phys.pos.x, projy, proj->phys.pos.z, &splash, lifetime, true, false);
            if(ps != NULL) ps->userdata = (int)proj->phys.curr_room;
        }
    }

    if(role != ROLE_SERVER)
    {
        bool explosive = RAND_FLOAT(0.0,1.0) <= proj_gun->explosion_chance;

        if(explosive)
        {
            explosion_add(proj->phys.pos.x, proj->phys.pos.y, proj_gun->explosion_radius, proj_gun->explosion_rate, proj->phys.curr_room, proj->from_player);
        }
    }

    if(proj_gun->orbital)
    {
        proj->orbital->count--;
        //proj->orbital->lerp_t = 0.0;
        //proj->orbital->lerp_factor = 0.2;

        int index = proj->orbital_index;

        // update all orbital projectiles
        for(int i = plist->count - 1; i >= 0; --i)
        {
            Projectile* proj2 = &projectiles[i];

            if(proj2->orbital == NULL)
                continue;

            if(proj2->orbital->body == NULL)
                continue;

            if(proj2->orbital->body != proj->orbital->body)
                continue;

            if(proj2->orbital->distance != proj->orbital->distance)
                continue;

            if(proj2->orbital_index > index)
                proj2->orbital_index--;

            proj2->orbital_pos_prior.x = proj2->orbital_pos.y;
            proj2->orbital_pos_prior.y = proj2->orbital_pos.y;
        }

        if(proj->orbital->count <= 0)
        {
            projectile_orbital_kill(proj->orbital);
        }
    }

    if(proj_gun->cluster)
    {

        if(!more_cluster)
            return;

        // printf("cluster\n");
        Gun gun;
        memcpy(&gun, proj_gun, sizeof(Gun));

        if(proj->cluster_stage >= (3) || proj->cluster_stage >= (gun.cluster_stages))
        {
            return;
        }

        gun.cluster = true;

        gun.scale1 = gun.cluster_scales[proj->cluster_stage]*proj->phys.scale;
        gun.scale2 = gun.scale1;

        gun.speed *= 0.6;

        gun.num = gun.cluster_num[proj->cluster_stage];

        float angle_deg = proj->angle_deg;

        gun.spread = 60.0;
        gun.cluster_stage = proj->cluster_stage+1;

        proj->phys.vel.x /= 2.0;
        proj->phys.vel.y /= 2.0;

        Vector3f pos = {proj->phys.pos.x, proj->phys.pos.y, proj->phys.height + proj->phys.pos.z};
        Vector3f vel = {proj->phys.vel.x, proj->phys.vel.y, 0.0};

        vel.z = gun.gravity_factor*120.0;

        projectile_add(pos, &vel, proj->phys.curr_room, proj->room_gun_index, angle_deg, proj->from_player, &proj->phys, proj->from_id);
    }
}

void projectile_update_all(float dt)
{
    // printf("projectile update\n");

    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

        Gun* proj_gun = &room_gun_list[proj->room_gun_index];

        if(role == ROLE_CLIENT)
        {
            projectile_lerp(proj, dt);
            continue;
        }

        if(proj->phys.dead)
            continue;

        if(proj->tts > 0)
        {
            proj->tts -= dt;
            if(proj->tts > 0) continue;
            //TODO
            if(proj->shooter)
            {
                proj->phys.pos.x = proj->shooter->pos.x;
                proj->phys.pos.y = proj->shooter->pos.y;
            }
        }

        if(proj->ttl <= 0)
        {
            projectile_kill(proj);
            continue;
        }

        float _dt = dt;

        if(proj->orbital && proj->orbital->body)
        {
            // make sure orbital projectile follows player through rooms
            proj->phys.curr_room = proj->orbital->body->curr_room;

            // evenly space the projectile in the orbital
            calc_orbital_target(proj);

            Vector2f pos = lerp2f(&proj->orbital_pos_prior, &proj->orbital_pos_target, proj->orbital->lerp_t);

            proj->orbital_pos.x = pos.x;
            proj->orbital_pos.y = pos.y;

            proj->phys.pos.x = pos.x + proj->orbital->body->pos.x;
            proj->phys.pos.y = pos.y + proj->orbital->body->pos.y;

            proj->phys.pos.z = proj->orbital->body->height / 2.0 + proj->orbital->body->pos.z;

            // set velocity for brevity? Although this isn't needed
            Vector2f f = {
                proj->phys.pos.x - proj->orbital->body->pos.x,
                proj->phys.pos.y - proj->orbital->body->pos.y
            };

            normalize(&f);

            proj->phys.vel.x = proj_gun->speed * f.y;
            proj->phys.vel.y = proj_gun->speed * -f.x;
        }
        else
        {
            RANGE(proj->ttl, 0.0, dt);

            // printf("%3d %.4f\n", i, delta_t);
            proj->ttl -= _dt;

            float angle = RAD(proj->angle_deg);
            float vel_magn = magn(proj->phys.vel);

            if(proj_gun->directional_accel != 0.0)
            {
                Vector2f f = {cos(angle), -sin(angle)};
                normalize(&f);

                float accel = proj_gun->directional_accel * 1500.0 * _dt;

                proj->phys.vel.x += (f.x * accel);
                proj->phys.vel.y += (f.y * accel);

                /*
                if(vel_magn > proj->phys.max_velocity)
                {
                    // cap vel
                    Vector2f c = {proj->phys.vel.x, proj->phys.vel.y};
                    normalize(&c);

                    proj->phys.vel.x = c.x * proj_gun->speed;
                    proj->phys.vel.y = c.y * proj_gun->speed;
                }
                */
            }

            if(proj->phys.base_friction > 0.0)
            {
                Vector2f c = {-proj->phys.vel.x, -proj->phys.vel.y};
                normalize(&c);

                float friction = _dt*4*proj_gun->speed*proj->phys.base_friction;

                float fx = MIN(ABS(proj->phys.vel.x), friction);
                float fy = MIN(ABS(proj->phys.vel.y), friction);

                proj->phys.vel.x += fx*c.x;
                proj->phys.vel.y += fy*c.y;
            }

            if(proj_gun->wave_amplitude > 0.0)
            {
                if(proj_gun->wave_period > 0.0)
                {
                    Vector2f f = {sin(angle), cos(angle)};
                    normalize(&f);

                    proj->wave_time += _dt;

                    if(proj->wave_time >= proj_gun->wave_period)
                    {
                        while(proj->wave_time >= proj_gun->wave_period)
                            proj->wave_time -= proj_gun->wave_period;

                        proj->wave_dir = proj->wave_dir == 1 ? -1 : 1;
                    }

                    float ampl = proj_gun->wave_amplitude;

                    f.x *= proj->wave_dir*ampl*10*_dt;
                    f.y *= proj->wave_dir*ampl*10*_dt;

                    //printf("f: %3.6f %3.6f\n", f.x, f.y);

                    proj->phys.vel.x += f.x;
                    proj->phys.vel.y += f.y;
                }
            }

            float cfactor = proj->ttl / proj_gun->lifetime;
            proj->color = gfx_blend_colors(proj_gun->color2, proj_gun->color1, cfactor);
            proj->phys.scale = lerp(proj_gun->scale2, proj_gun->scale1, cfactor);
            proj->phys.radius = (MAX(proj->phys.length, proj->phys.width) / 2.0) * proj->phys.scale;
        }

        if(!FEQ0(proj_gun->spin_factor))
        {
            proj->phys.rotation_deg += (proj_gun->spin_factor*1080.0*_dt);
            proj->phys.rotation_deg = fmod(proj->phys.rotation_deg, 360.0);
        }

        //printf("adding %f %f; vel: %f %f\n",f.x, f.y, proj->phys.vel.x, proj->phys.vel.y);

        proj->phys.prior_pos.x = proj->phys.pos.x;
        proj->phys.prior_pos.y = proj->phys.pos.y;
        proj->phys.prior_pos.z = proj->phys.pos.z;

        proj->phys.pos.x += _dt*proj->phys.vel.x;
        proj->phys.pos.y += _dt*proj->phys.vel.y;

        if(!proj_gun->orbital)
        {
            phys_apply_gravity(&proj->phys, proj_gun->gravity_factor, _dt);
        }

        // printf("proj->phys.pos.z: %.2f\n", proj->phys.pos.z);

        if(proj->phys.amorphous && proj->phys.pos.z <= 0.0)
        {
            proj->phys.collided = true; // hit floor
            projectile_kill(proj);
        }
    }

    for(int o = 0; o < orbital_count; ++o)
    {
        // update orbital time
        orbitals[o].base_angle += dt;
        orbitals[o].base_angle = fmod(orbitals[o].base_angle,2*PI);

        if(orbitals[o].lerp_t < 1.0)
        {
            orbitals[o].lerp_t += orbitals[o].lerp_factor*dt;
            orbitals[o].lerp_t = MIN(1.0, orbitals[o].lerp_t);
        }
    }

    // int count = 0;
    for(int i = plist->count - 1; i >= 0; --i)
    {
        if(projectiles[i].phys.dead)
        {
            // count++;
            projectile_remove(i);
        }
    }
    // if(count > 0) printf("removed %d\n", count);
}

void projectile_handle_collision(Projectile* proj, Entity* e)
{
    if(proj->phys.dead)
        return;

    Gun* gun = &room_gun_list[proj->room_gun_index];

    uint8_t curr_room = 0;
    Physics* phys = NULL;

    bool player_hitting_creature = proj->from_player && e->type == ENTITY_TYPE_CREATURE;

    if(player_hitting_creature)
    {
        Creature* c = (Creature*)e->ptr;
        if(c->friendly) return; // player can't hit friendly creatures
        if(c->phys.ethereal) return;
        // if(c->passive) return;

        curr_room = c->phys.curr_room;
        phys = &c->phys;
    }
    else if(!proj->from_player && e->type == ENTITY_TYPE_PLAYER)
    {
        Player* p = (Player*)e->ptr;

        curr_room = p->phys.curr_room;
        phys = &p->phys;
    }

    bool hit = false;

    if(phys)
    {
        if(phys->dead) return;
        if(proj->phys.curr_room != curr_room) return;

        Box proj_curr = {
            proj->phys.collision_rect.x,
            proj->phys.collision_rect.y,
            proj->phys.pos.z + phys->height/2.0,
            proj->phys.collision_rect.w*1.25,
            proj->phys.collision_rect.h*1.25,
            proj->phys.height*4.0,
        };

        float zb = phys->pos.z/2.0;
        if(phys->floating)
        {
            zb = 0.0;
        }

        Box check = {
            phys->collision_rect.x,
            phys->collision_rect.y,
            phys->pos.z + phys->height/2.0,
            phys->collision_rect.w*1.25,
            phys->collision_rect.h*1.25,
            phys->height*4.0,
        };

        hit = boxes_colliding(&proj_curr, &check);

        /*
        printf("\n");
        printf("Proj.   Box: {x: %5.2f, y: %5.2f, z: %5.2f}, {w: %5.2f, l: %5.2f, h: %5.2f}\n", proj_curr.x, proj_curr.y, proj_curr.z, proj_curr.w, proj_curr.l, proj_curr.h);
        printf("Entity  Box: {x: %5.2f, y: %5.2f, z: %5.2f}, {w: %5.2f, l: %5.2f, h: %5.2f}\n", check.x, check.y, check.z, check.w, check.l, check.h);
        printf("\n");
        */

        if(hit)
        {

            if(player_hitting_creature)
            {
                players[proj->from_id].total_hits++;
                players[proj->from_id].level_hits++;
                players[proj->from_id].room_hits++;
            }

            bool penetrate = RAND_FLOAT(0.0,1.0) <= gun->penetration_chance;
            if(!penetrate)
            {
                CollisionInfo ci = {0.0,0.0};
                proj->phys.pos.x = proj->phys.prior_pos.x;
                proj->phys.pos.y = proj->phys.prior_pos.y;

                phys_collision_correct(&proj->phys,phys,&ci);

                proj->phys.collided = true;
                projectile_kill(proj);
            }

            if(proj->cold)
            {
                status_effects_add_type(phys,proj->phys.curr_room, STATUS_EFFECT_COLD);
            }

            if(proj->poison)
            {
                status_effects_add_type(phys,proj->phys.curr_room, STATUS_EFFECT_POISON);
            }

            if(proj->fire)
            {
                status_effects_add_type(phys,proj->phys.curr_room, STATUS_EFFECT_FIRE);
            }

            switch(e->type)
            {
                case ENTITY_TYPE_PLAYER:
                    player_hurt((Player*)e->ptr, RAND_FLOAT(gun->damage_min, gun->damage_max));
                    break;
                case ENTITY_TYPE_CREATURE:
                    float damage = RAND_FLOAT(gun->damage_min, gun->damage_max);
                    // printf("%s damage: %.2f\n", __func__, damage);
                    creature_hurt((Creature*)e->ptr, damage);
                    break;
            }
        }
    }

    bool explosive = RAND_FLOAT(0.0,1.0) <= gun->explosion_chance;
    if(hit && explosive)
    {
        explosion_add(proj->phys.pos.x, proj->phys.pos.y, gun->explosion_radius, gun->explosion_rate, proj->phys.curr_room, proj->from_player);
    }
}

void projectile_draw(Projectile* proj)
{
    if(proj->phys.curr_room != player->phys.curr_room)
        return; // don't draw projectile if not in same room

    float opacity = proj->phys.ethereal ? 0.3 : 1.0;

    float y = proj->phys.pos.y - (proj->phys.vr.h + proj->phys.pos.z)/2.0;
    float scale = RANGE(pow(proj->phys.pos.z/20.0, 0.6), 0.90, 1.10) * proj->phys.scale;

    // printf("z: %.2f, scale: %.2f\n", proj->phys.pos.z, scale);
    gfx_sprite_batch_add(projectile_image, proj->sprite_index, proj->phys.pos.x, y, proj->color, false, scale, proj->phys.rotation_deg, opacity, false, true, false);
}

ProjectileOrbital* projectile_orbital_get(Physics* body, float distance)
{
    for(int i = 0; i < orbital_count; ++i)
    {
        if(orbitals[i].body == body && (distance == 0.0 || orbitals[i].distance == distance))
        {
            return &orbitals[i];
        }
    }

    return NULL;
}

void projectile_orbital_kill(ProjectileOrbital* orb)
{
    orb->body = NULL;
    orb->distance = 0.0;
    orb->base_angle = 0.0;
    orbital_count--;
}

void projectile_lerp(Projectile* p, double dt)
{
    p->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector3f lp = lerp3f(&p->server_state_prior.pos,&p->server_state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;
    p->phys.pos.z = lp.z;

    p->phys.rotation_deg = lerp_angle_deg(p->server_state_prior.angle, p->server_state_target.angle, t);

    //printf("prior_pos: %f %f, target_pos: %f %f, pos: %f %f, t: %f\n",p->server_state_prior.pos.x, p->server_state_prior.pos.y, p->server_state_target.pos.x, p->server_state_target.pos.y, p->phys.pos.x, p->phys.pos.y, t);
}

const char* projectile_def_get_name(ProjectileType proj_type)
{
    switch(proj_type)
    {
        case PROJECTILE_TYPE_PLAYER: return "Player";
        case PROJECTILE_TYPE_PLAYER_KINETIC_DISCHARGE: return "Kinetic Discharge";
        case PROJECTILE_TYPE_CREATURE_GENERIC: return "Creature";
        case PROJECTILE_TYPE_CREATURE_GEIZER: return "Geizer";
        case PROJECTILE_TYPE_CREATURE_CLINGER: return "Clinger";
        case PROJECTILE_TYPE_CREATURE_TOTEM_BLUE: return "Totem Blue";
        case PROJECTILE_TYPE_CREATURE_WATCHER: return "Watcher";
        default: return "???";
    }
}

const char* projectile_spread_type_get_name(SpreadType spread_type)
{
    switch(spread_type)
    {
        case SPREAD_TYPE_RANDOM: return "Random";
        case SPREAD_TYPE_UNIFORM: return "Uniform";
        default: break;
    }
    return "Unknown";
}

const char* projectile_charge_type_get_name(ChargeType charge_type)
{
    switch(charge_type)
    {
        case CHARGE_TYPE_SCALE_DAMAGE:      return "Scale Damage";
        case CHARGE_TYPE_SCALE_NUM:         return "Scale Num";
        case CHARGE_TYPE_SCALE_BURST_COUNT: return "Scale Burst Count";
        default: break;
    }
    return "Unknown";
}


// Gun File Handling
void gun_save_to_file(Gun* gun, Gun* based_on)
{
    char file_path[100] = {0};
    snprintf(file_path,50, "src/guns/%s.gun", gun->name);

    FILE* fp = fopen(file_path, "w");

    if(!fp)
    {
        LOGE("Failed to open file for writing");
        return;
    }

    bool base_gun = (based_on == NULL);

    fprintf(fp, "based_on: %s\n", base_gun ? "null" : based_on->name);
    fprintf(fp, "name: %s\n", gun->name);
    fprintf(fp, "desc: %s\n", gun->desc);

    if(base_gun)
    {
        fprintf(fp, "damage_min: %3.2f\n", gun->damage_min);
        fprintf(fp, "damage_max: %3.2f\n", gun->damage_max);
        fprintf(fp, "num: %d\n", gun->num);
        fprintf(fp, "speed: %3.2f\n", gun->speed);
        fprintf(fp, "scale1: %3.2f\n", gun->scale1);
        fprintf(fp, "scale2: %3.2f\n", gun->scale2);
        fprintf(fp, "lifetime: %3.2f\n", gun->lifetime);
        fprintf(fp, "cooldown: %3.2f\n", gun->cooldown);
        fprintf(fp, "burst_count: %d\n", gun->burst_count);
        fprintf(fp, "burst_rate: %3.2f\n", gun->burst_rate);
        fprintf(fp, "critical_hit_chance: %3.2f\n", gun->critical_hit_chance);
        fprintf(fp, "knockback_factor: %3.2f\n", gun->knockback_factor);
        fprintf(fp, "spread_type: %d\n", gun->spread_type);
        fprintf(fp, "spread: %3.2f\n", gun->spread);
        fprintf(fp, "gun_sprite_index: %d\n", gun->gun_sprite_index);
        fprintf(fp, "sprite_index: %d\n", gun->sprite_index);
        fprintf(fp, "spin_factor: %3.2f\n", gun->spin_factor);
        fprintf(fp, "color1: %06X\n", gun->color1);
        fprintf(fp, "color2: %06X\n", gun->color2);
        fprintf(fp, "directional_accel: %3.2f\n", gun->directional_accel);
        fprintf(fp, "gravity_factor: %3.2f\n", gun->gravity_factor);
        fprintf(fp, "air_friction: %3.2f\n", gun->air_friction);
        fprintf(fp, "wave_amplitude: %3.2f\n", gun->wave_amplitude);
        fprintf(fp, "wave_period: %3.2f\n", gun->wave_period);
        fprintf(fp, "ghost_chance: %3.2f\n", gun->ghost_chance);
        fprintf(fp, "homing_chance: %3.2f\n", gun->homing_chance);
        fprintf(fp, "bounce_chance: %3.2f\n", gun->bounce_chance);
        fprintf(fp, "penetration_chance: %3.2f\n", gun->penetration_chance);
        fprintf(fp, "explosion_chance: %3.2f\n", gun->explosion_chance);
        fprintf(fp, "explosion_radius: %3.2f\n", gun->explosion_radius);
        fprintf(fp, "explosion_rate: %3.2f\n", gun->explosion_rate);
        fprintf(fp, "fire_damage: %3.2f\n", gun->fire_damage);
        fprintf(fp, "cold_damage: %3.2f\n", gun->cold_damage);
        fprintf(fp, "lightning_damage: %3.2f\n", gun->lightning_damage);
        fprintf(fp, "poison_damage: %3.2f\n", gun->poison_damage);
        fprintf(fp, "chargeable: %s\n", gun->chargeable ? "true" : "false");
        fprintf(fp, "charge_type: %d\n", gun->charge_type);
        fprintf(fp, "charge_time_max: %3.2f\n", gun->charge_time_max);
        fprintf(fp, "cluster: %s\n", gun->cluster ? "true" : "false");
        fprintf(fp, "cluster_stages: %d\n", gun->cluster_stages);
        fprintf(fp, "cluster_num0: %d\n", gun->cluster_num[0]);
        fprintf(fp, "cluster_num1: %d\n", gun->cluster_num[1]);
        fprintf(fp, "cluster_scale0: %3.2f\n", gun->cluster_scales[0]);
        fprintf(fp, "cluster_scale1: %3.2f\n", gun->cluster_scales[1]);
        fprintf(fp, "orbital: %s\n", gun->orbital ? "true" : "false");
        fprintf(fp, "orbital_max_count: %d\n", gun->orbital_max_count);
        fprintf(fp, "orbital_distance: %3.2f\n", gun->orbital_distance);
        fprintf(fp, "orbital_speed_factor: %3.2f\n", gun->orbital_speed_factor);
    }
    else
    {
        if(gun->damage_min           != based_on->damage_min)           fprintf(fp, "damage_min: %3.2f\n", gun->damage_min);
        if(gun->damage_max           != based_on->damage_max)           fprintf(fp, "damage_max: %3.2f\n", gun->damage_max);
        if(gun->num                  != based_on->num)                  fprintf(fp, "num: %d\n", gun->num);
        if(gun->speed                != based_on->speed)                fprintf(fp, "speed: %3.2f\n", gun->speed);
        if(gun->scale1               != based_on->scale1)               fprintf(fp, "scale1: %3.2f\n", gun->scale1);
        if(gun->scale2               != based_on->scale2)               fprintf(fp, "scale2: %3.2f\n", gun->scale2);
        if(gun->lifetime             != based_on->lifetime)             fprintf(fp, "lifetime: %3.2f\n", gun->lifetime);
        if(gun->cooldown             != based_on->cooldown)             fprintf(fp, "cooldown: %3.2f\n", gun->cooldown);
        if(gun->burst_count          != based_on->burst_count)          fprintf(fp, "burst_count: %d\n", gun->burst_count);
        if(gun->burst_rate           != based_on->burst_rate)           fprintf(fp, "burst_rate: %3.2f\n", gun->burst_rate);
        if(gun->critical_hit_chance  != based_on->critical_hit_chance)  fprintf(fp, "critical_hit_chance: %3.2f\n", gun->critical_hit_chance);
        if(gun->knockback_factor     != based_on->knockback_factor)     fprintf(fp, "knockback_factor: %3.2f\n", gun->knockback_factor);
        if(gun->spread_type          != based_on->spread_type)          fprintf(fp, "spread_type: %d\n", gun->spread_type);
        if(gun->spread               != based_on->spread)               fprintf(fp, "spread: %3.2f\n", gun->spread);
        if(gun->gun_sprite_index     != based_on->gun_sprite_index)     fprintf(fp, "gun_sprite_index: %d\n", gun->gun_sprite_index);
        if(gun->sprite_index         != based_on->sprite_index)         fprintf(fp, "sprite_index: %d\n", gun->sprite_index);
        if(gun->spin_factor          != based_on->spin_factor)          fprintf(fp, "spin_factor: %3.2f\n", gun->spin_factor);
        if(gun->color1               != based_on->color1)               fprintf(fp, "color1: %06X\n", gun->color1);
        if(gun->color2               != based_on->color2)               fprintf(fp, "color2: %06X\n", gun->color2);
        if(gun->directional_accel    != based_on->directional_accel)    fprintf(fp, "directional_accel: %3.2f\n", gun->directional_accel);
        if(gun->gravity_factor       != based_on->gravity_factor)       fprintf(fp, "gravity_factor: %3.2f\n", gun->gravity_factor);
        if(gun->air_friction         != based_on->air_friction)         fprintf(fp, "air_friction: %3.2f\n", gun->air_friction);
        if(gun->wave_amplitude       != based_on->wave_amplitude)       fprintf(fp, "wave_amplitude: %3.2f\n", gun->wave_amplitude);
        if(gun->wave_period          != based_on->wave_period)          fprintf(fp, "wave_period: %3.2f\n", gun->wave_period);
        if(gun->ghost_chance         != based_on->ghost_chance)         fprintf(fp, "ghost_chance: %3.2f\n", gun->ghost_chance);
        if(gun->homing_chance        != based_on->homing_chance)        fprintf(fp, "homing_chance: %3.2f\n", gun->homing_chance);
        if(gun->bounce_chance        != based_on->bounce_chance)        fprintf(fp, "bounce_chance: %3.2f\n", gun->bounce_chance);
        if(gun->penetration_chance   != based_on->penetration_chance)   fprintf(fp, "penetration_chance: %3.2f\n", gun->penetration_chance);
        if(gun->explosion_chance     != based_on->explosion_chance)     fprintf(fp, "explosion_chance: %3.2f\n", gun->explosion_chance);
        if(gun->explosion_radius     != based_on->explosion_radius)     fprintf(fp, "explosion_radius: %3.2f\n", gun->explosion_radius);
        if(gun->explosion_rate       != based_on->explosion_rate)       fprintf(fp, "explosion_rate: %3.2f\n", gun->explosion_rate);
        if(gun->fire_damage          != based_on->fire_damage)          fprintf(fp, "fire_damage: %3.2f\n", gun->fire_damage);
        if(gun->cold_damage          != based_on->cold_damage)          fprintf(fp, "cold_damage: %3.2f\n", gun->cold_damage);
        if(gun->lightning_damage     != based_on->lightning_damage)     fprintf(fp, "lightning_damage: %3.2f\n", gun->lightning_damage);
        if(gun->poison_damage        != based_on->poison_damage)        fprintf(fp, "poison_damage: %3.2f\n", gun->poison_damage);
        if(gun->chargeable           != based_on->chargeable)           fprintf(fp, "chargeable: %s\n", gun->chargeable ? "true" : "false");
        if(gun->charge_type          != based_on->charge_type)          fprintf(fp, "charge_type: %d\n", gun->charge_type);
        if(gun->charge_time_max      != based_on->charge_time_max)      fprintf(fp, "charge_time_max: %3.2f\n", gun->charge_time_max);
        if(gun->cluster              != based_on->cluster)              fprintf(fp, "cluster: %s\n", gun->cluster ? "true" : "false");
        if(gun->cluster_stages       != based_on->cluster_stages)       fprintf(fp, "cluster_stages: %d\n", gun->cluster_stages);
        if(gun->cluster_num[0]       != based_on->cluster_num[0])       fprintf(fp, "cluster_num0: %d\n", gun->cluster_num[0]);
        if(gun->cluster_num[1]       != based_on->cluster_num[1])       fprintf(fp, "cluster_num1: %d\n", gun->cluster_num[1]);
        if(gun->cluster_scales[0]    != based_on->cluster_scales[0])    fprintf(fp, "cluster_scale0: %3.2f\n", gun->cluster_scales[0]);
        if(gun->cluster_scales[1]    != based_on->cluster_scales[1])    fprintf(fp, "cluster_scale1: %3.2f\n", gun->cluster_scales[1]);
        if(gun->orbital              != based_on->orbital)              fprintf(fp, "orbital: %s\n", gun->orbital ? "true" : "false");
        if(gun->orbital_max_count    != based_on->orbital_max_count)    fprintf(fp, "orbital_max_count: %d\n", gun->orbital_max_count);
        if(gun->orbital_distance     != based_on->orbital_distance)     fprintf(fp, "orbital_distance: %3.2f\n", gun->orbital_distance);
        if(gun->orbital_speed_factor != based_on->orbital_speed_factor) fprintf(fp, "orbital_speed_factor: %3.2f\n", gun->orbital_speed_factor);
    }

    fclose(fp);
}

void gun_print(Gun* gun)
{
    printf("name: %s\n", gun->name);
    printf("desc: %s\n", gun->desc);
    printf("damage_min: %3.2f\n", gun->damage_min);
    printf("damage_max: %3.2f\n", gun->damage_max);
    printf("num: %d\n", gun->num);
    printf("speed: %3.2f\n", gun->speed);
    printf("scale1: %3.2f\n", gun->scale1);
    printf("scale2: %3.2f\n", gun->scale2);
    printf("lifetime: %3.2f\n", gun->lifetime);
    printf("cooldown: %3.2f\n", gun->cooldown);
    printf("burst_count: %d\n", gun->burst_count);
    printf("burst_rate: %3.2f\n", gun->burst_rate);
    printf("critical_hit_chance: %3.2f\n", gun->critical_hit_chance);
    printf("knockback_factor: %3.2f\n", gun->knockback_factor);
    printf("\n");
    printf("spread_type: %d\n", gun->spread_type);
    printf("spread: %3.2f\n", gun->spread);
    printf("\n");
    printf("gun_sprite_index: %d\n", gun->gun_sprite_index);
    printf("sprite_index: %d\n", gun->sprite_index);
    printf("spin_factor: %3.2f\n", gun->spin_factor);
    printf("color1: %06X\n", gun->color1);
    printf("color2: %06X\n", gun->color2);
    printf("\n");
    printf("directional_accel: %3.2f\n", gun->directional_accel);
    printf("gravity_factor: %3.2f\n", gun->gravity_factor);
    printf("air_friction: %3.2f\n", gun->air_friction);
    printf("\n");
    printf("wave_amplitude: %3.2f\n", gun->wave_amplitude);
    printf("wave_period: %3.2f\n", gun->wave_period);
    printf("\n");
    printf("ghost_chance: %3.2f\n", gun->ghost_chance);
    printf("homing_chance: %3.2f\n", gun->homing_chance);
    printf("bounce_chance: %3.2f\n", gun->bounce_chance);
    printf("penetration_chance: %3.2f\n", gun->penetration_chance);
    printf("explosion_chance: %3.2f\n", gun->explosion_chance);
    printf("explosion_radius: %3.2f\n", gun->explosion_radius);
    printf("explosion_rate: %3.2f\n", gun->explosion_rate);
    printf("\n");
    printf("fire_damage: %3.2f\n", gun->fire_damage);
    printf("cold_damage: %3.2f\n", gun->cold_damage);
    printf("lightning_damage: %3.2f\n", gun->lightning_damage);
    printf("poison_damage: %3.2f\n", gun->poison_damage);
    printf("\n");
    printf("chargeable: %s\n", gun->chargeable ? "true" : "false");
    printf("charge_type: %d\n", gun->charge_type);
    printf("charge_time_max: %3.2f\n", gun->charge_time_max);
    printf("\n");
    printf("cluster: %s\n", gun->cluster ? "true" : "false");
    printf("cluster_stages: %d\n", gun->cluster_stages);
    printf("cluster_num0: %d\n", gun->cluster_num[0]);
    printf("cluster_num1: %d\n", gun->cluster_num[1]);
    printf("cluster_scale0: %3.2f\n", gun->cluster_scales[0]);
    printf("cluster_scale1: %3.2f\n", gun->cluster_scales[1]);
    printf("\n");
    printf("orbital: %s\n", gun->orbital ? "true" : "false");
    printf("orbital_max_count: %d\n", gun->orbital_max_count);
    printf("orbital_distance: %3.2f\n", gun->orbital_distance);
    printf("orbital_speed_factor: %3.2f\n", gun->orbital_speed_factor);
}

bool is_gun_file_base_gun(const char* file_name)
{
    char file_path[50] = {0};
    snprintf(file_path,50, "src/guns/%s", file_name);

    FILE* fp = fopen(file_path, "r");

    if(!fp)
    {
        LOGE("Failed to open file for reading");
        return false;
    }

    int c;
    int value = 0;

    char key[256] = {0}; int ikey = 0;
    char val[256] = {0}; int ival = 0;

    bool comment = false;

    for(;;)
    {
        c = fgetc(fp);

        if(c == EOF)
            break;

        if(c == '\n')
        {
            // set value
            char* val2 = str_trim_whitespace(val);

            if(STR_EQUAL(key, "based_on"))
            {
                if(STR_EQUAL(val2, "null"))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            
            // reset 
            comment = false;
            value = 0;
            memset(key, 0, 256);
            memset(val, 0, 256);
            ikey = 0;
            ival = 0;
            continue;
        }

        if(comment)
            continue;

        if(c == ':')
        {
            value = 1;
        }
        else if(c == '#')
        {
            comment = true;
        }
        else
        {
            if(c >= 32 && c <= 126)
            {
                if(value) val[ival++] = c;
                else      key[ikey++] = c;
            }
        }
    }

    return false;

    fclose(fp);

}

void gun_load_from_file(Gun* gun, const char* file_name)
{
    char file_path[50] = {0};
    snprintf(file_path,50, "src/guns/%s", file_name);

    FILE* fp = fopen(file_path, "r");

    if(!fp)
    {
        LOGE("Failed to open file for reading");
        return;
    }

    memset(gun, 0, sizeof(Gun));

    int c;
    int value = 0;

    char key[256] = {0}; int ikey = 0;
    char val[256] = {0}; int ival = 0;

    bool comment = false;

    for(;;)
    {
        c = fgetc(fp);

        if(c == EOF)
            break;

        if(c == '\n')
        {
            // set value
            char* val2 = str_trim_whitespace(val);

            if(STR_EQUAL(key, "based_on"))
            {
                if(!STR_EQUAL(val2, "null"))
                {
                    // get base gun from gun_list
                    Gun base_gun = {0};
                    gun_get_by_name(val2, &base_gun);

                    // copy gun to this gun
                    memcpy(gun, &base_gun, sizeof(Gun));
                    memcpy(gun->based_on_name, base_gun.name, strlen(base_gun.name));
                }
            }
            else if(STR_EQUAL(key, "name"))
                memcpy(gun->name, val2, sizeof(char)*MIN(GUN_NAME_MAX_LEN, strlen(val2)));
            else if(STR_EQUAL(key, "desc"))
                memcpy(gun->desc, val2, sizeof(char)*MIN(GUN_DESC_MAX_LEN, strlen(val2)));
            else if(STR_EQUAL(key, "damage_min"))
                gun->damage_min = atof(val2);
            else if(STR_EQUAL(key, "damage_max"))
                gun->damage_max = atof(val2);
            else if(STR_EQUAL(key, "num"))
                gun->num = atoi(val2);
            else if(STR_EQUAL(key, "speed"))
                gun->speed = atof(val2);
            else if(STR_EQUAL(key, "scale1"))
                gun->scale1 = atof(val2);
            else if(STR_EQUAL(key, "scale2"))
                gun->scale2 = atof(val2);
            else if(STR_EQUAL(key, "lifetime"))
                gun->lifetime = atof(val2);
            else if(STR_EQUAL(key, "cooldown"))
                gun->cooldown = atof(val2);
            else if(STR_EQUAL(key, "burst_count"))
                gun->burst_count = atoi(val2);
            else if(STR_EQUAL(key, "burst_rate"))
                gun->burst_rate = atof(val2);
            else if(STR_EQUAL(key, "critical_hit_chance"))
                gun->critical_hit_chance = atof(val2);
            else if(STR_EQUAL(key, "spread_type"))
                gun->spread_type = (SpreadType)atoi(val2);
            else if(STR_EQUAL(key, "spread"))
                gun->spread = atof(val2);
            else if(STR_EQUAL(key, "gun_sprite_index"))
                gun->gun_sprite_index = atoi(val2);
            else if(STR_EQUAL(key, "sprite_index"))
                gun->sprite_index = atoi(val2);
            else if(STR_EQUAL(key, "spin_factor"))
                gun->spin_factor = atof(val2);
            else if(STR_EQUAL(key, "color1"))
                gun->color1 = (int)strtol(val2, NULL, 16);
            else if(STR_EQUAL(key, "color2"))
                gun->color2 = (int)strtol(val2, NULL, 16);
            else if(STR_EQUAL(key, "directional_accel"))
                gun->directional_accel = atof(val2);
            else if(STR_EQUAL(key, "gravity_factor"))
                gun->gravity_factor = atof(val2);
            else if(STR_EQUAL(key, "air_friction"))
                gun->air_friction = atof(val2);
            else if(STR_EQUAL(key, "wave_amplitude"))
                gun->wave_amplitude = atof(val2);
            else if(STR_EQUAL(key, "wave_period"))
                gun->wave_period = atof(val2);
            else if(STR_EQUAL(key, "ghost_chance"))
                gun->ghost_chance = atof(val2);
            else if(STR_EQUAL(key, "homing_chance"))
                gun->homing_chance = atof(val2);
            else if(STR_EQUAL(key, "bounce_chance"))
                gun->bounce_chance = atof(val2);
            else if(STR_EQUAL(key, "penetration_chance"))
                gun->penetration_chance = atof(val2);
            else if(STR_EQUAL(key, "explosion_chance"))
                gun->explosion_chance = atof(val2);
            else if(STR_EQUAL(key, "explosion_radius"))
                gun->explosion_radius = atof(val2);
            else if(STR_EQUAL(key, "explosion_rate"))
                gun->explosion_rate = atof(val2);
            else if(STR_EQUAL(key, "fire_damage"))
                gun->fire_damage = atof(val2);
            else if(STR_EQUAL(key, "cold_damage"))
                gun->cold_damage = atof(val2);
            else if(STR_EQUAL(key, "lightning_damage"))
                gun->lightning_damage = atof(val2);
            else if(STR_EQUAL(key, "poison_damage"))
                gun->poison_damage = atof(val2);
            else if(STR_EQUAL(key, "chargeable"))
                gun->chargeable = STR_EQUAL(val2, "true") ? true : false;
            else if(STR_EQUAL(key, "charge_type"))
                gun->charge_type = atoi(val2);
            else if(STR_EQUAL(key, "charge_time_max"))
                gun->charge_time_max = atof(val2);
            else if(STR_EQUAL(key, "cluster"))
                gun->cluster = STR_EQUAL(val2, "true") ? true : false;
            else if(STR_EQUAL(key, "cluster_stages"))
                gun->cluster_stages = atoi(val2);
            else if(STR_EQUAL(key, "cluster_num0"))
                gun->cluster_num[0] = atoi(val2);
            else if(STR_EQUAL(key, "cluster_num1"))
                gun->cluster_num[1] = atoi(val2);
            else if(STR_EQUAL(key, "cluster_scale0"))
                gun->cluster_scales[0] = atof(val2);
            else if(STR_EQUAL(key, "cluster_scale1"))
                gun->cluster_scales[1] = atof(val2);
            else if(STR_EQUAL(key, "orbital"))
                gun->orbital = STR_EQUAL(val2, "true") ? true : false;
            else if(STR_EQUAL(key, "orbital_max_count"))
                gun->orbital_max_count = atoi(val2);
            else if(STR_EQUAL(key, "orbital_distance"))
                gun->orbital_distance = atof(val2);
            else if(STR_EQUAL(key, "orbital_speed_factor"))
                gun->orbital_speed_factor = atof(val2);

            // reset 
            comment = false;
            value = 0;
            memset(key, 0, 256);
            memset(val, 0, 256);
            ikey = 0;
            ival = 0;
            continue;
        }

        if(comment)
            continue;

        if(c == ':')
        {
            value = 1;
        }
        else if(c == '#')
        {
            comment = true;
        }
        else
        {
            if(c >= 32 && c <= 126)
            {
                if(value) val[ival++] = c;
                else      key[ikey++] = c;
            }
        }
    }

    fclose(fp);
}

bool gun_get_by_name(char* gun_name, Gun* gun)
{
    for(int i = 0; i < gun_list_count; ++i)
    {
        if(STR_EQUAL(gun_name, gun_list[i].name))
        {
            memcpy(gun, &gun_list[i], sizeof(Gun));
            return true;
        }
    }

    return false;
}

void gun_refresh_list()
{
    // Load up Gun List
    char gun_files[256][32] = {0};
    int gun_file_count = io_get_files_in_dir("src/guns",".gun", gun_files);

    gun_list_count = 0;

    int child_indices[256] = {0};
    int child_gun_count = 0;

    // load all base guns
    for (int i = 0; i < gun_file_count; ++i) 
    {
        bool base_gun = is_gun_file_base_gun(gun_files[i]);
        if(base_gun)
        {
            gun_load_from_file(&gun_list[gun_list_count++], gun_files[i]);
        }
        else
        {
            child_indices[child_gun_count++] = i;
        }
    }

    // load derivative guns
    for (int i = 0; i < child_gun_count; ++i) 
    {
        gun_load_from_file(&gun_list[gun_list_count++], gun_files[child_indices[i]]);
    }

    // insertion sort
    Gun key;

    int i, j;
    for (i = 1; i < gun_list_count; ++i) 
    {
        memcpy(&key, &gun_list[i], sizeof(Gun));
        j = i - 1;

        while (j >= 0 && strncmp(key.name,gun_list[j].name,GUN_NAME_MAX_LEN) < 0)
        {
            memcpy(&gun_list[j+1], &gun_list[j], sizeof(Gun));
            j = j - 1;
        }
        memcpy(&gun_list[j+1], &key, sizeof(Gun));
    }

    /*
    for (i = 0; i < gun_list_count; ++i) 
    {
        Gun* gun = &gun_list[i];
        printf("%s, %s, %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %s, %s\n",
                gun->name,
                gun->desc,
                gun->num,
                gun->burst_count,
                gun->speed,
                gun->lifetime,
                gun->cooldown,
                gun->spread,
                gun->damage_min,
                gun->damage_max,
                gun->critical_hit_chance,
                gun->knockback_factor,
                gun->ghost_chance,
                gun->homing_chance,
                gun->chargeable ? "true" : "false",
                gun->cluster ? "true" : "false"
              );
    }
    */
}

void refresh_visible_room_gun_list()
{
    if(!visible_room)
        return;

    // clear room gun list
    memset(room_gun_list, 0, MAX_ROOM_GUNS*sizeof(Gun));
    room_gun_count = 0;
    
    // add player guns
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];

        if(!p->active) continue;
        if(p->phys.dead) continue;
        if(room_gun_count >= MAX_ROOM_GUNS) break;

        memcpy(&room_gun_list[room_gun_count++], &p->gun, sizeof(Gun));
        p->room_gun_index = room_gun_count-1;
    }

    // add creature guns
    for(int i = clist->count; i >= 0; --i)
    {
        Creature* c = &creatures[i];

        if(c->phys.curr_room != visible_room->index) continue;
        if(room_gun_count >= MAX_ROOM_GUNS) break;

        Gun gun;
        char* gun_name = creature_get_gun_name(c->type);
        gun_get_by_name(gun_name, &gun);

        int existing_index = -1;
        for(int j = 0; j < room_gun_count; ++j)
        {
            if(memcmp(&room_gun_list[j],&gun, sizeof(Gun)) == 0)
            {
                existing_index = j;
                break;
            }
        }

        if(existing_index >= 0)
        {
            c->room_gun_index = existing_index;
            continue;
        }

        memcpy(&room_gun_list[room_gun_count++], &gun, sizeof(Gun));
        c->room_gun_index = room_gun_count-1;
    }
    
    // add room item guns
    for(int i = 0; i < item_list->count; ++i)
    {
        Item* it = &items[i];

        if(it->type != ITEM_GUN) continue;
        if(it->phys.curr_room != visible_room->index) continue;
        if(room_gun_count >= MAX_ROOM_GUNS) break;

        memcpy(&room_gun_list[room_gun_count++], &gun_list[it->user_data], sizeof(Gun));
        it->user_data3 = (uint8_t)(room_gun_count-1);
    }

    printf("Refreshing room gun list. Count: %d\n", room_gun_count);
    for(int i = 0; i < room_gun_count; ++i)
        printf("  %d) %s\n", i, room_gun_list[i].name);
}

int add_to_room_gun_list(Gun* g)
{
    if(!visible_room)
        return -1;

    if(room_gun_count >= MAX_ROOM_GUNS)
        return -1;

    memcpy(&room_gun_list[room_gun_count++], g, sizeof(Gun));

    return room_gun_count-1;
}

void replace_player_room_gun(void* p, Gun* g)
{
    if(!p)
        return;

    if(!visible_room)
        return; 

    if(room_gun_count >= MAX_ROOM_GUNS)
        return;

    Player* pl = (Player*)p;
    memcpy(&room_gun_list[pl->room_gun_index], g, sizeof(Gun));
}
