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
#include "glist.h"
#include "decal.h"

#define COOL_SERVER_PLAYER_LOGIC 1
#define REDUNDANT_INPUTS 1


#define ADDR_FMT "%u.%u.%u.%u:%u"
#define ADDR_LST(addr) (addr)->a,(addr)->b,(addr)->c,(addr)->d,(addr)->port

#define SERVER_PRINT_SIMPLE 1
// #define SERVER_PRINT_VERBOSE 1

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
#define TIMESTAMP_HISTORY_MAX 32
#define MAX_NET_EVENTS 255

typedef struct
{
    uint16_t id;
    double time;
} PacketTimestamp;

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

    glist* timestamp_history_list;
    PacketTimestamp timestamp_history[TIMESTAMP_HISTORY_MAX*MAX_CLIENTS];

} ClientInfo;

struct
{
    Address address;
    NodeInfo info;
    ClientInfo clients[MAX_CLIENTS];
    NetEvent events[MAX_NET_EVENTS];
    BitPack bp;
    double start_time;
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
    NetPlayerInput net_player_inputs[INPUT_QUEUE_MAX];
    CircBuf client_moves;
    BitPack bp;

    double time_of_connection;
    double time_of_latest_sent_packet;
    double time_of_last_ping;
    double time_of_last_received_ping;
    double server_connect_time;

    glist* timestamp_history_list;
    PacketTimestamp timestamp_history[TIMESTAMP_HISTORY_MAX];

    Packet state_packets[2]; // for jitter compensation
    int state_packet_count;

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
static inline void pack_double(Packet* pkt, double d);
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
static inline double   unpack_double(Packet* pkt, int* offset);
static inline void unpack_bytes(Packet* pkt, uint8_t* d, int len, int* offset);
static inline uint8_t unpack_string(Packet* pkt, char* s, int maxlen, int* offset);
static inline Vector2f unpack_vec2(Packet* pkt, int* offset);
static inline Vector3f unpack_vec3(Packet* pkt, int* offset);
static inline ItemType unpack_itemtype(Packet* pkt, int* offset);

static void pack_players(Packet* pkt, ClientInfo* cli); // temp
static void unpack_players(Packet* pkt, int* offset, WorldState* ws);
static void pack_creatures(Packet* pkt, ClientInfo* cli);
static void unpack_creatures(Packet* pkt, int* offset, WorldState* ws);
static void pack_projectiles(Packet* pkt, ClientInfo* cli);
static void unpack_projectiles(Packet* pkt, int* offset, WorldState* ws);
static void pack_items(Packet* pkt, ClientInfo* cli);
static void unpack_items(Packet* pkt, int* offset, WorldState* ws);
static void pack_decals(Packet* pkt, ClientInfo* cli);
static void unpack_decals(Packet* pkt, int* offset, WorldState* ws);
static void pack_other(Packet* pkt, ClientInfo* cli);
static void unpack_other(Packet* pkt, int* offset, WorldState* ws);
static void pack_events(Packet* pkt, ClientInfo* cli);
static void unpack_events(Packet* pkt, int* offset, WorldState* ws);

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
    // LOGN("[ADDR] %u.%u.%u.%u:%u",addr->a,addr->b,addr->c,addr->d,addr->port);
    LOGN("[ADDR] " ADDR_FMT, ADDR_LST(addr));
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

static int net_send(NodeInfo* node_info, Address* to, Packet* pkt, int count)
{
    int pkt_len = get_packet_size(pkt);
    int sent_bytes = 0;

    for(int i = 0; i < count; ++i)
        sent_bytes += socket_sendto(node_info->socket, to, (uint8_t*)pkt, pkt_len);

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
        LOGNV("[%d] addr: %s, name: %s", i, BOOLSTR(addr_check), BOOLSTR(name_check));
        LOGNV("    " ADDR_FMT " | " ADDR_FMT "", ADDR_LST(&server.clients[i].address), ADDR_LST(addr));
        LOGNV("    '%s' | '%s'", players[i].settings.name, name);

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
    glist* thl = cli->timestamp_history_list;
    thl->count = 0;

    // clear everything but address and pointers
    memset(cli,0, sizeof(ClientInfo));
    memcpy(&cli->address, &addr, sizeof(Address));
    cli->timestamp_history_list = thl;
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
            pack_double(&pkt,timer_get_time() - server.start_time); // client connect time
            net_send(&server.info,&cli->address,&pkt, 1);
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

            net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        case PACKET_TYPE_CONNECT_ACCEPTED:
        {
            cli->state = CONNECTED;
            pack_u8(&pkt, (uint8_t)cli->client_id);
            net_send(&server.info,&cli->address,&pkt, 1);

            server_send_message(TO_ALL, FROM_SERVER, "client added %u", cli->client_id);
        } break;

        case PACKET_TYPE_CONNECT_REJECTED:
        {
            pack_u8(&pkt, (uint8_t)cli->last_reject_reason);
            net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        case PACKET_TYPE_PING:
            pkt.data_len = 0;
            net_send(&server.info,&cli->address,&pkt, 1);
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

            net_send(&server.info, &cli->address, &pkt, 1);
        } break;

        case PACKET_TYPE_STATE:
        {

            bitpack_clear(&server.bp);

            pack_players(&pkt, cli);
            pack_creatures(&pkt, cli);
            pack_projectiles(&pkt, cli);
            pack_items(&pkt,cli);
            pack_decals(&pkt, cli);
            pack_events(&pkt,cli);
            pack_other(&pkt, cli);

            bitpack_flush(&server.bp);
            bitpack_seek_begin(&server.bp);
            //bitpack_print(&server.bp);

            int num_bytes = server.bp.words_written*4;
            pack_bytes(&pkt, (uint8_t*)server.bp.data, num_bytes);

            //print_packet(&pkt, true);

            if(memcmp(&cli->prior_state_pkt.data, &pkt.data, pkt.data_len) == 0)
                break;

            net_send(&server.info,&cli->address,&pkt, 1);

            // copy state packet to prior state packet
            memcpy(&cli->prior_state_pkt, &pkt, get_packet_size(&pkt));

        } break;

        case PACKET_TYPE_ERROR:
        {
            pack_u8(&pkt, (uint8_t)cli->last_packet_error);
            net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        case PACKET_TYPE_DISCONNECT:
        {
            cli->state = DISCONNECTED;
            pkt.data_len = 0;
            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
                net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        default:
            break;
    }
}

#define DBG() if(dbg) printf("%s(): %d\n", __func__, __LINE__)

static void server_simulate()
{
    static bool dbg = false;
    if(paused) return;

    if(level_generate_triggered)
    {
        game_generate_level();
        dbg = true;
        return;
    }

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
DBG();
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

DBG();
                player_update(p,cli->net_player_inputs[i].delta_t);
            }

            cli->input_count = 0;
        }
    }

