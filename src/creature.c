#include "headers.h"
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "level.h"
#include "log.h"
#include "player.h"
#include "glist.h"
#include "projectile.h"
#include "particles.h"
#include "creature.h"
#include "effects.h"
#include "decal.h"
#include "ai.h"

Creature prior_creatures[MAX_CREATURES] = {0};
Creature creatures[MAX_CREATURES] = {0};
glist* clist = NULL;

int creature_segment_count;
CreatureSegment creature_segments[MAX_SEGMENTS]; // 0=first body segment

#define PROJ_COLOR 0x00FF5050

static int creature_image_slug;
static int creature_image_clinger;
static int creature_image_geizer;
static int creature_image_floater;
static int creature_image_floater_big;
static int creature_image_buzzer;
static int creature_image_totem_red;
static int creature_image_totem_blue;
static int creature_image_totem_yellow;
static int creature_image_shambler;
static int creature_image_spiked_slug;
static int creature_image_infected;
static int creature_image_gravity_crystal;
static int creature_image_peeper;
static int creature_image_leeper;
static int creature_image_spawn_egg;
static int creature_image_spawn_spider;
static int creature_image_behemoth;
static int creature_image_beacon_red;
static int creature_image_watcher;
static int creature_image_golem;
static int creature_image_phantom;
static int creature_image_rock;
static int creature_image_uniball;
static int creature_image_slugzilla;
static int creature_image_ghost;
static int creature_image_boomer;
static int creature_image_wall_shooter;
static int creature_image_spider_queen;

static void creature_update_slug(Creature* c, float dt);
static void creature_update_clinger(Creature* c, float dt);
static void creature_update_geizer(Creature* c, float dt);
static void creature_update_floater(Creature* c, float dt);
static void creature_update_floater_big(Creature* c, float dt);
static void creature_update_buzzer(Creature* c, float dt);
static void creature_update_totem_red(Creature* c, float dt);
static void creature_update_totem_blue(Creature* c, float dt);
static void creature_update_totem_yellow(Creature* c, float dt);
static void creature_update_shambler(Creature* c, float dt);
static void creature_update_spiked_slug(Creature* c, float dt);
static void creature_update_infected(Creature* c, float dt);
static void creature_update_gravity_crystal(Creature* c, float dt);
static void creature_update_peeper(Creature* c, float dt);
static void creature_update_leeper(Creature* c, float dt);
static void creature_update_spawn_egg(Creature* c, float dt);
static void creature_update_spawn_spider(Creature* c, float dt);
static void creature_update_behemoth(Creature* c, float dt);
static void creature_update_beacon_red(Creature* c, float dt);
static void creature_update_watcher(Creature* c, float dt);
static void creature_update_golem(Creature* c, float dt);
static void creature_update_phantom(Creature* c, float dt);
static void creature_update_rock(Creature* c, float dt);
static void creature_update_uniball(Creature* c, float dt);
static void creature_update_slugzilla(Creature* c, float dt);
static void creature_update_ghost(Creature* c, float dt);
static void creature_update_boomer(Creature* c, float dt);
static void creature_update_wall_shooter(Creature* c, float dt);
static void creature_update_spider_queen(Creature* c, float dt);

static void creature_fire_projectile(Creature* c, float angle);

static uint16_t id_counter = 1;
static uint16_t get_id()
{
    if(id_counter >= 65535)
        id_counter = 0;

    return id_counter++;
}

void creature_init()
{
    clist = list_create((void*)creatures, MAX_CREATURES, sizeof(Creature), false);

    creature_image_slug = gfx_load_image("src/img/creature_slug.png", false, false, 17, 17);
    creature_image_clinger = gfx_load_image("src/img/creature_clinger.png", false, false, 32, 32);
    creature_image_geizer = gfx_load_image("src/img/creature_geizer.png", false, false, 32, 64);
    creature_image_floater = gfx_load_image("src/img/creature_floater.png", false, false, 16, 16);
    creature_image_floater_big = gfx_load_image("src/img/creature_floater_big.png", false, false, 48, 48);
    creature_image_buzzer  = gfx_load_image("src/img/creature_buzzer.png", false, false, 32, 32);
    creature_image_totem_red  = gfx_load_image("src/img/creature_totem_red.png", false, false, 32, 64);
    creature_image_totem_blue  = gfx_load_image("src/img/creature_totem_blue.png", false, false, 32, 64);
    creature_image_totem_yellow  = gfx_load_image("src/img/creature_totem_yellow.png", false, false, 32, 48);
    creature_image_shambler = gfx_load_image("src/img/creature_shambler.png", false, false, 32, 64);
    creature_image_spiked_slug = gfx_load_image("src/img/creature_spiked_slug.png", false, false, 32, 32);
    creature_image_infected = gfx_load_image("src/img/creature_infected.png", false, false, 32, 32);
    creature_image_gravity_crystal = gfx_load_image("src/img/creature_gravity_crystal.png", false, false, 48, 48);
    creature_image_peeper = gfx_load_image("src/img/creature_peeper.png", false, false, 32, 32);
    creature_image_leeper = gfx_load_image("src/img/creature_leeper.png", false, false, 48, 48);
    creature_image_spawn_egg = gfx_load_image("src/img/creature_spawn_egg.png", false, false, 32, 32);
    creature_image_spawn_spider = gfx_load_image("src/img/creature_spawn_spider.png", false, false, 16, 16);
    creature_image_behemoth = gfx_load_image("src/img/creature_behemoth.png", false, false, 96, 128);
    creature_image_beacon_red = gfx_load_image("src/img/creature_beacon_red.png", false, false, 32, 32);
    creature_image_watcher = gfx_load_image("src/img/creature_watcher.png", false, false, 32, 32);
    creature_image_golem = gfx_load_image("src/img/creature_infected.png", false, false, 32, 32);   //TEMP
    creature_image_phantom = gfx_load_image("src/img/creature_phantom.png", false, false, 64, 64);
    creature_image_rock = gfx_load_image("src/img/rock1.png", false, false, 32, 32);
    creature_image_uniball = gfx_load_image("src/img/creature_uniball.png", false, false, 32, 32);
    creature_image_slugzilla = gfx_load_image("src/img/creature_slugzilla.png", false, false, 32, 32);
    creature_image_ghost = gfx_load_image("src/img/creature_ghost.png", false, false, 24, 24);
    creature_image_boomer = gfx_load_image("src/img/creature_boomer.png", false, false, 32, 32);
    creature_image_wall_shooter = gfx_load_image("src/img/creature_wallshooter.png", false, false, 32, 32);
    creature_image_spider_queen = gfx_load_image("src/img/creature_spiderqueen.png", false, false, 128, 128);
}

char* creature_type_name(CreatureType type)
{
    switch(type)
    {
        case CREATURE_TYPE_SLUG:
            return "Slug";
        case CREATURE_TYPE_CLINGER:
            return "Clinger";
        case CREATURE_TYPE_GEIZER:
            return "Geizer";
        case CREATURE_TYPE_FLOATER:
            return "Floater";
        case CREATURE_TYPE_FLOATER_BIG:
            return "Big Floater";
        case CREATURE_TYPE_BUZZER:
            return "Buzzer";
        case CREATURE_TYPE_TOTEM_RED:
            return "Totem Red";
        case CREATURE_TYPE_TOTEM_BLUE:
            return "Totem Blue";
        case CREATURE_TYPE_TOTEM_YELLOW:
            return "Totem Yellow";
        case CREATURE_TYPE_SHAMBLER:
            return "Shambler";
        case CREATURE_TYPE_SPIKED_SLUG:
            return "Spiked Slug";
        case CREATURE_TYPE_INFECTED:
            return "Infected";
        case CREATURE_TYPE_GRAVITY_CRYSTAL:
            return "Gravity Crystal";
        case CREATURE_TYPE_PEEPER:
            return "Peeper";
        case CREATURE_TYPE_LEEPER:
            return "Leeper";
        case CREATURE_TYPE_SPAWN_EGG:
            return "Spawn Egg";
        case CREATURE_TYPE_SPAWN_SPIDER:
            return "Spawn Spider";
        case CREATURE_TYPE_BEHEMOTH:
            return "Behemoth";
        case CREATURE_TYPE_BEACON_RED:
            return "Beacon Red";
        case CREATURE_TYPE_WATCHER:
            return "Watcher";
        case CREATURE_TYPE_GOLEM:
            return "Golem";
        case CREATURE_TYPE_PHANTOM:
            return "Phantom";
        case CREATURE_TYPE_ROCK_MONSTER:
            return "Rock Monster";
        case CREATURE_TYPE_ROCK:
            return "Rock";
        case CREATURE_TYPE_UNIBALL:
            return "Uniball";
        case CREATURE_TYPE_SLUGZILLA:
            return "Slugzilla";
        case CREATURE_TYPE_GHOST:
            return "Ghost";
        case CREATURE_TYPE_BOOMER:
            return "Boomer";
        case CREATURE_TYPE_WALL_SHOOTER:
            return "Wall Shooter";
        case CREATURE_TYPE_SPIDER_QUEEN:
            return "Spider Queen";
        default:
            return "???";
    }
}

int creature_get_image(CreatureType type)
{
    switch(type)
    {
        case CREATURE_TYPE_SLUG:
            return creature_image_slug;
        case CREATURE_TYPE_CLINGER:
            return creature_image_clinger;
        case CREATURE_TYPE_GEIZER:
            return creature_image_geizer;
        case CREATURE_TYPE_FLOATER:
            return creature_image_floater;
        case CREATURE_TYPE_FLOATER_BIG:
            return creature_image_floater_big;
        case CREATURE_TYPE_BUZZER:
            return creature_image_buzzer;
        case CREATURE_TYPE_TOTEM_RED:
            return creature_image_totem_red;
        case CREATURE_TYPE_TOTEM_BLUE:
            return creature_image_totem_blue;
        case CREATURE_TYPE_TOTEM_YELLOW:
            return creature_image_totem_yellow;
        case CREATURE_TYPE_SHAMBLER:
            return creature_image_shambler;
        case CREATURE_TYPE_SPIKED_SLUG:
            return creature_image_spiked_slug;
        case CREATURE_TYPE_INFECTED:
            return creature_image_infected;
        case CREATURE_TYPE_GRAVITY_CRYSTAL:
            return creature_image_gravity_crystal;
        case CREATURE_TYPE_PEEPER:
            return creature_image_peeper;
        case CREATURE_TYPE_LEEPER:
            return creature_image_leeper;
        case CREATURE_TYPE_SPAWN_EGG:
            return creature_image_spawn_egg;
        case CREATURE_TYPE_SPAWN_SPIDER:
            return creature_image_spawn_spider;
        case CREATURE_TYPE_BEHEMOTH:
            return creature_image_behemoth;
        case CREATURE_TYPE_BEACON_RED:
            return creature_image_beacon_red;
        case CREATURE_TYPE_WATCHER:
            return creature_image_watcher;
        case CREATURE_TYPE_GOLEM:
            return creature_image_golem;
        case CREATURE_TYPE_PHANTOM:
            return creature_image_phantom;
        case CREATURE_TYPE_ROCK_MONSTER:
        case CREATURE_TYPE_ROCK:
            return creature_image_rock;
        case CREATURE_TYPE_UNIBALL:
            return creature_image_uniball;
        case CREATURE_TYPE_SLUGZILLA:
            return creature_image_slugzilla;
        case CREATURE_TYPE_GHOST:
            return creature_image_ghost;
        case CREATURE_TYPE_BOOMER:
            return creature_image_boomer;
        case CREATURE_TYPE_WALL_SHOOTER:
            return creature_image_wall_shooter;
        case CREATURE_TYPE_SPIDER_QUEEN:
            return creature_image_spider_queen;
        default:
            return -1;
    }
}

char* creature_get_gun_name(CreatureType type)
{
    switch(type)
    {
        case CREATURE_TYPE_WALL_SHOOTER: return "creature_straight";
        case CREATURE_TYPE_CLINGER:    return "clinger";
        case CREATURE_TYPE_GEIZER:     return "geizer";
        case CREATURE_TYPE_TOTEM_BLUE: return "totem_blue";
        case CREATURE_TYPE_TOTEM_YELLOW: return "creature_straight";
        case CREATURE_TYPE_WATCHER:    return "watcher";
        case CREATURE_TYPE_GHOST:      return "chevrons";
        case CREATURE_TYPE_BOOMER:     return "boomerang_creature";
        case CREATURE_TYPE_BEHEMOTH:   return "behemoth";
        default:                       return "creature";
    }
}

void print_creature_dimensions(Creature* c)
{
#if 0
    static bool _prnt[CREATURE_TYPE_MAX] = {0};

    if(!_prnt[c->type])
    {
        _prnt[c->type] = true;
        printf("[%s Phys Dimensions]\n", creature_type_name(c->type));
        phys_print_dimensions(&c->phys);
    }
#endif
}

