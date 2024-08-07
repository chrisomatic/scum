#pragma once

#include "math2d.h"


#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*(_ARR))))

void imgui_begin(char* name, int x, int y);
void imgui_begin_panel(char* name, int x, int y, bool moveable);
Rect imgui_end(); // returns size of imgui area
bool imgui_clicked();
bool imgui_active();
bool imgui_is_mouse_inside();

// widgets
void imgui_text(char* text, ...);
void imgui_text_colored(uint32_t color, char* text, ...);
void imgui_text_sized(float pxsize, char* text, ...);
bool imgui_button(char* text, ...);
bool imgui_image_button(int img_index, int sprite_index, float scale, char* label, ...);
void imgui_toggle_button(bool* toggle, char* text, ...);
void imgui_checkbox(char* label, bool* result);
void imgui_color_picker(char* label, uint32_t* result);
void imgui_slider_float(char* label, float min, float max, float* result);
Vector2f imgui_number_box(char* label, int min, int max, int* result);
Vector2f imgui_number_box_formatted(char* label, int min, int max, char* format, int* result);
uint32_t imgui_text_box(char* label, char* buf, int bufsize);
uint32_t imgui_text_box_sized(char* label, char* buf, int bufsize, int width, int height);
void imgui_focus_text_box(uint32_t hash);
void imgui_set_button_select(int index, int num_buttons, char* button_labels[], char* label);
void imgui_button_select(int num_buttons, char* button_labels[], char* label, int* selection);
void imgui_dropdown(char* options[], int num_options, char* label, int* selected_index, bool* interacted);
bool imgui_listbox(char* options[], int num_options, char* label, int* selected_index, int max_display);
void imgui_tooltip(char* tooltip, ...);

Rect imgui_draw_demo(int x, int y); // for showcasing widgets

// theme
void imgui_theme_editor(); // for editing theme properties
void imgui_theme_selector();
bool imgui_load_theme(char* file_name);

void imgui_store_theme();
void imgui_restore_theme();

// properties
void imgui_set_text_size(float pxsize);
void imgui_set_text_color(uint32_t color);
void imgui_set_text_padding(int padding);
void imgui_set_spacing(int spacing);
void imgui_set_slider_width(int width);
void imgui_set_global_opacity_scale(float opacity_scale);

int imgui_get_text_cursor_index();
void imgui_set_text_cursor_indices(int i0, int i1);
void imgui_get_text_cursor_indices(int* i0, int* i1);
void imgui_text_cursor_inc(int val);

bool imgui_text_set_enter();
bool imgui_text_get_enter();

bool imgui_text_set_ctrl_enter();
bool imgui_text_get_ctrl_enter();

// formatting
void imgui_indent_begin(int indentpx);
void imgui_indent_end();
void imgui_newline();
void imgui_horizontal_line();
void imgui_horizontal_begin();
void imgui_horizontal_end();
void imgui_deselect_text_box();
void imgui_reset_cursor_blink();
