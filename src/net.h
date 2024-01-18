#pragma once

#include "socket.h"
#include "math2d.h"

#define TICK_RATE 20.0f
#define LOCAL_SERVER_IP "127.0.0.1"
#define ONLINE_SERVER_IP "66.228.36.123"

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

PACK(struct PacketHeader
{
    uint32_t game_id;
    uint16_t id;
    uint16_t ack;
    uint32_t ack_bitfield;
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

extern char* server_ip_address;


//
// Net Events
//

typedef enum
{
    EVENT_TYPE_NONE = 0,
    EVENT_TYPE_MESSAGE,
    EVENT_TYPE_PARTICLES,
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
} NetEventParticles;

typedef struct
{
    NetEventType type;

    union
    {
        NetEventMessage   message;
        NetEventParticles particles;
        // ...
    } data;
} NetEvent;

extern char* server_ip_address;


// Server
int net_server_start();
bool net_server_add_event(NetEvent* event);

void server_send_message(uint8_t to, uint8_t from, char* fmt, ...);

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
bool net_client_received_init_packet();
bool net_client_is_connected();
void net_client_disconnect();
bool net_client_set_server_ip(char* address);
void net_client_get_server_ip_str(char* ip_str);
int net_client_data_waiting();
double net_client_get_rtt();
double net_client_get_connected_time();
uint32_t net_client_get_sent_bytes();
uint32_t net_client_get_recv_bytes();
void net_client_send_message(char* fmt, ...);
int net_client_send(uint8_t* data, uint32_t len);
int net_client_recv(Packet* pkt);
void net_client_deinit();

void test_packing();