DBG();
    level_update(dt);
DBG();
    projectile_update_all(dt);
DBG();
    creature_update_all(dt);
DBG();
    item_update_all(dt);
DBG();
    explosion_update_all(dt);
DBG();
    decal_update_all(dt);

DBG();
    entity_build_all();
DBG();
    entity_update_all(dt);
DBG();

    dbg = false;
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

    for(int i = 0; i < MAX_CLIENTS; ++i)
        server.clients[i].timestamp_history_list = list_create(server.clients[i].timestamp_history, TIMESTAMP_HISTORY_MAX,sizeof(PacketTimestamp), true);

    LOGN("Server Started with tick rate %f.", TICK_RATE);

    double t0=timer_get_time();
    double t1=0.0;
    double accum = 0.0;

    server.start_time = t0;

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

                    if(!cli->timestamp_history_list) printf("history list is null\n");
                    else cli->timestamp_history_list->count = 0;

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
                            if(ret == 1 || players[cli->client_id].phys.curr_room != players[i].phys.curr_room)
                            {
                                Vector2i tile = level_get_room_coords_by_pos(players[i].phys.pos.x, players[i].phys.pos.y);
                                player_send_to_room(&players[cli->client_id], players[i].phys.curr_room, true, tile);
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

#if 1
                bool is_latest = is_packet_id_greater(recv_pkt.hdr.id, cli->remote_latest_packet_id);
                if(!is_latest)
                {
                    LOGN("Not latest packet from client. Ignoring...");
                    //timer_delay_us(1000); // delay 1ms
                    break;
                }
#endif

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
#if REDUNDANT_INPUTS
                        // check if received input already
                        bool redundant = false;

                        for(int i = 0; i < cli->timestamp_history_list->count; ++i)
                        {
                            PacketTimestamp* pts = (PacketTimestamp*)list_get(cli->timestamp_history_list, i);
                            if(pts->id == recv_pkt.hdr.id)
                            {
                                redundant = true;
                                break;
                            }
                        }

                        if(redundant)
                            break;

                        // add to packet timestamp history
                        PacketTimestamp pts = {.id = recv_pkt.hdr.id, .time = timer_get_time()};
                        list_add(cli->timestamp_history_list, &pts);
#endif
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

                        bool is_cmd = false;
                        if(msg_len > 1)
                        {
                            if(memcmp("$", msg, 1) == 0)
                            {
                                is_cmd = true;
                                char* argv[20] = {0};
                                int argc = 0;

                                for(int i = 0; i < 20; ++i)
                                {
                                    char* s = string_split_index_copy(msg+1, " ", i, true);
                                    if(!s) break;
                                    argv[argc++] = s;
                                }

                                bool ret = server_process_command(argv, argc, cli->client_id);

                                if(!ret)
                                {
                                    server_send_message(from, FROM_SERVER, "Invalid command or command syntax");
                                }
                                else
                                {
                                    // server_send_message(from, FROM_SERVER, "%s has entered a command!", players[cli->client_id].settings.name);
                                }

                                // free
                                for(int i = 0; i < argc; ++i)
                                {
                                    free(argv[i]);
                                }

                            }
                        }

                        if(!is_cmd)
                        {
                            server_send_message(TO_ALL, from, "%s", msg);
                        }
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
        double _dt = 1.0/TARGET_FPS;
        while(accum_g >= _dt)
        {
            g_timer += _dt;
            server_simulate();
            accum_g -= _dt;
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

        timer_delay_us(1000); // 1ms delay to prevent cpu % from going nuts
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

    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int size = vsnprintf(NULL, 0, fmt, args);
    char* msg = calloc(size+1, sizeof(char));
    if(!msg) return;
    vsnprintf(msg, size+1, fmt, args2);
    va_end(args);
    va_end(args2);

    if(role == ROLE_LOCAL)
    {
        // text_list_add(text_lst, player->settings.color, 10.0, "%s: %s", player->settings.name, chat_text);
        text_list_add(text_lst, COLOR_WHITE, 10.0, "Server: %s", msg);
    }

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
            net_send(&server.info,&cli->address,&pkt, 1);
        }
    }
    else
    {
        if(to > MAX_PLAYERS) return;
        ClientInfo* cli = &server.clients[to];
        if(cli == NULL) return;
        if(cli->state != CONNECTED) return;

        pkt.hdr.ack = cli->remote_latest_packet_id;
        net_send(&server.info,&cli->address,&pkt, 1);
    }
}

/*

add xp <amount> <target>
    $add xp 100 all
    $add xp 20 3

add hp <amount> <target>
    $add hp 2 all
    $add hp 4 2

$kill creatures

$kill player <target>

$get id

*/