void creature_init_props(Creature* c)
{
    c->image = creature_get_image(c->type);
    Rect* vr = &gfx_images[c->image].visible_rects[0];
    // c->phys.width  = vr->w * 0.80;
    // c->phys.length = vr->w * 0.80;
    // c->phys.height = vr->h * 0.80;
    // c->phys.radius = c->phys.width / 2.0;
    c->phys.scale = 1.0;
    c->phys.stun_timer = 0.0;
    c->phys.vr = *vr;

    c->damage = 1;

    switch(c->type)
    {
        case CREATURE_TYPE_SLUG:
        {
            c->phys.speed = 60.0;
            c->act_time_min = 1.0;
            c->act_time_max = 1.0;
            c->phys.mass = 0.5;
            c->phys.base_friction = 0.0;
            c->phys.hp_max = 1.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 15;
        } break;
        case CREATURE_TYPE_CLINGER:
        {
            c->phys.scale = 0.8;
            c->phys.speed = 40.0;
            c->act_time_min = 0.1;
            c->act_time_max = 0.3;
            c->phys.mass = 1000.0;
            c->phys.base_friction = 0.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = false;
            c->windup_max = 0.5;
            c->phys.crawling = true;
            c->xp = 25;
        } break;
        case CREATURE_TYPE_GEIZER:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 0.30;
            c->act_time_max = 0.50;
            c->phys.mass = 1000.0; // so it doesn't slide when hit
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 4.0;
            c->painful_touch = false;
            c->windup_max = 0.5;
            c->xp = 35;
        } break;
        case CREATURE_TYPE_FLOATER:
        {
            c->phys.speed = 80.0;
            c->act_time_min = 0.2;
            c->act_time_max = 0.4;
            c->phys.mass = 1.0;
            c->phys.base_friction = 0.3;
            c->phys.hp_max = 2.0;
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->painful_touch = true;
            c->xp = 10;
        } break;
        case CREATURE_TYPE_FLOATER_BIG:
        {
            c->phys.scale = 0.7;
            c->phys.speed = 80.0;
            c->act_time_min = 0.2;
            c->act_time_max = 0.5;
            c->phys.mass = 1000.0;
            c->phys.hp_max = 5.0;
            c->phys.base_friction = 0.0;
            c->phys.elasticity = 1.0;
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->painful_touch = true;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_BUZZER:
        {
            c->phys.speed = 30.0;
            c->act_time_min = 0.50;
            c->act_time_max = 1.00;
            c->phys.mass = 1.0;
            c->phys.base_friction = 10.0;
            c->phys.hp_max = 3.0;
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->painful_touch = true;
            c->windup_max = 0.5;
            c->xp = 40;
        } break;
        case CREATURE_TYPE_TOTEM_RED:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 2.00;
            c->act_time_max = 3.00;
            c->phys.mass = 100000.0;
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 127;
            c->phys.floating = false;
            c->painful_touch = false;
            c->invincible = true;
            c->windup_max = 0.5;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_TOTEM_BLUE:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 4.00;
            c->act_time_max = 6.00;
            c->phys.mass = 100000.0;
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 127;
            c->phys.floating = false;
            // c->proj_type = PROJECTILE_TYPE_CREATURE_TOTEM_BLUE;
            c->painful_touch = false;
            c->invincible = true;
            c->windup_max = 0.5;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_TOTEM_YELLOW:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 0.80;
            c->act_time_max = 0.80;
            c->phys.mass = 100000.0;
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 127;
            c->phys.floating = false;
            // c->proj_type = PROJECTILE_TYPE_CREATURE_TOTEM_YELLOW;
            c->painful_touch = false;
            c->invincible = true;
            c->windup_max = 0.5;
            c->xp = 24;
        } break;
        case CREATURE_TYPE_SHAMBLER:
        {
            c->phys.speed = 50.0;
            c->act_time_min = 0.50;
            c->act_time_max = 1.00;
            c->phys.mass = 3.0;
            c->phys.base_friction = 15.0;
            c->phys.hp_max = 50.0;
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->painful_touch = true;
            c->windup_max = 0.5;
            c->xp = 300;
        } break;
        case CREATURE_TYPE_SPIKED_SLUG:
        {
            c->phys.speed = 30.0;
            c->act_time_min = 0.3;
            c->act_time_max = 1.0;
            c->phys.mass = 1.0;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 25;
        } break;
        case CREATURE_TYPE_INFECTED:
        {
            c->phys.speed = 75.0;
            c->act_time_min = 0.3;
            c->act_time_max = 1.0;
            c->phys.mass = 1.0;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 4.0;
            c->painful_touch = true;
            c->xp = 20;
            c->phys.rotation_deg = 1.01;    // avoid horizontal bool in phys_calc_collision_rect()
        } break;
        case CREATURE_TYPE_GRAVITY_CRYSTAL:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 1.0;
            c->act_time_max = 1.0;
            c->phys.mass = 400000.0;
            c->phys.base_friction = 50.0;
            c->phys.hp_max = 127;
            c->painful_touch = false;
            c->phys.elasticity = 0.0;
            c->phys.crawling = true;
            c->passive = true;
            c->xp = 0;
        } break;
        case CREATURE_TYPE_PEEPER:
        {
            c->phys.speed = 120.0;
            c->act_time_min = 1.0;
            c->act_time_max = 2.0;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = false;
            c->phys.underground = true;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_LEEPER:
        {
            c->phys.scale = 1.0;
            c->phys.speed = 120.0;
            c->act_time_min = 0.2;
            c->act_time_max = 0.8;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = true;
            c->phys.radius = 0.4*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = false;
            c->xp = 20;
        } break;
        case CREATURE_TYPE_SPAWN_EGG:
        {
            c->phys.speed = 0.0;
            c->act_time_min = 1.0;
            c->act_time_max = 1.5;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 4.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 50;
        } break;
        case CREATURE_TYPE_SPAWN_SPIDER:
        {
            c->phys.speed = 90.0;
            c->act_time_min = 0.5;
            c->act_time_max = 1.5;
            c->phys.mass = 0.1;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 2.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 10;
            c->ai_state = 0;
            c->ai_value = rand() % 8;
        } break;
        case CREATURE_TYPE_BEHEMOTH:
        {
            c->phys.scale = 0.8;
            c->phys.speed = 50.0;
            c->act_time_min = 1.00;
            c->act_time_max = 2.00;
            c->phys.mass = 1000.0;
            c->phys.base_friction = 15.0;
            c->phys.hp_max = 100.0;
            c->phys.floating = false;
            c->painful_touch = true;
            c->windup_max = 0.5;
            c->xp = 500;
        } break;
        case CREATURE_TYPE_BEACON_RED:
        {
            c->phys.speed = 50.0;
            c->act_time_min = 2.00;
            c->act_time_max = 5.00;
            c->phys.mass = 1000.0;
            c->phys.base_friction = 15.0;
            c->phys.hp_max = 7.0;
            c->phys.floating = false;
            c->painful_touch = false;
            c->windup_max = 0.5;
            c->xp = 50;
        } break;
        case CREATURE_TYPE_WATCHER:
        {
            c->phys.speed = 50.0;
            c->act_time_min = 0.30;
            c->act_time_max = 0.50;
            c->phys.mass = 10.0;
            c->phys.base_friction = 15.0;
            c->phys.hp_max = 7.0;
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->painful_touch = true;
            c->windup_max = 0.5;
            c->xp = 80;
        } break;
        case CREATURE_TYPE_GOLEM:
        {
            c->phys.speed = 75.0;
            c->act_time_min = 0.3;
            c->act_time_max = 1.0;
            c->phys.mass = 1.0;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 6.0;
            c->painful_touch = false;
            c->passive = true;
            c->friendly = true;
            c->xp = 20;
            c->color = COLOR_BLUE;
        } break;
        case CREATURE_TYPE_PHANTOM:
        {
            c->phys.ethereal = true;
            c->phys.speed = 30.0;
            c->act_time_min = 1.0;
            c->act_time_max = 1.0;
            c->phys.mass = 100.0;
            c->phys.base_friction = 10.0;
            c->phys.hp_max = 127.0;
            c->painful_touch = true;
            c->passive = true;
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->xp = 1000;
            c->damage = 127;
        } break;
        case CREATURE_TYPE_ROCK:
        case CREATURE_TYPE_ROCK_MONSTER:
        {
            c->phys.speed = 90.0;
            c->act_time_min = 1.0;
            c->act_time_max = 1.0;
            c->phys.mass = 10000.0;
            c->phys.base_friction = 100.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = false;
            c->passive = true;
            c->phys.elasticity = 0.0;
            c->xp = 20;
            c->damage = 0;
            c->phys.crawling = true;
        } break;
        case CREATURE_TYPE_UNIBALL:
        {
            c->phys.speed = 100.0;
            c->act_time_min = 0.5;
            c->act_time_max = 2.0;
            c->phys.mass = 10000.0;
            c->phys.base_friction = 0.0;
            c->phys.hp_max = 127.0;
            c->phys.elasticity = 1.0;
            c->phys.ethereal = false;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = false;
            c->phys.floating = true;
            c->phys.bobbing = false;
            c->passive = false;
            c->invincible = true;
            c->xp = 15;
        } break;
        case CREATURE_TYPE_SLUGZILLA:
        {
            c->phys.speed = 100.0;
            c->act_time_min = 0.5;
            c->act_time_max = 0.5;
            c->phys.mass = 10000.0;
            c->phys.base_friction = 0.0;
            c->phys.hp_max = 25.0;
            c->phys.elasticity = 0.0;
            c->phys.ethereal = false;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->phys.floating = false;
            c->phys.bobbing = false;
            c->passive = false;
            c->invincible = false;
            c->xp = 100;
            c->segmented = true;
        } break;
        case CREATURE_TYPE_GHOST:
        {
            c->phys.speed = 60.0;
            c->act_time_min = 0.5;
            c->act_time_max = 2.0;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 3.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.floating = true;
            c->phys.bobbing = true;
            c->phys.crawling = false;
            c->xp = 30;
        } break;
        case CREATURE_TYPE_BOOMER:
        {
            c->phys.speed = 250.0;
            c->act_time_min = 0.5;
            c->act_time_max = 0.5;
            c->phys.mass = 0.5;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 4.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->xp = 30;
        } break;
        case CREATURE_TYPE_WALL_SHOOTER:
        {
            c->phys.scale = 0.8;
            c->phys.speed = 0.0;
            c->act_time_min = 1.0;
            c->act_time_max = 1.0;
            c->phys.mass = 10000.0;
            c->phys.base_friction = 0.0;
            c->phys.hp_max = 127.0;
            c->painful_touch = false;
            c->windup_max = 0.5;
            c->phys.crawling = true;
            c->invincible = true;
            c->xp = 25;
        } break;
        case CREATURE_TYPE_SPIDER_QUEEN:
        {
            c->phys.speed = 90.0;
            c->act_time_min = 0.5;
            c->act_time_max = 1.5;
            c->phys.mass = 100.0;
            c->phys.base_friction = 20.0;
            c->phys.hp_max = 80.0;
            c->painful_touch = true;
            c->phys.radius = 0.5*MAX(c->phys.width,c->phys.height);
            c->phys.crawling = true;
            c->xp = 100;
            c->ai_state = 0;
            c->ai_value = rand() % 8;
        } break;
    }

    if(c->phys.crawling)
    {
        c->phys.width  = c->phys.vr.w;
        c->phys.length = c->phys.vr.h;
        c->phys.height = MIN(c->phys.width, c->phys.length);
    }
    else
    {
        c->phys.width  = c->phys.vr.w;
        c->phys.length = c->phys.vr.w;
        c->phys.height = c->phys.vr.h;
    }
    c->phys.radius = MIN(c->phys.vr.w, c->phys.vr.h)/2.0 * c->phys.scale * 0.90;

    if(!c->phys.crawling)
    {
        c->phys.radius *= 0.8;
    }

    // c->phys.radius *= c->phys.scale;

    // if(c->phys.crawling)
    // {
    //     c->phys.length = c->phys.height;
    //     c->phys.height = 20;
    // }

    c->phys.speed_factor = 1.0;

    print_creature_dimensions(c);
}

void creature_clear_all()
{
    list_clear(clist);
}

void creature_clear_room(uint8_t room_index)
{
    for(int i = clist->count; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        if(c->phys.curr_room == room_index)
        {
            list_remove(clist, i);
        }
    }
}

void creature_kill_all()
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        creature_die(&creatures[i]);
        list_remove(clist, i);
    }
}

void creature_kill_room(uint8_t room_index)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        if(creatures[i].phys.curr_room == room_index)
        {
            if(creatures[i].friendly) continue;
            if(creatures[i].passive) continue;
            creature_die(&creatures[i]);
            list_remove(clist, i);
        }
    }
}

