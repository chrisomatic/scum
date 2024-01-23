#pragma once

#define DEFAULT_SETTINGS "scum.settings"
#define PLAYER_NAME_MAX 16

typedef struct
{
    char name[PLAYER_NAME_MAX+1];
    uint8_t class;
    uint32_t color;
} Settings;

bool settings_save(char* filename, Settings* s);
bool settings_load(char* filename, Settings* s);