bool server_process_command(char* argv[20], int argc, int client_id)
{
    if(argc <= 0) return false;

    for(int i = 0; i < argc; ++i)
    {
        printf("  %d: '%s'\n", i, argv[i]);
    }

    bool err = false;
    if(STR_EQUAL(argv[0], "pause"))
    {
        paused = !paused;
    }
    else if(STR_EQUAL(argv[0], "add"))
    {
        // $add hp 6 all
        if(argc != 4) return false;
        int o = 1;

        char* add_str = argv[o++];
        int add_type = 0;
        if(STR_EQUAL(add_str, "xp"))
        {
            add_type = 0;
        }
        else if(STR_EQUAL(add_str, "hp"))
        {
            add_type = 1;
        }
        else
        {
            return false;
        }

        int val = atoi(argv[o++]);
        if(val <= 0) return false;

        char* t = argv[o++];
        if(STR_EQUAL(t, "all"))
        {
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(!players[i].active) continue;
                if(add_type == 0)
                    player_add_xp(&players[i], val);
                else if(add_type == 1)
                    player_add_hp(&players[i], val);
            }
        }
        else
        {
            int idx = atoi(t);
            if(idx < 0 || idx >= MAX_CLIENTS) return false;
            if(players[idx].active)
            {
                if(add_type == 0)
                    player_add_xp(&players[idx], val);
                else if(add_type == 1)
                    player_add_hp(&players[idx], val);
            }
        }
    }
    else if(STR_EQUAL(argv[0], "kill"))
    {
        // $kill creatures all

        if(argc < 2) return false;

        int o = 1;
        char* kill_str = argv[o++];
        int kill_type = 0;

        if(STR_EQUAL(kill_str, "creatures"))
        {
            kill_type = 0;
        }
        else if(STR_EQUAL(kill_str, "player"))
        {
            kill_type = 1;
        }
        else
        {
            return false;
        }

        if(kill_type == 0)
        {
            creature_kill_room(players[client_id].phys.curr_room);
        }
        else if(kill_type == 1)
        {
            if(argc != 3) return false;

            char* t = argv[o++];
            if(STR_EQUAL(t, "all"))
            {
                for(int i = 0; i < MAX_CLIENTS; ++i)
                {
                    if(!players[i].active) continue;
                    if(players[i].phys.dead) continue;
                    player_die(&players[i]);
                }
            }
            else
            {
                int idx = atoi(t);
                if(idx < 0 || idx >= MAX_CLIENTS) return false;
                if(players[idx].active && !players[idx].phys.dead)
                {
                    player_die(&players[idx]);
                }
            }

        }


    }
    else if(STR_EQUAL(argv[0], "get"))
    {
        if(argc < 2) return false;

        int o = 1;
        char* get_str = argv[o++];
        int get_type = 0;

        if(STR_EQUAL(get_str, "id"))
        {
            get_type = 0;
        }
        else
        {
            return false;
        }

        if(get_type == 0)
        {
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                Player* p = &players[i];
                if(p->active)
                {
                    server_send_message(TO_ALL, FROM_SERVER, "[%s] id: %d", p->settings.name, i);
                }
            }
        }
    }
    else if(STR_EQUAL(argv[0], "level"))
    {
        if(argc != 3) return false;

        int _seed = atoi(argv[1]);
        int _rank = atoi(argv[1]);
        trigger_generate_level(_seed, _rank, 2, __LINE__);
        if(role == ROLE_SERVER)
        {
            NetEvent ev = {.type = EVENT_TYPE_NEW_LEVEL};
            net_server_add_event(&ev);
        }
    }
    else if(STR_EQUAL(argv[0], "stats"))
    {
        // $stats max
        // $stats reset

        if(argc != 2) return false;

        if(STR_EQUAL(argv[1], "max"))
        {
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(server.clients[i].state != DISCONNECTED)
                {
                    for(int j = 0; j < MAX_STAT_TYPE; ++j)
                    {
                        player_add_stat(&players[i], j, 100);
                    }
                }
            }
        }
        else if(STR_EQUAL(argv[1], "reset"))
        {
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(server.clients[i].state != DISCONNECTED)
                {
                    for(int j = 0; j < MAX_STAT_TYPE; ++j)
                    {
                        players[i].stats[j] = 1;
                    }
                }
            }
        }

    }
    else
    {
        return false;
    }

    return true;
}

// =========
// @CLIENT
// =========

