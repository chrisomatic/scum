#include "headers.h"

#if _WIN32
#include <WinSock2.h>
#else
#include <sys/select.h>
#endif

#include "core/timer.h"
#include "core/window.h"
#include "core/log.h"
#include "core/circbuf.h"
#include "core/bitpack.h"

#include "main.h"
#include "net.h"
#include "player.h"
#include "creature.h"
#include "level.h"
#include "particles.h"
#include "effects.h"
#include "projectile.h"
#include "explosion.h"
#include "settings.h"

#define COOL_SERVER_PLAYER_LOGIC 1

//#define SERVER_PRINT_SIMPLE 1
//#define SERVER_PRINT_VERBOSE 1

#if SERVER_PRINT_VERBOSE
    #define LOGNV(format, ...) LOGN(format, ##__VA_ARGS__)
#else
    #define LOGNV(format, ...) 0
#endif

#define GAME_ID 0x308B4134

#define PORT 27001

#define MAXIMUM_RTT 1.0f

#define DEFAULT_TIMEOUT 1.0f // seconds
#define PING_PERIOD 3.0f
#define DISCONNECTION_TIMEOUT 7.0f // seconds
#define INPUT_QUEUE_MAX 16
#define MAX_NET_EVENTS 255

typedef struct
{
    int socket;
    uint16_t local_latest_packet_id;
    uint16_t remote_latest_packet_id;
} NodeInfo;

// Info server stores about a client
typedef struct
{
    int client_id;
    Address address;
    ConnectionState state;
    uint16_t remote_latest_packet_id;
    double  time_of_latest_packet;
    uint8_t client_salt[8];
    uint8_t server_salt[8];
    uint8_t xor_salts[8];
    ConnectionRejectionReason last_reject_reason;
    PacketError last_packet_error;
    Packet prior_state_pkt;
    NetPlayerInput net_player_inputs[INPUT_QUEUE_MAX];
    int input_count;
} ClientInfo;

struct
{
    Address address;
    NodeInfo info;
    ClientInfo clients[MAX_CLIENTS];
    NetEvent events[MAX_NET_EVENTS];
    BitPack bp;
    int event_count;
    int num_clients;
} server = {0};

struct
{
    int id;
    bool received_init_packet;

    Address address;
    NodeInfo info;
    ConnectionState state;
    CircBuf input_packets;
    BitPack bp;

    double time_of_connection;
    double time_of_latest_sent_packet;
    double time_of_last_ping;
    double time_of_last_received_ping;
    double rtt;

    uint32_t bytes_received;
    uint32_t bytes_sent;

    uint32_t largest_packet_size_sent;
    uint32_t largest_packet_size_recv;

    uint8_t player_count;
    uint8_t server_salt[8];
    uint8_t client_salt[8];
    uint8_t xor_salts[8];

} client = {0};

// ---

#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

// ---

static NetPlayerInput net_player_inputs[INPUT_QUEUE_MAX]; // shared
static int input_count = 0;
static int inputs_per_packet = 1.0; //(TARGET_FPS/TICK_RATE);

static inline void pack_bool(Packet* pkt, bool d);
static inline void pack_u8(Packet* pkt, uint8_t d);
static inline void pack_u8_at(Packet* pkt, uint8_t d, int index);
static inline void pack_u16(Packet* pkt, uint16_t d);
static inline void pack_u16_at(Packet* pkt, uint16_t d, int index);
static inline void pack_u32(Packet* pkt, uint32_t d);
static inline void pack_i32(Packet* pkt, int32_t d);
static inline void pack_u64(Packet* pkt, uint64_t d);
static inline void pack_float(Packet* pkt, float d);
static inline void pack_bytes(Packet* pkt, uint8_t* d, uint32_t len);
static inline void pack_string(Packet* pkt, char* s, uint8_t max_len);
static inline void pack_vec2(Packet* pkt, Vector2f d);
static inline void pack_vec3(Packet* pkt, Vector3f d);
static inline void pack_itemtype(Packet* pkt, ItemType type);

static inline bool unpack_bool(Packet* pkt, int* offset);
static inline uint8_t  unpack_u8(Packet* pkt, int* offset);
static inline uint16_t unpack_u16(Packet* pkt, int* offset);
static inline uint32_t unpack_u32(Packet* pkt, int* offset);
static inline int32_t unpack_i32(Packet* pkt, int* offset);
static inline uint64_t unpack_u64(Packet* pkt, int* offset);
static inline float    unpack_float(Packet* pkt, int* offset);
static inline void unpack_bytes(Packet* pkt, uint8_t* d, int len, int* offset);
static inline uint8_t unpack_string(Packet* pkt, char* s, int maxlen, int* offset);
static inline Vector2f unpack_vec2(Packet* pkt, int* offset);
static inline Vector3f unpack_vec3(Packet* pkt, int* offset);
static inline ItemType unpack_itemtype(Packet* pkt, int* offset);

static void pack_players_bp(Packet* pkt, ClientInfo* cli); // temp
static void unpack_players_bp(Packet* pkt, int* offset);
static void pack_creatures_bp(Packet* pkt, ClientInfo* cli);
static void unpack_creatures_bp(Packet* pkt, int* offset);
static void pack_projectiles_bp(Packet* pkt, ClientInfo* cli);
static void unpack_projectiles_bp(Packet* pkt, int* offset);
static void pack_items_bp(Packet* pkt, ClientInfo* cli);
static void unpack_items_bp(Packet* pkt, int* offset);
static void pack_decals_bp(Packet* pkt, ClientInfo* cli);
static void unpack_decals_bp(Packet* pkt, int* offset);
static void pack_other_bp(Packet* pkt, ClientInfo* cli);
static void unpack_other_bp(Packet* pkt, int* offset);
static void pack_events_bp(Packet* pkt, ClientInfo* cli);
static void unpack_events_bp(Packet* pkt, int* offset);

static uint64_t rand64(void)
{
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned) rand();
    }
    return r;
}

static Timer server_timer = {0};

static inline int get_packet_size(Packet* pkt)
{
    return (sizeof(pkt->hdr) + pkt->data_len + sizeof(pkt->data_len));
}

static inline bool is_packet_id_greater(uint16_t id, uint16_t cmp)
{
    return ((id >= cmp) && (id - cmp <= 32768)) ||
           ((id <= cmp) && (cmp - id  > 32768));
}

static char* packet_type_to_str(PacketType type)
{
    switch(type)
    {
        case PACKET_TYPE_INIT: return "INIT";
        case PACKET_TYPE_CONNECT_REQUEST: return "CONNECT REQUEST";
        case PACKET_TYPE_CONNECT_CHALLENGE: return "CONNECT CHALLENGE";
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP: return "CONNECT CHALLENGE RESP";
        case PACKET_TYPE_CONNECT_ACCEPTED: return "CONNECT ACCEPTED";
        case PACKET_TYPE_CONNECT_REJECTED: return "CONNECT REJECTED";
        case PACKET_TYPE_DISCONNECT: return "DISCONNECT";
        case PACKET_TYPE_PING: return "PING";
        case PACKET_TYPE_INPUT: return "INPUT";
        case PACKET_TYPE_SETTINGS: return "SETTINGS";
        case PACKET_TYPE_STATE: return "STATE";
        case PACKET_TYPE_MESSAGE: return "MESSAGE";
        case PACKET_TYPE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static char* connect_reject_reason_to_str(ConnectionRejectionReason reason)
{
    switch(reason)
    {
        case CONNECT_REJECT_REASON_SERVER_FULL: return "SERVER FULL";
        case CONNECT_REJECT_REASON_INVALID_PACKET: return "INVALID PACKET FORMAT";
        case CONNECT_REJECT_REASON_FAILED_CHALLENGE: return "FAILED CHALLENGE";
        default: return "UNKNOWN";
    }
}

static void print_salt(uint8_t* salt)
{
    LOGN("[SALT] %02X %02X %02X %02X %02X %02X %02X %02X",
            salt[0], salt[1], salt[2], salt[3],
            salt[4], salt[5], salt[6], salt[7]
    );
}

static void store_xor_salts(uint8_t* cli_salt, uint8_t* svr_salt, uint8_t* xor_salts)
{
    for(int i = 0; i < 8; ++i)
    {
        xor_salts[i] = (cli_salt[i] ^ svr_salt[i]);
    }
}

static bool is_local_address(Address* addr)
{
    return (addr->a == 127 && addr->b == 0 && addr->c == 0 && addr->d == 1);
}

static bool is_empty_address(Address* addr)
{
    return (addr->a == 0 && addr->b == 0 && addr->c == 0 && addr->d == 0);
}

static void print_address(Address* addr)
{
    // printf( "is local: %d\n", is_local_address(addr));
    LOGN("[ADDR] %u.%u.%u.%u:%u",addr->a,addr->b,addr->c,addr->d,addr->port);
}

static bool compare_address(Address* addr1, Address* addr2, bool incl_port)
{
    // // if both are local ip address then compare port as well
    // if(is_local_address(addr1) && is_local_address(addr2))
    // {
    //     return (memcmp(addr1, addr2, sizeof(Address)) == 0);
    // }

    if(incl_port)
    {
        return (memcmp(addr1, addr2, sizeof(Address)) == 0);
    }


    return (addr1->a == addr2->a && addr1->b == addr2->b && addr1->c == addr2->c && addr1->d == addr2->d);
}


static void print_packet(Packet* pkt, bool full)
{
    LOGN("Game ID:      0x%08x",pkt->hdr.game_id);
    LOGN("Packet ID:    %u",pkt->hdr.id);
    LOGN("Packet Type:  %s (%02X)",packet_type_to_str(pkt->hdr.type),pkt->hdr.type);
    LOGN("Data (%u):", pkt->data_len);

    #define PSIZE 32

    int idx = 0;
    for(;;)
    {
        int size = MIN(PSIZE, (int)pkt->data_len - idx);
        if(size <= 0) break;

        char data[3*PSIZE+5] = {0};
        char byte[4] = {0};

        for(int i = 0; i < size; ++i)
        {
            sprintf(byte,"%02X ",pkt->data[idx+i]);
            memcpy(data+(3*i), byte,3);
        }

        if(!full && pkt->data_len > PSIZE)
        {
            LOGN("%s ...", data);
            return;
        }

        LOGN("%s", data);

        idx += size;
    }

}

static void print_packet_simple(Packet* pkt, const char* hdr)
{
    LOGN("[%s][ID: %u] %s (%u B)",hdr, pkt->hdr.id, packet_type_to_str(pkt->hdr.type), pkt->data_len);
}

static bool has_data_waiting(int socket)
{

    fd_set readfds;

    //clear the socket set
    FD_ZERO(&readfds);

    //add client socket to set
    FD_SET(socket, &readfds);

    int activity;

    struct timeval tv = {0};
    activity = select(socket + 1 , &readfds , NULL , NULL , &tv);

    if ((activity < 0) && (errno!=EINTR))
    {
        perror("select error");
        return false;
    }

    bool has_data = FD_ISSET(socket , &readfds);
    return has_data;
}

static int net_send(NodeInfo* node_info, Address* to, Packet* pkt)
{
    int pkt_len = get_packet_size(pkt);
    int sent_bytes = socket_sendto(node_info->socket, to, (uint8_t*)pkt, pkt_len);

#if SERVER_PRINT_SIMPLE==1
    print_packet_simple(pkt,"SEND");
#elif SERVER_PRINT_VERBOSE==1
    LOGN("[SENT] Packet %d (%u B)",pkt->hdr.id,sent_bytes);
    print_packet(pkt, false);
#endif

    node_info->local_latest_packet_id++;

    if(node_info == &client.info)
    {
        client.bytes_sent += sent_bytes;
        if(sent_bytes > client.largest_packet_size_sent)
        {
            client.largest_packet_size_sent = sent_bytes;
        }
    }

    return sent_bytes;
}

static int net_recv(NodeInfo* node_info, Address* from, Packet* pkt)
{
    int recv_bytes = socket_recvfrom(node_info->socket, from, (uint8_t*)pkt);

#if SERVER_PRINT_SIMPLE
    print_packet_simple(pkt,"RECV");
#elif SERVER_PRINT_VERBOSE
    LOGN("[RECV] Packet %d (%u B)",pkt->hdr.id,recv_bytes);
    print_address(from);
    print_packet(pkt, false);
#endif

    if(node_info == &client.info)
    {
        client.bytes_received += recv_bytes;
        if(recv_bytes > client.largest_packet_size_recv)
        {
            client.largest_packet_size_recv = recv_bytes;
        }
    }

    return recv_bytes;
}

static bool validate_packet_format(Packet* pkt)
{
    if(pkt->hdr.game_id != GAME_ID)
    {
        LOGN("Game ID of packet doesn't match, %08X != %08X",pkt->hdr.game_id, GAME_ID);
        return false;
    }

    if(pkt->hdr.type < 0 || pkt->hdr.type >= PACKET_TYPE_MAX)
    {
        LOGN("Invalid Packet Type: %d", pkt->hdr.type);
        return false;
    }

    return true;
}

static bool authenticate_client(Packet* pkt, ClientInfo* cli)
{
    bool valid = true;

    switch(pkt->hdr.type)
    {
        case PACKET_TYPE_CONNECT_REQUEST:
            valid &= (pkt->data_len == 1024); // must be padded out to 1024
            valid &= (memcmp(&pkt->data[0],cli->client_salt, 8) == 0);
            break;
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
            valid &= (pkt->data_len == 1024); // must be padded out to 1024
            valid &= (memcmp(&pkt->data[0],cli->xor_salts, 8) == 0);
           break;
        default:
            valid &= (memcmp(&pkt->data[0],cli->xor_salts, 8) == 0);
            break;
    }

    return valid;
}

static int server_get_client(Address* addr, ClientInfo** cli)
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        bool addr_check = compare_address(&server.clients[i].address, addr, true);
        bool conn = server.clients[i].state != DISCONNECTED;

        if(addr_check && conn)
        {
            *cli = &server.clients[i];
            return i;
        }
    }

    return -1;
}

