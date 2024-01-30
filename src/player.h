#pragma once

#include "main.h"
#include "physics.h"
#include "gfx.h"
#include "net.h"
#include "item.h"
#include "skills.h"
#include "projectile.h"
#include "settings.h"


#define MAX_PLAYERS 4

#define MAX_SKILL_CHOICES  6

#define SPRITE_UP    0
#define SPRITE_DOWN  4
#define SPRITE_LEFT  8
#define SPRITE_RIGHT 12

#define PLAYER_NAME_MAX 16
#define PLAYER_MAX_SKILLS 20


#define PLAYER_GAUNTLET_MAX 8

typedef enum
{
    PLAYER_ACTION_UP,
    PLAYER_ACTION_DOWN,
    PLAYER_ACTION_LEFT,
    PLAYER_ACTION_RIGHT,
    PLAYER_ACTION_JUMP,

    PLAYER_ACTION_SHOOT_UP,
    PLAYER_ACTION_SHOOT_DOWN,
    PLAYER_ACTION_SHOOT_LEFT,
    PLAYER_ACTION_SHOOT_RIGHT,

    PLAYER_ACTION_ACTIVATE,

    PLAYER_ACTION_USE_ITEM,
    PLAYER_ACTION_DROP_ITEM,

    PLAYER_ACTION_ITEM_1,
    PLAYER_ACTION_ITEM_2,
    PLAYER_ACTION_ITEM_3,
    PLAYER_ACTION_ITEM_4,
    PLAYER_ACTION_ITEM_5,
    PLAYER_ACTION_ITEM_6,
    PLAYER_ACTION_ITEM_7,
    PLAYER_ACTION_ITEM_8,

    PLAYER_ACTION_TAB_CYCLE,
    PLAYER_ACTION_RSHIFT,

    PLAYER_ACTION_MAX
} PlayerActions;

typedef enum
{
    PLAYER_CLASS_SPACEMAN,
    PLAYER_CLASS_PHYSICIST,
    PLAYER_CLASS_ROBOT,
    PLAYER_CLASS_MAX,
} PlayerClass;

#define MAX_TIMED_ITEMS 10


typedef struct
{
    //phys
    float speed;
    float speed_factor;
    float max_velocity;
    float base_friction;
    float mass;
    float elasticity;

    // projectile
    float proj_cooldown_max;
    ProjectileDef projdef;
    ProjectileSpawn projspawn;

} PlayerAttributes;

typedef struct
{
    Vector3f pos;
    float invulnerable_temp_time;
} PlayerNetLerp;


#define NUM_NEAR_ITEMS  12

typedef struct
{
    bool active;
    int index;
    int image;

    bool ignore_player_collision;

    Physics phys;
    float vel_factor;

    float aim_deg;
    float scale;

    PlayerClass class;

    uint16_t xp;
    uint8_t level;
    uint8_t new_levels;

    Settings settings;
    PlayerInput actions[PLAYER_ACTION_MAX];

    PlayerActions last_shoot_action;
    float shoot_sprite_cooldown;


    // Item gauntlet_item;
    uint8_t gauntlet_selection;
    uint8_t gauntlet_slots;
    Item gauntlet[PLAYER_GAUNTLET_MAX];

    ItemType timed_items[MAX_TIMED_ITEMS];
    float timed_items_ttl[MAX_TIMED_ITEMS];

    uint16_t skills[PLAYER_MAX_SKILLS];
    uint8_t skill_count;
    uint8_t num_skill_choices;

    uint8_t skill_selection;
    uint8_t num_skill_selection_choices;
    uint16_t skill_choices[MAX_SKILL_CHOICES];


    ProjectileDef proj_def;
    ProjectileSpawn proj_spawn;

    GFXAnimation anim;

    uint8_t sprite_index;
    uint8_t curr_room;
    uint8_t transition_room;
    Dir door;

    int light_index;
    float light_radius;

    Vector2i curr_tile;
    Vector2i last_safe_tile;

    uint8_t proj_charge;
    float proj_cooldown;
    float proj_cooldown_max;

    bool invulnerable;

    bool invulnerable_temp;
    float invulnerable_temp_time;
    float invulnerable_temp_max;

    // Item* highlighted_item;
    int32_t highlighted_item_id;
    int highlighted_index;
    ItemSort near_items[NUM_NEAR_ITEMS];
    int near_items_count;

    // periodic shot
    float periodic_shot_counter;
    float periodic_shot_counter_max;

    // Networking
    NetPlayerInput input;
    NetPlayerInput input_prior;

    float lerp_t;
    PlayerNetLerp server_state_prior;
    PlayerNetLerp server_state_target;

} Player;

extern int player_ignore_input;
extern Player players[MAX_PLAYERS];
extern Player* player;
extern Player* player2;
extern int xp_levels[];
extern char* class_strs[];


extern float jump_vel_z;
extern int shadow_image;

void player_init();
uint8_t player_get_gauntlet_count(Player* p);
void player_drop_item(Player* p, Item* it);
bool player_gauntlet_full(Player* p);
void player_init_keys();
void player2_init_keys();
void player_send_to_room(Player* p, uint8_t room_index, bool instant, Vector2i tile);
void player_send_to_level_start(Player* p);
void player_update_all(float dt);
void player_update(Player* p, float dt);
void player_draw(Player* p);
void player_draw_debug(Player* p);
void player_lerp(Player* p, float dt);
void player_handle_net_inputs(Player* p, double dt);
void player_add_xp(Player* p, int xp);
void player_hurt_no_inv(Player* p, int damage);
void player_hurt(Player* p, int damage);
void player_add_hp(Player* p, int hp);
void player_die(Player* p);
void player_reset(Player* p);
void player_draw_room_transition();
void player_start_room_transition(Player* p);
void player_set_class(Player* p, PlayerClass class);

void player_print(Player* p);

void player_set_active(Player* p, bool active);
int player_get_active_count();
void player_check_stuck_in_wall(Player* p);
bool player_check_other_player_collision(Player* p);
void player_handle_collision(Player* p, Entity* e);
bool is_any_player_room(uint8_t curr_room);
int player_get_count_in_room(uint8_t curr_room);

void draw_hearts();
void draw_xp_bar();
void draw_gauntlet();
void draw_timed_items();
void randomize_skill_choices(Player* p);
void draw_skill_selection();

int get_xp_req(int level);