bool net_client_add_player_input(NetPlayerInput* input, WorldState* state)
{
    if(input_count >= INPUT_QUEUE_MAX)
    {
        LOGW("Input array is full!");
        return false;
    }

    memcpy(&client.net_player_inputs[input_count], input, sizeof(NetPlayerInput));

    if(state)
    {
        ClientMove m = {0};

        m.id            = client.info.local_latest_packet_id + input_count;
        m.input.delta_t = input->delta_t;
        m.input.keys    = input->keys;

        memcpy(&m.state, state, sizeof(WorldState));

        circbuf_add(&client.client_moves,&m);
    }

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

double net_client_get_server_time()
{
    return (timer_get_time() - client.server_connect_time);
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
    circbuf_create(&client.client_moves, 32, sizeof(ClientMove));

    client.timestamp_history_list = list_create(client.timestamp_history, TIMESTAMP_HISTORY_MAX,sizeof(PacketTimestamp), true);

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
    circbuf_clear_items(&client.client_moves);
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

    PacketTimestamp pts = {.id = pkt.hdr.id, .time = timer_get_time()};
    list_add(client.timestamp_history_list, &pts);

    LOGNV("%s() : %s", __func__, packet_type_to_str(type));

    switch(type)
    {
        case PACKET_TYPE_CONNECT_REQUEST:
        {
            uint64_t salt = rand64();
            memcpy(client.client_salt, (uint8_t*)&salt,8);

            pack_bytes(&pkt, (uint8_t*)client.client_salt, 8);
            // pack_string(&pkt, (uint8_t*)client.client_salt, 8);
            // printf("packing name: '%s'\n", player->settings.name);
            pack_string(&pkt, player->settings.name, PLAYER_NAME_MAX);
            pkt.data_len = 1024; // pad to 1024

            net_send(&client.info,&server.address,&pkt, 1);
        } break;

        case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
        {
            store_xor_salts(client.client_salt, client.server_salt, client.xor_salts);

            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            pkt.data_len = 1024; // pad to 1024

            net_send(&client.info,&server.address,&pkt, 1);
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

            net_send(&client.info,&server.address,&pkt, 1);
        } break;

        case PACKET_TYPE_PING:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            net_send(&client.info,&server.address,&pkt, 1);
        } break;

        case PACKET_TYPE_INPUT:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);
            pack_u8(&pkt, input_count);
            for(int i = 0; i < input_count; ++i)
            {
                pack_bytes(&pkt, (uint8_t*)&client.net_player_inputs[i], sizeof(NetPlayerInput));
            }

#if REDUNDANT_INPUTS
            net_send(&client.info,&server.address,&pkt, 3);
#else
            net_send(&client.info,&server.address,&pkt, 1);
#endif
        } break;

        case PACKET_TYPE_DISCONNECT:
        {
            pack_bytes(&pkt, (uint8_t*)client.xor_salts, 8);

            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
            {
                net_send(&client.info,&server.address,&pkt, 1);
                pkt.hdr.id = client.info.local_latest_packet_id;
            }
        } break;

        default:
            break;
    }

    client.time_of_latest_sent_packet = timer_get_time();
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
                        client.state = DISCONNECTED;
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
    for(;;)
    {
        bool data_waiting = _client_data_waiting(); // non-blocking
        if(!data_waiting)
            break;

        Packet srvpkt = {0};

        int recv_bytes = net_client_recv(&srvpkt);

        if(recv_bytes == 0)
            break;

        for(int i = 0; i < client.timestamp_history_list->count; ++i)
        {
            PacketTimestamp* pts = (PacketTimestamp*)list_get(client.timestamp_history_list, i);

            if(pts->id == srvpkt.hdr.ack)
            {
                double rtt = timer_get_time() - pts->time;

                if(client.rtt == 0.0)
                    client.rtt = rtt;
                else
                    client.rtt += 0.10*(rtt - client.rtt); // exponential smoothing

                list_remove(client.timestamp_history_list, i);
                break;
            }
        }

        int offset = 0;

        switch(srvpkt.hdr.type)
        {
            case PACKET_TYPE_INIT:
            {
                int seed = unpack_u32(&srvpkt,&offset);
                uint8_t rank = unpack_u8(&srvpkt,&offset);
                client.server_connect_time = unpack_double(&srvpkt,&offset);

                printf("Server start time: %.3f\n", client.server_connect_time);

                trigger_generate_level(seed,rank,0, __LINE__);

                client.received_init_packet = true;
            } break;
            case PACKET_TYPE_STATE:
            {
                bool is_latest = is_packet_id_greater(srvpkt.hdr.id, client.info.remote_latest_packet_id);
                if(!is_latest)
                    break;

                // copy received state into small queue
                if(client.state_packet_count == 0)
                    memcpy(&client.state_packets[0], &srvpkt, sizeof(Packet));
                else if(client.state_packet_count == 1)
                    memcpy(&client.state_packets[1], &srvpkt, sizeof(Packet));
                else
                {
                    memcpy(&client.state_packets[0], &client.state_packets[1], sizeof(Packet)); // move old state to index 0
                    memcpy(&client.state_packets[1], &srvpkt, sizeof(Packet));
                }

                if(client.state_packet_count < 2)
                    client.state_packet_count++;

                //print_packet(&srvpkt, true);

            } break;

            case PACKET_TYPE_PING:
            {
                client.time_of_last_received_ping = timer_get_time();
                //client.rtt = 1000.0f*(client.time_of_last_received_ping - client.time_of_last_ping);
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

    // apply state to client
    if(client.state_packet_count > 0)
    {
        int offset = 0;

        Packet* pkt = &client.state_packets[client.state_packet_count-1];

        // apply state to client
        bitpack_clear(&client.bp);
        bitpack_memcpy(&client.bp, &pkt->data[offset], pkt->data_len);
        bitpack_seek_begin(&client.bp);
        //bitpack_print(&client.bp);

        WorldState world_state = {0};

        unpack_players(pkt, &offset, &world_state);
        unpack_creatures(pkt, &offset, &world_state);
        unpack_projectiles(pkt, &offset, &world_state);
        unpack_items(pkt, &offset, &world_state);
        unpack_decals(pkt, &offset, &world_state);
        unpack_events(pkt,&offset, &world_state);
        unpack_other(pkt, &offset, &world_state);

        //apply_world_state_to_game(&world_state);

        int bytes_read = 4*(client.bp.word_index+1);
        offset += bytes_read;

        client.player_count = player_get_active_count();

        client.state_packet_count--;
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
    return (1000.0*client.rtt);
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

    net_send(&client.info,&server.address,&pkt, 1);
}


int net_client_send(uint8_t* data, uint32_t len)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = client.info.local_latest_packet_id,
    };

    memcpy(pkt.data,data,len);
    pkt.data_len = len;

    int sent_bytes = net_send(&client.info, &server.address, &pkt, 1);
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

static inline void pack_double(Packet* pkt, double d)
{
    memcpy(&pkt->data[pkt->data_len],&d,sizeof(double));
    pkt->data_len+=sizeof(double);
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

static inline double unpack_double(Packet* pkt, int* offset)
{
    double r;
    memcpy(&r, &pkt->data[*offset], sizeof(double));
    (*offset) += sizeof(double);
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

static void pack_players(Packet* pkt, ClientInfo* cli)
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
            BPW(&server.bp, 7,  (uint32_t)p->phys.curr_room);
            BPW(&server.bp, 8,  (uint32_t)p->phys.hp);
            BPW(&server.bp, 8,  (uint32_t)p->phys.hp_max);
            BPW(&server.bp, 8,  (uint32_t)p->coins);
            BPW(&server.bp, 4,  (uint32_t)p->revives);
            BPW(&server.bp, 16, (uint32_t)(p->highlighted_item_id+1));

            // BPW(&server.bp, 12, (uint32_t)(p->xp));
            // BPW(&server.bp, 5, (uint32_t)(p->level));
            // BPW(&server.bp, 5, (uint32_t)(p->new_levels));

            BPW(&server.bp, 1, (uint32_t)(p->invulnerable_temp ? 1 : 0));
            BPW(&server.bp, 6, (uint32_t)p->invulnerable_temp_time);
            BPW(&server.bp, 4, (uint32_t)p->door);
            BPW(&server.bp, 1, (uint32_t)(p->phys.dead ? 1 : 0));

            BPW(&server.bp, 2,  (uint32_t)p->weapon.state);
            if(p->weapon.state > WEAPON_STATE_NONE)
            {
                //BPW(&server.bp, 10,  (uint32_t)p->weapon.pos.x);
                //BPW(&server.bp, 10,  (uint32_t)p->weapon.pos.y);
                //BPW(&server.bp, 6,   (uint32_t)p->weapon.pos.z);
                BPW(&server.bp, 8,   (uint32_t)(p->weapon.scale * 255.0));

                uint32_t dir = 0;
                if(p->weapon.rotation_deg == 0.0)
                    dir = 1;
                else if(p->weapon.rotation_deg == 270.0)
                    dir = 2;
                else if(p->weapon.rotation_deg == 180.0)
                    dir = 3;

                BPW(&server.bp, 2, dir);
            }
        }
    }
}

static void unpack_players(Packet* pkt, int* offset, WorldState* ws)
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
        uint32_t hp                  = bitpack_read(&client.bp, 8);
        uint32_t hp_max              = bitpack_read(&client.bp, 8);
        uint32_t coins               = bitpack_read(&client.bp, 8);
        uint32_t revives             = bitpack_read(&client.bp, 4);
        uint32_t highlighted_item_id = bitpack_read(&client.bp, 16);

        // uint32_t xp                  = bitpack_read(&client.bp, 12);
        // uint32_t level               = bitpack_read(&client.bp, 5);
        // uint32_t new_levels          = bitpack_read(&client.bp, 5);

        uint32_t invunerable_temp = bitpack_read(&client.bp, 1);
        uint32_t inv_temp_time    = bitpack_read(&client.bp, 6);
        uint32_t door             = bitpack_read(&client.bp, 4);
        uint32_t dead             = bitpack_read(&client.bp, 1);

        uint32_t weapon_state = bitpack_read(&client.bp, 2);

        //uint32_t weapon_x = 0.0;
        //uint32_t weapon_y = 0.0;
        //uint32_t weapon_z = 0.0;
        uint32_t weapon_scale = 0.0;
        uint32_t weapon_dir = 0;

        if(weapon_state > WEAPON_STATE_NONE)
        {
            //weapon_x = bitpack_read(&client.bp, 10);
            //weapon_y = bitpack_read(&client.bp, 10);
            //weapon_z = bitpack_read(&client.bp, 6);
            weapon_scale = bitpack_read(&client.bp, 8);
            weapon_dir = bitpack_read(&client.bp, 2);
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
        p->coins  = (uint16_t)coins;
        p->revives  = (uint8_t)revives;

        p->highlighted_item_id = ((int32_t)highlighted_item_id)-1;

        // p->xp = (uint16_t)xp;
        // p->level = (uint8_t)level;
        // p->new_levels = (uint8_t)new_levels;

        p->invulnerable_temp = invunerable_temp == 1 ? true : false;
        float invulnerable_temp_time = (float)inv_temp_time;
        p->door  = (Dir)door;
        p->phys.dead  = dead == 1 ? true: false;

        p->weapon.state = (WeaponState)weapon_state;
        //p->weapon.pos.x = (float)weapon_x;
        //p->weapon.pos.y = (float)weapon_y;
        //p->weapon.pos.z = (float)weapon_z;
        p->weapon.color = COLOR_TINT_NONE;

        switch(weapon_dir)
        {
            case 0: p->weapon.rotation_deg =  90.0; break;
            case 1: p->weapon.rotation_deg =   0.0; break;
            case 2: p->weapon.rotation_deg = 270.0; break;
            case 3: p->weapon.rotation_deg = 180.0; break;
        }

        if(!prior_active[client_id])
        {
            p->phys.curr_room = curr_room;
            p->transition_room = p->phys.curr_room;
        }

        uint8_t prior_curr_room = p->phys.curr_room;

        // moving between rooms
        if(curr_room != p->phys.curr_room)
        {
            if(p == player)
            {
                p->transition_room = p->phys.curr_room;
                p->phys.curr_room = curr_room;

                if(level_transition_state == 0 && !level_generate_triggered)
                {
                    player_start_room_transition(p);
                }
                else
                {
                    p->transition_room = p->phys.curr_room;
                }

            }
            else
            {
                p->transition_room = curr_room;
                p->phys.curr_room = curr_room;
            }
            // avoid lerping
            p->phys.pos.x = pos.x;
            p->phys.pos.y = pos.y;
            p->phys.pos.z = pos.z;
            p->weapon.scale = (float)(weapon_scale/255.0f);
        }

        if(p == player)
            visible_room = level_get_room_by_index(&level, p->phys.curr_room);

        for(int i = 0; i < 32; ++i)
        {
            ClientMove* move = (ClientMove*)circbuf_get_item(&client.client_moves, i);

            //if(move->time > pkt->hdr.time && i > 0)
            if(move->id != pkt->hdr.ack)
            {
                //move = (ClientMove*)circbuf_get_item(&client.client_moves, i-1);
                //printf("Found move! (time %f, server %f). Client Pos: %f %f %f, Server Pos: %f %f %f\n", move->time, pkt->hdr.time, p->phys.pos.x, p->phys.pos.y, p->phys.pos.z, pos.x, pos.y, pos.z);
                break;
            }
        }

        p->lerp_t = 0.0;

        p->server_state_prior.pos.x = p->phys.pos.x;
        p->server_state_prior.pos.y = p->phys.pos.y;
        p->server_state_prior.pos.z = p->phys.pos.z;
        p->server_state_prior.weapon_scale = p->weapon.scale;
        p->server_state_prior.invulnerable_temp_time = p->invulnerable_temp_time;

        p->server_state_target.pos.x = pos.x;
        p->server_state_target.pos.y = pos.y;
        p->server_state_target.pos.z = pos.z;
        p->server_state_target.weapon_scale = (float)(weapon_scale/255.0f);
        p->server_state_target.invulnerable_temp_time = invulnerable_temp_time;

        if(!prior_active[client_id])
        {
            printf("first state packet for %d\n", client_id);
            memcpy(&p->server_state_prior, &p->server_state_target, sizeof(p->server_state_target));
            p->transition_room = p->phys.curr_room;
        }

        //player_print(p);
    }
}