// 0: unable to assign new client
// 1: assigned new client
// 2: assigned new client and assigned them to their previous spot
static int server_assign_new_client(Address* addr, ClientInfo** cli, char* name)
{
    LOGN("server_assign_new_client()");
    print_address(addr);

#if COOL_SERVER_PLAYER_LOGIC
    int cnt = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state != DISCONNECTED)
        {
            cnt++;
        }
    }
    if(cnt == MAX_CLIENTS)
    {
        LOGN("Server is full and can't accept new clients.");
        return 0;
    }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        // print_address(&server.clients[i].address);

        bool addr_check = compare_address(&server.clients[i].address, addr, false);
        bool name_check = memcmp(name, players[i].settings.name, PLAYER_NAME_MAX*sizeof(char)) == 0;

        if(addr_check && name_check)
        {
            if(server.clients[i].state == DISCONNECTED)
            {

                *cli = &server.clients[i];
                (*cli)->client_id = i;
                LOGN("Found client entry with same name and address (not connected) id: %d", (*cli)->client_id);
                return 2;
            }
            else
            {
                LOGN("Client already connected with this name and address");
                return 0;
            }
        }
    }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        // look for empty slot first
        if(server.clients[i].state == DISCONNECTED && is_empty_address(&server.clients[i].address))
        {
            *cli = &server.clients[i];
            (*cli)->client_id = i;
            LOGN("Assigning new client: %d", (*cli)->client_id);
            // printf("%s() new client %d\n", __func__, i);
            return 1;
        }
    }

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        // take any disconnected slot
        if(server.clients[i].state == DISCONNECTED)
        {
            *cli = &server.clients[i];
            (*cli)->client_id = i;
            LOGN("Assigning new client: %d", (*cli)->client_id);
            // printf("%s() new client %d\n", __func__, i);
            return 1;
        }
    }

    return 0;

#else

    // new client
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state == DISCONNECTED)
        {
            *cli = &server.clients[i];
            (*cli)->client_id = i;
            LOGN("Assigning new client: %d", (*cli)->client_id);
            // printf("%s() new client %d\n", __func__, i);
            return 1;
        }
    }

    LOGN("Server is full and can't accept new clients.");
    return 0;
#endif
}

static void update_server_num_clients()
{
    int num_clients = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state != DISCONNECTED)
        {
            num_clients++;
        }
    }
    server.num_clients = num_clients;

// #if COOL_SERVER_PLAYER_LOGIC
#if 1
    if(server.num_clients == 0)
    {
        LOGN("Everyone left!");
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            player_reset(&players[i]);
            // memset(&server.clients[i],0, sizeof(ClientInfo));
        }
        trigger_generate_level(rand(), 1, 0, __LINE__);
    }
#endif

}

static void remove_client(ClientInfo* cli)
{
    LOGN("Remove client: %d", cli->client_id);
    cli->state = DISCONNECTED;
    cli->remote_latest_packet_id = 0;
    player_set_active(&players[cli->client_id],false);

#if COOL_SERVER_PLAYER_LOGIC
    Address addr = cli->address;
    // clear everything but address
    memset(cli,0, sizeof(ClientInfo));
    memcpy(&cli->address, &addr, sizeof(Address));
#else
    memset(cli,0, sizeof(ClientInfo));
#endif

    update_server_num_clients();
}

static void server_send(PacketType type, ClientInfo* cli)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = server.info.local_latest_packet_id,
        .hdr.ack = cli->remote_latest_packet_id,
        .hdr.type = type
    };

    LOGNV("%s() : %s", __func__, packet_type_to_str(type));

    switch(type)
    {
        case PACKET_TYPE_INIT:
        {
            pack_u32(&pkt,level_seed);
            pack_u8(&pkt,(uint8_t)level_rank);
            net_send(&server.info,&cli->address,&pkt);
        } break;

        case PACKET_TYPE_CONNECT_CHALLENGE:
        {
            uint64_t salt = rand64();
            memcpy(cli->server_salt, (uint8_t*)&salt,8);

            // store xor salts
            store_xor_salts(cli->client_salt, cli->server_salt, cli->xor_salts);
            print_salt(cli->xor_salts);

            pack_bytes(&pkt, cli->client_salt, 8);
            pack_bytes(&pkt, cli->server_salt, 8);

            net_send(&server.info,&cli->address,&pkt);
        } break;

        case PACKET_TYPE_CONNECT_ACCEPTED:
        {
            cli->state = CONNECTED;
            pack_u8(&pkt, (uint8_t)cli->client_id);
            net_send(&server.info,&cli->address,&pkt);

            server_send_message(TO_ALL, FROM_SERVER, "client added %u", cli->client_id);
        } break;

        case PACKET_TYPE_CONNECT_REJECTED:
        {
            pack_u8(&pkt, (uint8_t)cli->last_reject_reason);
            net_send(&server.info,&cli->address,&pkt);
        } break;

        case PACKET_TYPE_PING:
            pkt.data_len = 0;
            net_send(&server.info,&cli->address,&pkt);
            break;

        case PACKET_TYPE_SETTINGS:
        {
            int num_clients = 0;

            pkt.data_len = 1;

            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                if (server.clients[i].state == CONNECTED)
                {
                    pack_u8(&pkt, (uint8_t)i);
                    pack_u8(&pkt, players[i].settings.class);
                    pack_u32(&pkt, players[i].settings.color);
                    pack_string(&pkt, players[i].settings.name, PLAYER_NAME_MAX);

                    LOGN("Sending Settings, Client ID: %d", i);
                    LOGN("  name: %s", players[i].settings.name);
                    LOGN("  class: %u", players[i].settings.class);
                    LOGN("  color: 0x%08x", players[i].settings.color);

                    num_clients++;
                }
            }

            pkt.data[0] = num_clients;

            net_send(&server.info, &cli->address, &pkt);
        } break;

        case PACKET_TYPE_STATE:
        {

            bitpack_clear(&server.bp);

            pack_events_bp(&pkt,cli);
            pack_players_bp(&pkt, cli);
            pack_creatures_bp(&pkt, cli);
            pack_projectiles_bp(&pkt, cli);
            pack_items_bp(&pkt,cli);
            pack_decals_bp(&pkt, cli);
            pack_other_bp(&pkt, cli);

            bitpack_flush(&server.bp);
            bitpack_seek_begin(&server.bp);
            //bitpack_print(&server.bp);

            int num_bytes = server.bp.words_written*4;
            pack_bytes(&pkt, (uint8_t*)server.bp.data, num_bytes);

            //print_packet(&pkt, true);

            if(memcmp(&cli->prior_state_pkt.data, &pkt.data, pkt.data_len) == 0)
                break;

            net_send(&server.info,&cli->address,&pkt);
            memcpy(&cli->prior_state_pkt, &pkt, get_packet_size(&pkt));

        } break;

        case PACKET_TYPE_ERROR:
        {
            pack_u8(&pkt, (uint8_t)cli->last_packet_error);
            net_send(&server.info,&cli->address,&pkt);
        } break;

        case PACKET_TYPE_DISCONNECT:
        {
            cli->state = DISCONNECTED;
            pkt.data_len = 0;
            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
                net_send(&server.info,&cli->address,&pkt);
        } break;

        default:
            break;
    }
}


static void server_simulate()
{
    game_generate_level();

    float dt = 1.0/TARGET_FPS;

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        ClientInfo* cli = &server.clients[i];
        if(cli->state != CONNECTED)
            continue;

        Player* p = &players[cli->client_id];

        //printf("Applying inputs to player. input count: %d\n", cli->input_count);

        if(cli->input_count == 0)
        {
            player_update(p,dt);
        }
        else
        {
            for(int i = 0; i < cli->input_count; ++i)
            {
                // apply input to player
                for(int j = 0; j < PLAYER_ACTION_MAX; ++j)
                {
                    bool key_state = (cli->net_player_inputs[i].keys & ((uint32_t)1<<j)) != 0;
                    p->actions[j].state = key_state;
                }

                player_update(p,cli->net_player_inputs[i].delta_t);
            }

            cli->input_count = 0;
        }
    }

    level_update(dt);
    projectile_update_all(dt);
    creature_update_all(dt);
    item_update_all(dt);
    explosion_update_all(dt);
    decal_update_all(dt);

    entity_build_all();
    entity_handle_collisions();
    entity_handle_status_effects(dt);
}