static void add_to_random_wall_tile(Creature* c)
{
    int tile_x = 0;
    int tile_y = 0;

    Dir wall = (Dir)rand() % 4;

    switch(wall)
    {
        case DIR_RIGHT:
            creature_set_sprite_index(c, 2);
            tile_x = ROOM_TILE_SIZE_X;
            tile_y = (rand() % (ROOM_TILE_SIZE_Y-1));
            if(tile_y >= ROOM_TILE_SIZE_Y/2.0)tile_y+=1;
            break;
        case DIR_UP:
            creature_set_sprite_index(c, 3);
            tile_x = (rand() % (ROOM_TILE_SIZE_X-1));
            if(tile_x >= ROOM_TILE_SIZE_X/2.0)tile_x+=1;
            tile_y = -1;
            break;
        case DIR_LEFT:
            creature_set_sprite_index(c, 0);
            tile_x = -1;
            tile_y = (rand() % (ROOM_TILE_SIZE_Y-1));
            if(tile_y >= ROOM_TILE_SIZE_Y/2.0)tile_y+=1;
            break;
        case DIR_DOWN:
            creature_set_sprite_index(c, 1);
            tile_x = (rand() % (ROOM_TILE_SIZE_X-1));
            if(tile_x >= ROOM_TILE_SIZE_X/2.0)tile_x+=1;
            tile_y = ROOM_TILE_SIZE_Y;
            break;
    }

    Rect rp = level_get_tile_rect(tile_x, tile_y);

    phys_set_collision_pos(&c->phys, rp.x, rp.y);
    c->phys.pos.z = 0.0;

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;
}

static void add_to_wall_tile(Creature* c, int tile_x, int tile_y)
{
    Dir wall = DIR_NONE;
    if(tile_x == -1)
    {
        wall = DIR_LEFT;
    }
    else if(tile_x == ROOM_TILE_SIZE_X)
    {
        wall = DIR_RIGHT;
    }
    else if(tile_y == -1)
    {
        wall = DIR_UP;
    }
    else if(tile_y == ROOM_TILE_SIZE_Y)
    {
        wall = DIR_DOWN;
    }
    // printf("wall: %d\n", wall);

    switch(wall)
    {
        case DIR_RIGHT:
            creature_set_sprite_index(c, 2);
            break;
        case DIR_UP:
            creature_set_sprite_index(c, 3);
            break;
        case DIR_LEFT:
            creature_set_sprite_index(c, 0);
            break;
        case DIR_DOWN:
            creature_set_sprite_index(c, 1);
            break;
    }

    Rect rp = level_get_tile_rect(tile_x, tile_y);

    phys_set_collision_pos(&c->phys, rp.x, rp.y);

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;
}

static void add_to_random_tile(Creature* c, Room* room)
{
    Vector2i tilec = {0};
    Vector2f tilep = {0};
    level_get_rand_floor_tile(room, &tilec, &tilep);

    c->phys.pos.x = tilep.x;
    c->phys.pos.y = tilep.y;
    c->spawn_tile_x = tilec.x;
    c->spawn_tile_y = tilec.y;
}

static void add_to_tile(Creature* c, int tile_x, int tile_y)
{
    Rect rp = level_get_tile_rect(tile_x, tile_y);
    c->phys.pos.x = rp.x;
    c->phys.pos.y = rp.y;

    RectXY rxy = {0};
    rect_to_rectxy(&room_area, &rxy);

    c->spawn_tile_x = tile_x;
    c->spawn_tile_y = tile_y;
}

// specify first 3 args, or the last
Creature* creature_add(Room* room, CreatureType type, Vector2i* tile, Creature* creature)
{
    Creature c = {0};

    if(creature != NULL)
    {
        memcpy(&c, creature, sizeof(Creature));
        room = level_get_room_by_index(&level, c.phys.curr_room);
        if(!room)
        {
            // printf("no room\n");
            return NULL;
        }
    }
    else
    {
        if(room == NULL) return NULL;
        c.phys.curr_room = room->index;

        c.id = get_id();
        c.type = type;
        c.target_tile.x = -1;
        c.target_tile.y = -1;
        c.subtarget_tile.x = -1;
        c.subtarget_tile.y = -1;


        c.base_color = COLOR_TINT_NONE;
        c.color = c.base_color;

        creature_set_sprite_index(&c, 0);

        phys_calc_collision_rect(&c.phys);

        if(c.type == CREATURE_TYPE_CLINGER || c.type == CREATURE_TYPE_WALL_SHOOTER)
        {
            if(tile)
                add_to_wall_tile(&c, tile->x, tile->y);
            else
                add_to_random_wall_tile(&c);
        }
        else
        {
            if(tile)
                add_to_tile(&c, tile->x, tile->y);
            else
                add_to_random_tile(&c, room);
        }
    }

    creature_init_props(&c);

    if(creature == NULL)
    {
        c.phys.hp = c.phys.hp_max;
    }

    ai_init_action(&c);

    phys_calc_collision_rect(&c.phys);
    phys_set_collision_pos(&c.phys, c.phys.pos.x, c.phys.pos.y);
    // c->phys.radius = calc_radius_from_rect(&c->phys.collision_rect);


    if(role != ROLE_CLIENT)
    {
        if(c.segmented)
        {
            creature_segment_count = 0;
            // initialize segments
            memset(creature_segments, 0, sizeof(CreatureSegment)*MAX_SEGMENTS);

            for(int i = 0; i < MAX_SEGMENTS;++i)
            {
                CreatureSegment* cs = &creature_segments[i];

                cs->dir = angle_to_dir_cardinal(c.phys.rotation_deg);
                cs->pos.x = c.phys.collision_rect.x - (TILE_SIZE*(i+1));
                cs->pos.y = c.phys.collision_rect.y;
                cs->pos.z = c.phys.pos.z;
                cs->tail  = (i == (MAX_SEGMENTS-1));

                memcpy(&cs->collision_rect, &c.phys.collision_rect, sizeof(Rect));
                cs->collision_rect.x = cs->pos.x;
                cs->collision_rect.y = cs->pos.y;

                cs->curr_tile = level_get_room_coords_by_pos(cs->collision_rect.x, cs->collision_rect.y);

                creature_segment_count++;
            }
        }
    }

    list_add(clist, (void*)&c);

    return &creatures[clist->count-1];
}

void creature_update(Creature* c, float dt)
{
    if(role == ROLE_CLIENT)
    {
        creature_lerp(c, dt);
        return;
    }

    if(c->phys.dead)
        return;

    //HACK
    if(c->type == CREATURE_TYPE_PHANTOM)
    {
        c->phys.curr_room = visible_room->index;
    }

    if(!is_any_player_room(c->phys.curr_room))
        return;

    phys_add_circular_time(&c->phys, dt);

    if(c->phys.floating && c->phys.bobbing)
    {
        c->phys.pos.z = c->phys.height/2.0 + 10.0 + 3*sinf(5*c->phys.circular_dt);
    }

    //c->phys.prior_tile = c->phys.curr_tile;
    //c->phys.curr_tile = level_get_room_coords_by_pos(c->phys.collision_rect.x, c->phys.collision_rect.y);

    if(level_grace_time <= 0.0 && c->phys.stun_timer <= 0.0 && creatures_can_move)
    {
        switch(c->type)
        {
            case CREATURE_TYPE_SLUG:
                creature_update_slug(c,dt);
                break;
            case CREATURE_TYPE_CLINGER:
                creature_update_clinger(c,dt);
                break;
            case CREATURE_TYPE_GEIZER:
                creature_update_geizer(c,dt);
                break;
            case CREATURE_TYPE_FLOATER:
                creature_update_floater(c,dt);
                break;
            case CREATURE_TYPE_FLOATER_BIG:
                creature_update_floater_big(c,dt);
                break;
            case CREATURE_TYPE_BUZZER:
                creature_update_buzzer(c,dt);
                break;
            case CREATURE_TYPE_TOTEM_RED:
                creature_update_totem_red(c,dt);
                break;
            case CREATURE_TYPE_TOTEM_BLUE:
                creature_update_totem_blue(c,dt);
                break;
            case CREATURE_TYPE_TOTEM_YELLOW:
                creature_update_totem_yellow(c,dt);
                break;
            case CREATURE_TYPE_SHAMBLER:
                creature_update_shambler(c,dt);
                break;
            case CREATURE_TYPE_SPIKED_SLUG:
                creature_update_spiked_slug(c,dt);
                break;
            case CREATURE_TYPE_INFECTED:
                creature_update_infected(c,dt);
                break;
            case CREATURE_TYPE_GRAVITY_CRYSTAL:
                creature_update_gravity_crystal(c,dt);
                break;
            case CREATURE_TYPE_PEEPER:
                creature_update_peeper(c,dt);
                break;
            case CREATURE_TYPE_LEEPER:
                creature_update_leeper(c,dt);
                break;
            case CREATURE_TYPE_SPAWN_EGG:
                creature_update_spawn_egg(c,dt);
                break;
            case CREATURE_TYPE_SPAWN_SPIDER:
                creature_update_spawn_spider(c,dt);
                break;
            case CREATURE_TYPE_BEHEMOTH:
                creature_update_behemoth(c,dt);
                break;
            case CREATURE_TYPE_BEACON_RED:
                creature_update_beacon_red(c,dt);
                break;
            case CREATURE_TYPE_WATCHER:
                creature_update_watcher(c,dt);
                break;
            case CREATURE_TYPE_GOLEM:
                creature_update_golem(c,dt);
                break;
            case CREATURE_TYPE_PHANTOM:
                creature_update_phantom(c,dt);
                break;
            case CREATURE_TYPE_ROCK_MONSTER:
            case CREATURE_TYPE_ROCK:
                creature_update_rock(c,dt);
                break;
            case CREATURE_TYPE_UNIBALL:
                creature_update_uniball(c,dt);
                break;
            case CREATURE_TYPE_SLUGZILLA:
                creature_update_slugzilla(c,dt);
                break;
            case CREATURE_TYPE_GHOST:
                creature_update_ghost(c,dt);
                break;
            case CREATURE_TYPE_BOOMER:
                creature_update_boomer(c,dt);
                break;
            case CREATURE_TYPE_WALL_SHOOTER:
                creature_update_wall_shooter(c,dt);
                break;
            case CREATURE_TYPE_SPIDER_QUEEN:
                creature_update_spider_queen(c,dt);
                break;
        }
    }
    else
    {
        ai_stop_imm(c);
    }


    float speed = c->phys.speed*c->phys.speed_factor;

    bool moving = (c->h != 0.0 || c->v != 0.0);

    float h_speed = speed*c->h;
    float v_speed = speed*c->v;

    float vel_magn = magn(c->phys.vel);

    if(vel_magn < speed)
    {
        c->phys.vel.x += dt*h_speed;
        c->phys.vel.y += dt*v_speed;
    }
    else
    {
        // cap at speed
        Vector2f tv = {.x=c->phys.vel.x, .y=c->phys.vel.y};
        normalize(&tv);
        c->phys.vel.x = speed*tv.x;
        c->phys.vel.y = speed*tv.y;
    }

    // determine color
    c->color = gfx_blend_colors(COLOR_BLUE, c->base_color, c->phys.speed_factor); // cold effect
    c->color = c->windup && ((int)(c->phys.circular_dt * 100) % 2 == 0)? gfx_blend_colors(COLOR_YELLOW,c->color, 0.1) : c->color; // windup indication
    c->color = c->damaged ? COLOR_RED : c->color; // damaged indication

    if(c->damaged)
    {
        c->damaged_time += dt;
        if(c->damaged_time >= DAMAGED_TIME_MAX)
            c->damaged = false;
    }

    // if(c->type == CREATURE_TYPE_LEEPER)
    // {
    //     printf("  %.2f, %.2f, %.2f\n", c->phys.vel.x, c->phys.vel.y, c->phys.vel.z);
    // }

    if(!moving)
    {
        phys_apply_friction(&c->phys,c->phys.base_friction,dt);
    }

    c->phys.pos.x += dt*c->phys.vel.x;
    c->phys.pos.y += dt*c->phys.vel.y;

    if(c->segmented)
    {
        for(int i = 0; i < creature_segment_count; ++i)
        {
            CreatureSegment* cs = &creature_segments[i];

            cs->pos.x += dt*cs->vel.x;
            cs->pos.y += dt*cs->vel.y;
            cs->collision_rect.x = cs->pos.x;
            cs->collision_rect.y = cs->pos.y;
        }
    }

    phys_apply_gravity(&c->phys, GRAVITY_EARTH, dt);

    Rect r = RECT(c->phys.pos.x, c->phys.pos.y, 1, 1);
    Rect limit = room_area;

    //HACK
    if(c->type == CREATURE_TYPE_PHANTOM)
    {
        limit.w *= 2.0;
        limit.h *= 2.0;
    }

    Vector2f adj = limit_rect_pos(&limit, &r);
    c->phys.pos.x += adj.x;
    c->phys.pos.y += adj.y;

    // if(c->type == CREATURE_TYPE_LEEPER)
    // {
    //     printf("%.2f, %.2f, %.2f\n", c->phys.pos.x, c->phys.pos.y, c->phys.pos.z);
    // }


    c->phys.prior_tile = c->phys.curr_tile;
    c->phys.curr_tile = level_get_room_coords_by_pos(c->phys.collision_rect.x, c->phys.collision_rect.y);

    if(c->segmented)
    {
        for(int i = 0; i < creature_segment_count; ++i)
        {
            CreatureSegment* cs = &creature_segments[i];
            cs->curr_tile = level_get_room_coords_by_pos(cs->collision_rect.x, cs->collision_rect.y);
        }
    }

    float prior_tile_counter = c->phys.curr_tile_counter;
    if(c->phys.curr_tile.x == c->phys.prior_tile.x && c->phys.curr_tile.y == c->phys.prior_tile.y)
    {
        c->phys.curr_tile_counter += dt;
    }
    else
    {
        c->phys.curr_tile_counter = 0.0;
    }

    TileType tt = level_get_tile_type(visible_room, c->phys.curr_tile.x, c->phys.curr_tile.y);
    if(tt == TILE_BREAKABLE_FLOOR && !c->phys.floating && !c->phys.dead && !c->phys.underground)
    {
        int tc1 = (int)prior_tile_counter;
        int tc2 = (int)c->phys.curr_tile_counter;
        if(tc2-tc1 >= 1)
            visible_room->breakable_floor_state[c->phys.curr_tile.x][c->phys.curr_tile.y]++;
    }

    c->hurt = false;

}

