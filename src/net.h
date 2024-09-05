#pragma once

#include "socket.h"
#include "math2d.h"

#define TICK_RATE 30.0f
#define LOCAL_SERVER_IP "127.0.0.1"
#define ONLINE_SERVER_IP "66.228.36.123"
#define BITPACK_SIZE 32768

#define MAX_CLIENTS     MAX_PLAYERS

#define FROM_SERVER 0xFF    //for messaging
#define TO_ALL      0xFF    //for messaging

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#define CONN_RC_CHALLENGED    (-1)
#define CONN_RC_INVALID_SALT  (-2)
#define CONN_RC_REJECTED      (-3)
#define CONN_RC_NO_DATA       (-4)

#define DUMB_CLIENT 1

typedef enum
{
    PACKET_TYPE_INIT = 0,
    PACKET_TYPE_CONNECT_REQUEST,
    PACKET_TYPE_CONNECT_CHALLENGE,
    PACKET_TYPE_CONNECT_CHALLENGE_RESP,
    PACKET_TYPE_CONNECT_ACCEPTED,
    PACKET_TYPE_CONNECT_REJECTED,
    PACKET_TYPE_DISCONNECT,
    PACKET_TYPE_PING,
    PACKET_TYPE_INPUT,
    PACKET_TYPE_STATE,
    PACKET_TYPE_SETTINGS,
    PACKET_TYPE_MESSAGE,
    PACKET_TYPE_ERROR,
    PACKET_TYPE_MAX,
} PacketType;

typedef enum
{
    INVALID = -1,
    DISCONNECTED = 0,
    SENDING_CONNECTION_REQUEST,
    SENDING_CHALLENGE_RESPONSE,
    CONNECTED,
} ConnectionState;

typedef enum
{
    CONNECT_REJECT_REASON_SERVER_FULL,
    CONNECT_REJECT_REASON_INVALID_PACKET,
    CONNECT_REJECT_REASON_FAILED_CHALLENGE,
} ConnectionRejectionReason;

typedef enum
{
    PACKET_ERROR_NONE = 0,
    PACKET_ERROR_BAD_FORMAT,
    PACKET_ERROR_INVALID,
} PacketError;

// 16 bytes
PACK(struct PacketHeader
{
    uint32_t game_id;
    uint16_t id;
    uint16_t ack;
    uint32_t ack_bitfield;
    uint8_t frame_no;
    uint8_t type;
    uint8_t pad[3]; // pad to be 4-byte aligned
});

typedef struct PacketHeader PacketHeader;

PACK(struct Packet
{
    PacketHeader hdr;

    uint32_t data_len;
    uint8_t  data[MAX_PACKET_DATA_SIZE];
});

typedef struct Packet Packet;

PACK(struct NetPlayerInput
{
    double delta_t;
    uint32_t keys;
});

typedef struct NetPlayerInput NetPlayerInput;


typedef struct
{
    Vector3f pos;
} PlayerState;

typedef struct
{
    uint8_t  id;
    uint8_t  type;
    Vector3f pos;
    float    width;
    uint8_t  sprite_index;
    uint8_t  curr_room;
    int8_t   hp;
    uint32_t color;
} CreatureState;

typedef struct
{
    uint8_t  id;
    Vector3f pos;
    uint32_t color;
    uint8_t  player_id;
    uint8_t  curr_room;
    float    scale;
    bool     from_player;
} ProjectileState;

typedef struct
{
    uint8_t  id;
    uint8_t  type;
    Vector3f pos;
    uint8_t  curr_room;
    float    angle;
    bool     used;
} ItemState;

typedef struct
{
    Vector2f pos;
    uint8_t  sprite_index;
    uint32_t tint;
    float scale;
    float rotation;
    float opacity;
    float ttl;
    uint8_t curr_room;
    uint8_t fade_pattern;
} DecalState;

typedef struct
{
    Vector2f pos;
    uint8_t effect_index;
    float scale;
    uint32_t color1;
    uint32_t color2;
    uint32_t color3;
    float lifetime;
    uint8_t curr_room;
} EventState;

typedef struct
{
    bool doors_locked;
    bool paused;
} OtherState;

typedef struct
{
    PlayerState     players[1];
} WorldState;

typedef struct
{
    uint16_t       id;
    NetPlayerInput input;
    WorldState     state;
} ClientMove;

//
// Net Events
//

typedef enum
{
    EVENT_TYPE_NONE = 0,
    EVENT_TYPE_MESSAGE,
    EVENT_TYPE_PARTICLES,
    EVENT_TYPE_NEW_LEVEL,
    EVENT_TYPE_FLOOR_STATE,
    EVENT_TYPE_GUN_LIST,
} NetEventType;

typedef struct
{
    uint8_t to;
    uint8_t from;
    char* msg;
} NetEventMessage;

typedef struct
{
    uint8_t effect_index;
    Vector2f pos;
    float scale;
    uint32_t color1;
    uint32_t color2;
    uint32_t color3;
    float lifetime;
    uint8_t room_index;
} NetEventParticles;

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t state;
} NetEventFloorState;


typedef struct
{
    uint8_t count;
    char gun_names[16][32+1]; //[MAX_ROOM_GUNS][GUN_NAME_MAX_LEN+1]
    // uint32_t seed[MAX_ROOM_GUNS]; //TODO
} NetEventGunList;

typedef struct
{
    NetEventType type;

    union
    {
        NetEventMessage   message;
        NetEventParticles particles;
        NetEventFloorState floor_state;
        NetEventGunList gun_list;
        // ...
    } data;
} NetEvent;

extern char* server_ip_address;

// Server
int net_server_start();
bool net_server_add_event(NetEvent* event);

void server_send_message(uint8_t to, uint8_t from, char* fmt, ...);
bool server_process_command(char* argv[20], int argc, int client_id);

// Client
bool net_client_init();
int net_client_connect(); // blocking
bool net_client_connect_update(); // non-blocking

void net_client_update();
uint8_t net_client_get_player_count();
int net_client_get_id();
ConnectionState net_client_get_state();
int net_client_get_input_count();
uint16_t net_client_get_latest_local_packet_id();
bool net_client_add_player_input(NetPlayerInput* input);
bool net_client_record_player_state(NetPlayerInput* input, WorldState* state);
bool net_client_received_init_packet();
bool net_client_is_connected();
void net_client_disconnect();
void net_client_send_settings();
void net_client_send_inputs();
bool net_client_set_server_ip(char* address);
void net_client_get_server_ip_str(char* ip_str);
int net_client_data_waiting();
double net_client_get_server_time();
double net_client_get_rtt();
double net_client_get_connected_time();
uint32_t net_client_get_sent_bytes();
uint32_t net_client_get_recv_bytes();
void net_client_send_message(char* fmt, ...);
uint32_t net_client_get_largest_packet_size_recv();
uint32_t net_client_get_largest_packet_size_sent();
int net_client_send(uint8_t* data, uint32_t len);
int net_client_recv(Packet* pkt);
void net_client_deinit();

void test_packing();