int net_server_start()
{
    LOGN("%s()", __func__);

    // init
    socket_initialize();

    memset(server.clients, 0, sizeof(ClientInfo)*MAX_CLIENTS);
    server.num_clients = 0;

    int sock;

    // set timers
    timer_set_fps(&game_timer,TARGET_FPS);
    timer_set_fps(&server_timer,TICK_RATE);

    timer_begin(&game_timer);
    timer_begin(&server_timer);

    LOGN("Creating socket.");
    socket_create(&sock);

    LOGN("Binding socket %u to any local ip on port %u.", sock, PORT);
    socket_bind(sock, NULL, PORT);
    server.info.socket = sock;

    bitpack_create(&server.bp, BITPACK_SIZE);

    LOGN("Server Started with tick rate %f.", TICK_RATE);

    double t0=timer_get_time();
    double t1=0.0;
    double accum = 0.0;

    double t0_g=timer_get_time();
    double t1_g=0.0;
    double accum_g = 0.0;

    const double dt = 1.0/TICK_RATE;

    for(;;)
    {
        // handle connections, receive inputs
        for(;;)
        {
            // Read all pending packets
            bool data_waiting = has_data_waiting(server.info.socket);
            if(!data_waiting)
            {
                break;
            }

            Address from = {0};
            Packet recv_pkt = {0};
            int offset = 0;

            int bytes_received = net_recv(&server.info, &from, &recv_pkt);

            if(!validate_packet_format(&recv_pkt))
            {
                LOGN("Invalid packet format!");
                timer_delay_us(1000); // delay 1ms
                continue;
            }

            ClientInfo* cli = NULL;

            if(recv_pkt.hdr.type == PACKET_TYPE_CONNECT_REQUEST)
            {
                if(recv_pkt.data_len != 1024)
                {
                    LOGN("Packet length doesn't equal %d",1024);
                    // remove_client(cli);
                    break;
                }

                uint8_t salt[8] = {0};
                unpack_bytes(&recv_pkt, salt, 8, &offset);

                char name[PLAYER_NAME_MAX+1] = {0};
                uint8_t namelen = unpack_string(&recv_pkt, name, PLAYER_NAME_MAX, &offset);
                if(namelen == 0) printf("namelen is 0!\n");

                int ret = server_assign_new_client(&from, &cli, name);

                if(ret > 0)
                {
                    cli->state = SENDING_CONNECTION_REQUEST;
                    memcpy(&cli->address,&from,sizeof(Address));
                    update_server_num_clients();

                    LOGN("Welcome New Client! (%d/%d)", server.num_clients, MAX_CLIENTS);
                    print_address(&cli->address);

                    if(ret == 1)
                    {
                        player_reset(&players[cli->client_id]);
                    }

                    for(int i = 0; i < MAX_CLIENTS; ++i)
                    {
                        if(i == cli->client_id) continue;
                        if(players[i].active)
                        {
                            if(ret == 1 || players[cli->client_id].curr_room != players[i].curr_room)
                            {
                                Vector2i tile = level_get_room_coords_by_pos(players[i].phys.pos.x, players[i].phys.pos.y);
                                player_send_to_room(&players[cli->client_id], players[i].curr_room, true, tile);
                            }
                            break;
                        }
                    }

                    // store salt
                    memcpy(cli->client_salt, salt, 8);
                    // unpack_bytes(&recv_pkt, cli->client_salt, 8, &offset);
                    server_send(PACKET_TYPE_CONNECT_CHALLENGE, cli);
                }
                else
                {
                    LOGNV("Creating temporary client");
                    // create a temporary ClientInfo so we can send a reject packet back
                    ClientInfo tmp_cli = {0};
                    memcpy(&tmp_cli.address,&from,sizeof(Address));

                    tmp_cli.last_reject_reason = CONNECT_REJECT_REASON_SERVER_FULL;
                    server_send(PACKET_TYPE_CONNECT_REJECTED, &tmp_cli);
                    break;
                }

            }
            else
            {

               int client_id = server_get_client(&from, &cli);
               if(client_id == -1) break;

                // existing client
                bool auth = authenticate_client(&recv_pkt,cli);
                offset = 8;

                if(!auth)
                {
                    LOGN("Client Failed authentication");

                    if(recv_pkt.hdr.type == PACKET_TYPE_CONNECT_CHALLENGE_RESP)
                    {
                        cli->last_reject_reason = CONNECT_REJECT_REASON_FAILED_CHALLENGE;
                        server_send(PACKET_TYPE_CONNECT_REJECTED,cli);
                        remove_client(cli);
                    }
                    break;
                }

                bool is_latest = is_packet_id_greater(recv_pkt.hdr.id, cli->remote_latest_packet_id);
                if(!is_latest)
                {
                    LOGN("Not latest packet from client. Ignoring...");
                    timer_delay_us(1000); // delay 1ms
                    break;
                }

                cli->remote_latest_packet_id = recv_pkt.hdr.id;
                cli->time_of_latest_packet = timer_get_time();

                LOGNV("%s() : %s", __func__, packet_type_to_str(recv_pkt.hdr.type));

                switch(recv_pkt.hdr.type)
                {

                    case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
                    {
                        cli->state = SENDING_CHALLENGE_RESPONSE;
                        LOGI("Accept client: %d", cli->client_id);
                        player_set_active(&players[cli->client_id],true);

                        server_send(PACKET_TYPE_CONNECT_ACCEPTED,cli);
                        server_send(PACKET_TYPE_INIT, cli);
                        server_send(PACKET_TYPE_STATE,cli);

                    } break;

                    case PACKET_TYPE_INPUT:
                    {
                        uint8_t _input_count = unpack_u8(&recv_pkt, &offset);
                        for(int i = 0; i < _input_count; ++i)
                        {
                            // get input, copy into array
                            unpack_bytes(&recv_pkt, (uint8_t*)&cli->net_player_inputs[cli->input_count++], sizeof(NetPlayerInput), &offset);
                        }
                    } break;

                    case PACKET_TYPE_MESSAGE:
                    {
                        uint8_t from = cli->client_id;
                        // uint8_t to = unpack_u8(&recv_pkt, &offset);
                        char msg[255+1] = {0};
                        uint8_t msg_len = unpack_string(&recv_pkt, msg, 255, &offset);

#if SERVER_PRINT_VERBOSE || 1
                        LOGN("received message");
                        LOGN("  from: %u", from);
                        // LOGN("  to:   %u", to);
                        LOGN("  msg:  %s", msg);
#endif
                        server_send_message(TO_ALL, from, "%s", msg);
                    } break;

                    case PACKET_TYPE_SETTINGS:
                    {
                        Player* p = &players[cli->client_id];

                        uint8_t class = unpack_u8(&recv_pkt, &offset);
                        p->settings.class = class;
                        player_set_class(p, class);

                        uint32_t color = unpack_u32(&recv_pkt, &offset);
                        p->settings.color = color;

                        memset(p->settings.name, 0, PLAYER_NAME_MAX);
                        uint8_t namelen = unpack_string(&recv_pkt, p->settings.name, PLAYER_NAME_MAX, &offset);

                        LOGN("Server Received Settings, Client ID: %d", cli->client_id);
                        LOGN("  color: 0x%08x", p->settings.color);
                        LOGN("  class: %u", p->settings.class);
                        LOGN("  name (%u): %s", namelen, p->settings.name);

                        for(int i = 0; i < MAX_CLIENTS; ++i)
                        {
                            ClientInfo* cli = &server.clients[i];
                            if(cli == NULL) continue;
                            if(cli->state != CONNECTED) continue;

                            server_send(PACKET_TYPE_SETTINGS,cli);
                        }
                    } break;

                    case PACKET_TYPE_PING:
                    {
                        server_send(PACKET_TYPE_PING, cli);
                    } break;

                    case PACKET_TYPE_DISCONNECT:
                    {
                        remove_client(cli);
                    } break;

                    default:
                    break;
                }
            }

            //timer_delay_us(1000); // delay 1ms
        }

        t1_g = timer_get_time();
        double elapsed_time_g = t1_g - t0_g;
        t0_g = t1_g;

        accum_g += elapsed_time_g;
        while(accum_g >= 1.0/TARGET_FPS)
        {
            server_simulate();
            accum_g -= 1.0/TARGET_FPS;
        }

        t1 = timer_get_time();
        double elapsed_time = t1 - t0;
        t0 = t1;

        accum += elapsed_time;

        if(accum >= dt)
        {
            // send state packet to all clients
            if(server.num_clients > 0)
            {
                // disconnect any client that hasn't sent a packet in DISCONNECTION_TIMEOUT
                for(int i = 0; i < MAX_CLIENTS; ++i)
                {
                    ClientInfo* cli = &server.clients[i];

                    if(cli == NULL) continue;
                    if(cli->state == DISCONNECTED) continue;

                    if(cli->time_of_latest_packet > 0)
                    {
                        double time_elapsed = timer_get_time() - cli->time_of_latest_packet;

                        if(time_elapsed >= DISCONNECTION_TIMEOUT)
                        {
                            LOGN("Client timed out. Elapsed time: %f", time_elapsed);

                            // disconnect client
                            server_send(PACKET_TYPE_DISCONNECT,cli);
                            remove_client(cli);
                            continue;
                        }
                    }

                    // send world state to connected clients...
                    server_send(PACKET_TYPE_STATE,cli);
                }

                // clear out any queued events
                server.event_count = 0;

            }
            accum = 0.0;
            decal_clear_all();
        }

        // don't wait, just proceed to handling packets
        //timer_wait_for_frame(&server_timer);
        timer_delay_us(1000);
    }
}

bool net_server_add_event(NetEvent* event)
{
    if(server.event_count >= MAX_NET_EVENTS)
    {
        LOGW("Too many events!");
        return false;
    }

    memcpy(&server.events[server.event_count++], event, sizeof(NetEvent));
}

void server_send_message(uint8_t to, uint8_t from, char* fmt, ...)
{
    if(role != ROLE_SERVER)
    {
        return;
    }

    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = server.info.local_latest_packet_id,
        // .hdr.ack = cli->remote_latest_packet_id,
        .hdr.type = PACKET_TYPE_MESSAGE
    };

    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int size = vsnprintf(NULL, 0, fmt, args);
    char* msg = calloc(size+1, sizeof(char));
    if(!msg) return;
    vsnprintf(msg, size+1, fmt, args2);
    va_end(args);
    va_end(args2);

    pack_u8(&pkt, from);
    pack_string(&pkt, msg, 255);

    free(msg);

    if(to == 0xFF) //send to all
    {
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            ClientInfo* cli = &server.clients[i];
            if(cli == NULL) continue;
            if(cli->state != CONNECTED) continue;

            pkt.hdr.ack = cli->remote_latest_packet_id;
            net_send(&server.info,&cli->address,&pkt);
        }
    }
    else
    {
        if(to > MAX_PLAYERS) return;
        ClientInfo* cli = &server.clients[to];
        if(cli == NULL) return;
        if(cli->state != CONNECTED) return;

        pkt.hdr.ack = cli->remote_latest_packet_id;
        net_send(&server.info,&cli->address,&pkt);
    }
}

