#pragma once

#include "level.h"


// +2 for walls
#define OBJECTS_MAX_X (ROOM_TILE_SIZE_X+2)
#define OBJECTS_MAX_Y (ROOM_TILE_SIZE_Y+2)

void room_editor_init();
void room_editor_start();
bool room_editor_update(float dt);
void room_editor_draw();