static void pack_creatures(Packet* pkt, ClientInfo* cli)
{
    uint16_t num_creatures = creature_get_count();
    uint16_t num_visible_creatures = 0;

    for(int i = 0; i < num_creatures; ++i)
    {
        if(!is_any_player_room(creatures[i].phys.curr_room))
            continue;

        num_visible_creatures++;
    }

    BPW(&server.bp, 8,  (uint32_t)num_visible_creatures);

    for(int i = 0; i < num_creatures; ++i)
    {
        Creature* c = &creatures[i];

        if(!is_any_player_room(c->phys.curr_room))
            continue;

        uint8_t r = (c->color >> 16) & 0xFF;
        uint8_t g = (c->color >>  8) & 0xFF;
        uint8_t b = (c->color >>  0) & 0xFF;

        BPW(&server.bp, 16, (uint32_t)c->id);
        BPW(&server.bp, 6,  (uint32_t)c->type);
        BPW(&server.bp, 10, (uint32_t)c->phys.pos.x);
        BPW(&server.bp, 10, (uint32_t)c->phys.pos.y);
        BPW(&server.bp, 9,  (uint32_t)c->phys.pos.z);
        BPW(&server.bp, 8,  (uint32_t)c->phys.width);
        BPW(&server.bp, 6,  (uint32_t)c->sprite_index);
        BPW(&server.bp, 7,  (uint32_t)c->phys.curr_room);
        BPW(&server.bp, 8,  (uint32_t)c->phys.hp);
        BPW(&server.bp, 1,  (uint32_t)(c->phys.underground ? 0x01 : 0x00));

        BPW(&server.bp, 8,  (uint32_t)r);
        BPW(&server.bp, 8,  (uint32_t)g);
        BPW(&server.bp, 8,  (uint32_t)b);

        BPW(&server.bp, 1,  (uint32_t)c->segmented ? 0x01 : 0x00);
        if(c->segmented)
        {
            BPW(&server.bp, 3, (uint32_t)creature_segment_count);

            for(int i = 0; i < creature_segment_count; ++i)
            {
                CreatureSegment* cs = &creature_segments[i];

                BPW(&server.bp, 4, (uint32_t)cs->curr_tile.x);
                BPW(&server.bp, 4, (uint32_t)cs->curr_tile.y);
                BPW(&server.bp, 3, (uint32_t)cs->dir);
                BPW(&server.bp, 1, (uint32_t)(cs->tail ? 0x01 : 0x00));
            }
        }
    }
}