// =========
// @CLIENT
// =========

bool net_client_add_player_input(NetPlayerInput* input)
{
    if(input_count >= INPUT_QUEUE_MAX)
    {
        LOGW("Input array is full!");
        return false;
    }

    memcpy(&net_player_inputs[input_count], input, sizeof(NetPlayerInput));
    input_count++;

    return true;
}

int net_client_get_input_count()
{
    return input_count;
}

uint8_t net_client_get_player_count()
{
    return client.player_count;
}

ConnectionState net_client_get_state()
{
    return client.state;
}

int net_client_get_id()
{
    return client.id;
}

uint16_t net_client_get_latest_local_packet_id()
{
    return client.info.local_latest_packet_id;
}

void net_client_get_server_ip_str(char* ip_str)
{
    if(!ip_str)
        return;

    sprintf(ip_str,"%u.%u.%u.%u:%u",server.address.a,server.address.b, server.address.c, server.address.d, server.address.port);
    return;
}

bool net_client_set_server_ip(char* address)
{
    // example input:
    // 200.100.24.10

    char num_str[3] = {0};
    uint8_t   bytes[4]  = {0};

    uint8_t   num_str_index = 0, byte_index = 0;

    for(int i = 0; i < strlen(address)+1; ++i)
    {
        if(address[i] == '.' || address[i] == '\0')
        {
            bytes[byte_index++] = atoi(num_str);
            memset(num_str,0,3*sizeof(char));
            num_str_index = 0;
            continue;
        }

        num_str[num_str_index++] = address[i];
    }

    server.address.a = bytes[0];
    server.address.b = bytes[1];
    server.address.c = bytes[2];
    server.address.d = bytes[3];

    server.address.port = PORT;

    return true;
}

// client information
bool net_client_init()
{
    socket_initialize();

    int sock;

    LOGN("Creating socket.");
    socket_create(&sock);

    client.id = -1;
    client.info.socket = sock;
    circbuf_create(&client.input_packets,10, sizeof(Packet));

    bitpack_create(&client.bp, BITPACK_SIZE);

    return true;
}

static bool _client_data_waiting()
{
    bool data_waiting = has_data_waiting(client.info.socket);
    return data_waiting;
}

static void client_clear()
{
    circbuf_clear_items(&client.input_packets);
    client.received_init_packet = false;
    client.time_of_latest_sent_packet = 0.0;
    client.time_of_last_ping = 0.0;
    client.time_of_last_received_ping = 0.0;
    client.rtt = 0.0;
    client.id = -1;

}

static void client_send(PacketType type)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = client.info.local_latest_packet_id,
        .hdr.ack = client.info.remote_latest_packet_id,
        .hdr.type = type
    };

    memset(pkt.data, 0, MAX_PACKET_DATA_SIZE);

    LOGNV("%s() : %s", __func__, packet_type_to_str(type));

    switch(type)
    {
        case PACKET_TYPE_CONNECT_REQUEST:
        {
            uint64_t salt = rand64();
            memcpy(client.client_salt, (uint8_t*)&salt,8);

            pack_bytes(&pkt, (uint8_t*)client.client_salt, 8);
            // pack_string(&pkt, (uint8_t*)client.client_salt, 8);
            pack_string(&pkt, player->settings.name, PLAYER_NAME_MAX);
            pkt.data_len = 1024; // pad to 1024

            net_send(&client.info,&server.address,&pkt);
        } break;

        case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
        {
            store_xor_salts(client.client_salt, client.server_salt, client.xor_salts);

            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            pkt.data_len = 1024; // pad to 1024

            net_send(&client.info,&server.address,&pkt);
        } break;

        case PACKET_TYPE_SETTINGS:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            pack_u8(&pkt, player->settings.class);
            pack_u32(&pkt, player->settings.color);
            pack_string(&pkt, player->settings.name, PLAYER_NAME_MAX);

            // LOGN("Client Send Settings");
            // LOGN("  color: 0x%08x", player->settings.color);
            // LOGN("  sprite index: %u", player->settings.sprite_index);
            // LOGN("  name (%d): %s", strlen(player->settings.name), player->settings.name);

            net_send(&client.info,&server.address,&pkt);
        } break;

        case PACKET_TYPE_PING:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            net_send(&client.info,&server.address,&pkt);
        } break;

        case PACKET_TYPE_INPUT:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            pack_u8(&pkt, input_count);
            for(int i = 0; i < input_count; ++i)
            {
                pack_bytes(&pkt, (uint8_t*)&net_player_inputs[i], sizeof(NetPlayerInput));
            }

            circbuf_add(&client.input_packets,&pkt);
            net_send(&client.info,&server.address,&pkt);
        } break;

        case PACKET_TYPE_DISCONNECT:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);

            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
            {
                net_send(&client.info,&server.address,&pkt);
                pkt.hdr.id = client.info.local_latest_packet_id;
            }
        } break;

        default:
            break;
    }

    client.time_of_latest_sent_packet = timer_get_time();
}


static bool client_get_input_packet(Packet* input, int packet_id)
{
    for(int i = client.input_packets.count -1; i >= 0; --i)
    {
        Packet* pkt = (Packet*)circbuf_get_item(&client.input_packets,i);

        if(pkt)
        {
            if(pkt->hdr.id == packet_id)
            {
                input = pkt;
                return true;
            }
        }
    }
    return false;
}

bool net_client_connect_update()
{
    if(client.state == DISCONNECTED)
    {
        client.state = SENDING_CONNECTION_REQUEST;
        client_send(PACKET_TYPE_CONNECT_REQUEST);
        return true;
    }
    else if(client.state == SENDING_CONNECTION_REQUEST)
    {
        int ret = net_client_data_waiting();
        if(ret == 2)
        {
            Packet srvpkt = {0};
            int offset = 0;
            int recv_bytes = net_client_recv(&srvpkt);

            if(recv_bytes > 0 && srvpkt.hdr.type == PACKET_TYPE_CONNECT_CHALLENGE)
            {
                uint8_t srv_client_salt[8] = {0};
                unpack_bytes(&srvpkt, srv_client_salt, 8, &offset);
                if(memcmp(srv_client_salt, client.client_salt, 8) != 0)
                {
                    LOGN("Server sent client salt doesn't match actual client salt");
                    return false;
                }

                unpack_bytes(&srvpkt, client.server_salt, 8, &offset);
                LOGN("Received Connect Challenge.");

                client.state = SENDING_CHALLENGE_RESPONSE;
                client_send(PACKET_TYPE_CONNECT_CHALLENGE_RESP);
                return true;
            }
        }
    }
    else if(client.state == SENDING_CHALLENGE_RESPONSE)
    {
        int ret = net_client_data_waiting();
        if(ret == 2)
        {
            Packet srvpkt = {0};
            int offset = 0;
            int recv_bytes = net_client_recv(&srvpkt);

            if(recv_bytes > 0 && srvpkt.hdr.type == PACKET_TYPE_CONNECT_ACCEPTED)
            {
                LOGN("Received Connection Accepted.");

                client.state = CONNECTED;
                client.time_of_connection = timer_get_time();
                uint8_t cid = unpack_u8(&srvpkt, &offset);
                printf("cid: %u\n",cid);
                if(cid < 0 || cid >= MAX_CLIENTS)
                {
                    LOGN("Invalid Client ID");
                    return false;
                }

                client.id = (int)cid;
                return true;
            }
        }
    }

    return false;
}


// blocking connect
int net_client_connect()
{
    if(client.state != DISCONNECTED)
        return -1; // temporary, handle different states in the future

    for(;;)
    {
        client.state = SENDING_CONNECTION_REQUEST;
        client_send(PACKET_TYPE_CONNECT_REQUEST);

        for(;;)
        {
            bool data_waiting = _client_data_waiting();

            if(!data_waiting)
            {
                double time_elapsed = timer_get_time() - client.time_of_latest_sent_packet;
                if(time_elapsed >= DEFAULT_TIMEOUT)
                    break; // retry sending

                timer_delay_us(1000); // delay 1ms
                continue;
            }

            Packet srvpkt = {0};
            int offset = 0;

            int recv_bytes = net_client_recv(&srvpkt);
            if(recv_bytes > 0)
            {
                switch(srvpkt.hdr.type)
                {
                    case PACKET_TYPE_CONNECT_CHALLENGE:
                    {
                        uint8_t srv_client_salt[8] = {0};
                        unpack_bytes(&srvpkt, srv_client_salt, 8, &offset);
                        if(memcmp(srv_client_salt, client.client_salt, 8) != 0)
                        {
                            LOGN("Server sent client salt doesn't match actual client salt");
                            return -1;
                        }

                        unpack_bytes(&srvpkt, client.server_salt, 8, &offset);
                        LOGN("Received Connect Challenge.");

                        client.state = SENDING_CHALLENGE_RESPONSE;
                        client_send(PACKET_TYPE_CONNECT_CHALLENGE_RESP);
                    } break;

                    case PACKET_TYPE_CONNECT_ACCEPTED:
                    {
                        client.state = CONNECTED;
                        client.time_of_connection = timer_get_time();
                        uint8_t client_id = unpack_u8(&srvpkt, &offset);
                        client.id = client_id;
                        return (int)client_id;
                    } break;

                    case PACKET_TYPE_CONNECT_REJECTED:
                    {
                        uint8_t reason = unpack_u8(&srvpkt, &offset);
                        LOGN("Rejection Reason: %s (%02X)", connect_reject_reason_to_str(reason), reason);
                        client.state = DISCONNECTED; // TODO: is this okay?
                        client.id = -1;
                        client_clear();
                    } break;
                }
            }

            timer_delay_us(1000); // delay 1000 us
        }
    }
}


// 0: no data waiting
// 1: timeout
// 2: got data
int net_client_data_waiting()
{
    bool data_waiting = _client_data_waiting();

    if(!data_waiting)
    {
        double time_elapsed = timer_get_time() - client.time_of_latest_sent_packet;
        if(time_elapsed >= DEFAULT_TIMEOUT)
        {
            printf("disconnecting?\n");
            net_client_disconnect();
            return 1;
        }

        return 0;
    }

    return 2;
}

