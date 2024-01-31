#pragma once

#define UI_MESSAGE_MAX 256

void ui_message_set_small(float duration, char* fmt, ...);
void ui_message_set_title(float duration, uint32_t color, char* fmt, ...);

void ui_update(float dt);
void ui_draw();