static void unpack_creatures(Packet* pkt, int* offset, WorldState* ws)
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
        uint32_t z            = bitpack_read(&client.bp, 9);
        uint32_t width        = bitpack_read(&client.bp, 8);
        uint32_t sprite_index = bitpack_read(&client.bp, 6);
        uint32_t curr_room    = bitpack_read(&client.bp, 7);
        uint32_t hp           = bitpack_read(&client.bp, 8);
        uint32_t underground  = bitpack_read(&client.bp, 1);
        uint32_t r            = bitpack_read(&client.bp, 8);
        uint32_t g            = bitpack_read(&client.bp, 8);
        uint32_t b            = bitpack_read(&client.bp, 8);
        uint32_t segmented    = bitpack_read(&client.bp, 1);

        if(segmented > 0x00)
        {
            uint32_t num_segments = bitpack_read(&client.bp, 3);

            creature_segment_count = 0;

            for(int j = 0; j < num_segments; ++j)
            {
                uint32_t seg_tile_x = bitpack_read(&client.bp, 4);
                uint32_t seg_tile_y = bitpack_read(&client.bp, 4);
                uint32_t seg_dir = bitpack_read(&client.bp, 3);
                uint32_t seg_tail = bitpack_read(&client.bp, 1);

                CreatureSegment* cs = &creature_segments[creature_segment_count++];

                cs->curr_tile.x = seg_tile_x;
                cs->curr_tile.y = seg_tile_y;
                cs->dir = (Dir)seg_dir;
                cs->tail = (seg_tail == 0x00 ? false : true);
            }
        }

        Creature creature = {0};
        creature.id = (uint16_t)id;
        creature.type = (CreatureType)type;
        creature.phys.pos.x = (float)x;
        creature.phys.pos.y = (float)y;
        creature.phys.pos.z = (float)z;
        creature.phys.width = (float)width;
        creature.phys.collision_rect.w = (float)width;
        creature.sprite_index = (uint8_t)sprite_index;
        creature.phys.curr_room = (uint8_t)curr_room;
        creature.phys.hp = (int8_t)hp;
        creature.color = COLOR(r,g,b);

        Creature* c = creature_add(NULL, 0, NULL, &creature);
        if(!c) continue;

        c->phys.underground = (bool)(underground == 0x01);

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

static void pack_projectiles(Packet* pkt, ClientInfo* cli)
{
    uint16_t num_projectiles = (uint16_t)plist->count;
    uint16_t num_visible_projectiles = 0;

    for(int i = 0; i < num_projectiles; ++i)
    {
        if(!is_any_player_room(projectiles[i].phys.curr_room))
            continue;

        num_visible_projectiles++;
    }

    BPW(&server.bp, 12,  (uint32_t)num_visible_projectiles);

    for(int i = 0; i < num_projectiles; ++i)
    {
        Projectile* p = &projectiles[i];

        if(!is_any_player_room(p->phys.curr_room))
            continue;

        uint8_t r = (p->color >> 16) & 0xFF;
        uint8_t g = (p->color >>  8) & 0xFF;
        uint8_t b = (p->color >>  0) & 0xFF;

        if(p->phys.rotation_deg < 0.0) p->phys.rotation_deg = 0.0;
        else if(p->phys.rotation_deg > 360.0) fmod(p->phys.rotation_deg, 360.0);

        BPW(&server.bp, 16, (uint32_t)p->id);
        BPW(&server.bp, 5,  (uint32_t)p->sprite_index);
        BPW(&server.bp, 10, (uint32_t)p->phys.pos.x);
        BPW(&server.bp, 10, (uint32_t)p->phys.pos.y);
        BPW(&server.bp, 9,  (uint32_t)p->phys.pos.z);
        BPW(&server.bp, 8,  (uint32_t)r);
        BPW(&server.bp, 8,  (uint32_t)g);
        BPW(&server.bp, 8,  (uint32_t)b);
        BPW(&server.bp, 3,  (uint32_t)(p->player_id));
        BPW(&server.bp, 7,  (uint32_t)(p->phys.curr_room));
        BPW(&server.bp, 10,  (uint32_t)(p->phys.scale*255.0f));
        BPW(&server.bp, 10,  (uint32_t)(p->phys.rotation_deg));
        BPW(&server.bp, 1,  (uint32_t)(p->from_player ? 0x01 : 0x00));
    }
}