void creature_lerp(Creature* c, float dt)
{
    if(c->phys.dead) return;

    c->lerp_t += dt;

    float tick_time = 1.0/TICK_RATE;
    float t = (c->lerp_t / tick_time);


    Vector3f lp = lerp3f(&c->server_state_prior.pos, &c->server_state_target.pos, t);
    c->phys.pos.x = lp.x;
    c->phys.pos.y = lp.y;
    c->phys.pos.z = lp.z;

    // printf("%.1f | %.1f -> %.1f  =  %.1f\n", t, c->server_state_prior.pos.x, c->server_state_target.pos.x, lp.x);
}

void creature_update_all(float dt)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        creature_update(c, dt);
        if(c->phys.dead)
        {
            list_remove(clist, i);
        }
    }
}

void creature_handle_collision(Creature* c, Entity* e)
{
    if(c->phys.ethereal) return;

    switch(e->type)
    {
        case ENTITY_TYPE_CREATURE:
        {
            Creature* c2 = (Creature*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&c->phys,&c2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&c->phys, &c2->phys,&ci);
            }
        } break;
        case ENTITY_TYPE_ITEM:
        {
            Item* p2 = (Item*)e->ptr;

            CollisionInfo ci = {0};
            bool collided = phys_collision_circles(&c->phys,&p2->phys, &ci);

            if(collided)
            {
                phys_collision_correct(&c->phys, &p2->phys,&ci);
            }
        } break;
    }
}

void creature_draw(Creature* c)
{
    if(c->phys.curr_room != player->phys.curr_room)
        return;

    if(c->phys.dead) return;

    bool horizontal = (c->phys.rotation_deg == 0.0 || c->phys.rotation_deg == 180.0);
    float dim = horizontal ? c->phys.vr.w : c->phys.vr.h;

    float y = c->phys.pos.y - (dim + c->phys.pos.z)/2.0;

    uint32_t color = c->color;

    float op = c->phys.ethereal ? 0.5 : 1.0;

    if(!c->phys.underground)
    {
        if(c->segmented)
        {
            // draw segments if there are any
            for(int i = creature_segment_count -1; i >= 0; --i)
            {
                CreatureSegment* cs = &creature_segments[i];

                int sprite_index = cs->dir+4;
                if(cs->dir == DIR_NONE) sprite_index = 4;
                if(cs->tail)
                {
                    sprite_index += 4;
                }

                gfx_sprite_batch_add(c->image, sprite_index, cs->pos.x, cs->pos.y, color, false, c->phys.scale, 0.0, op, true, false, false);
            }
        }

        // draw creature
        gfx_sprite_batch_add(c->image, c->sprite_index, c->phys.pos.x, y, color, false, c->phys.scale, 0.0, op, true, false, false);
    }
}


void creature_die(Creature* c)
{
    c->phys.dead = true;

    ProjectileOrbital* orb = projectile_orbital_get(&c->phys, 0.0);
    if(orb)
    {
        projectile_orbital_kill(orb);
    }

    bool drop_coin = true;
    bool blood = true;

    int effect_type = EFFECT_BLOOD2;

    if(c->type == CREATURE_TYPE_ROCK)
    {
        effect_type = EFFECT_SMOKE2;
        drop_coin = true;
        blood = false;
    }

    ParticleEffect* eff = &particle_effects[effect_type];

    if(role == ROLE_SERVER)
    {
        NetEvent ev = {
            .type = EVENT_TYPE_PARTICLES,
            .data.particles.effect_index = effect_type,
            .data.particles.pos = { c->phys.collision_rect.x, c->phys.collision_rect.y },
            .data.particles.scale = 1.0,
            .data.particles.color1 = eff->color1,
            .data.particles.color2 = eff->color2,
            .data.particles.color3 = eff->color3,
            .data.particles.lifetime = 0.5,
            .data.particles.room_index = c->phys.curr_room,
        };

        net_server_add_event(&ev);
    }
    else
    {
        ParticleSpawner* ps = particles_spawn_effect(c->phys.collision_rect.x,c->phys.collision_rect.y, 0.0, eff, 0.5, true, false);
        if(ps != NULL) ps->userdata = (int)c->phys.curr_room;
    }

    status_effects_clear(&c->phys);

    // // player_add_xp(player, c->xp);
    // Room* room = level_get_room_by_index(&level, c->phys.curr_room);
    // if(room == NULL)
    // {
    //     LOGE("room is null %u", c->phys.curr_room);
    //     return;
    // }

    // printf("room xp %d -> ", room->xp);
    level_room_xp += c->xp;
    // room->xp += c->xp;
    // printf("%d\n", room->xp);

    if(blood)
    {
        Decal d = {0};
        d.image = particles_image;
        d.sprite_index = 0;
        // creature_set_sprite_index(c, 0);
        d.tint = COLOR_RED;
        d.scale = 1.0;
        d.rotation = rand() % 360;
        d.opacity = 0.6;
        d.ttl = 10.0;
        d.pos.x = c->phys.collision_rect.x;
        d.pos.y = c->phys.collision_rect.y;
        d.room = c->phys.curr_room;
        decal_add(d);
    }

    if(drop_coin)
    {
        int num = 0;
        int r = rand()%100+1;
        if(r <= 60)
            num = 0;
        else if(r <= 90)
            num = 1;
        else if(r <= 98)
            num = 2;
        else if(r <= 99)    // 1/100 chance
        {
            if(rand()%4 == 0)   // 1/4 chance
            num = 10+rand()%11;
        }

        if(num > 0)
        {
            for(int i = 0; i < num; ++i)
            {
                Item* it = item_add(item_get_random_coin(), c->phys.collision_rect.x, c->phys.collision_rect.y, c->phys.curr_room);
                if(it)
                {
                    it->phys.vel.z = rand()%200+100.0;
                    it->phys.vel.x = RAND_PN()*RAND_FLOAT(0,100);
                    it->phys.vel.y = RAND_PN()*RAND_FLOAT(0,100);
                }
            }
        }
    }

    // handle_room_completion(c->phys.curr_room);
    // handle_room_completion(room);
}

Creature* creature_get_by_id(uint16_t id)
{
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].id == id)
        {
            return &creatures[i];
        }
    }
    return NULL;
}


bool creature_hurt(Creature* c, float damage, uint16_t from_id)
{
    if(c->invincible)
        return false;

    if(role == ROLE_CLIENT)
        return false;

    if(level_grace_time > 0)
    {
        return false;
    }

    float hp = (float)c->phys.hp;

    // printf("damage: %.2f, hp: %.2f -> ", damage, hp);

    hp -= damage;

    // printf("%.2f ", hp);

    if(hp <= 0)
    {
        c->phys.hp = 0;
    }
    else
    {
        c->phys.hp = (int8_t)hp;
    }

    // printf("(%d)\n", c->phys.hp);

    if(from_id != 0xFFFF)
    {
        c->hurt = true;
        c->hurt_from_id = from_id;
    }

    c->damaged = true;
    c->damaged_time = 0.0;

    if(c->phys.hp <= 0)
    {
        creature_die(c);
        return true;
    }

    return false;
}

uint16_t creature_get_count()
{
    return (uint16_t)clist->count;
}

uint16_t creature_get_room_count(uint8_t room_index, bool count_passive)
{
    uint16_t count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        // if(creatures[i].phys.curr_room == room_index && !creatures[i].phys.dead && !creatures[i].passive)
        if(creatures[i].phys.curr_room == room_index && !creatures[i].phys.dead)
        {
            if(count_passive)
                count++;
            else if(!creatures[i].passive && !creatures[i].invincible)
                count++;
        }
    }
    return count;
}

bool creature_is_on_tile(Room* room, int tile_x, int tile_y)
{
    for(int i = clist->count-1; i >= 0; --i)
    {
        Creature* c = &creatures[i];
        if(c->phys.curr_room != room->index)
            continue;

        if(c->phys.dead)
            continue;

        // printf("  creature check (%d, %d)", tile_x, tile_y);

        // simple check
        if(c->phys.curr_tile.x == tile_x && c->phys.curr_tile.y == tile_y)
        {
            // printf("  on the tile!\n");
            return true;
        }

        // if it's overlapping with the tile at all
        Vector2f pos = level_get_pos_by_room_coords(tile_x, tile_y);
        float d = dist(pos.x, pos.y, c->phys.collision_rect.x, c->phys.collision_rect.y);
        if(d <= (TILE_SIZE/2 + c->phys.radius))
        {
            // printf("  overlap with tile!\n");
            return true;
        }
        // else
        // {
        //     // printf(" tpos(%.2f, %.2f)  cpos(%.2f, %.2f)  dist(%.2f)  radius(%.2f)\n", pos.x, pos.y, c->phys.collision_rect.x, c->phys.collision_rect.y, d, c->phys.radius);
        // }

        // printf(" not on tile\n");
    }
    return false;
}

CreatureType creature_get_random()
{
    int r = rand() % 6;
    switch(r)
    {
        case 0: return CREATURE_TYPE_SLUG;
        case 1: return CREATURE_TYPE_GEIZER;
        case 2: return CREATURE_TYPE_FLOATER;
        case 3: return CREATURE_TYPE_BUZZER;
        case 4: return CREATURE_TYPE_SPIKED_SLUG;
        case 5: return CREATURE_TYPE_INFECTED;
    }

    return CREATURE_TYPE_SHAMBLER; // should never happen
}

Creature* creature_get_nearest(uint8_t room_index, float x, float y)
{
    float min_dist = 100000.0;
    int min_index = -1;

    for(int i = 0; i < clist->count; ++i)
    {
        Creature* c = &creatures[i];
        if(c->phys.dead) continue;
        if(c->passive) continue;
        if(c->friendly) continue;
        if(c->phys.curr_room != room_index) continue;

        float d = dist(x,y, c->phys.pos.x, c->phys.pos.y);
        if(d < min_dist)
        {
            min_dist = d;
            min_index = i;
        }
    }

    if(min_index < 0) return NULL;

    return &creatures[min_index];
}

static void creature_update_slug(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        ai_stop_imm(c);
        ai_move_imm(c, ai_rand(4), c->phys.speed);
    }
}

static void creature_update_spiked_slug(Creature* c, float dt)
{

    Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
    Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
    normalize(&v);

    float d = 0.0;
    Vector2f dir = {0.0, 0.0};

    switch(c->sprite_index)
    {
        case DIR_UP: {
            dir.x = 0.0;
            dir.y = -1.0;
            d = vec_dot(v, dir);
        } break;
        case DIR_RIGHT: {
            dir.x = 1.0;
            dir.y = 0.0;
            d = vec_dot(v, dir);
        } break;
        case DIR_DOWN: {
            dir.x = 0.0;
            dir.y = 1.0;
            d = vec_dot(v, dir);
        } break;
        case DIR_LEFT: {
            dir.x = -1.0;
            dir.y = 0.0;
            d = vec_dot(v, dir);
        } break;
    }

    if(d >= 0.95 && d <= 1.00)
    {
        // in line of sight, charge
        c->h = dir.x * 6.0*c->phys.speed;
        c->v = dir.y * 6.0*c->phys.speed;
    }
    else
    {
        bool act = ai_update_action(c, dt);

        if(act)
        {
            ai_stop_imm(c);
            ai_random_walk_cardinal(c);
        }
    }
}

static void creature_fire_projectile(Creature* c, float angle)
{
    Vector3f pos = {c->phys.pos.x, c->phys.pos.y, c->phys.height + c->phys.pos.z};
    Vector3f vel = {c->phys.vel.x, c->phys.vel.y, 0.0};

    if(c->type == CREATURE_TYPE_GEIZER)
    {
        vel.z = rand()%100+50;
        projectile_add(pos, &vel, c->phys.curr_room, c->room_gun_index, angle, false, &c->phys, c->id);
        return;
    }

    Gun* gun = &room_gun_list[c->room_gun_index];
    vel.z = MIN(1.0, gun->gravity_factor)*120.0;

    projectile_add(pos, &vel, c->phys.curr_room, c->room_gun_index, angle, false, &c->phys, c->id);
}