void net_client_update()
{
    bool data_waiting = _client_data_waiting(); // non-blocking

    if(data_waiting)
    {
        Packet srvpkt = {0};
        int offset = 0;

        int recv_bytes = net_client_recv(&srvpkt);

        bool is_latest = is_packet_id_greater(srvpkt.hdr.id, client.info.remote_latest_packet_id);

        if(recv_bytes > 0 && is_latest)
        {
            switch(srvpkt.hdr.type)
            {
                case PACKET_TYPE_INIT:
                {
                    int seed = unpack_u32(&srvpkt,&offset);
                    uint8_t rank = unpack_u8(&srvpkt,&offset);
                    trigger_generate_level(seed,rank,0, __LINE__);

                    client.received_init_packet = true;
                } break;
                case PACKET_TYPE_STATE:
                {
                    //print_packet(&srvpkt, true);

                    bitpack_clear(&client.bp);
                    bitpack_memcpy(&client.bp, &srvpkt.data[offset], srvpkt.data_len);
                    bitpack_seek_begin(&client.bp);
                    //bitpack_print(&client.bp);

                    unpack_events_bp(&srvpkt,&offset);
                    unpack_players_bp(&srvpkt, &offset);
                    unpack_creatures_bp(&srvpkt, &offset);
                    unpack_projectiles_bp(&srvpkt, &offset);
                    unpack_items_bp(&srvpkt,&offset);
                    unpack_decals_bp(&srvpkt, &offset);
                    unpack_other_bp(&srvpkt,&offset);

                    int bytes_read = 4*(client.bp.word_index+1);
                    offset += bytes_read;

                    client.player_count = player_get_active_count();
                } break;

                case PACKET_TYPE_PING:
                {
                    client.time_of_last_received_ping = timer_get_time();
                    client.rtt = 1000.0f*(client.time_of_last_received_ping - client.time_of_last_ping);
                } break;

                case PACKET_TYPE_MESSAGE:
                {
                    uint8_t from = unpack_u8(&srvpkt, &offset);

                    char msg[255+1] = {0};
                    uint8_t msg_len = unpack_string(&srvpkt, msg, 255, &offset);

                    if(from < MAX_PLAYERS)
                    {
                        text_list_add(text_lst, players[from].settings.color, 10.0, "%s: %s", players[from].settings.name, msg);
                    }
                    else
                    {
                        text_list_add(text_lst, COLOR_WHITE, 10.0, "Server: %s", msg);
                    }

                } break;

                case PACKET_TYPE_SETTINGS:
                {
                    uint8_t num_players = unpack_u8(&srvpkt, &offset);

                    for(int i = 0; i < num_players; ++i)
                    {
                        uint8_t client_id = unpack_u8(&srvpkt, &offset);

                        //LOGN("  %d: Client ID %d", i, client_id);

                        if(client_id >= MAX_CLIENTS)
                        {
                            LOGE("Client ID is too large: %d", client_id);
                            break;
                        }

                        Player* p = &players[client_id];
                        p->settings.class = unpack_u8(&srvpkt, &offset);
                        p->settings.color = unpack_u32(&srvpkt, &offset);
                        uint8_t namelen = unpack_string(&srvpkt, p->settings.name, PLAYER_NAME_MAX, &offset);

                        player_set_class(p, p->settings.class);

                        LOGN("Client Received Settings, Client ID: %d", client_id);
                        LOGN("  class: %u", p->settings.class);
                        LOGN("  color: 0x%08x", p->settings.color);
                        LOGN("  name (%u): %s", namelen, p->settings.name);

                    }

                } break;

                case PACKET_TYPE_DISCONNECT:
                    client.state = DISCONNECTED;
                    client.id = -1;
                    break;
            }
        }
    }

    // handle pinging server
    double time_elapsed = timer_get_time() - client.time_of_last_ping;
    if(time_elapsed >= PING_PERIOD)
    {
        client_send(PACKET_TYPE_PING);
        client.time_of_last_ping = timer_get_time();
    }

    if(client.state == CONNECTED && client.time_of_last_received_ping > 0.0)
    {
        // handle disconnection from server if haven't received ping
        double time_since_server_ping = timer_get_time() - client.time_of_last_received_ping;
        if(time_since_server_ping >= DISCONNECTION_TIMEOUT)
        {
            LOGN("Server not responding. Elapsed time: %f", time_since_server_ping);
            net_client_disconnect();
        }
    }

    // handle publishing inputs
    if(input_count >= inputs_per_packet)
    {
        client_send(PACKET_TYPE_INPUT);
        input_count = 0;
    }
}

bool net_client_is_connected()
{
    return (client.state == CONNECTED);
}

double net_client_get_rtt()
{
    return client.rtt;
}

uint32_t net_client_get_sent_bytes()
{
    return client.bytes_sent;
}

uint32_t net_client_get_recv_bytes()
{
    return client.bytes_received;
}

uint32_t net_client_get_largest_packet_size_recv()
{
    return client.largest_packet_size_recv;
}

uint32_t net_client_get_largest_packet_size_sent()
{
    return client.largest_packet_size_sent;
}

double net_client_get_connected_time()
{
    return client.time_of_connection;
}

void net_client_disconnect()
{
    if(client.state != DISCONNECTED)
    {
        client_send(PACKET_TYPE_DISCONNECT);
        client.state = DISCONNECTED;
        client_clear();
    }
}

void net_client_send_settings()
{
    client_send(PACKET_TYPE_SETTINGS);
}

bool net_client_received_init_packet()
{
    return client.received_init_packet;
}

void net_client_send_message(char* fmt, ...)
{
    if(role != ROLE_CLIENT)
    {
        return;
    }

    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = client.info.local_latest_packet_id,
        .hdr.ack = client.info.remote_latest_packet_id,
        .hdr.type = PACKET_TYPE_MESSAGE
    };

    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int size = vsnprintf(NULL, 0, fmt, args);
    char* msg = calloc(size+1, sizeof(char));
    if(!msg) return;
    vsnprintf(msg, size+1, fmt, args2);
    va_end(args);
    va_end(args2);

    pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
    // pack_u8(&pkt, TO_ALL);
    pack_string(&pkt, msg, 255);

    free(msg);

    net_send(&client.info,&server.address,&pkt);
}


int net_client_send(uint8_t* data, uint32_t len)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = client.info.local_latest_packet_id,
    };

    memcpy(pkt.data,data,len);
    pkt.data_len = len;

    int sent_bytes = net_send(&client.info, &server.address, &pkt);
    return sent_bytes;
}

int net_client_recv(Packet* pkt)
{
    Address from = {0};
    int recv_bytes = net_recv(&client.info, &from, pkt);
    return recv_bytes;
}

void net_client_deinit()
{
    socket_close(client.info.socket);
}

static inline void pack_bool(Packet* pkt, bool d)
{
    pkt->data[pkt->data_len] = d ? 0x01 : 0x00;
    pkt->data_len += sizeof(uint8_t);
}

static inline void pack_u8(Packet* pkt, uint8_t d)
{
    pkt->data[pkt->data_len] = d;
    pkt->data_len += sizeof(uint8_t);
}

static inline void pack_u8_at(Packet* pkt, uint8_t d, int index)
{
    pkt->data[index] = d;
}

static inline void pack_u16(Packet* pkt, uint16_t d)
{
    pkt->data[pkt->data_len+0] = (d>>8) & 0xFF;
    pkt->data[pkt->data_len+1] = (d) & 0xFF;

    pkt->data_len+=sizeof(uint16_t);
}

static inline void pack_u16_at(Packet* pkt, uint16_t d, int index)
{
    pkt->data[index+0] = (d>>8) & 0xFF;
    pkt->data[index+1] = (d) & 0xFF;
}
static inline void pack_u32(Packet* pkt, uint32_t d)
{
    pkt->data[pkt->data_len+0] = (d>>24) & 0xFF;
    pkt->data[pkt->data_len+1] = (d>>16) & 0xFF;
    pkt->data[pkt->data_len+2] = (d>>8) & 0xFF;
    pkt->data[pkt->data_len+3] = (d) & 0xFF;

    pkt->data_len+=sizeof(uint32_t);
}

static inline void pack_i32(Packet* pkt, int32_t d)
{
    pkt->data[pkt->data_len+0] = (d>>24) & 0xFF;
    pkt->data[pkt->data_len+1] = (d>>16) & 0xFF;
    pkt->data[pkt->data_len+2] = (d>>8) & 0xFF;
    pkt->data[pkt->data_len+3] = (d) & 0xFF;

    pkt->data_len+=sizeof(uint32_t);
}

static inline void pack_u64(Packet* pkt, uint64_t d)
{
    pkt->data[pkt->data_len+0] = (d>>56) & 0xFF;
    pkt->data[pkt->data_len+1] = (d>>48) & 0xFF;
    pkt->data[pkt->data_len+2] = (d>>40) & 0xFF;
    pkt->data[pkt->data_len+3] = (d>>32) & 0xFF;
    pkt->data[pkt->data_len+4] = (d>>24) & 0xFF;
    pkt->data[pkt->data_len+5] = (d>>16) & 0xFF;
    pkt->data[pkt->data_len+6] = (d>>8) & 0xFF;
    pkt->data[pkt->data_len+7] = (d) & 0xFF;

    pkt->data_len+=sizeof(uint64_t);
}


static inline void pack_float(Packet* pkt, float d)
{
    memcpy(&pkt->data[pkt->data_len],&d,sizeof(float));
    pkt->data_len+=sizeof(float);
}

static inline void pack_bytes(Packet* pkt, uint8_t* d, uint32_t len)
{
    memcpy(&pkt->data[pkt->data_len],d,sizeof(uint8_t)*len);
    pkt->data_len+=len*sizeof(uint8_t);
}

static inline void pack_string(Packet* pkt, char* s, uint8_t max_len)
{
    uint8_t slen = (uint8_t)MIN(max_len,strlen(s));
    pkt->data[pkt->data_len] = slen;
    pkt->data_len++;

    memcpy(&pkt->data[pkt->data_len],s,slen*sizeof(char));
    pkt->data_len += slen*sizeof(char);
}

static inline void pack_vec2(Packet* pkt, Vector2f d)
{
    memcpy(&pkt->data[pkt->data_len],&d,sizeof(Vector2f));
    pkt->data_len+=sizeof(Vector2f);
}

static inline void pack_vec3(Packet* pkt, Vector3f d)
{
    memcpy(&pkt->data[pkt->data_len],&d,sizeof(Vector3f));
    pkt->data_len+=sizeof(Vector3f);
}

static inline void pack_itemtype(Packet* pkt, ItemType type)
{
    pack_u8(pkt, (uint8_t)type);
}


static inline bool unpack_bool(Packet* pkt, int* offset)
{
    uint8_t r = pkt->data[*offset];
    (*offset)++;
    return (r == 0x01);
}

static inline uint8_t  unpack_u8(Packet* pkt, int* offset)
{
    uint8_t r = pkt->data[*offset];
    (*offset)++;
    return r;
}

static inline uint16_t unpack_u16(Packet* pkt, int* offset)
{
    uint16_t r = pkt->data[*offset] << 8 | pkt->data[*offset+1];
    (*offset)+=sizeof(uint16_t);
    return r;
}