static void unpack_projectiles(Packet* pkt, int* offset, WorldState* ws)
{
    memcpy(prior_projectiles, projectiles, sizeof(Projectile)*MAX_PROJECTILES);

    // load projectiles
    uint32_t num_projectiles = bitpack_read(&client.bp, 12);

    list_clear(plist);
    plist->count = num_projectiles;

    for(int i = 0; i < num_projectiles; ++i)
    {
        Projectile* p = &projectiles[i];

        uint32_t id           = bitpack_read(&client.bp, 16);
        uint32_t sprite_index = bitpack_read(&client.bp, 5);
        uint32_t x            = bitpack_read(&client.bp, 10);
        uint32_t y            = bitpack_read(&client.bp, 10);
        uint32_t z            = bitpack_read(&client.bp, 9);
        uint32_t r            = bitpack_read(&client.bp, 8);
        uint32_t g            = bitpack_read(&client.bp, 8);
        uint32_t b            = bitpack_read(&client.bp, 8);
        uint32_t pid          = bitpack_read(&client.bp, 3);
        uint32_t curr_room    = bitpack_read(&client.bp, 7);
        uint32_t scale        = bitpack_read(&client.bp, 10);
        uint32_t _angle       = bitpack_read(&client.bp, 10);
        uint32_t from_player  = bitpack_read(&client.bp, 1);

        float angle = (float)_angle;

        p->id = (uint16_t)id;
        p->sprite_index = (int)sprite_index;
        p->color = COLOR(r,g,b);
        p->phys.curr_room = (uint8_t)curr_room;
        p->phys.rotation_deg = angle;
        p->phys.scale = (float)(scale / 255.0f);
        p->from_player = from_player == 0x01 ? true : false;
        p->player_id = (uint8_t)pid;
        p->lerp_t = 0.0;

        p->server_state_prior.pos.x = (float)x;
        p->server_state_prior.pos.y = (float)y;
        p->server_state_prior.pos.z = (float)z;
        p->server_state_prior.angle = angle;

        //find the prior
        for(int j = i; j < MAX_PROJECTILES; ++j)
        {
            Projectile* pj = &prior_projectiles[j];
            if(pj->id == p->id)
            {
                p->server_state_prior.pos.x = pj->phys.pos.x;
                p->server_state_prior.pos.y = pj->phys.pos.y;
                p->server_state_prior.pos.z = pj->phys.pos.z;
                p->server_state_prior.angle = pj->phys.rotation_deg;
                break;
            }
        }

        p->server_state_target.pos.x = (float)x;
        p->server_state_target.pos.y = (float)y;
        p->server_state_target.pos.z = (float)z;
        p->server_state_target.angle = angle;
    }
}

static void pack_items(Packet* pkt, ClientInfo* cli)
{
    uint16_t num_items = (uint16_t)item_list->count;
    uint8_t num_visible_items = 0;

    for(int i = 0; i < num_items; ++i)
    {
        Item* it = &items[i];
        if(!is_any_player_room(it->phys.curr_room))
            continue;
        num_visible_items++;
    }

    BPW(&server.bp, 6,  (uint32_t)num_visible_items);

    for(int i = 0; i < num_items; ++i)
    {
        Item* it = &items[i];

        if(!is_any_player_room(it->phys.curr_room))
            continue;

        BPW(&server.bp, 16, (uint32_t)it->id);
        BPW(&server.bp, 6,  (uint32_t)(it->type + 1));
        BPW(&server.bp, 10, (uint32_t)it->phys.pos.x);
        BPW(&server.bp, 10, (uint32_t)it->phys.pos.y);
        BPW(&server.bp, 6,  (uint32_t)it->phys.pos.z);
        BPW(&server.bp, 8,  (uint32_t)(it->phys.scale*255.0f));
        BPW(&server.bp, 7,  (uint32_t)it->phys.curr_room);
        BPW(&server.bp, 10, (uint32_t)(it->angle));
        BPW(&server.bp, 1,  (uint32_t)(it->used ? 0x01 : 0x00));
        BPW(&server.bp, 8,  (uint32_t)(it->user_data));
        BPW(&server.bp, 8,  (uint32_t)(it->user_data2));
        BPW(&server.bp, 32, (uint32_t)(it->seed));

        uint8_t dlen = strlen(it->desc);
        // printf("[%u] dlen: %u '%s'\n", it->id, dlen, it->desc);
        if(dlen > 0)
        {
            BPW(&server.bp, 1, 0x01);
            BPW(&server.bp, 8, dlen);
            for(int j = 0; j < dlen; ++j)
            {
                BPW(&server.bp, 8, (uint8_t)it->desc[j]);
            }
        }
        else
            BPW(&server.bp, 1, 0x00);

    }
}