static Vector3f creature_drop_projectile(Creature* c, int tile_x, int tile_y, float vel0_z, uint32_t color)
{
    Gun* gun = &room_gun_list[c->room_gun_index];

    // gun->sprite_index = 4;
    // gun->color1 = 0x32251E;
    // gun->color2 = 0x4C3228;
    // gun->scale1 += 0.80;
    // gun->scale2 += 0.80;

    Rect r = level_get_tile_rect(tile_x, tile_y);
    Vector3f pos = {r.x-r.w/2.0+rand()%(TILE_SIZE), r.y-r.h/2.0+rand()%(TILE_SIZE), 400.0};

    Vector3f vel = {0.0, 0.0, vel0_z};
    projectile_add(pos, &vel, c->phys.curr_room, c->room_gun_index, 0.0, false, &c->phys, c->id);
    return pos;
}

static void creature_update_clinger(Creature* c, float dt)
{
    Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);

    float d = dist(p->phys.pos.x, p->phys.pos.y, c->phys.pos.x, c->phys.pos.y);
    bool horiz = (c->spawn_tile_y == -1 || c->spawn_tile_y == ROOM_TILE_SIZE_Y);

    if(c->ai_state == 1)
    {
        // firing state

        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3; // time between shots

        c->ai_counter += dt;
        if(c->ai_counter >= c->ai_counter_max)
        {
            if(c->ai_value >= 3) // total number of shots
            {
                c->ai_state = 0;
                return;
            }

            c->ai_counter = 0.0;

            // fire
            if(horiz) creature_fire_projectile(c, dir_to_angle_deg(c->spawn_tile_y == -1 ? DIR_DOWN : DIR_UP));
            else      creature_fire_projectile(c, dir_to_angle_deg(c->spawn_tile_x == -1 ? DIR_RIGHT : DIR_LEFT));

            c->ai_value++;
        }
        return;
    }

    bool player_far = (d > 200.0);

    if(player_far)
    {
        c->phys.speed = 40.0;

        // random direction to move
        bool act = ai_update_action(c, dt);
        if(act)
        {
            if(ai_flip_coin())
                ai_move_imm(c, horiz ? DIR_LEFT : DIR_UP, c->phys.speed);
            else
                ai_move_imm(c, horiz ? DIR_RIGHT : DIR_DOWN, c->phys.speed);
        }
    }
    else
    {
        // player is close
        c->phys.speed = 80.0; // speed up

        float delta_x = p->phys.pos.x - c->phys.pos.x;
        float delta_y = p->phys.pos.y - c->phys.pos.y;

        bool shooting_distance = (horiz && ABS(delta_x) < 1.0) || (!horiz && ABS(delta_y) < 1.0);

        if(shooting_distance)
        {
            ai_stop_imm(c);

            if(c->ai_state == 0)
            {
                // go to firing state
                c->ai_counter = 0.0;
                c->ai_state = 1;
                c->ai_value = 0;
            }
        }
        else
        {
            // move
            if(horiz)
            {
                ai_move_imm(c, delta_x < 0.0 ? DIR_LEFT : DIR_RIGHT, c->phys.speed);
            }
            else
            {
                ai_move_imm(c, delta_y < 0.0 ? DIR_UP : DIR_DOWN, c->phys.speed);
            }
        }
    }
}

static void creature_update_geizer(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);
    if(act)
    {
        int n_orbs = rand() % 4 + 3;
        for(int i = 0; i < n_orbs; ++i)
        {
            int angle = rand() % 360;
            creature_fire_projectile(c, angle);
        }
    }
}

static void creature_update_floater(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        ai_random_walk(c);
    }
}

static void creature_update_floater_big(Creature* c, float dt)
{
    if(c->ai_state == 0)
    {
        // initialize direction
        Vector2f v = {0.0,0.0};

        int r = rand() % 4;
        switch(r)
        {
            case 0: v.x = 1.0;  v.y = -1.0; break;
            case 1: v.x = 1.0;  v.y = 1.0; break;
            case 2: v.x = -1.0; v.y = 1.0; break;
            case 3: v.x = -1.0; v.y = -1.0; break;
        }
        c->phys.vel.x = c->phys.speed * v.x;
        c->phys.vel.y = c->phys.speed * v.y;

        c->ai_state = 1;
    }

    if(c->ai_state == 1 && c->phys.hp < 0.5*c->phys.hp_max)
    {
        c->phys.speed *= 2.0;
        c->phys.vel.x *= 2.0;
        c->phys.vel.y *= 2.0;

        c->base_color = 0x00FF9090;
        c->ai_state = 2;
    }
}

static void creature_update_buzzer(Creature* c, float dt)
{
    if(c->ai_state == 0)
    {
        // move
        bool act = ai_update_action(c, dt);

        if(act)
        {
            if(ai_rand(5) < 4)
            {
                ai_random_walk(c);
            }
            else
            {
                c->ai_counter = 0.0;
                c->ai_state = 1;
                c->ai_value = 0;
                ai_stop_moving(c);
            }
        }

    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;//RAND_FLOAT(0.2,0.5);

        c->ai_counter += dt;

        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            if(c->ai_value % 2 == 0)
            {
                // fire shots
                Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y) + RAND_FLOAT(-10.0,10.0);
                creature_fire_projectile(c, angle);
            }

            c->ai_value++;
            if(c->ai_value >= 6)
            {
                c->ai_state = 0;
            }
        }
    }

}

static void creature_update_totem_red(Creature* c, float dt)
{

    int creature_count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].phys.curr_room == c->phys.curr_room)
        {
            if(creatures[i].passive) continue;

            // same room
            if(creatures[i].type != CREATURE_TYPE_TOTEM_RED)
            {
                creature_count++;
            }
        }
    }

    if(creature_count == 0)
    {

        float a = rand()%360;
        float aplus = 360.0 / 6;
        for(int i = 0; i < 6; ++i)
        {
            creature_fire_projectile(c, a);
            a += aplus;
        }

        // deactivate totem
        creature_die(c);
    }

    if(c->ai_state == 0)
    {
        // move
        bool act = ai_update_action(c, dt);

        if(act)
        {
            c->ai_counter = 0.0;
            c->ai_state = 1;
            c->ai_value = 0;
        }
    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;

        c->ai_counter += dt;

        if(c->windup)
        {
            if(c->ai_counter >= c->windup_max)
            {
                c->ai_counter = 0.0;
                c->windup = false;
            }
        }
        else
        {
            if(c->ai_counter >= c->ai_counter_max)
            {
                c->ai_counter = 0.0;

                // fire shots
                Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
                creature_fire_projectile(c, angle);

                c->ai_value++;
                if(c->ai_value >= 3)
                {
                    c->ai_state = 0;
                }
            }
        }
    }
}

static void creature_update_totem_blue(Creature* c, float dt)
{


    uint32_t color = 0x003030FF;

    int creature_count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].phys.curr_room == c->phys.curr_room)
        {
            if(creatures[i].passive) continue;

            // same room
            if(!creatures[i].invincible)
            {
                creature_count++;
            }
        }
    }

    if(creature_count == 0)
    {
        float a = rand()%360;
        float aplus = 360.0 / 6;
        for(int i = 0; i < 6; ++i)
        {
            creature_fire_projectile(c, a);
            a += aplus;
        }

        // deactivate totem
        creature_die(c);
    }

    if(c->ai_state == 0)
    {
        bool act = ai_update_action(c, dt);

        if(act)
        {
            c->ai_counter = 0.0;
            c->ai_state = 1;
            c->ai_value = 0;
            c->windup = true;
        }
    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;

        c->ai_counter += dt;

        if(c->windup)
        {
            if(c->ai_counter >= c->windup_max)
            {
                c->ai_counter = 0.0;
                c->windup = false;
            }
        }
        else
        {
            if(c->ai_counter >= c->ai_counter_max)
            {
                c->ai_counter = 0.0;

                // fire shots
                Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);

                creature_fire_projectile(c, angle);
                creature_fire_projectile(c, angle-30);
                creature_fire_projectile(c, angle-60);
                creature_fire_projectile(c, angle+30);
                creature_fire_projectile(c, angle+60);

                c->ai_value++;
                c->ai_state = 0;
            }
        }

    }
}

static void creature_update_totem_yellow(Creature* c, float dt)
{


    int creature_count = 0;
    for(int i = 0; i < clist->count; ++i)
    {
        if(creatures[i].phys.curr_room == c->phys.curr_room)
        {
            if(creatures[i].passive) continue;

            // same room
            if(!creatures[i].invincible)
            {
                creature_count++;
            }
        }
    }

    if(creature_count == 0)
    {
        float a = rand()%360;
        float aplus = 360.0 / 6;
        for(int i = 0; i < 6; ++i)
        {
            creature_fire_projectile(c, a);
            a += aplus;
        }

        // creature_fire_projectile(c, 0.0);
        // creature_fire_projectile(c, 90.0);
        // creature_fire_projectile(c, 180.0);
        // creature_fire_projectile(c, 270.0);

        // deactivate totem
        creature_die(c);
    }

    if(c->ai_state == 0)
    {
        bool act = ai_update_action(c, dt);

        if(act)
        {
            c->ai_counter = 0.0;
            c->ai_state = 1;
            c->sprite_index = 0;
            if(c->ai_value > 1)
            {
                c->ai_value = 0;
                c->sprite_index = 0;
            }
            else if(c->ai_value == 1)
            {
                c->sprite_index = 1;
            }
            c->windup = true;
        }
    }
    else if(c->ai_state == 1)
    {
        if(c->ai_counter == 0.0)
            c->ai_counter_max = 0.3;

        c->ai_counter += dt;

        if(c->windup)
        {
            if(c->ai_counter >= c->windup_max)
            {
                c->ai_counter = 0.0;
                c->windup = false;
            }
        }
        else
        {
            if(c->ai_counter >= c->ai_counter_max)
            {
                c->ai_counter = 0.0;

                // fire shots
                if(c->ai_value == 0)
                {
                    creature_fire_projectile(c, 0.0);
                    creature_fire_projectile(c, 90.0);
                    creature_fire_projectile(c, 180.0);
                    creature_fire_projectile(c, 270.0);
                }
                else
                {
                    creature_fire_projectile(c, 45.0);
                    creature_fire_projectile(c, 135.0);
                    creature_fire_projectile(c, 225.0);
                    creature_fire_projectile(c, 315.0);
                }

                c->ai_value++;
                c->ai_state = 0;
            }
        }

    }
}

static void creature_update_shambler(Creature* c, float dt)
{
    bool low_health = (c->phys.hp < 0.30*c->phys.hp_max);
    if(low_health)
    {
        // insane-mode
        c->phys.speed = 150.0;
        c->base_color = COLOR_RED;
    }

    if(c->ai_state > 0)
    {
        c->ai_counter += dt;
        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            if(c->ai_state == 1)
            {
                // fire 5 shots
                Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
                // float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y-c->phys.pos.z/2.0, p->phys.collision_rect.x, p->phys.collision_rect.y);
                float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.collision_rect.x, p->phys.collision_rect.y);
                creature_fire_projectile(c, angle + RAND_FLOAT(-2.0,2.0));

                c->ai_value++;
                if(c->ai_value >= 5)
                {
                    c->ai_state = 0;
                }
            }
            else if(c->ai_state == 2)
            {
                // fire 4 orthogonal shots
                if(c->ai_value % 2 == 0)
                {
                    float aoffset = c->ai_value*10.0;
                    creature_fire_projectile(c, 0.0 + aoffset);
                    creature_fire_projectile(c, 90.0 + aoffset);
                    creature_fire_projectile(c, 180.0 + aoffset);
                    creature_fire_projectile(c, 270.0 + aoffset);
                }

                c->ai_value++;
                if(c->ai_value >= 18)
                {
                    c->ai_state = 0;
                }
            }
            else if(c->ai_state == 3)
            {
                // printf("teleport complete\n");
                c->ai_state = 0;

                Vector2f pos = level_get_pos_by_room_coords(c->target_tile.x, c->target_tile.y);
                c->phys.pos.x = pos.x;
                c->phys.pos.y = pos.y;

                // explode projectiles
                int num_projectiles = low_health ? 12 : 6;
                float a = 0.0;
                float aplus = 360.0 / num_projectiles;
                for(int i = 0; i < num_projectiles; ++i)
                {
                    creature_fire_projectile(c, a);
                    a += aplus;
                }
            }
        }
        return;
    }

    if(c->ai_counter == 0.0)
    {
        c->ai_counter_max = low_health ? RAND_FLOAT(0.5,1.0) : RAND_FLOAT(2.0,5.0);
    }

    c->ai_counter += dt;

    bool act = ai_update_action(c, dt);

    if(act)
    {
        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            int choice = ai_rand(low_health ? 3 : 2);

            if(choice == 0)
            {
                // printf("teleport start\n");
                // teleport
                Room* room = level_get_room_by_index(&level, c->phys.curr_room);
                Vector2i tilec = {0};
                Vector2f tilep = {0};

                level_get_rand_floor_tile(room, &tilec, &tilep);

                float duration = low_health ? 1.0 : 2.0;

                Decal d = {0};
                d.image = particles_image;
                d.sprite_index = 42;
                d.tint = COLOR_PURPLE;
                d.scale = 1.0;
                d.rotation = 0.0;
                d.opacity = 1.0;
                d.ttl = duration;
                d.pos.x = tilep.x;
                d.pos.y = tilep.y;
                d.room = c->phys.curr_room;
                d.fade_pattern = 1;
                decal_add(d);

                c->target_tile.x = tilec.x;
                c->target_tile.y = tilec.y;

                c->ai_state = 3;
                c->ai_value = 0;
                c->ai_counter = 0;
                c->ai_counter_max = duration;

                ai_random_walk(c);
            }
            else if(choice == 1)
            {
                c->ai_state = 1;
                c->ai_value = 0;
                c->ai_counter = 0;
                c->ai_counter_max = 0.2;
            }
            else if(choice == 3)    //insane-mode
            {
                c->ai_state = 2;
                c->ai_value = 0;
                c->ai_counter = 0;
                c->ai_counter_max = 0.1;
            }

        }
        else
        {
            // printf("random walk\n");
            ai_random_walk(c);
        }

    }
}