static inline uint32_t unpack_u32(Packet* pkt, int* offset)
{
    uint32_t r = pkt->data[*offset] << 24 | pkt->data[*offset+1] << 16 | pkt->data[*offset+2] << 8 | pkt->data[*offset+3];
    (*offset)+=sizeof(uint32_t);
    return r;
}

static inline int32_t unpack_i32(Packet* pkt, int* offset)
{
    int32_t r = pkt->data[*offset] << 24 | pkt->data[*offset+1] << 16 | pkt->data[*offset+2] << 8 | pkt->data[*offset+3];
    (*offset)+=sizeof(uint32_t);
    return r;
}

static inline uint64_t unpack_u64(Packet* pkt, int* offset)
{
    uint64_t r = (uint64_t)(pkt->data[*offset]) << 56;
    r |= (uint64_t)(pkt->data[*offset+1]) << 48;
    r |= (uint64_t)(pkt->data[*offset+2]) << 40;
    r |= (uint64_t)(pkt->data[*offset+3]) << 32;
    r |= (uint64_t)(pkt->data[*offset+4]) << 24;
    r |= (uint64_t)(pkt->data[*offset+5]) << 16;
    r |= (uint64_t)(pkt->data[*offset+6]) << 8;
    r |= (uint64_t)(pkt->data[*offset+7]);

    (*offset)+=sizeof(uint64_t);
    return r;
}

static inline float unpack_float(Packet* pkt, int* offset)
{
    float r;
    memcpy(&r, &pkt->data[*offset], sizeof(float));
    (*offset) += sizeof(float);
    return r;
}

static inline void unpack_bytes(Packet* pkt, uint8_t* d, int len, int* offset)
{
    memcpy(d, &pkt->data[*offset], len*sizeof(uint8_t));
    (*offset) += len*sizeof(uint8_t);
}


static inline uint8_t unpack_string(Packet* pkt, char* s, int maxlen, int* offset)
{
    uint8_t len = unpack_u8(pkt, offset);
    if(len > maxlen)
        LOGW("unpack_string(): len > maxlen (%u > %u)", len, maxlen);

    uint8_t copy_len = MIN(len,maxlen);
    memset(s, 0, maxlen);
    memcpy(s, &pkt->data[*offset], copy_len*sizeof(char));
    (*offset) += len; //traverse the actual total length
    return copy_len;
}

static inline Vector2f unpack_vec2(Packet* pkt, int* offset)
{
    Vector2f r;
    memcpy(&r, &pkt->data[*offset], sizeof(Vector2f));
    (*offset) += sizeof(Vector2f);
    return r;
}

static inline Vector3f unpack_vec3(Packet* pkt, int* offset)
{
    Vector3f r;
    memcpy(&r, &pkt->data[*offset], sizeof(Vector3f));
    (*offset) += sizeof(Vector3f);
    return r;
}

static inline ItemType unpack_itemtype(Packet* pkt, int* offset)
{
    int8_t type = (int8_t)unpack_u8(pkt, offset);
    return (ItemType)type;
}


void test_packing()
{
    Packet pkt = {0};

    uint16_t id0;
    Vector2f pos0;
    Vector3f pos1;
    float a0;
    uint8_t pid0;
    uint32_t color0;
    uint64_t u64_0;

    id0 = 12037;
    pack_u16(&pkt,id0);

    pos0.x = 123.456;
    pos0.y = 69.420;

    pos1.x = 123.456;
    pos1.y = 69.420;
    pos1.z = 8008.135;

    pack_vec2(&pkt,pos0);
    pack_vec3(&pkt,pos1);

    a0 = 234.1354;
    pack_float(&pkt,a0);

    pid0 = 85;
    pack_u8(&pkt,pid0);

    color0 = 2342612;
    pack_u32(&pkt,color0);

    u64_0 = 0xab000000cd000001;
    pack_u64(&pkt,u64_0);


    LOGI("Packet Length: %d", pkt.data_len);
    print_hex(pkt.data, pkt.data_len);

    uint16_t id1;
    Vector2f pos2;
    Vector3f pos3;
    float a1;
    uint8_t pid1;
    uint32_t color1;
    uint64_t u64_1;

    int index = 0;
    id1 = unpack_u16(&pkt, &index);
    pos2 = unpack_vec2(&pkt, &index);
    pos3 = unpack_vec3(&pkt, &index);
    a1 = unpack_float(&pkt, &index);
    pid1 = unpack_u8(&pkt, &index);
    color1 = unpack_u32(&pkt, &index);
    u64_1 = unpack_u64(&pkt, &index);

    LOGI("Unpack final index: %d", index);

    LOGI("ID: %u  =  %u", id0, id1);
    LOGI("Pos: %.3f, %.3f  =  %.3f, %.3f", pos0.x, pos0.y, pos2.x, pos2.y);
    LOGI("Pos: %.3f, %.3f, %.3f =  %.3f, %.3f, %.3f", pos1.x, pos1.y, pos1.z, pos3.x, pos3.y, pos3.z);
    LOGI("Angle: %.4f  =  %.4f", a0, a1);
    LOGI("P ID: %u  =  %u", pid0, pid1);
    LOGI("Color: %u  =  %u", color0, color1);
    LOGI("u64: %llu  =  %llu", u64_0, u64_1);
}

#define BPW(a,b,c) {if(!bitpack_write(a,b,c)) printf("ERROR: %d\n", __LINE__);}

static void pack_players_bp(Packet* pkt, ClientInfo* cli)
{
    uint8_t player_count = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state == CONNECTED)
            player_count++;
    }

    BPW(&server.bp, 4,  (uint32_t)player_count);

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state == CONNECTED)
        {
            Player* p = &players[i];

            // LOGN("Packing player %d (%d)", i, server.clients[i].client_id);

            BPW(&server.bp, 3,  (uint32_t)i);
            BPW(&server.bp, 10, (uint32_t)p->phys.pos.x);
            BPW(&server.bp, 10, (uint32_t)p->phys.pos.y);
            BPW(&server.bp, 6,  (uint32_t)p->phys.pos.z);
            BPW(&server.bp, 5,  (uint32_t)p->sprite_index+p->anim.curr_frame);
            BPW(&server.bp, 7,  (uint32_t)p->curr_room);
            BPW(&server.bp, 4,  (uint32_t)p->phys.hp);
            BPW(&server.bp, 4,  (uint32_t)p->phys.hp_max);
            BPW(&server.bp, 16, (uint32_t)(p->highlighted_item_id+1));

            BPW(&server.bp, 5, (uint32_t)(p->skill_count));
            for(int j = 0; j < p->skill_count; ++j)
            {
                BPW(&server.bp, 8, (uint32_t)(p->skills[j]));
            }

            BPW(&server.bp, 12, (uint32_t)(p->xp));
            BPW(&server.bp, 5, (uint32_t)(p->level));
            BPW(&server.bp, 5, (uint32_t)(p->new_levels));

            BPW(&server.bp, 3, (uint32_t)(p->skill_selection));
            BPW(&server.bp, 3, (uint32_t)(p->num_skill_selection_choices));
            for(int j = 0; j < p->num_skill_selection_choices; ++j)
            {
                BPW(&server.bp, 8, (uint32_t)(p->skill_choices[j]));
            }

            BPW(&server.bp, 4,  (uint32_t)p->gauntlet_selection);
            BPW(&server.bp, 4,  (uint32_t)p->gauntlet_slots);

            for(int g = 0; g < PLAYER_GAUNTLET_MAX; ++g)
                BPW(&server.bp, 6,  (uint32_t)(p->gauntlet[g].type + 1));

            BPW(&server.bp, 1, (uint32_t)(p->invulnerable_temp ? 1 : 0));
            BPW(&server.bp, 6, (uint32_t)p->invulnerable_temp_time);
            BPW(&server.bp, 4, (uint32_t)p->door);
            BPW(&server.bp, 1, (uint32_t)(p->phys.dead ? 1 : 0));

            uint32_t timed_items_count = 0;
            for(int t = 0; t < MAX_TIMED_ITEMS; ++t)
                if(p->timed_items[t] != ITEM_NONE)
                    timed_items_count++;

            BPW(&server.bp, 4,  (uint32_t)timed_items_count);
            for(int t = 0; t < MAX_TIMED_ITEMS; ++t)
            {
                if(p->timed_items[t] == ITEM_NONE)
                    continue;

                BPW(&server.bp, 6,  (uint32_t)(p->timed_items[t] + 1));
                BPW(&server.bp, 8,  (uint32_t)(p->timed_items_ttl[t] * 10.0));
            }
        }
    }
}

