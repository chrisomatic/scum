#pragma once

#include "room_editor.h"

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
    int creature_locations_x[100];
    int creature_locations_y[100];
    uint8_t creature_orientations[100];
    int   creature_count;

    // items
    int   item_types[100];
    int item_locations_x[100];
    int item_locations_y[100];
    int   item_count;

    // doors
    bool doors[4];

    int file_index;
    time_t mtime;
    size_t fsize;

} RoomFileData;

extern char room_files[256][32];
extern char* p_room_files[256];
extern int  room_file_count;

// extern RoomFileData room_list[MAX_ROOM_LIST_COUNT];
extern RoomFileData room_list[200];
extern int room_list_count;

void room_file_save(RoomFileData* rfd, char* path, ...);
bool room_file_load(RoomFileData* rfd, bool force, bool print_errors, char* path, ...);
bool room_file_load_all(bool force);
bool room_file_get_all(bool force);
