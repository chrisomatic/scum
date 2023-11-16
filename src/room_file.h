#pragma once

#define FORMAT_VERSION 1

typedef struct
{
    // header info
    uint8_t version;
    int size_x, size_y;
    uint8_t type;
    uint8_t rank;
    
    // tile
    int tiles[32][32];

    // creatures
    int   creature_types[100];
    float creature_locations_x[100];
    float creature_locations_y[100];
    int   creature_count;

    // items
    int   item_types[100];
    float item_locations_x[100];
    float item_locations_y[100];
    int   item_count;

    // doors
    bool doors[4];

} RoomFileData;


extern char room_files[100][32];
extern char* p_room_files[100];
extern int  room_file_count;

void room_file_save(RoomFileData* rfd, char* path, ...);
bool room_file_load(RoomFileData* rfd, char* path, ...);
void room_file_get_all();
