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

static int projectile_image;

// .color = 0x003030FF,

Gun gun_lookup[] = {
    {
        // player
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 215.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.0,
        .lifetime = 0.4,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,

        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,

        .sprite_index = 0,
        .color1 = 0x0050A0FF,
        .color2 = 0x0050A0FF,

        .scale1 = 1.0,
        .scale2 = 1.0,

        .cluster = false,
        .cluster_stages = 1,
        .cluster_num = {8, 2, 2},
        .cluster_scales = {0.5, 0.5, 0.5},

        .is_orbital = false,
        .orbital_distance = 32.0,
        .orbital_speed_factor = 3.0,
        .orbital_max_count = 5,

        .burst_count = 0,
        .burst_rate = 0.2,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 10.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .fire_chance = 0.0,
        .cold_chance = 0.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
    {
        // player - kinetic discharge skill
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 200.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.0,
        .lifetime = 3.0,
        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,
        .cluster = false,

        .sprite_index = 0,
        .color1 = COLOR_WHITE,
        .color2 = COLOR_WHITE,

        .scale1 = 0.5,
        .scale2 = 0.5,

        .burst_count = 0,
        .burst_rate = 0.2,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 360.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.20,
        .fire_chance = 0.0,
        .cold_chance = 0.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
    {
        // creature generic
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 200.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.5,
        .lifetime = 3.0,
        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,
        .cluster = false,

        .sprite_index = 0,
        .color1 = 0x00FF5050,
        .color2 = 0x00FF5050,
        .scale1 = 0.8,
        .scale2 = 0.8,

        .burst_count = 0,
        .burst_rate = 0.2,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .fire_chance = 0.0,
        .cold_chance = 0.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
    {
        // geizer
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 100.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.5,
        .lifetime = 3.0,
        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,
        .cluster = false,

        .sprite_index = 0,
        .color1 = 0x00FF5050,
        .color2 = 0x00FF5050,
        .scale1 = 0.8,
        .scale2 = 0.8,

        .burst_count = 0,
        .burst_rate = 0.2,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .fire_chance = 0.0,
        .cold_chance = 0.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
    {
        // clinger
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 160.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.5,
        .lifetime = 3.0,
        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,
        .cluster = false,

        .sprite_index = 0,
        .color1 = 0x00FF5050,
        .color2 = 0x00FF5050,
        .scale1 = 0.8,
        .scale2 = 0.8,

        .burst_count = 0,
        .burst_rate = 0.2,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .fire_chance = 0.0,
        .cold_chance = 0.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
    {
        // totem blue
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 200.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.5,
        .lifetime = 3.0,
        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,
        .cluster = true,

        .sprite_index = 0,
        .color1 = 0x00FF5050,
        .color2 = 0x00FF5050,
        .scale1 = 0.8,
        .scale2 = 0.8,

        .burst_count = 0,
        .burst_rate = 0.2,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 0.0,
        .ghost_chance = 0.0,
        .homing_chance = 0.0,
        .fire_chance = 0.0,
        .cold_chance = 1.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
    {
        // watcher
        .damage_min = 1.0,
        .damage_max = 1.0,
        .speed = 200.0,
        .cooldown = 0.40,
        .directional_accel = 0.0,
        .gravity_factor = 0.5,
        .lifetime = 3.0,
        .explosion_chance = 0.0,
        .explosion_radius = 15.0,
        .explosion_rate   = 100.0,
        .bounce_chance = 0.0,
        .penetration_chance = 0.0,
        .cluster = false,
        
        .sprite_index = 0,
        .color1 = 0x00FF5050,
        .color2 = 0x00FF5050,
        .scale1 = 1.0,
        .scale2 = 1.0,

        .burst_count = 0,
        .burst_rate = 0.2,

        .is_orbital = true,
        .orbital_distance = 50.0,
        .orbital_speed_factor = 4.0,
        .orbital_max_count = 4,

        .num = 1,
        .spread_type = SPREAD_TYPE_RANDOM,
        .spread = 0.0,
        .ghost_chance = 1.0,
        .homing_chance = 0.0,
        .fire_chance = 0.0,
        .cold_chance = 0.0,
        .lightning_chance = 0.0,
        .poison_chance = 0.0,
    },
};

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

void projectile_add(Vector3f pos, Vector3f* vel, uint8_t curr_room, Gun* gun, uint32_t color, float angle_deg, bool from_player, Physics* phys)
{
    if(role == ROLE_CLIENT)
        return;

    Projectile proj = {0};

    proj.gun = *gun;

    Rect vr = gfx_images[projectile_image].visible_rects[0];

    vr.w *= gun->scale1;
    vr.h *= gun->scale1;

    proj.color = gun->color1;
    proj.sprite_index = gun->sprite_index;
    proj.phys.height = vr.h;
    proj.phys.width =  vr.w;
    proj.phys.length = vr.w;
    proj.phys.scale = gun->scale1;
    proj.phys.vr = vr;
    proj.phys.pos.x = pos.x;
    proj.phys.pos.y = pos.y;
    proj.phys.pos.z = pos.z;
    proj.phys.mass = 1.0;
    proj.phys.base_friction = gun->air_friction;
    proj.phys.speed = gun->speed;
    proj.phys.max_velocity = 2.0*gun->speed;
    proj.phys.radius = (MAX(proj.phys.length, proj.phys.width) / 2.0) * gun->scale1;

    bool bouncy = RAND_FLOAT(0.0,1.0) <= gun->bounce_chance;
    proj.phys.amorphous = bouncy ? false : true;
    proj.phys.elasticity = bouncy ? 1.0 : 0.1;

    proj.phys.curr_room = curr_room;
    proj.from_player = from_player;
    proj.angle_deg = angle_deg;
    proj.ttl = gun->lifetime;
    proj.wave_time = gun->wave_period / 2.0;
    proj.wave_dir = 1;

    if(gun->is_orbital)
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

            if(phys == orb->body && gun->orbital_distance == orb->distance)
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
            orbital->distance = gun->orbital_distance;
            orbital->speed_factor = gun->orbital_speed_factor;
            orbital->max_count = gun->orbital_max_count;
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

    proj.cluster_stage = gun->cluster_stage;

    float spread = gun->spread_type == SPREAD_TYPE_RANDOM ? gun->spread/2.0 : gun->spread / gun->num;
    float spread_angle_start = angle_deg - (gun->spread/2.0);

    uint16_t target_ids[32] = {0};
    int target_count = 0;

    for(int i = 0; i < gun->num; ++i)
    {
        Projectile p = {0};
        memcpy(&p, &proj, sizeof(Projectile));
        p.id = get_id();

        p.phys.ethereal = RAND_FLOAT(0.0,1.0) <= gun->ghost_chance;

        p.fire = RAND_FLOAT(0.0,1.0) <= gun->fire_chance;
        p.cold = RAND_FLOAT(0.0,1.0) <= gun->cold_chance;
        p.lightning = RAND_FLOAT(0.0,1.0) <= gun->lightning_chance;
        p.poison = RAND_FLOAT(0.0,1.0) <= gun->poison_chance;

        bool homing = RAND_FLOAT(0.0,1.0) <= gun->homing_chance;

        if(!FEQ0(spread))
        {
            if(gun->spread_type == SPREAD_TYPE_RANDOM)
            {
                p.angle_deg += RAND_FLOAT(-spread, spread);
            }
            else if(i > 0 && gun->spread_type == SPREAD_TYPE_UNIFORM)
            {
                p.angle_deg = angle_deg + (i*spread);
                if(p.angle_deg > angle_deg + gun->spread/2.0)
                    p.angle_deg += (360 - gun->spread);
            }
        }

        float angle = RAD(p.angle_deg);

        p.phys.vel.x = +(p.gun.speed)*cosf(angle) + vel->x;
        p.phys.vel.y = -(p.gun.speed)*sinf(angle) + vel->y;
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
                p.phys.vel.x = v.x * p.gun.speed;
                p.phys.vel.y = v.y * p.gun.speed;

                target_ids[target_count++] = id;
            }
            else
            {
                p.angle_deg = rand() % 360;
                float angle = RAD(p.angle_deg);
                p.phys.vel.x = +(p.gun.speed)*cosf(angle) + vel->x;
                p.phys.vel.y = -(p.gun.speed)*sinf(angle) + vel->y;
                p.phys.vel.z = 80.0;
            }
        }

        // p.source_explode = audio_source_create(false);
        // audio_source_assign_buffer(p.source_explode, audio_buffer_explode);

        // printf("%s damage: [%.2f, %.2f]\n", __func__, p.gun.damage_min, p.gun.damage_max);

        // p.phys.pos.z = 400.0;
        // p.phys.vel.x = 0;
        // p.phys.vel.y = 0;
        // p.angle_deg = 0.0;

        list_add(plist, (void*)&p);
    }
}

void projectile_fire(Physics* phys, uint8_t curr_room, Gun* gun, uint32_t color, float angle_deg, bool from_player)
{
    Vector3f pos = {phys->pos.x, phys->pos.y, phys->height/2.0 + phys->pos.z};
    Vector3f vel = {phys->vel.x, phys->vel.y, 0.0};

    projectile_add(pos, &vel, curr_room, gun, color, angle_deg, from_player, phys);
}

void projectile_lob(Physics* phys, float vel0_z, uint8_t curr_room, Gun* gun, uint32_t color, float angle_deg, bool from_player)
{
    Vector3f pos = {phys->pos.x, phys->pos.y, phys->height + phys->pos.z};
    //Vector3f pos = {phys->pos.x, phys->pos.y, phys->height*2.0 + phys->pos.z};

    Vector3f vel = {0.0, 0.0, vel0_z};

    float angle = RAD(angle_deg);
    vel.x = phys->vel.x;
    vel.y = phys->vel.y;

    projectile_add(pos, &vel, curr_room, gun, color, angle_deg, from_player, phys);
}

void projectile_kill(Projectile* proj)
{
    if(role == ROLE_CLIENT)
        return;

    proj->phys.dead = true;

    //audio_source_play(proj->source_explode);
    //audio_source_delete(proj->source_explode);

    Gun gun = proj->gun;
    bool more_cluster = proj->gun.cluster && (proj->cluster_stage < (3) && proj->cluster_stage < (gun.cluster_stages));

    const float scale_particle_thresh = 0.25;
    float pscale = MIN(1.0, proj->gun.scale2);

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
        bool explosive = RAND_FLOAT(0.0,1.0) <= proj->gun.explosion_chance;

        if(explosive)
        {
            explosion_add(proj->phys.pos.x, proj->phys.pos.y, proj->gun.explosion_radius, proj->gun.explosion_rate, proj->phys.curr_room, proj->from_player);
        }
    }

    if(proj->gun.is_orbital)
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

    if(proj->gun.cluster)
    {

        if(!more_cluster)
            return;

        // printf("cluster\n");
        Gun gun = proj->gun;

        if(proj->cluster_stage >= (3) || proj->cluster_stage >= (gun.cluster_stages))
        {
            return;
        }

        // gun.damage_min *= 0.8;
        // gun.damage_max *= 0.8;

        gun.cluster = true;
        gun.scale1 = gun.cluster_scales[proj->cluster_stage];
        gun.scale2 = gun.cluster_scales[proj->cluster_stage];

        // pd.cluster = false;
        // pd.scale *= 0.5;
        // if(pd.scale <= 0.05)
        //     return;
        gun.speed = 50.0;

        gun.num = gun.cluster_num[proj->cluster_stage];

        float angle_deg = proj->angle_deg;
        // float angle_deg = rand() % 360;
        gun.spread = 210.0;

        // sp.spread = 360.0;
        gun.cluster_stage = proj->cluster_stage+1;
        // proj->phys.vel.x = 0.0;
        // proj->phys.vel.y = 0.0;
        proj->phys.vel.x /= 2.0;
        proj->phys.vel.y /= 2.0;

        projectile_fire(&proj->phys, proj->phys.curr_room, &gun, proj->color, angle_deg, proj->from_player);
    }
}

void projectile_update_all(float dt)
{
    // printf("projectile update\n");

    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

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

        if(proj->gun.is_orbital && proj->orbital->body)
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

            proj->phys.vel.x = proj->gun.speed * f.y;
            proj->phys.vel.y = proj->gun.speed * -f.x;
        }
        else
        {
            RANGE(proj->ttl, 0.0, dt);

            // printf("%3d %.4f\n", i, delta_t);
            proj->ttl -= _dt;

            float angle = RAD(proj->angle_deg);
            float vel_magn = magn(proj->phys.vel);

            if(proj->gun.directional_accel != 0.0)
            {
                Vector2f f = {cos(angle), -sin(angle)};
                normalize(&f);

                float accel = proj->gun.directional_accel * 1500.0 * _dt;

                proj->phys.vel.x += (f.x * accel);
                proj->phys.vel.y += (f.y * accel);

                /*
                if(vel_magn > proj->phys.max_velocity)
                {
                    // cap vel
                    Vector2f c = {proj->phys.vel.x, proj->phys.vel.y};
                    normalize(&c);

                    proj->phys.vel.x = c.x * proj->gun.speed;
                    proj->phys.vel.y = c.y * proj->gun.speed;
                }
                */
            }

            if(proj->phys.base_friction > 0.0)
            {
                Vector2f c = {-proj->phys.vel.x, -proj->phys.vel.y};
                normalize(&c);

                float friction = _dt*4*proj->gun.speed*proj->phys.base_friction;

                float fx = MIN(ABS(proj->phys.vel.x), friction);
                float fy = MIN(ABS(proj->phys.vel.y), friction);

                proj->phys.vel.x += fx*c.x;
                proj->phys.vel.y += fy*c.y;
            }

            if(proj->gun.wave_amplitude > 0.0)
            {
                if(proj->gun.wave_period > 0.0)
                {
                    Vector2f f = {sin(angle), cos(angle)};
                    normalize(&f);

                    proj->wave_time += _dt;

                    if(proj->wave_time >= proj->gun.wave_period)
                    {
                        while(proj->wave_time >= proj->gun.wave_period)
                            proj->wave_time -= proj->gun.wave_period;

                        proj->wave_dir = proj->wave_dir == 1 ? -1 : 1;
                    }

                    float ampl = proj->gun.wave_amplitude;

                    f.x *= proj->wave_dir*ampl*10*_dt;
                    f.y *= proj->wave_dir*ampl*10*_dt;

                    //printf("f: %3.6f %3.6f\n", f.x, f.y);

                    proj->phys.vel.x += f.x;
                    proj->phys.vel.y += f.y;
                }
            }


            float cfactor = proj->ttl / proj->gun.lifetime;
            proj->color = gfx_blend_colors(proj->gun.color2, proj->gun.color1, cfactor);
            proj->phys.scale = lerp(proj->gun.scale2, proj->gun.scale1, cfactor);
        }

        if(!FEQ0(proj->gun.spin_factor))
        {
            proj->phys.rotation_deg += (proj->gun.spin_factor*1080.0*_dt);
            proj->phys.rotation_deg = fmod(proj->phys.rotation_deg, 360.0);
        }

        //printf("adding %f %f; vel: %f %f\n",f.x, f.y, proj->phys.vel.x, proj->phys.vel.y);

        proj->phys.prior_pos.x = proj->phys.pos.x;
        proj->phys.prior_pos.y = proj->phys.pos.y;
        proj->phys.prior_pos.z = proj->phys.pos.z;

        proj->phys.pos.x += _dt*proj->phys.vel.x;
        proj->phys.pos.y += _dt*proj->phys.vel.y;

        if(!proj->gun.is_orbital)
        {
            phys_apply_gravity(&proj->phys, proj->gun.gravity_factor, _dt);
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

    Gun* gun = &proj->gun;

    uint8_t curr_room = 0;
    Physics* phys = NULL;

    if(proj->from_player && e->type == ENTITY_TYPE_CREATURE)
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
            proj->phys.collision_rect.w,
            proj->phys.collision_rect.h,
            proj->phys.height*2,
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
            phys->collision_rect.w,
            phys->collision_rect.h,
            phys->height*2,
        };

        hit = boxes_colliding(&proj_curr, &check);

        if(hit)
        {
            bool penetrate = RAND_FLOAT(0.0,1.0) <= proj->gun.penetration_chance;
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
        explosion_add(proj->phys.pos.x, proj->phys.pos.y, proj->gun.explosion_radius, proj->gun.explosion_rate, proj->phys.curr_room, proj->from_player);
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
        case CHARGE_TYPE_SCALE_DAMAGE: return "Scale Damage";
        case CHARGE_TYPE_SCALE_BURST_COUNT: return "Scale Burst Count";
        default: break;
    }
    return "Unknown";
}