static void creature_update_infected(Creature* c, float dt)
{
    // target closest player
    Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);

    if(p)
    {
        c->target_tile.x = p->phys.curr_tile.x;
        c->target_tile.y = p->phys.curr_tile.y;

        // path find to tile
        if(ai_on_target_tile(c))
        {
            // on player tile move to player
            Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
            normalize(&v);
            c->phys.vel.x = c->phys.speed*v.x;
            c->phys.vel.y = c->phys.speed*v.y;
        }
        else
        {
            ai_path_find_to_target_tile(c);
        }
    }
}

static void creature_update_gravity_crystal(Creature* c, float dt)
{
    // F = (G*m1*m2)/(r^2)

    const double G = 1.50;

    float m1 = c->phys.mass;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player* p = &players[i];

        if(!p->active) continue;
        if(p->phys.dead) continue;
        if(p->phys.curr_room != c->phys.curr_room) continue;

        double m2 = p->phys.mass;
        double r = dist(c->phys.pos.x,c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
        double f_n = (G*m1*m2);
        double f = (f_n)/(20.0*r);

        // printf("f: %f\n", f);

        // apply gravitational pull
        Vector2f v = {c->phys.pos.x - p->phys.pos.x, c->phys.pos.y - p->phys.pos.y};
        normalize(&v);

        v.x *= f;
        v.y *= f;

        p->phys.vel.x += dt*v.x;
        p->phys.vel.y += dt*v.y;
    }
}

static void creature_update_peeper(Creature* c, float dt)
{
    // states
    // 0: Underground
    // 1: Above Ground, Shoot
    // 2: After shooting

    if(c->ai_state == 0)
    {
        c->action_counter += dt;

        if(c->action_counter >= 8.0)
        {
            ai_clear_target(c);
            c->action_counter = 0;
            c->ai_state = 1;
        }

        // underground
        c->phys.underground = true;

        if(ai_has_target(c))
        {
            // path find to tile
            if(ai_path_find_to_target_tile(c))
            {
                ai_clear_target(c);
                c->ai_state = 1;
            }
        }
        else
        {
            // choose a random tile
            ai_target_rand_tile(c);
        }

        return;
    }

    if(c->ai_state == 1)
    {
        // above ground
        c->phys.underground = false;

        bool act = ai_update_action(c, dt);
        if(act)
        {
            ai_shoot_nearest_player(c);
            c->act_time_min = 2.0;
            c->act_time_max = 4.0;
            ai_choose_new_action_max(c);
            c->ai_state = 2;
        }
        return;
    }

    if(c->ai_state == 2)
    {
        // wait to return to underground
        if(ai_update_action(c, dt))
        {
            c->act_time_min = 1.0;
            c->act_time_max = 2.0;
            ai_choose_new_action_max(c);
            c->ai_state = 0;
        }
    }

}

static void creature_update_leeper(Creature* c, float dt)
{

    if(c->ai_state == 0)
    {

        bool act = ai_update_action(c, dt);

        if(act)
        {
            Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
            if(p)
            {
                c->target_pos.x = p->phys.pos.x;
                c->target_pos.y = p->phys.pos.y;
                c->phys.vel.z = 300.0;
                c->ai_state = 1;
            }
        }
        return;
    }

    if(c->ai_state == 1)
    {

        if(c->phys.pos.z > 0.0)
        {
            c->sprite_index = 1;
            Vector2f v = {c->target_pos.x - c->phys.pos.x, c->target_pos.y - c->phys.pos.y};
            normalize(&v);
            c->phys.vel.x = c->phys.speed*v.x;
            c->phys.vel.y = c->phys.speed*v.y;
        }
        else
        {
            c->sprite_index = 0;
            ai_stop_moving(c);
            c->ai_state = 0;
        }
        return;
    }

}

static void creature_update_spawn_egg(Creature* c, float dt)
{

    bool act = ai_update_action(c, dt);
    if(!act) return;

    c->ai_state++;

    if(c->ai_state == 3)
    {
        Vector2i t = {.x=c->phys.curr_tile.x, .y=c->phys.curr_tile.y};
        Room* room = level_get_room_by_index(&level, c->phys.curr_room);

        for(int i = 0; i < 3+rand()%3; ++i)
        {
            creature_add(room, CREATURE_TYPE_SPAWN_SPIDER, &t, NULL);
        }

        c->xp = 0;
        creature_die(c);
        return;
    }

    c->sprite_index = c->ai_state;
}

static void creature_update_spawn_spider(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        int r = ai_rand(2);

        if(r == 0)
        {
            // move toward player
            c->ai_state = 1;
            c->act_time_min = 1.5;
            c->act_time_min = 2.5;
        }
        else
        {
            // random movement
            c->ai_state = 0;
            c->ai_value = rand() % 8;
            c->act_time_min = 0.2;
            c->act_time_min = 0.3;
            ai_stop_imm(c);
        }
    }


    if(c->ai_state == 0)
    {
        ai_walk_dir(c,c->ai_value);
        return;
    }

    if(c->ai_state == 1)
    {
        // move toward nearest player
        Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);

        Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
        normalize(&v);

        float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);

        Dir dir = angle_to_dir_cardinal(angle);
        creature_set_sprite_index(c, dir);

        c->phys.vel.x = c->phys.speed*v.x;
        c->phys.vel.y = c->phys.speed*v.y;
        return;
    }

    return;
}

static void creature_update_behemoth(Creature* c, float dt)
{
    bool angry = (c->phys.hp < 0.5*c->phys.hp_max);
    if(angry)
    {
        c->base_color = COLOR_RED;
    }

    if(c->ai_state == 0)
    {
        bool act = ai_update_action(c, dt);
        if(!act) return;

        // choose the next state

        int r = angry ? ai_rand(3) : ai_rand(2);

        if(r == 0 && !c->phys.underground)
        {
            // go underground and spawn creatures
            // c->phys.underground = true;
            c->ai_state = 1;
            c->act_time_min = 0.5;
            c->act_time_max = 1.0;
            ai_choose_new_action_max(c);
        }
        else
        {
            if(c->phys.underground)
            {
                c->phys.vel.z = rand()%200+100.0;
            }
            // rain fire
            c->phys.underground = false;
            c->ai_state = 3;
            c->ai_value = 0;
            c->act_time_min = angry ? 0.2 : 0.3;
            c->act_time_max = angry ? 0.5 : 0.6;
            ai_choose_new_action_max(c);
        }
        return;
    }

    if(c->ai_state == 1)
    {
        bool act = ai_update_action(c, dt);
        if(!act) return;

        Vector2i rand_tile;
        Vector2f rand_pos;
        // spawn creatures
        int n = angry ? ai_rand(4) + 3 : ai_rand(2) + 2;
        for(int i = 0; i < n; ++i)
        {
            level_get_rand_floor_tile(visible_room, &rand_tile, &rand_pos);
            Creature* c = creature_add(visible_room, CREATURE_TYPE_PEEPER, &rand_tile, NULL);
            if(c)
            {
                refresh_visible_room_gun_list();
                // add_to_room_gun_list_creature(c);
                ParticleSpawner* ps = particles_spawn_effect(rand_pos.x,rand_pos.y, 0.0, &particle_effects[EFFECT_SMOKE], 0.3, true, false);
                if(ps != NULL) ps->userdata = (int)c->phys.curr_room;
            }
        }

        c->ai_state = 2;
        c->phys.underground = true;
        if(angry)
        {
            // wait longer underground
            c->act_time_min = 12.0;
            c->act_time_max = 15.0;
        }
        else
        {
            c->act_time_min = 8.0;
            c->act_time_max = 10.0;
        }
        ai_choose_new_action_max(c);
        return;
    }

    // underground state
    if(c->ai_state == 2)
    {
        uint16_t ccount = creature_get_room_count(c->phys.curr_room, false);
        bool act = ai_update_action(c, dt);

        if(act || ccount == 1)
        {
            c->ai_state = 0;
            c->act_time_min = 0.5;
            c->act_time_max = 1.0;
            ai_choose_new_action_max(c);
            return;
        }
    }

    if(c->ai_state == 3)
    {

        bool act = ai_update_action(c, dt);
        if(!act) return;

        if(c->ai_value == 0)
        {
            //choose starting point
            Dir dir = rand()%4;
            c->ai_value2 = dir;    // represents the direction of the wave of falling projectiles
            if(dir == DIR_RIGHT)
                c->ai_value3 = 0;
            else if(dir == DIR_LEFT)
                c->ai_value3 = ROOM_TILE_SIZE_X-1;
            else if(dir == DIR_DOWN)
                c->ai_value3 = 0;
            else if(dir == DIR_UP)
                c->ai_value3 = ROOM_TILE_SIZE_Y-1;
        }

        Dir dir = c->ai_value2;

        int lst[ROOM_TILE_SIZE_X] = {0};
        int lst_size = 0;
        int num_projectiles = 0;
        if(dir == DIR_LEFT || dir == DIR_RIGHT)
        {
            lst_size = ROOM_TILE_SIZE_Y;
            num_projectiles = angry ? 6 : 4;
        }
        else
        {
            lst_size = ROOM_TILE_SIZE_X;
            num_projectiles = angry ? 10 : 8;
        }

        // initialize the list
        for(int i = 0; i < lst_size; ++i)
            lst[i] = i;

        // shuffle the list
        for(int i = 0; i < lst_size; ++i)
        {
            int idx = rand() % lst_size;
            int value = lst[idx];
            lst[idx] = lst[i];
            lst[i] = value;
        }

        for(int i = 0; i < num_projectiles; ++i)
        {
            int value = lst[i];
            Vector2i tile = {0};

            if(dir == DIR_LEFT || dir == DIR_RIGHT)
            {
                tile.x = c->ai_value3;
                tile.y = value;
            }
            else
            {
                tile.x = value;
                tile.y = c->ai_value3;
            }

            Rect trect = level_get_tile_rect(tile.x, tile.y);
            Vector3f pos = creature_drop_projectile(c, tile.x, tile.y, (angry ? -400.0 : 0.0), COLOR_RED);

            // add decal to show where projectile is going
            Decal d = {0};
            d.image = particles_image;
            d.sprite_index = 80;
            d.tint = COLOR_RED;
            d.scale = 1.0;
            d.rotation = 0;
            d.opacity = 0.6;
            d.ttl = 2.0;
            d.pos.x = trect.x;
            d.pos.y = trect.y;
            d.room = c->phys.curr_room;
            d.fade_pattern = 2;
            decal_add(d);
        }

        bool finished = false;
        if(dir == DIR_LEFT)
        {
            c->ai_value3--;
            if(c->ai_value3 < 0) finished = true;
        }
        else if(dir == DIR_RIGHT)
        {
            c->ai_value3++;
            if(c->ai_value3 >= ROOM_TILE_SIZE_X) finished = true;
        }
        else if(dir == DIR_UP)
        {
            c->ai_value3--;
            if(c->ai_value3 < 0) finished = true;
        }
        else if(dir == DIR_DOWN)
        {
            c->ai_value3++;
            if(c->ai_value3 >= ROOM_TILE_SIZE_Y) finished = true;
        }

        if(finished)
        {
            c->act_time_min = 1.00;
            c->act_time_max = 2.00;
            ai_choose_new_action_max(c);
            c->ai_state = 0;
        }

        c->ai_value++;
        return;
    }

}