static void unpack_items(Packet* pkt, int* offset, WorldState* ws)
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
        uint32_t _scale      = bitpack_read(&client.bp, 8);
        uint32_t c_room      = bitpack_read(&client.bp, 7);
        uint32_t _angle      = bitpack_read(&client.bp, 10);
        uint32_t used        = bitpack_read(&client.bp, 1);
        uint8_t user_data    = bitpack_read(&client.bp, 8);
        uint8_t user_data2   = bitpack_read(&client.bp, 8);
        uint32_t _seed       = bitpack_read(&client.bp, 32);
        bool has_desc        = bitpack_read(&client.bp, 1);

        if(has_desc)
        {
            char desc[32] = {0};
            uint8_t dlen = bitpack_read(&client.bp, 8);
            for(int j = 0; j < dlen; ++j)
            {
                desc[j] = (char)bitpack_read(&client.bp, 8);
            }
            memcpy(it->desc, desc, 32*sizeof(char));
            // printf("[%u] dlen: %u '%s'\n", id, dlen, desc);
        }

        it->id = id;
        it->type = (int32_t)type-1;
        it->phys.curr_room = (uint8_t)c_room;
        it->highlighted = false;
        it->used = used == 1 ? true : false;
        float angle = (float)_angle;
        Vector3f pos = {(float)x,(float)y,(float)z};
        it->phys.scale = (float)(_scale/255.0f);
        it->user_data = user_data;
        it->user_data2 = user_data2;
        it->seed = _seed;

        // printf("%u\n", it->id);
        // printf(" %d\n", it->type);
        // printf(" %u\n", it->phys.curr_room);
        // printf(" %d\n", it->used);

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

static void pack_decals(Packet* pkt, ClientInfo* cli)
{
    BPW(&server.bp, 7,  (uint32_t)decal_list->count);

    for(int i = 0; i < decal_list->count; ++i)
    {
        Decal* d = &decals[i];

        BPW(&server.bp, 7,  (uint32_t)d->sprite_index);
        BPW(&server.bp, 32, (uint32_t)d->tint);
        BPW(&server.bp, 8,  (uint32_t)(d->scale*255.0f));
        BPW(&server.bp, 9,  (uint32_t)(d->rotation));
        BPW(&server.bp, 8,  (uint32_t)(d->opacity*255.0f));
        BPW(&server.bp, 8,  (uint32_t)(d->ttl*10.0f));
        BPW(&server.bp, 10, (uint32_t)d->pos.x);
        BPW(&server.bp, 10, (uint32_t)d->pos.y);
        BPW(&server.bp, 7, (uint32_t)d->room);
        BPW(&server.bp, 2, (uint32_t)d->fade_pattern);
    }
}

static void unpack_decals(Packet* pkt, int* offset, WorldState* ws)
{
    uint8_t count = (uint8_t)bitpack_read(&client.bp, 7);

    for(int i = 0; i < count; ++i)
    {
        Decal d = {0};

        d.image = particles_image;

        d.sprite_index = (uint8_t)bitpack_read(&client.bp,7);
        d.tint         = (uint32_t)bitpack_read(&client.bp,32);
        d.scale        = (float)(bitpack_read(&client.bp,8)/255.0f);
        d.rotation     = (float)(bitpack_read(&client.bp,9));
        d.opacity      = (float)(bitpack_read(&client.bp,8)/255.0f);
        d.ttl          = (float)(bitpack_read(&client.bp,8)/10.0f);
        d.pos.x        = (float)bitpack_read(&client.bp,10);
        d.pos.y        = (float)bitpack_read(&client.bp,10);
        d.room         = (uint8_t)bitpack_read(&client.bp,7);
        d.fade_pattern = (uint8_t)bitpack_read(&client.bp,2);

        decal_add(d);
    }
}

static void pack_other(Packet* pkt, ClientInfo* cli)
{
    Room* room = level_get_room_by_index(&level, (int)players[cli->client_id].phys.curr_room);

    BPW(&server.bp, 1,  (uint32_t)(room->doors_locked ? 0x01 : 0x00));
    BPW(&server.bp, 1,  (uint32_t)(level.darkness_curse ? 0x01 : 0x00));
    BPW(&server.bp, 1,  (uint32_t)(paused ? 0x01 : 0x00));
    BPW(&server.bp, 1,  (uint32_t)((int)(g_timer) % 2 == 0 ? 0x01 : 0x00));
}

static void unpack_other(Packet* pkt, int* offset, WorldState* ws)
{
    Room* room = level_get_room_by_index(&level, (int)player->phys.curr_room);

    if(!room)
    {
        LOGW("room is null!");
        bitpack_read(&client.bp, 1);
    }
    else
    {
        room->doors_locked = bitpack_read(&client.bp, 1) == 0x01 ? true : false;
    }

    level.darkness_curse = bitpack_read(&client.bp, 1) == 0x01 ? true : false;

    paused = bitpack_read(&client.bp, 1) == 0x01 ? true : false;
    g_spikes = bitpack_read(&client.bp, 1) == 0x01 ? true : false;
}

static void pack_events(Packet* pkt, ClientInfo* cli)
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
            } break;

            case EVENT_TYPE_FLOOR_STATE:
            {
                BPW(&server.bp, 4,  (uint32_t)ev->data.floor_state.x);
                BPW(&server.bp, 4,  (uint32_t)ev->data.floor_state.y);
                BPW(&server.bp, 3,  (uint32_t)ev->data.floor_state.state);
            } break;

            default:
                break;
        }
    }
}

static void unpack_events(Packet* pkt, int* offset, WorldState* ws)
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

                if(room_index != player->phys.curr_room)
                {
                    // printf("%u != %u\n", room_index, player->phys.curr_room);
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
            } break;

            case EVENT_TYPE_FLOOR_STATE:
            {
                uint8_t x  = (uint8_t)bitpack_read(&client.bp,4);
                uint8_t y  = (uint8_t)bitpack_read(&client.bp,4);
                uint8_t st = (uint8_t)bitpack_read(&client.bp,3);

                visible_room->breakable_floor_state[x][y] = st;
            } break;

            default:
                break;
        }
    }
}


/*
void apply_world_state_to_game(WorldState* ws)
{
    // players
    for(int i = 0; i < ws->player_count; ++i)
    {
        _apply_world_state_to_player(ws, &players[i]);
    }



}
*/
