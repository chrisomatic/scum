#pragma once

#include "main.h"
#include "physics.h"
#include "gfx.h"
#include "net.h"
#include "item.h"
#include "skills.h"
#include "projectile.h"
#include "weapon.h"
#include "settings.h"

#define MAX_PLAYERS 4

#define SPRITE_UP    0
#define SPRITE_DOWN  4
#define SPRITE_LEFT  8
#define SPRITE_RIGHT 12

#define PLAYER_NAME_MAX 16
#define PLAYER_MAX_SKILLS 5


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
    PLAYER_ACTION_SELECT_SKILL, //show skill selection

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

typedef enum
{
    STRENGTH,
    DEFENSE,
    MOVEMENT_SPEED,
    ATTACK_SPEED,
    ATTACK_RANGE,
    LUCK,

    MAX_STAT_TYPE
} StatType;

#define MAX_TIMED_ITEMS 10

typedef struct
{
    float speed;
    float max_velocity;
    float base_friction;

    bool floating;
} PlayerSkillMods;


typedef struct
{
    Vector3f pos;
    float invulnerable_temp_time;
    float weapon_scale;
} PlayerNetLerp;


#define NUM_NEAR_ITEMS  12

typedef struct
{
    bool active;
    int index;
    int image;

    bool ignore_player_collision;

    Physics phys;
    float anim_factor;

    float aim_deg;

    uint16_t coins;

    uint16_t xp;
    uint8_t level;
    uint8_t new_levels;

    PlayerSkillMods skill_mods;

    uint8_t stats[MAX_STAT_TYPE];

    Settings settings;
    PlayerInput actions[PLAYER_ACTION_MAX];

    PlayerActions last_shoot_action;
    float shoot_sprite_cooldown;

    Skill skills[PLAYER_MAX_SKILLS];
    int skill_count;

    Gun gun;
    uint8_t room_gun_index;

    uint16_t total_hits;
    uint16_t level_hits;
    uint16_t room_hits;

    Weapon weapon;

    GFXAnimation anim;

    uint8_t sprite_index;

    uint8_t transition_room;
    Dir door;

    int light_index;
    float light_radius;

    Vector2i last_safe_tile;

    float proj_cooldown;

    bool invulnerable;

    bool invulnerable_temp;
    float invulnerable_temp_time;
    float invulnerable_temp_max;

    int32_t highlighted_item_id;
    int highlighted_index;
    ItemSort near_items[NUM_NEAR_ITEMS];
    int near_items_count;

    // periodic shot
    float periodic_shot_counter;
    float periodic_shot_counter_max;

    // Audio
    int source_run1;
    int source_run2;
    int source_jump;
    int source_shoot;
    int source_shoot_explode;

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
extern text_list_t* ptext;
extern int xp_levels[];
extern char* class_strs[];
extern Room* visible_room;

extern int class_image_spaceman;
extern int class_image_robot;
extern int class_image_physicist;

extern Rect player_pit_rect;
extern Rect tile_pit_rect;
extern float jump_vel_z;
extern int shadow_image;

void player_init();
void player_drop_item(Player* p, Item* it);
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
bool player_add_skill(Player* p, SkillType type);
bool player_add_stat(Player* p, StatType stat, int val);
void player_add_coins(Player* p, int val);
bool player_add_hp(Player* p, int hp);
bool player_add_mp(Player* p, int mp);
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
Player* player_get_nearest(uint8_t room_index, float x, float y);

void player_set_gun(Player* p, uint8_t room_gun_index, bool drop_old_gun);

void draw_all_other_player_info();
void draw_hearts();
void draw_mp_bar();
void draw_equipped_gun();

int get_xp_req(int level);

void draw_stats();
const char* stats_get_name(StatType stat);
ItemType stats_get_item(StatType stat);
Vector2i stats_get_img(StatType stat);