static void creature_update_beacon_red(Creature* c, float dt)
{
    if(c->ai_state == 0)
    {
        bool act = ai_update_action(c, dt);
        if(!act) return;

        c->ai_state = 1; // start raining fire
        c->act_time_min = 0.5;
        c->act_time_max = 0.5;

        Decal d = {
            .image = particles_image,
            .sprite_index = 80,
            .tint = COLOR_RED,
            .scale = 1.0,
            .rotation = 0.0,
            .opacity = 0.6,
            .ttl = 5.0,
            .pos.x = c->phys.pos.x,
            .pos.y = c->phys.pos.y,
            .room = c->phys.curr_room,
            .fade_pattern = 0
        };

        decal_add(d);

        ai_choose_new_action_max(c);
        return;
    }

    bool act = ai_update_action(c, dt);
    if(!act) return;

    Room* room = level_get_room_by_index(&level, c->phys.curr_room);

    Vector2i rand_tile;
    Vector2f rand_pos;

    if(c->ai_value >=  10)
    {
        // return to waiting state
        c->act_time_min = 2.0;
        c->act_time_max = 5.0;
        ai_choose_new_action_max(c);
        c->ai_value = 0;
        c->ai_state = 0;
        return;
    }

    // rain fire
    int n = ai_rand(15) + 5;

    for(int i = 0; i < n; ++i)
    {
        level_get_rand_floor_tile(room, &rand_tile, &rand_pos);
        creature_drop_projectile(c, rand_tile.x, rand_tile.y, 0.0, COLOR_RED);

        // add decal to show where projectile is going
        Decal d = {
            .image = particles_image,
            .sprite_index = 80,
            .tint = COLOR_RED,
            .scale = 1.0,
            .rotation = 0.0, //rand() % 360,
            .opacity = 0.6,
            .ttl = 2.0,
            .pos.x = rand_pos.x,
            .pos.y = rand_pos.y,
            .room = c->phys.curr_room,
            .fade_pattern = 1
        };

        decal_add(d);
    }

    c->ai_value++;
    return;
}

static void creature_update_watcher(Creature* c, float dt)
{

    Gun* gun = &room_gun_list[c->room_gun_index];

    ProjectileOrbital* orb = projectile_orbital_get(&c->phys, gun->orbital_distance);

    if(!orb)
    {
        for(int i = 0; i < 4; ++i)
            creature_fire_projectile(c, 0.0);
    }

    bool act = ai_update_action(c, dt);

    if(act)
    {
        if(ai_flip_coin())
        {
            ai_random_walk(c);
        }
    }
}

static void creature_update_golem(Creature* c, float dt)
{
    Vector2i target = {0};
    Vector2f target_pos = {0};

    // target closest creature
    Creature* ct = creature_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
    if(ct)
    {
        printf("found creature target\n");
        target.x = c->phys.curr_tile.x;
        target.y = c->phys.curr_tile.y;
        target_pos.x = ct->phys.collision_rect.x;
        target_pos.y = ct->phys.collision_rect.y;
    }
    else
    {
        Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);
        if(!p) return;
        target.x = p->phys.curr_tile.x;
        target.y = p->phys.curr_tile.y;
        target_pos.x = p->phys.collision_rect.x;
        target_pos.y = p->phys.collision_rect.y;
    }

    Dir dir = get_dir_from_coords2(target.x, target.y, c->phys.curr_tile.x, c->phys.curr_tile.y);
    if(dir == DIR_NONE)
    {
        float angle_deg = calc_angle_deg(target_pos.x, target_pos.y, c->phys.pos.x, c->phys.pos.y);
        dir = angle_to_dir_cardinal(angle_deg);
    }


    Vector2i o = get_dir_offsets(dir);

    //TODO
    o.x *= 1;
    o.y *= 1;


    target.x += o.x;
    target.y += o.y;

    c->target_tile.x = target.x;
    c->target_tile.y = target.y;

    printf("%s  (%d, %d) -> (%d, %d)\n", get_dir_name(dir), c->phys.curr_tile.x, c->phys.curr_tile.y, c->target_tile.x, c->target_tile.y);

    // Vector2f pos = level_get_pos_by_room_coords(target.x, target.y);

    if(ai_on_target_tile(c))
    {
        printf("on target\n");
        // // on player tile move to target
        // Vector2f v = {pos.x - c->phys.pos.x, pos.y - c->phys.pos.y};
        // normalize(&v);
        // c->phys.vel.x = c->phys.speed*v.x;
        // c->phys.vel.y = c->phys.speed*v.y;
    }
    else
    {
        printf("finding path\n");
        ai_path_find_to_target_tile(c);
    }

}

static void creature_update_phantom(Creature* c, float dt)
{
    // target closest player
    Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);

    if(!p) return;

    Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
    normalize(&v);

    //c->phys.vel.x = c->phys.speed*v.x;
    //c->phys.vel.y = c->phys.speed*v.y;

    c->h = v.x;
    c->v = v.y;

    Dir dir = get_dir_from_coords2(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);
    _update_sprite_index(c, dir);
}

static void creature_update_rock(Creature* c, float dt)
{
    if(c->type == CREATURE_TYPE_ROCK)
    {
        if(c->phys.hp == c->phys.hp_max)
            c->sprite_index = 0;
        else if(c->phys.hp == c->phys.hp_max-1)
            c->sprite_index = 1;
        else
            c->sprite_index = 2;
        return;
    }

    if(c->type == CREATURE_TYPE_ROCK_MONSTER)
    {
        if(c->phys.hp != c->phys.hp_max)
        {
            c->passive = false;
        }

        if(!c->passive)
        {
            // target closest player
            Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);

            if(!p) return;

            Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
            normalize(&v);

            //c->phys.vel.x = c->phys.speed*v.x;
            //c->phys.vel.y = c->phys.speed*v.y;

            c->h = v.x;
            c->v = v.y;
        }
    }
}

static void creature_update_uniball(Creature* c, float dt)
{
    printf("creature rot deg: %f, vel: %f %f\n", c->phys.rotation_deg, c->phys.vel.x, c->phys.vel.y);

    if(c->phys.rotation_deg == 0.0 && c->phys.vel.x == 0.0)
    {
        c->phys.vel.x = c->phys.speed;
    }
    else if(c->phys.rotation_deg == 90.0 && c->phys.vel.y == 0.0)
    {
        c->phys.vel.y = -c->phys.speed;
    }
    else if(c->phys.rotation_deg == 180.0 && c->phys.vel.x == 0.0)
    {
        c->phys.vel.x = -c->phys.speed;
    }
    else if(c->phys.rotation_deg == 270.0 && c->phys.vel.y == 0.0)
    {
        c->phys.vel.y = c->phys.speed;
    }
}
static void creature_update_slugzilla(Creature* c, float dt)
{
    //ai_align_to_grid(c);

    if(ai_on_target(c))
    {
        ai_clear_target(c);
    }

    if(!ai_has_target(c))
    {
        bool found_tile = ai_target_random_adjacent_tile(c);

        if(!found_tile)
        {
            return;
        }

        //printf("Picking new target: [%d, %d]\n", c->target_tile.x, c->target_tile.y);
        
        if(c->target_tile.x > c->phys.curr_tile.x)      _update_sprite_index(c,DIR_RIGHT); 
        else if(c->target_tile.x < c->phys.curr_tile.x) _update_sprite_index(c,DIR_LEFT);
        else if(c->target_tile.y > c->phys.curr_tile.y) _update_sprite_index(c,DIR_DOWN);
        else if(c->target_tile.y < c->phys.curr_tile.y) _update_sprite_index(c,DIR_UP);

        // propagate targets to segments
        for(int i = 0; i < MAX_SEGMENTS; ++i)
        {
            CreatureSegment* cs = &creature_segments[i];

            if(i == 0)
                cs->target_tile = c->phys.curr_tile;
            else
                cs->target_tile = creature_segments[i-1].curr_tile;

            if(cs->target_tile.x > cs->curr_tile.x)      cs->dir = DIR_RIGHT;
            else if(cs->target_tile.x < cs->curr_tile.x) cs->dir = DIR_LEFT;
            else if(cs->target_tile.y > cs->curr_tile.y) cs->dir = DIR_DOWN;
            else if(cs->target_tile.y < cs->curr_tile.y) cs->dir = DIR_UP;
                
            //printf("  Propagating target to segment %d: [%d, %d]\n", i, cs->target_tile.x, cs->target_tile.y);
        }
    }

    // move head
    ai_move_imm_to_target(c, dt);

    // move segments
    for(int i = 0; i < MAX_SEGMENTS; ++i)
    {
        CreatureSegment* seg = &creature_segments[i];

        Vector2f target_pos = level_get_pos_by_room_coords(seg->target_tile.x, seg->target_tile.y);
        Vector2f v = {target_pos.x - seg->pos.x, target_pos.y - seg->pos.y};

        normalize(&v);

        float speed = c->phys.speed*c->phys.speed_factor;

        float h_speed = speed*v.x;
        float v_speed = speed*v.y;

        seg->vel.x = h_speed;
        seg->vel.y = v_speed;
    }
}


#if 0
static void creature_update_slugzilla(Creature* c, float dt)
{

#define _TEST_    0
#define _DEBUG_   0

    if(c->ai_state == 0)
    {
        if(!ai_has_target(c))
        {
#if !_TEST_
            for(int i = 0; i < 5; ++i)
            {
                srand(time(0));
                level_get_rand_floor_tile(visible_room, &c->target_tile, &c->target_pos);
                astar_set_traversable_func(&level.asd, level_tile_traversable_func);
                bool traversable = astar_traverse(&level.asd, c->phys.curr_tile.x, c->phys.curr_tile.y, c->target_tile.x, c->target_tile.y);
                if(!traversable)
                {
                    c->target_tile.x = -1;
                    c->target_tile.y = -1;
                }
                else
                {
                    // printf("set the target to %d, %d\n", c->target_tile.x, c->target_tile.y);
                    break;
                }
            }
#else
            c->target_tile.x = c->phys.curr_tile.x+3;
            c->target_tile.y = c->phys.curr_tile.y+2;
#endif
        }

        if(!ai_has_subtarget(c))
        {
            astar_set_traversable_func(&level.asd, level_tile_traversable_func);
            bool traversable = astar_traverse(&level.asd, c->phys.curr_tile.x, c->phys.curr_tile.y, c->target_tile.x, c->target_tile.y);
            if(!traversable)
            {
                c->target_tile.x = -1;
                c->target_tile.y = -1;
                return;
            }

            AStarNode_t* next = NULL;
            if(level.asd.pathlen == 1)
                next = &level.asd.path[0];
            else
                next = &level.asd.path[1];

            Vector2i end = {.x=next->x, .y=next->y};

            if(c->subtarget_tile.x != c->target_tile.x || c->subtarget_tile.y != c->target_tile.y)
            {
                Vector2i dif = {0};
                dif.x = c->phys.curr_tile.x - next->x;
                dif.y = c->phys.curr_tile.y - next->y;

                // look for straight paths
                for(int i = 2; i < level.asd.pathlen; ++i)
                {
                    AStarNode_t* n = &level.asd.path[i];

                    Vector2i dif2 = {0};
                    dif2.x = end.x - n->x;
                    dif2.y = end.y - n->y;

                    if(dif.x == dif2.x && dif.y == dif2.y)
                    {
                        end.x = n->x;
                        end.y = n->y;
                    }
                    else
                        break;
                }
            }

            c->subtarget_tile.x = end.x;
            c->subtarget_tile.y = end.y;

            Rect end_pos = level_get_tile_rect(c->subtarget_tile.x, c->subtarget_tile.y);
            for(int i = 0; i < creature_segment_count; ++i)
            {
                CreatureSegment* cs = &creature_segments[i];

                if(cs->target_count == 0)
                {
#if _DEBUG_
                    if(cs->tail)
                    {
                        printf("initializing targets\n");
                    }
#endif
                    cs->target_tile[cs->target_count].x = c->phys.curr_tile.x;
                    cs->target_tile[cs->target_count].y = c->phys.curr_tile.y;
                    Rect pos = level_get_tile_rect(c->phys.curr_tile.x, c->phys.curr_tile.y);
                    cs->target_pos[cs->target_count].x = pos.x;
                    cs->target_pos[cs->target_count].y = pos.y;
                    cs->target_count++;
                }

                if(cs->target_count == 5)
                {
                    printf("target count is too big!\n");
                    continue;
                }


                cs->target_tile[cs->target_count].x = c->subtarget_tile.x;
                cs->target_tile[cs->target_count].y = c->subtarget_tile.y;
                cs->target_pos[cs->target_count].x = end_pos.x;
                cs->target_pos[cs->target_count].y = end_pos.y;
                cs->target_count++;
#if _DEBUG_
                if(cs->tail)
                {
                    printf("Sub targets %d\n", cs->target_count);
                    for(int j = 0; j < cs->target_count; ++j)
                        printf("  %d) %d, %d\n", j, cs->target_tile[j].x, cs->target_tile[j].y);
                }
#endif
            }

        }

        Rect end_pos = level_get_tile_rect(c->subtarget_tile.x, c->subtarget_tile.y);
        Vector2f v = {end_pos.x - c->phys.collision_rect.x, end_pos.y - c->phys.collision_rect.y};

        if(ABS(v.x) < 1.0 && ABS(v.y) < 1.0)
        {

            ai_stop_imm(c);
            if(c->subtarget_tile.x == c->target_tile.x && c->subtarget_tile.y == c->target_tile.y)
            {
                // printf("reached the target\n");
                c->target_tile.x = -1;
                c->target_tile.y = -1;
                c->ai_state = 1;

                for(int i = 0; i < creature_segment_count; ++i)
                {
                    CreatureSegment* cs = &creature_segments[i];
                    // memset(cs->target_tile, 0, 5*sizeof(Vector2i));
                    // cs->target_count = 0;
                    cs->vel.x = 0;
                    cs->vel.y = 0;
                }

                return;
            }
            c->subtarget_tile.x = -1;
            c->subtarget_tile.y = -1;
        }
        else
        {
            Dir direction = get_dir_from_pos(c->phys.collision_rect.x, c->phys.collision_rect.y, end_pos.x, end_pos.y);

            _update_sprite_index(c, direction);

            float range = c->phys.speed/300.0*5.0;
            float speed = c->phys.speed;
            if(ABS(v.x) <= range && ABS(v.y) <= range)
                speed = MIN(80.0, c->phys.speed/4.0);

            c->curr_speed = speed;
            ai_move_imm(c, direction, speed);

        }

        for(int i = 0; i < creature_segment_count; ++i)
        {
            CreatureSegment* cs = &creature_segments[i];
            if(cs->target_count <= 0) continue;

            Vector2f v = {cs->target_pos[0].x - cs->pos.x, cs->target_pos[0].y - cs->pos.y};

            if(ABS(v.x) < 1.0 && ABS(v.y) < 1.0)
            {
                cs->pos.x = cs->target_pos[0].x;
                cs->pos.y = cs->target_pos[0].y;
#if _DEBUG_
                if(cs->tail)
                {
                    printf("Reached %d, %d\n", cs->target_tile[0].x, cs->target_tile[0].y);
                }
#endif

                cs->target_count--;
                for(int j = 1; j < 5; ++j)
                {
                    memcpy(&cs->target_tile[j-1], &cs->target_tile[j], sizeof(Vector2i));
                    memcpy(&cs->target_pos[j-1], &cs->target_pos[j], sizeof(Vector2f));
                }
                // v.x = cs->target_pos[0].x - cs->pos.x;
                // v.y = cs->target_pos[0].y - cs->pos.y;
#if _DEBUG_
                if(cs->tail)
                {
                    printf("(count: %d) moving to %d, %d\n", cs->target_count, cs->target_tile[0].x, cs->target_tile[0].y);
                }
#endif
            }

            if(cs->target_count > 0)
            {
                Dir direction = get_dir_from_pos(cs->pos.x, cs->pos.y, cs->target_pos[0].x, cs->target_pos[0].y);
                cs->dir = direction;
                Vector2i o = get_dir_offsets(direction);
                cs->vel.x = o.x*c->curr_speed;
                cs->vel.y = o.y*c->curr_speed;
#if _DEBUG_
                if(cs->tail)
                {
                    printf("  moving %s  %.2f, %.2f -> %.2f, %.2f\n", get_dir_name(direction), cs->pos.x, cs->pos.y, cs->target_pos[0].x, cs->target_pos[0].y);
                }
#endif
            }
            else
            {
                cs->vel.x = 0.0;
                cs->vel.y = 0.0;
            }


        }
    }
    else if(c->ai_state == 1)
    {
        bool act = ai_update_action(c, dt);
#if !_TEST_
        if(act) c->ai_state = 0;
#endif
    }

}