static void unpack_players_bp(Packet* pkt, int* offset)
{
    uint8_t player_count = (uint8_t)bitpack_read(&client.bp, 4);
    client.player_count = player_count;

    bool prior_active[MAX_CLIENTS] = {0};
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        prior_active[i] = players[i].active;
        players[i].active = false;
    }

    for(int i = 0; i < player_count; ++i)
    {
        uint32_t id                  = bitpack_read(&client.bp, 3);
        uint32_t x                   = bitpack_read(&client.bp, 10);
        uint32_t y                   = bitpack_read(&client.bp, 10);
        uint32_t z                   = bitpack_read(&client.bp, 6);
        uint32_t sprite_index        = bitpack_read(&client.bp, 5);
        uint32_t c_room              = bitpack_read(&client.bp, 7);
        uint32_t hp                  = bitpack_read(&client.bp, 4);
        uint32_t hp_max              = bitpack_read(&client.bp, 4);
        uint32_t highlighted_item_id = bitpack_read(&client.bp, 16);
        uint32_t skill_count         = bitpack_read(&client.bp, 5);

        uint32_t skills[PLAYER_MAX_SKILLS] = {0};
        for(int j = 0; j < skill_count; ++j)
            skills[j] =  bitpack_read(&client.bp, 8);

        uint32_t xp                  = bitpack_read(&client.bp, 12);
        uint32_t level               = bitpack_read(&client.bp, 5);
        uint32_t new_levels          = bitpack_read(&client.bp, 5);

        uint32_t skill_selection     = bitpack_read(&client.bp, 3);
        uint32_t num_skill_choices   = bitpack_read(&client.bp, 3);

        uint32_t skill_choices[MAX_SKILL_CHOICES] = {0};
        for(int j = 0; j < num_skill_choices; ++j)
            skill_choices[j] =  bitpack_read(&client.bp, 8);

        uint32_t gauntlet_selection  = bitpack_read(&client.bp, 4);
        uint32_t gauntlet_slots      = bitpack_read(&client.bp, 4);

        uint32_t gauntlet_types[PLAYER_GAUNTLET_MAX] = {0};
        for(int g = 0; g < PLAYER_GAUNTLET_MAX; ++g)
            gauntlet_types[g] = bitpack_read(&client.bp, 6);

        uint32_t invunerable_temp = bitpack_read(&client.bp, 1);
        uint32_t inv_temp_time    = bitpack_read(&client.bp, 6);
        uint32_t door             = bitpack_read(&client.bp, 4);
        uint32_t dead             = bitpack_read(&client.bp, 1);

        uint32_t timed_items_count = bitpack_read(&client.bp, 4);

        uint32_t timed_items[MAX_TIMED_ITEMS] = {0};
        uint32_t timed_items_ttl[MAX_TIMED_ITEMS] = {0};

        for(int t = 0; t < timed_items_count; ++t)
        {
            timed_items[t]     = bitpack_read(&client.bp, 6);
            timed_items_ttl[t] = bitpack_read(&client.bp, 8);
        }

        uint8_t client_id = (uint8_t)id;

        if(client_id >= MAX_CLIENTS)
        {
            LOGE("Client ID is too large: %u",client_id);
            return; //TODO
        }

        Player* p = &players[client_id];
        p->active = true;

        Vector3f pos = {(float)x,(float)y,(float)z};
        p->sprite_index = (uint8_t)sprite_index;
        uint8_t curr_room  = (uint8_t)c_room;
        p->phys.hp  = (uint8_t)hp;
        p->phys.hp_max  = (uint8_t)hp_max;

        p->highlighted_item_id = ((int32_t)highlighted_item_id)-1;

        p->skill_count = (int)skill_count;
        for(int j = 0; j < p->skill_count; ++j)
        {
            p->skills[j] = (int)skills[j];
        }

        p->xp = (uint16_t)xp;
        p->level = (uint8_t)level;
        p->new_levels = (uint8_t)new_levels;
        p->skill_selection = (uint8_t)skill_selection;
        p->num_skill_selection_choices = (uint8_t)num_skill_choices;

        for(int j = 0; j < MAX_SKILL_CHOICES; ++j)
        {
            p->skill_choices[j] = (uint16_t)skill_choices[j];
        }

        p->gauntlet_selection = (uint8_t)gauntlet_selection;
        p->gauntlet_slots = (uint8_t)gauntlet_slots;
        for(int g = 0; g < PLAYER_GAUNTLET_MAX; ++g)
        {
            int8_t type = ((int8_t)gauntlet_types[g]) - 1;
            p->gauntlet[g].type = (ItemType)type;
        }

        p->invulnerable_temp = invunerable_temp == 1 ? true : false;
        float invulnerable_temp_time = (float)inv_temp_time;
        p->door  = (Dir)door;
        p->phys.dead  = dead == 1 ? true: false;

        for(int i = 0; i < MAX_TIMED_ITEMS; ++i)
            p->timed_items[i] = ITEM_NONE;

        for(int t = 0; t < timed_items_count; ++t)
        {
            int   ti     = (int)timed_items[t];
            float ti_ttl = (float)timed_items_ttl[t];

            p->timed_items[t]     = (ItemType)(ti-1);
            p->timed_items_ttl[t] = (ti_ttl/10.0f);
        }

        if(!prior_active[client_id])
        {
            p->curr_room = curr_room;
            p->transition_room = p->curr_room;
        }

        uint8_t prior_curr_room = p->curr_room;

        // moving between rooms
        if(curr_room != p->curr_room)
        {
            if(p == player)
            {
                p->transition_room = p->curr_room;
                p->curr_room = curr_room;

                if(level_transition_state == 0 && !level_generate_triggered)
                {
                    player_start_room_transition(p);
                }

            }
            else
            {
                p->transition_room = curr_room;
                p->curr_room = curr_room;
            }
            // avoid lerping
            p->phys.pos.x = pos.x;
            p->phys.pos.y = pos.y;
            p->phys.pos.z = pos.z;
        }

        p->lerp_t = 0.0;

        p->server_state_prior.pos.x = p->phys.pos.x;
        p->server_state_prior.pos.y = p->phys.pos.y;
        p->server_state_prior.pos.z = p->phys.pos.z;
        p->server_state_prior.invulnerable_temp_time = p->invulnerable_temp_time;

        p->server_state_target.pos.x = pos.x;
        p->server_state_target.pos.y = pos.y;
        p->server_state_target.pos.z = pos.z;
        p->server_state_target.invulnerable_temp_time = invulnerable_temp_time;

        if(!prior_active[client_id])
        {
            printf("first state packet for %d\n", client_id);
            memcpy(&p->server_state_prior, &p->server_state_target, sizeof(p->server_state_target));
            p->transition_room = p->curr_room;
        }

        //player_print(p);
    }
}

static void pack_creatures_bp(Packet* pkt, ClientInfo* cli)
{
    uint16_t num_creatures = creature_get_count();
    uint16_t num_visible_creatures = 0;

    for(int i = 0; i < num_creatures; ++i)
    {
        if(!is_any_player_room(creatures[i].curr_room))
            continue;

        num_visible_creatures++;
    }

    BPW(&server.bp, 8,  (uint32_t)num_visible_creatures);

    for(int i = 0; i < num_creatures; ++i)
    {
        Creature* c = &creatures[i];

        if(!is_any_player_room(c->curr_room))
            continue;

        uint8_t r = (c->color >> 16) & 0xFF;
        uint8_t g = (c->color >>  8) & 0xFF;
        uint8_t b = (c->color >>  0) & 0xFF;

        BPW(&server.bp, 16, (uint32_t)c->id);
        BPW(&server.bp, 6,  (uint32_t)c->type);
        BPW(&server.bp, 10, (uint32_t)c->phys.pos.x);
        BPW(&server.bp, 10, (uint32_t)c->phys.pos.y);
        BPW(&server.bp, 6,  (uint32_t)c->phys.pos.z);
        BPW(&server.bp, 8,  (uint32_t)c->phys.width);
        BPW(&server.bp, 6,  (uint32_t)c->sprite_index);
        BPW(&server.bp, 7,  (uint32_t)c->curr_room);
        BPW(&server.bp, 8,  (uint32_t)c->phys.hp);

        BPW(&server.bp, 8,  (uint32_t)r);
        BPW(&server.bp, 8,  (uint32_t)g);
        BPW(&server.bp, 8,  (uint32_t)b);
    }
}

static void unpack_creatures_bp(Packet* pkt, int* offset)
{
    memcpy(prior_creatures, creatures, sizeof(Creature)*MAX_CREATURES);
    creature_clear_all();

    uint32_t num_creatures = bitpack_read(&client.bp, 8);

    for(int i = 0; i < num_creatures; ++i)
    {
        uint32_t id           = bitpack_read(&client.bp, 16);
        uint32_t type         = bitpack_read(&client.bp, 6);
        uint32_t x            = bitpack_read(&client.bp, 10);
        uint32_t y            = bitpack_read(&client.bp, 10);
        uint32_t z            = bitpack_read(&client.bp, 6);
        uint32_t width        = bitpack_read(&client.bp, 8);
        uint32_t sprite_index = bitpack_read(&client.bp, 6);
        uint32_t curr_room    = bitpack_read(&client.bp, 7);
        uint32_t hp           = bitpack_read(&client.bp, 8);
        uint32_t r            = bitpack_read(&client.bp, 8);
        uint32_t g            = bitpack_read(&client.bp, 8);
        uint32_t b            = bitpack_read(&client.bp, 8);

        Creature creature = {0};
        creature.id = (uint16_t)id;
        creature.type = (CreatureType)type;
        creature.phys.pos.x = (float)x;
        creature.phys.pos.y = (float)y;
        creature.phys.pos.z = (float)z;
        creature.phys.width = (float)width;
        creature.phys.collision_rect.w = (float)width;
        creature.sprite_index = (uint8_t)sprite_index;
        creature.curr_room = (uint8_t)curr_room;
        creature.phys.hp = (int8_t)hp;
        creature.color = COLOR(r,g,b);

        Creature* c = creature_add(NULL, 0, NULL, &creature);

        c->server_state_prior.pos.x = x;
        c->server_state_prior.pos.y = y;
        c->server_state_prior.pos.z = z;

        //find the prior
        for(int j = i; j < MAX_CREATURES; ++j)
        {
            Creature* cj = &prior_creatures[j];
            if(cj->id == c->id)
            {
                c->server_state_prior.pos.x = cj->phys.pos.x;
                c->server_state_prior.pos.y = cj->phys.pos.y;
                c->server_state_prior.pos.z = cj->phys.pos.z;
                break;
            }
        }

        c->lerp_t = 0.0;
        c->server_state_target.pos.x = x;
        c->server_state_target.pos.y = y;
        c->server_state_target.pos.z = z;
    }
}


static void pack_projectiles_bp(Packet* pkt, ClientInfo* cli)
{
    uint16_t num_projectiles = (uint16_t)plist->count;
    uint16_t num_visible_projectiles = 0;

    for(int i = 0; i < num_projectiles; ++i)
    {
        if(!is_any_player_room(projectiles[i].curr_room))
            continue;

        num_visible_projectiles++;
    }

    BPW(&server.bp, 12,  (uint32_t)num_visible_projectiles);

    for(int i = 0; i < num_projectiles; ++i)
    {
        Projectile* p = &projectiles[i];

        if(!is_any_player_room(p->curr_room))
            continue;

        uint8_t r = (p->color >> 16) & 0xFF;
        uint8_t g = (p->color >>  8) & 0xFF;
        uint8_t b = (p->color >>  0) & 0xFF;

        BPW(&server.bp, 16, (uint32_t)p->id);
        BPW(&server.bp, 10, (uint32_t)p->phys.pos.x);
        BPW(&server.bp, 10, (uint32_t)p->phys.pos.y);
        BPW(&server.bp, 6,  (uint32_t)p->phys.pos.z);
        BPW(&server.bp, 8,  (uint32_t)r);
        BPW(&server.bp, 8,  (uint32_t)g);
        BPW(&server.bp, 8,  (uint32_t)b);
        BPW(&server.bp, 3,  (uint32_t)(p->player_id));
        BPW(&server.bp, 7,  (uint32_t)(p->curr_room));
        BPW(&server.bp, 8,  (uint32_t)(p->def.scale*255.0f));
        BPW(&server.bp, 1,  (uint32_t)(p->from_player ? 0x01 : 0x00));
    }
}

static void unpack_projectiles_bp(Packet* pkt, int* offset)
{
    memcpy(prior_projectiles, projectiles, sizeof(Projectile)*MAX_PROJECTILES);

    // load projectiles
    uint32_t num_projectiles = bitpack_read(&client.bp, 12);

    list_clear(plist);
    plist->count = num_projectiles;

    for(int i = 0; i < num_projectiles; ++i)
    {
        Projectile* p = &projectiles[i];

        uint32_t id          = bitpack_read(&client.bp, 16);
        uint32_t x           = bitpack_read(&client.bp, 10);
        uint32_t y           = bitpack_read(&client.bp, 10);
        uint32_t z           = bitpack_read(&client.bp, 6);
        uint32_t r           = bitpack_read(&client.bp, 8);
        uint32_t g           = bitpack_read(&client.bp, 8);
        uint32_t b           = bitpack_read(&client.bp, 8);
        uint32_t pid         = bitpack_read(&client.bp, 3);
        uint32_t curr_room   = bitpack_read(&client.bp, 7);
        uint32_t scale       = bitpack_read(&client.bp, 8);
        uint32_t from_player = bitpack_read(&client.bp, 1);

        p->id = (uint16_t)id;
        p->color = COLOR(r,g,b);
        p->curr_room = (uint8_t)curr_room;
        p->def.scale = (float)(scale / 255.0f);
        p->from_player = from_player == 0x01 ? true : false;
        p->player_id = (uint8_t)pid;
        p->lerp_t = 0.0;

        p->server_state_prior.pos.x = (float)x;
        p->server_state_prior.pos.y = (float)y;
        p->server_state_prior.pos.z = (float)z;

        //find the prior
        for(int j = i; j < MAX_PROJECTILES; ++j)
        {
            Projectile* pj = &prior_projectiles[j];
            if(pj->id == p->id)
            {
                p->server_state_prior.pos.x = pj->phys.pos.x;
                p->server_state_prior.pos.y = pj->phys.pos.y;
                p->server_state_prior.pos.z = pj->phys.pos.z;
                break;
            }
        }

        p->server_state_target.pos.x = (float)x;
        p->server_state_target.pos.y = (float)y;
        p->server_state_target.pos.z = (float)z;
    }
}

