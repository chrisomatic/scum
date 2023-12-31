#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_PACKET_DATA_SIZE 32768
#define MAX_PACKET_SIZE MAX_PACKET_DATA_SIZE + 20

typedef struct
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint16_t port;
} Address;

bool socket_initialize();
void socket_shutdown();

bool socket_create(int* socket_handle);
bool socket_bind(int socket_handle, Address* address, uint16_t port);
void socket_close(int socket_handle);

int socket_sendto(int socket_handle, Address* address, uint8_t* pkt, uint32_t pkt_size);
int socket_recvfrom(int socket_handle, Address* address, uint8_t* pkt);