//#else

static void creature_update_slugzilla(Creature* c, float dt)
{
    if(creature_segment_count != 4)
    {
        // initialize segments
        memset(creature_segments, 0, sizeof(CreatureSegment)*MAX_SEGMENTS);

        for(int i = 0; i < 4;++i)
        {
            CreatureSegment* cs = &creature_segments[i];

            cs->dir = angle_to_dir_cardinal(c->phys.rotation_deg);
            cs->pos.x = c->phys.collision_rect.x - (TILE_SIZE*(i+1));
            cs->pos.y = c->phys.collision_rect.y;
            cs->pos.z = c->phys.pos.z;
            cs->tail  = (i == 3);

            memcpy(&cs->collision_rect, &c->phys.collision_rect, sizeof(Rect));

            creature_segment_count++;
        }
    }

    if(c->ai_state == 0)
    {
        if(!ai_has_target(c))
        {
            for(int i = 0; i < 5; ++i)
            {
                level_get_rand_floor_tile(visible_room, &c->target_tile, &c->target_pos);
                astar_set_traversable_func(&level.asd, level_tile_traversable_func);
                bool traversable = astar_traverse(&level.asd, c->phys.curr_tile.x, c->phys.curr_tile.y, c->target_tile.x, c->target_tile.y);
                if(!traversable)
                {
                    c->target_tile.x = -1;
                    c->target_tile.y = -1;
                }
                else
                {
                    // printf("set the target to %d, %d\n", c->target_tile.x, c->target_tile.y);
                    break;
                }
            }
        }
        else
        {
            if(!ai_has_subtarget(c))
            {
                astar_set_traversable_func(&level.asd, level_tile_traversable_func);
                bool traversable = astar_traverse(&level.asd, c->phys.curr_tile.x, c->phys.curr_tile.y, c->target_tile.x, c->target_tile.y);
                if(!traversable)
                {
                    c->target_tile.x = -1;
                    c->target_tile.y = -1;
                    return;
                }

                AStarNode_t* next = NULL;
                if(level.asd.pathlen == 1)
                    next = &level.asd.path[0];
                else
                    next = &level.asd.path[1];

                Vector2i end = {.x=next->x, .y=next->y};

                if(c->subtarget_tile.x != c->target_tile.x || c->subtarget_tile.y != c->target_tile.y)
                {
                    Vector2i dif = {0};
                    dif.x = c->phys.curr_tile.x - next->x;
                    dif.y = c->phys.curr_tile.y - next->y;

                    // look for straight paths
                    for(int i = 2; i < level.asd.pathlen; ++i)
                    {

                        AStarNode_t* n = &level.asd.path[i];

                        Vector2i dif2 = {0};
                        dif2.x = end.x - n->x;
                        dif2.y = end.y - n->y;

                        if(dif.x == dif2.x && dif.y == dif2.y)
                        {
                            end.x = n->x;
                            end.y = n->y;
                        }
                        else
                            break;
                    }
                }

                c->subtarget_tile.x = end.x;
                c->subtarget_tile.y = end.y;
            }

            Rect end_pos = level_get_tile_rect(c->subtarget_tile.x, c->subtarget_tile.y);
            Vector2f v = {end_pos.x - c->phys.collision_rect.x, end_pos.y - c->phys.collision_rect.y};

            if(ABS(v.x) < 1.0 && ABS(v.y) < 1.0)
            {
                ai_stop_imm(c);
                if(c->subtarget_tile.x == c->target_tile.x && c->subtarget_tile.y == c->target_tile.y)
                {
                    // printf("reached the target\n");
                    c->target_tile.x = -1;
                    c->target_tile.y = -1;
                    c->ai_state = 1;
                    return;
                }
                c->subtarget_tile.x = -1;
                c->subtarget_tile.y = -1;
            }
            else
            {
                Dir direction = get_dir_from_pos(c->phys.collision_rect.x, c->phys.collision_rect.y, end_pos.x, end_pos.y);

                _update_sprite_index(c, direction);

                float range = c->phys.speed/300.0*5.0;
                float speed = c->phys.speed;
                if(ABS(v.x) <= range && ABS(v.y) <= range)
                    speed = MIN(80.0, c->phys.speed/4.0);

                ai_move_imm(c, direction, speed);

                for(int i = 0; i < creature_segment_count; ++i)
                {
                    CreatureSegment* cs = &creature_segments[i];

                    // update direction

                    float _posx = c->phys.collision_rect.x;
                    float _posy = c->phys.pos.y;

                    if(i > 0)
                    {
                        _posx = creature_segments[i-1].pos.x;
                        _posy = creature_segments[i-1].pos.y;
                    }

                    float delta_x = _posx - cs->pos.x;
                    float delta_y = _posy - cs->pos.y;

                    if(ABS(delta_x) >= ABS(delta_y))
                    {
                        cs->dir = delta_x >= 0 ? DIR_RIGHT : DIR_LEFT;
                    }
                    else
                    {
                        cs->dir = delta_y >= 0 ? DIR_DOWN : DIR_UP;
                    }

                    // move segment
                    switch(cs->dir)
                    {
                        case DIR_UP:
                            cs->vel.x = 0.0;
                            cs->vel.y = -speed;
                            break;
                        case DIR_RIGHT:
                            cs->vel.x = +speed;
                            cs->vel.y = 0.0;
                            break;
                        case DIR_DOWN:
                            cs->vel.x = 0.0;
                            cs->vel.y = +speed;
                            break;
                        case DIR_LEFT:
                            cs->vel.x = -speed;
                            cs->vel.y = 0.0;
                            break;
                    }

                }
            }
        }
    }
    else if(c->ai_state == 1)
    {
        bool act = ai_update_action(c, dt);
        if(act) c->ai_state = 0;
    }

}
#endif

static void creature_update_ghost(Creature* c, float dt)
{
    if(c->ai_state > 0)
    {
        c->ai_counter += dt;
        if(c->ai_counter >= c->ai_counter_max)
        {
            c->ai_counter = 0.0;

            if(c->ai_state == 1)
            {
                c->ai_state = 0;

                Vector2f pos = level_get_pos_by_room_coords(c->target_tile.x, c->target_tile.y);
                c->phys.pos.x = pos.x;
                c->phys.pos.y = pos.y;

                // explode projectiles
                int num_projectiles = 6;
                float a = 0.0;
                float aplus = 360.0 / num_projectiles;
                for(int i = 0; i < num_projectiles; ++i)
                {
                    creature_fire_projectile(c, a);
                    a += aplus;
                }
            }
        }

        return;
    }

    bool act = ai_update_action(c, dt);

    if(act)
    {
        Room* room = level_get_room_by_index(&level, c->phys.curr_room);
        Vector2i tilec = {0};
        Vector2f tilep = {0};

        level_get_rand_floor_tile(room, &tilec, &tilep);

        float duration = 2.0;

        Decal d = {0};
        d.image = particles_image;
        d.sprite_index = 42;
        d.tint = COLOR_GREEN;
        d.scale = 1.0;
        d.rotation = 0.0;
        d.opacity = 0.5;
        d.ttl = duration;
        d.pos.x = tilep.x;
        d.pos.y = tilep.y;
        d.room = c->phys.curr_room;
        d.fade_pattern = 1;
        decal_add(d);

        c->target_tile.x = tilec.x;
        c->target_tile.y = tilec.y;

        c->ai_state = 1;
        c->ai_value = 0;
        c->ai_counter = 0;
        c->ai_counter_max = duration;
    }
}

static void creature_update_boomer(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        ai_stop_imm(c);

        if(c->ai_state < 3)
        {
            // walk
            ai_random_walk_cardinal(c);
            c->ai_state++;
        }
        else
        {
            // shoot
            creature_fire_projectile(c, c->phys.rotation_deg);
            c->ai_state = 0;
        }
    }
}

static void creature_update_wall_shooter(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);
    if(act)
    {
        bool horiz = (c->spawn_tile_y == -1 || c->spawn_tile_y == ROOM_TILE_SIZE_Y);
        if(horiz) creature_fire_projectile(c, dir_to_angle_deg(c->spawn_tile_y == -1 ? DIR_DOWN : DIR_UP));
        else      creature_fire_projectile(c, dir_to_angle_deg(c->spawn_tile_x == -1 ? DIR_RIGHT : DIR_LEFT));
    }
}

static void creature_update_spider_queen(Creature* c, float dt)
{
    bool act = ai_update_action(c, dt);

    if(act)
    {
        int r = ai_rand(2);

        if(r == 0)
        {
            // move toward player
            c->ai_state = 1;
            c->act_time_min = 1.5;
            c->act_time_min = 2.5;
        }
        else
        {
            // random movement
            c->ai_state = 0;
            c->ai_value = rand() % 8;
            c->act_time_min = 0.2;
            c->act_time_min = 0.3;
            ai_stop_imm(c);
        }
    }

    if(c->ai_state == 0)
    {
        ai_walk_dir(c,c->ai_value);
        return;
    }

    if(c->ai_state == 1)
    {
        // move toward nearest player
        Player* p = player_get_nearest(c->phys.curr_room, c->phys.pos.x, c->phys.pos.y);

        Vector2f v = {p->phys.pos.x - c->phys.pos.x, p->phys.pos.y - c->phys.pos.y};
        normalize(&v);

        float angle = calc_angle_deg(c->phys.pos.x, c->phys.pos.y, p->phys.pos.x, p->phys.pos.y);

        Dir dir = angle_to_dir_cardinal(angle);
        creature_set_sprite_index(c, dir);

        c->phys.vel.x = c->phys.speed*v.x;
        c->phys.vel.y = c->phys.speed*v.y;
    }
}