static void pack_items_bp(Packet* pkt, ClientInfo* cli)
{
    uint16_t num_items = (uint16_t)item_list->count;
    uint8_t num_visible_items = 0;

    for(int i = 0; i < num_items; ++i)
    {
        Item* it = &items[i];
        if(!is_any_player_room(it->curr_room))
            continue;
        if(it->picked_up)
            continue;
        num_visible_items++;
    }

    BPW(&server.bp, 6,  (uint32_t)num_visible_items);

    for(int i = 0; i < num_items; ++i)
    {
        Item* it = &items[i];

        if(!is_any_player_room(it->curr_room))
            continue;

        if(it->picked_up)
            continue;

        BPW(&server.bp, 16, (uint32_t)it->id);
        BPW(&server.bp, 6,  (uint32_t)(it->type + 1));
        BPW(&server.bp, 10, (uint32_t)it->phys.pos.x);
        BPW(&server.bp, 10, (uint32_t)it->phys.pos.y);
        BPW(&server.bp, 6,  (uint32_t)it->phys.pos.z);
        BPW(&server.bp, 7,  (uint32_t)it->curr_room);
        BPW(&server.bp, 9,  (uint32_t)(it->angle));
        BPW(&server.bp, 1,  (uint32_t)(it->highlighted ? 0x01 : 0x00));
        BPW(&server.bp, 1,  (uint32_t)(it->used ? 0x01 : 0x00));
    }
}

static void unpack_items_bp(Packet* pkt, int* offset)
{
    memcpy(prior_items, items, sizeof(Item)*MAX_ITEMS);

    uint32_t num_items   = bitpack_read(&client.bp, 6);

    list_clear(item_list);
    item_list->count = num_items;

    for(int i = 0; i < num_items; ++i)
    {
        Item* it = &items[i];
        uint32_t id          = bitpack_read(&client.bp, 16);
        uint32_t type        = bitpack_read(&client.bp, 6);
        uint32_t x           = bitpack_read(&client.bp, 10);
        uint32_t y           = bitpack_read(&client.bp, 10);
        uint32_t z           = bitpack_read(&client.bp, 6);
        uint32_t c_room      = bitpack_read(&client.bp, 7);
        uint32_t _angle      = bitpack_read(&client.bp, 9);
        uint32_t highlighted = bitpack_read(&client.bp, 1);
        uint32_t used        = bitpack_read(&client.bp, 1);

        it->id = id;
        it->type = (int32_t)type-1;
        it->curr_room = (uint8_t)c_room;
        it->highlighted = highlighted == 1 ? true : false;
        it->used = used == 1 ? true : false;
        float angle = (float)_angle;
        Vector3f pos = {(float)x,(float)y,(float)z};

        it->lerp_t = 0.0;

        it->server_state_prior.pos.x = pos.x;
        it->server_state_prior.pos.y = pos.y;
        it->server_state_prior.pos.z = pos.z;
        it->server_state_prior.angle = angle;

        bool found_prior = false;

        //find the prior
        for(int j = i; j < MAX_ITEMS; ++j)
        {
            Item* itj = &prior_items[j];
            if(itj->type == ITEM_NONE) break;
            if(itj->id == it->id)
            {
                // printf("found prior: %d\n", itj->id);
                found_prior = true;
                it->server_state_prior.pos.x = itj->phys.pos.x;
                it->server_state_prior.pos.y = itj->phys.pos.y;
                it->server_state_prior.pos.z = itj->phys.pos.z;
                it->server_state_prior.angle = itj->angle;
                break;
            }
        }

        it->server_state_target.pos.x = pos.x;
        it->server_state_target.pos.y = pos.y;
        it->server_state_target.pos.z = pos.z;
        it->server_state_target.angle = angle;

        if(!found_prior)
        {
            // printf("didn't find prior\n");
            it->server_state_prior.pos.x = it->server_state_target.pos.x;
            it->server_state_prior.pos.y = it->server_state_target.pos.y;
            it->server_state_prior.pos.z = it->server_state_target.pos.z;
            it->server_state_prior.angle = it->server_state_target.angle;
        }

    }
}

static void pack_decals_bp(Packet* pkt, ClientInfo* cli)
{
    BPW(&server.bp, 7,  (uint32_t)decal_list->count);

    for(int i = 0; i < decal_list->count; ++i)
    {
        Decal* d = &decals[i];

        BPW(&server.bp, 5,  (uint32_t)d->sprite_index);
        BPW(&server.bp, 32, (uint32_t)d->tint);
        BPW(&server.bp, 8,  (uint32_t)(d->scale*255.0f));
        BPW(&server.bp, 9,  (uint32_t)(d->rotation));
        BPW(&server.bp, 8,  (uint32_t)(d->opacity*255.0f));
        BPW(&server.bp, 8,  (uint32_t)(d->ttl*10.0f));
        BPW(&server.bp, 10, (uint32_t)d->pos.x);
        BPW(&server.bp, 10, (uint32_t)d->pos.y);
        BPW(&server.bp, 7, (uint32_t)d->room);
    }
}

static void unpack_decals_bp(Packet* pkt, int* offset)
{
    uint8_t count = (uint8_t)bitpack_read(&client.bp, 7);

    for(int i = 0; i < count; ++i)
    {
        Decal d = {0};

        d.image = particles_image;

        d.sprite_index = (uint8_t)bitpack_read(&client.bp,5);
        d.tint         = (uint32_t)bitpack_read(&client.bp,32);
        d.scale        = (float)(bitpack_read(&client.bp,8)/255.0f);
        d.rotation     = (float)(bitpack_read(&client.bp,9));
        d.opacity      = (float)(bitpack_read(&client.bp,8)/255.0f);
        d.ttl          = (float)(bitpack_read(&client.bp,8)/10.0f);
        d.pos.x        = (float)bitpack_read(&client.bp,10);
        d.pos.y        = (float)bitpack_read(&client.bp,10);
        d.room         = (uint8_t)bitpack_read(&client.bp,7);

        decal_add(d);
    }
}

static void pack_other_bp(Packet* pkt, ClientInfo* cli)
{
    Room* room = level_get_room_by_index(&level, (int)players[cli->client_id].curr_room);

    // doors locked

    BPW(&server.bp, 1,  (uint32_t)(room->doors_locked ? 0x01 : 0x00));
}

static void unpack_other_bp(Packet* pkt, int* offset)
{
    Room* room = level_get_room_by_index(&level, (int)player->curr_room);

    // doors locked
    room->doors_locked = bitpack_read(&client.bp, 1) == 0x01 ? true : false;
}

static void pack_events_bp(Packet* pkt, ClientInfo* cli)
{
    BPW(&server.bp, 8,  (uint32_t)server.event_count);

    for(int i = server.event_count-1; i >= 0; --i)
    {
        NetEvent* ev = &server.events[i];

        BPW(&server.bp, 4,  (uint32_t)ev->type);

        switch(ev->type)
        {
            case EVENT_TYPE_PARTICLES:
            {
                BPW(&server.bp, 5,  (uint32_t)ev->data.particles.effect_index);
                BPW(&server.bp, 10, (uint32_t)ev->data.particles.pos.x);
                BPW(&server.bp, 10, (uint32_t)ev->data.particles.pos.y);
                BPW(&server.bp, 8,  (uint32_t)(255.0f * ev->data.particles.scale));
                BPW(&server.bp, 32, (uint32_t)ev->data.particles.color1);
                BPW(&server.bp, 32, (uint32_t)ev->data.particles.color2);
                BPW(&server.bp, 32, (uint32_t)ev->data.particles.color3);
                BPW(&server.bp, 8,  (uint32_t)(10.0f * ev->data.particles.lifetime));
                BPW(&server.bp, 7,  (uint32_t)(ev->data.particles.room_index));

            } break;

            case EVENT_TYPE_NEW_LEVEL:
            {
                BPW(&server.bp, 32,  (uint32_t)level_seed);
                BPW(&server.bp,  8,  (uint32_t)level_rank);
            }

            default:
                break;
        }
    }
}

static void unpack_events_bp(Packet* pkt, int* offset)
{
    uint8_t event_count = (uint8_t)bitpack_read(&client.bp,8);

    for(int i = 0; i < event_count; ++i)
    {
        NetEventType type = (NetEventType)bitpack_read(&client.bp,4);

        switch(type)
        {
            case EVENT_TYPE_PARTICLES:
            {
                uint8_t effect_index = (uint8_t)bitpack_read(&client.bp,5);
                float x              = (float)bitpack_read(&client.bp,10);
                float y              = (float)bitpack_read(&client.bp,10);
                float scale          = (float)(bitpack_read(&client.bp,8)/255.0f);
                uint32_t color1      = bitpack_read(&client.bp,32);
                uint32_t color2      = bitpack_read(&client.bp,32);
                uint32_t color3      = bitpack_read(&client.bp,32);
                float lifetime       = (float)(bitpack_read(&client.bp,8)/10.0f);
                uint8_t room_index   = (uint8_t)bitpack_read(&client.bp,7);

                if(room_index != player->curr_room)
                {
                    // printf("%u != %u\n", room_index, player->curr_room);
                    continue;
                }

                ParticleEffect effect = {0};
                memcpy(&effect, &particle_effects[effect_index], sizeof(ParticleEffect));

                effect.scale.init_min *= scale;
                effect.scale.init_max *= scale;

                effect.color1 = color1;
                effect.color2 = color2;
                effect.color3 = color3;

                ParticleSpawner* ps = particles_spawn_effect(x, y, 0.0, &effect, lifetime, true, false);
                if(ps != NULL) ps->userdata = (int)room_index;

            } break;

            case EVENT_TYPE_NEW_LEVEL:
            {
                level_seed = bitpack_read(&client.bp,32);
                level_rank = (uint8_t)bitpack_read(&client.bp,8);

                // printf("[event] new level\n");
                trigger_generate_level(level_seed, level_rank, 1, __LINE__);
            }

            default:
                break;
        }
    }
}
