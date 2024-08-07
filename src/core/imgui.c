#include "headers.h"
#include "math2d.h"
#include "gfx.h"
#include "window.h"
#include "hash.h"
#include "log.h"
#include "files.h"
#include "imgui.h"
#include <math.h>

#define NOMINAL_FONT_SIZE 64.0 // px for 1.0 scale
#define MAX_CONTEXTS 32
#define MAX_INT_LOOKUPS 256
#define MAX_FLOAT_LOOKUPS 32
#define MAX_HORIZONTAL_STACK 10
#define MAX_TOOLTIP_LEN 256
#define CURSOR_BLINK_TIME 0.5

#define DRAW_DEBUG_BOXES 0


typedef struct
{
    bool used;
    uint32_t hash;
    int val;
} IntLookup;

typedef struct
{
    bool used;
    uint32_t hash;
    float val;
} FloatLookup;

typedef struct
{
    bool text_click_held;

    int text_cursor_index_held_from;
    int text_cursor_index;

    float text_cursor_x_held_from;
    float text_cursor_x;

    bool cursor_show;
    double cursor_blink_time;
    double cursor_blink_t0;

    bool highlighted;
} TextBoxProps;

typedef struct
{
    uint32_t hash;
    float pos;
    float held_start_pos;
    bool scrollbar_held;
    bool in_gutter;
    float press_time;
} ScrollProps;

typedef struct
{
    uint32_t hash;
    int selected_index;
    char** options;
    int num_options;
    char label[32];
    float text_size_px;
    Rect size;
} DropdownProps;

typedef struct
{
    int panel_width;
    int panel_height;
    float offset_x, offset_y;
} PanelProps;

typedef struct
{
    uint32_t hash;

    uint32_t highlighted_id;
    uint32_t active_id;
    uint32_t focused_text_id;

    // formatting
    Rect curr;

    int start_x, start_y;
    int max_width;
    int accum_width;
    int horiontal_max_height;

    int indent_amount;

    bool horizontal_stack[MAX_HORIZONTAL_STACK];
    bool is_horizontal;

    IntLookup int_lookups[MAX_INT_LOOKUPS];
    FloatLookup float_lookups[MAX_FLOAT_LOOKUPS];

    float prior_mouse_x, prior_mouse_y;
    float mouse_x, mouse_y;

    float slider_mouse_x, slider_mouse_y; //starting coords for adjusting slider

    PanelProps panel_props;
    TextBoxProps text_box_props;
    ScrollProps scroll_props;

    bool has_tooltip;
    char tooltip[MAX_TOOLTIP_LEN+1];
    uint32_t tooltip_hash;

    DropdownProps dropdown_props;

    float global_opacity_scale;

    Rect prior_size;
} ImGuiContext;

#define IMGUI_THEME_VERSION 0
typedef struct
{
    uint32_t version;

    uint32_t color_text;
    uint32_t color_background;
    uint32_t color_highlight;
    uint32_t color_highlight_subtle;
    uint32_t color_active;
    uint32_t color_panel;
    uint32_t color_slider;

    int spacing;
    float text_size_px;
    float text_scale;
    int text_padding;
    float button_opacity;
    int button_min_width;
    int checkbox_size;
    float slider_width;
    float slider_handle_width;
    float slider_opacity;
    int number_box_width;
    int text_box_width;
    int text_box_height;
    float panel_opacity;
    int panel_min_width;
    int panel_spacing;
    int panel_header_height;

    int dropdown_max_height;

    int listbox_width;
    int listbox_height;
    int listbox_gutter_width;

} ImGuiTheme;

static ImGuiContext contexts[MAX_CONTEXTS] = {0};
static ImGuiContext default_context = {0};
static ImGuiTheme theme;
static ImGuiTheme stored_theme;

static ImGuiContext* ctx;
static bool theme_initialized = false;

static inline bool is_mouse_inside(int x, int y, int width, int height);
static inline bool is_highlighted(uint32_t hash);
static inline bool is_active(uint32_t hash);
static inline bool is_inside(Rect* r);
static inline void set_highlighted(uint32_t hash);
static inline void set_active(uint32_t hash);
static inline void clear_highlighted();
static inline void clear_active();

static void assign_context(uint32_t hash);
static void handle_highlighting(uint32_t hash, Rect* r);
static void handle_tooltip();
static bool dropdown_on_top(uint32_t hash, Rect* r);
static void imgui_slider_float_internal(char* label, float min, float max, float* result, char* format);
static IntLookup* get_int_lookup(uint32_t hash);
static FloatLookup* get_float_lookup(uint32_t hash);
static void progress_pos();
static void mask_off_hidden(char* label, char* new_label, int max_size);
static void set_default_theme();
static ImGuiContext* get_active_textbox_context();

static void draw_button(uint32_t hash, char* str, Rect* r);
static void draw_image_button(uint32_t hash, char* str, Rect* r, int img_index, int sprite_index,float scale);
static void draw_toggle_button(uint32_t hash, char* str, Rect* r, bool toggled);
static void draw_checkbox(uint32_t hash, char* label, bool result);
static void draw_slider(uint32_t hash, char* str, int slider_x, char* val_format, float val);
static void draw_color_box(Rect* r, uint32_t color);
static void draw_label(int x, int y, uint32_t color, char* label);
static void draw_number_box(uint32_t hash, char* label, Rect* r, int val, int max, char* format);
static void draw_text_box(uint32_t hash, char* label, Rect* r, char* text);
static void draw_dropdown(uint32_t hash, char* str, char* options[], int num_options, int selected_index, Rect* r);
static void draw_listbox(uint32_t hash, char* str, char* options[], int num_options, int selected_index, float scrollbar_offset, Rect* r, int max_display);
static void draw_panel(uint32_t hash, bool moveable);
static void draw_tooltip();

static bool _enter = false;
static bool _ctrl_enter = false;
static bool _editor_test = false;
static char _editor_text[20]= {0};
static char _editor_file_name[40] = {0};
static bool save_result = false;
static bool saved = false;
static int num_themes = 0;
static int theme_index = -1;
static char theme_files[32][32] = {0};
static char* theme_file_ptrs[32] = {0};
static void load_themes();

void imgui_begin(char* name, int x, int y)
{
    uint32_t hash = hash_str(name,strlen(name),0x0);
    assign_context(hash);

    ctx->start_x = x;
    ctx->start_y = y;

    ctx->curr.x = x;
    ctx->curr.y = y;

    ctx->indent_amount = 0;
    ctx->max_width = 0;
    ctx->accum_width = 0;
    ctx->horiontal_max_height = 0;

    if(ctx->global_opacity_scale == 0.0)
        ctx->global_opacity_scale = 1.0;

    memset(ctx->horizontal_stack,false,sizeof(bool)*MAX_HORIZONTAL_STACK);

    ctx->text_box_props.highlighted = false;
    ctx->has_tooltip = false;
    ctx->tooltip_hash = 0x0;

    if(!theme_initialized)
    {
        set_default_theme();
    }

    ctx->prior_mouse_x = ctx->mouse_x;
    ctx->prior_mouse_y = ctx->mouse_y;

    bool mouse_went_down = window_mouse_left_went_down();
    if(mouse_went_down)
    {
        ctx->slider_mouse_x = ctx->mouse_x;
        ctx->slider_mouse_y = ctx->mouse_y;
    }

    window_get_mouse_view_coords(&ctx->mouse_x, &ctx->mouse_y);
}

void imgui_begin_panel(char* name, int x, int y, bool moveable)
{
    imgui_begin(name, x,y);

    ctx->curr.x = ctx->start_x + ctx->panel_props.offset_x;
    ctx->curr.y = ctx->start_y + ctx->panel_props.offset_y;

    uint32_t hash = hash_str(name,strlen(name),0x0);

    if(moveable)
    {
        if(is_active(hash))
        {
            ctx->panel_props.offset_x += (ctx->mouse_x - ctx->prior_mouse_x);
            ctx->panel_props.offset_y += (ctx->mouse_y - ctx->prior_mouse_y);

            if(window_mouse_left_went_up())
                clear_active();
        }
        else if(is_highlighted(hash))
        {
            if(window_mouse_left_went_down())
            {
                set_active(hash);
            }
        }

        Rect interactive = {ctx->curr.x, ctx->curr.y, ctx->panel_props.panel_width, theme.panel_header_height};
        handle_highlighting(hash, &interactive);
    }

    draw_panel(hash, moveable);

    char new_label[256] = {0};
    mask_off_hidden(name, new_label, 256);
    ctx->curr.x += 8;
    imgui_text_sized(16,new_label);
    ctx->curr.x -= 8;

    ctx->curr.x += theme.panel_spacing;
    imgui_horizontal_line(1);
}


void imgui_set_text_size(float pxsize)
{
    theme.text_size_px = pxsize;
    theme.text_scale = theme.text_size_px / NOMINAL_FONT_SIZE;
}

void imgui_set_text_color(uint32_t color)
{
    theme.color_text = color;
}

void imgui_set_text_padding(int padding)
{
    theme.text_padding = padding;
}

void imgui_set_spacing(int spacing)
{
    theme.spacing = spacing;
}

void imgui_set_panel_min_width(int width)
{
    theme.panel_min_width = width;
}

void imgui_store_theme()
{
    memcpy(&stored_theme, &theme, sizeof(ImGuiTheme));
}

void imgui_restore_theme()
{
    memcpy(&theme, &stored_theme, sizeof(ImGuiTheme));
}

void imgui_set_slider_width(int width)
{
    theme.slider_width = width;
}

void imgui_set_global_opacity_scale(float opacity_scale)
{
    ctx->global_opacity_scale = opacity_scale;
}

bool imgui_load_theme(char* file_name)
{
    load_themes();
    for(int i = 0; i < num_themes; ++i)
    {
        if(strcmp((char*)theme_files[i], file_name) == 0)
        {
            char file_path[64]= {0};
            snprintf(file_path,63,"src/themes/%s",file_name);
            FILE* fp = fopen(file_path,"rb");

            if(fp)
            {
                size_t n = fread(&theme,sizeof(ImGuiTheme),1,fp);
                fclose(fp);
                bool success = (n > 0);
                theme_initialized = success;
                if(success)
                    theme_index = i;
                return success;
            }
            else
            {
                LOGW("Theme not found: %s\n", file_path);
            }
        }
    }

    return false;
}

void imgui_text(char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    imgui_text_colored(theme.color_text, str);
}

void imgui_text_sized(float pxsize, char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx->curr.x, ctx->curr.y, theme.color_text, (pxsize / NOMINAL_FONT_SIZE), 0.0, 1.0, false, false, 0, str);
    ctx->curr.w = size.x+1.0*theme.spacing;
    ctx->curr.h = 1.3*size.y;

    progress_pos();
}

void imgui_text_colored(uint32_t color, char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx->curr.x, ctx->curr.y, color, theme.text_scale, 0.0, 1.0, false, false, 0, str);
    ctx->curr.w = size.x+1.0*theme.spacing;
    ctx->curr.h = size.y + theme.spacing;

    progress_pos();
}

void imgui_indent_begin(int indentpx)
{
    ctx->indent_amount = indentpx;
    ctx->curr.x += indentpx;
}

void imgui_indent_end()
{
    ctx->curr.x -= ctx->indent_amount;
    ctx->indent_amount = 0;
}

void imgui_newline()
{
    ctx->curr.y += theme.text_size_px;
}

void imgui_horizontal_line(int thickness)
{
    float width = ctx->panel_props.panel_width;//  - theme.spacing - 2.0*theme.text_padding;
    gfx_draw_rect_xywh(ctx->curr.x - theme.panel_spacing + width/2.0, ctx->curr.y, width,thickness, theme.color_text, 1.0, 0.0, 1.0, true,false);

    ctx->curr.y += (2+theme.text_padding);
}

void imgui_horizontal_begin()
{
    for(int i = 0; i < MAX_HORIZONTAL_STACK; ++i)
    {
        if(ctx->horizontal_stack[i] == false)
        {
            ctx->horizontal_stack[i] = true;
            ctx->is_horizontal = true;
            break;
        }
    }
}

void imgui_horizontal_end()
{
    for(int i = 0; i < MAX_HORIZONTAL_STACK; ++i)
    {
        if(ctx->horizontal_stack[i] == false)
        {
            if(i > 0)
            {
                ctx->horizontal_stack[i-1] = false;

                if(i == 1)
                {
                    ctx->is_horizontal = false;
                }
                else
                {
                    progress_pos();
                }
            }
            break;
        }
    }

    if(!ctx->is_horizontal)
    {
        ctx->curr.x = ctx->start_x + theme.panel_spacing + ctx->indent_amount + ctx->panel_props.offset_x;
        ctx->curr.y += ctx->horiontal_max_height;

        ctx->horiontal_max_height = 0;

        ctx->accum_width += ctx->curr.w;
        if(ctx->accum_width + ctx->indent_amount > ctx->max_width)
            ctx->max_width = ctx->accum_width + ctx->indent_amount;

        ctx->accum_width = 0;
    }
}

bool imgui_button(char* label, ...)
{
    bool result = false;
    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    uint32_t hash = hash_str(str,strlen(str),0x0);

    char new_label[256] = {0};
    mask_off_hidden(str, new_label, 256);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                result = true;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);
    int width = MAX(theme.button_min_width, text_size.x + 2*theme.text_padding);
    Rect interactive = {ctx->curr.x, ctx->curr.y, width, text_size.y + 2*theme.text_padding};
    handle_highlighting(hash, &interactive);

    draw_button(hash, new_label, &interactive);

    ctx->curr.w = width;
    ctx->curr.h = text_size.y + 2*theme.text_padding + theme.spacing;

    progress_pos();

    return result;
}

bool imgui_image_button(int img_index, int sprite_index, float scale, char* label, ...)
{
    bool result = false;
    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    uint32_t hash = hash_str(str,strlen(str),0x0);

    char new_label[256] = {0};
    mask_off_hidden(str, new_label, 256);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                result = true;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    GFXImage* img = &gfx_images[img_index];
    Rect interactive = {ctx->curr.x, ctx->curr.y, img->element_width*scale,img->element_height*scale};
    handle_highlighting(hash, &interactive);

    draw_image_button(hash, new_label, &interactive,img_index,sprite_index,scale);

    ctx->curr.w = interactive.w + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = interactive.h + 2*theme.text_padding + theme.spacing;

    progress_pos();

    return result;
}

void imgui_toggle_button(bool* toggle, char* label, ...)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            if(*toggle)
                *toggle = false;
            else
                *toggle = true;
        }
    }

    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, str);

    Rect interactive = {ctx->curr.x, ctx->curr.y, text_size.x + 2*theme.text_padding, text_size.y + 2*theme.text_padding};
    handle_highlighting(hash, &interactive);

    draw_toggle_button(hash, str, &interactive, *toggle);

    ctx->curr.w = text_size.x + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = text_size.y + 2*theme.text_padding + theme.spacing;

    progress_pos();
}

void imgui_set_button_select(int index, int num_buttons, char* button_labels[], char* label)
{
    char _str[100] = {0};
    snprintf(_str,99,"%s_%s##select%d",label,button_labels[0],num_buttons);

    uint32_t hash = hash_str(_str,strlen(_str),0x0);
    IntLookup* lookup = get_int_lookup(hash);
    if (!lookup)
        return;

    int* val = &lookup->val;
    *val = index;

    // bool results[32] = {false};
    // int selection = 0;

    // int prior_spacing = theme.spacing;

    // for(int i = 0; i < num_buttons; ++i)
    // {
    //     if(i == *val)
    //     {
    //         results[i] = true;
    //     }

    //     if(results[i])
    //     {
    //         selection = i;
    //         *val = i;
    //     }
    // }
}

void imgui_button_select(int num_buttons, char* button_labels[], char* label, int* selection)
{
    if(num_buttons < 0 || num_buttons >= 32)
        return;

    char _str[100] = {0};
    snprintf(_str,99,"%s_%s##select%d",label,button_labels[0],num_buttons);

    bool results[32] = {false};

    int prior_spacing = theme.spacing;
    imgui_set_spacing(1);
    imgui_horizontal_begin();

    for(int i = 0; i < num_buttons; ++i)
    {
        if(i == *selection)
        {
            results[i] = true;
        }

        imgui_toggle_button(&results[i], button_labels[i]);

        if(results[i])
        {
            *selection = i;
        }
    }
    imgui_set_spacing(prior_spacing);
    imgui_text(label);
    imgui_horizontal_end();
}

void imgui_dropdown(char* options[], int num_options, char* label, int* selected_index, bool* interacted)
{
    if(interacted != NULL)
        *interacted = false;

    if(!options)
        return;

    if(!options[0])
        return;

    if(num_options < 0 || num_options >= 32)
        return;

    char _str[100] = {0};
    snprintf(_str,99,"%s_%s##dropdown%d",label,options[0],num_options);

    uint32_t hash = hash_str(_str,strlen(_str),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    bool results[32] = {false};
    int selection = 0;

    float max_height = NOMINAL_FONT_SIZE*theme.text_scale + 2*theme.text_padding;
    float max_width = 0.0;

    for(int i = 0; i < num_options; ++i)
    {
        if(options[i])
        {
            Vector2f text_size = gfx_string_get_size(theme.text_scale, options[i]);
            if(text_size.x > max_width)
            {
                max_width = text_size.x;
            }
        }
    }

    max_width += 16; // size of +/-

    bool active = is_active(hash);

    if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            if(interacted != NULL) *interacted = true;
            if(active)
            {
                // make new selection
                float y_diff = ctx->mouse_y - ctx->curr.y;
                int index = floor(y_diff / max_height) - 1;
                if(index >= 0 && index < num_options)
                {
                    ctx->dropdown_props.selected_index = index;
                    *selected_index = index;
                }
                clear_active();
            }
            else
            {
                ctx->dropdown_props.selected_index = *selected_index;
                set_active(hash);
            }
        }
    }
    else if(is_active(hash))
    {
        if(interacted != NULL) *interacted = true;
        if(window_mouse_left_went_down())
        {
            clear_active();
        }
    }

    int display_count = active ? num_options +1 : 1;

    Rect interactive = {ctx->curr.x, ctx->curr.y, max_width+2*theme.text_padding, MIN(display_count*max_height, theme.dropdown_max_height)};
    handle_highlighting(hash, &interactive);

    if(active)
    {
        // cache needed properties to draw later
        ctx->dropdown_props.hash = hash;
        ctx->dropdown_props.options = (char**)options;
        ctx->dropdown_props.num_options = num_options;
        ctx->dropdown_props.text_size_px = theme.text_size_px;
        memcpy(ctx->dropdown_props.label,new_label,31);
        memcpy(&ctx->dropdown_props.size,&interactive,sizeof(Rect));
    }
    else
    {
        // draw now since the drop down isn't expanded
        draw_dropdown(hash, new_label, options, num_options, *selected_index, &interactive);
    }

    Vector2f label_size = gfx_string_get_size(theme.text_scale, new_label);

    ctx->curr.w = interactive.w + label_size.x + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = max_height + theme.spacing;

    progress_pos();

    //return ctx->dropdown_props.selected_index;
}

bool imgui_listbox(char* options[], int num_options, char* label, int* selected_index, int max_display)
{
    if(!options)
        return false;

    if(!options[0])
        return false;

    if(num_options < 0 || num_options >= 1000)
        return false;


    char _str[100] = {0};
    snprintf(_str,99,"%s_%s##listbox%d",label,options[0],num_options);

    uint32_t hash = hash_str(_str,strlen(_str),0x0);

    FloatLookup* lookup = get_float_lookup(hash);
    if (!lookup)
        return false;

    float *val = &lookup->val;

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    float max_width = 0.0;

    bool is_active_scroll = (ctx->scroll_props.hash == hash);

    for(int i = 0; i < num_options; ++i)
    {
        if(options[i])
        {
            Vector2f text_size = gfx_string_get_size(theme.text_scale, options[i]);
            if(text_size.x > max_width)
            {
                max_width = text_size.x;
            }
        }
    }

    Vector2f label_size = gfx_string_get_size(theme.text_scale, label);
    
    float mouse_dist_from_right_edge = theme.listbox_width - (ctx->mouse_x - ctx->curr.x);

    float box_height = NOMINAL_FONT_SIZE*theme.text_scale + 2*theme.text_padding;
    float listbox_height = box_height * max_display;
    int scrollbar_height = listbox_height*(listbox_height / (num_options*box_height));
    int scrollbar_height_space = listbox_height - scrollbar_height;
    float y_diff = ctx->mouse_y - (ctx->curr.y+label_size.y+theme.text_padding);
    bool show_scrollbar = num_options*box_height > listbox_height;

    if(ctx->scroll_props.press_time > 0.25)
        ctx->scroll_props.press_time = 0.00;

    if(is_active_scroll && is_highlighted(hash))
    {
        ctx->scroll_props.in_gutter = (mouse_dist_from_right_edge > 0.0 && mouse_dist_from_right_edge < theme.listbox_gutter_width);

        if(window_controls_is_key_state(GLFW_KEY_DOWN, GLFW_PRESS))
        {
            if(ctx->scroll_props.press_time == 0.0)
            {
                if(*selected_index < num_options -1) (*selected_index)++;

                *val = (MAX(0, (*selected_index-(max_display/2.0))) / (float)num_options)*listbox_height;
                *val = RANGE(*val,0,scrollbar_height_space);
            }

            ctx->scroll_props.press_time += 0.01667;
        }
        else if(window_controls_is_key_state(GLFW_KEY_UP, GLFW_PRESS))
        {
            if(ctx->scroll_props.press_time == 0.00)
            {
                if(*selected_index > 0) (*selected_index)--;

                *val = (MAX(0, (*selected_index-(max_display/2.0))) / (float)num_options)*listbox_height;
                *val = RANGE(*val,0,scrollbar_height_space);
            }

            ctx->scroll_props.press_time += 0.01667;
        }
        else
        {
            ctx->scroll_props.press_time = 0.00;
        }

    }

    bool _interacted = false;

    if(is_active_scroll)
    {
        if(ctx->scroll_props.scrollbar_held)
        {
            _interacted = true;
            if(window_mouse_left_went_up())
            {
                ctx->scroll_props.scrollbar_held = false;
            }
            else
            {
                float amt_moved = ctx->mouse_y - ctx->scroll_props.held_start_pos;
                ctx->scroll_props.held_start_pos = ctx->mouse_y;

                *val += amt_moved;
                *val = RANGE(*val,0,scrollbar_height_space);
            }
        }
    }

    if(is_highlighted(hash))
    {
        ctx->scroll_props.hash = hash;

        double scroll_offset_x = 0.0;
        double scroll_offset_y = 0.0;

        if(window_has_scrolled())
        {
            _interacted = true;
            window_get_scroll_offsets(&scroll_offset_x, &scroll_offset_y);
        }

        if(show_scrollbar && !ctx->scroll_props.scrollbar_held)
        {
            *val -= 0.5*box_height*scroll_offset_y;
            *val = RANGE(*val,0,scrollbar_height_space);
        }
        if(ctx->scroll_props.in_gutter)
        {
            // scrollbar
            if(!ctx->scroll_props.scrollbar_held && window_mouse_left_went_down())
            {
                _interacted = true;
                if(y_diff >= *val && y_diff <= *val + scrollbar_height)
                {
                    ctx->scroll_props.scrollbar_held = true;
                    ctx->scroll_props.held_start_pos = ctx->mouse_y;
                }
            }
        }
        else
        {
            if(!ctx->scroll_props.scrollbar_held && window_mouse_left_went_down())
            {
                _interacted = true;
                // make new selection
                float ratio = (num_options*box_height) / listbox_height;
                float offset = ratio*(*val);

                int index = floor(num_options*((offset + y_diff)/(box_height*num_options)));

                if(index >= 0 && index < num_options)
                {
                    *selected_index = index;
                }
            }
        }

    }

    Rect interactive = {ctx->curr.x, ctx->curr.y + label_size.y + theme.text_padding, MAX(max_width+2*theme.text_padding, theme.listbox_width), listbox_height};

    handle_highlighting(hash, &interactive);

    draw_listbox(hash, new_label, options, num_options, *selected_index, *val, &interactive, max_display);

    ctx->curr.w = interactive.w + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = label_size.y + theme.text_padding + interactive.h + box_height + theme.spacing;

    progress_pos();

    return _interacted;
}

void imgui_tooltip(char* tooltip, ...)
{
    if(!tooltip)
        return;

    if(is_highlighted(ctx->tooltip_hash))
        return;

    va_list args;
    va_start(args, tooltip);
    char str[256] = {0};
    vsprintf(str,tooltip, args);
    va_end(args);

    memset(ctx->tooltip,0,MAX_TOOLTIP_LEN+1);

    int len = MIN(strlen(str),MAX_TOOLTIP_LEN);
    strncpy(ctx->tooltip,str,len);

    ctx->tooltip_hash = 0x0;
    ctx->has_tooltip = true;
}


void imgui_checkbox(char* label, bool* result)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                bool curr_result = *result;
                *result = !curr_result;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, theme.checkbox_size + theme.text_padding + text_size.x, theme.checkbox_size};
    handle_highlighting(hash, &interactive);

    draw_checkbox(hash, label, *result);
    progress_pos();
}

void imgui_color_picker(char* label, uint32_t* result)
{
    int r = (*result >> 16) & 0xFF;
    int g = (*result >>  8) & 0xFF;
    int b = (*result >>  0) & 0xFF;

    char lr[32] = {0};
    char lg[32] = {0};
    char lb[32] = {0};

    snprintf(lr,31,"##%s_R",label);
    snprintf(lg,31,"##%s_G",label);
    snprintf(lb,31,"##%s_B",label);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    int prior_spacing = theme.spacing;
    imgui_set_spacing(2);
    imgui_horizontal_begin();

        Vector2f s1 = imgui_number_box_formatted(lr, 0, 255, "%02X", &r);
        Vector2f s2 = imgui_number_box_formatted(lg, 0, 255, "%02X", &g);
        Vector2f s3 = imgui_number_box_formatted(lb, 0, 255, "%02X", &b);
        *result = COLOR((uint8_t)r,(uint8_t)g,(uint8_t)b);

        Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);
        Rect box = {ctx->curr.x,ctx->curr.y, 20, text_size.y+2.0*theme.text_padding};
        draw_color_box(&box,*result);

        draw_label(ctx->curr.x + box.w + theme.text_padding, ctx->curr.y-(text_size.y-box.h)/2.0, theme.color_text, new_label);

        ctx->curr.w = s1.x + s2.x + s3.x + box.w + text_size.x + 2.0*theme.text_padding;
        ctx->curr.h = MAX(text_size.y,box.h);
    imgui_horizontal_end();
    imgui_set_spacing(prior_spacing);
}

void imgui_slider_float(char* label, float min, float max, float* result)
{
    imgui_slider_float_internal(label, min, max, result, "%.4f");
    progress_pos();
}

Vector2f imgui_number_box_formatted(char* label, int min, int max, char* format, int* result)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    Vector2f ret = {ctx->curr.w, ctx->curr.h};

    IntLookup* lookup = get_int_lookup(hash);
    if (!lookup)
        return ret;

    int* val = &lookup->val;

    if(*result >= min && *result <= max)
        *val = *result;

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            clear_active();
        }
        else
        {
            // update val
            int range = max - min;
            int thresh = RANGE((view_width/4) / range, 1, 10);
            // printf("thresh: %d\n", thresh);
            int dif = ctx->mouse_x - ctx->slider_mouse_x;
            if(ABS(dif) >= thresh)
            {
                *val += (dif/thresh);
                ctx->slider_mouse_x = ctx->mouse_x;
            }
            *val = RANGE(*val, min,max);
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, theme.number_box_width, text_size.y + 2*theme.text_padding};
    handle_highlighting(hash, &interactive);

    draw_number_box(hash, new_label, &interactive, *val,max,format);

    ctx->curr.w = theme.number_box_width + text_size.x + theme.spacing;
    ctx->curr.h = text_size.y + 2*theme.text_padding + theme.spacing;

    progress_pos();

    *result = *val;

    return ret;
}

Vector2f imgui_number_box(char* label, int min, int max, int* result)
{
    return imgui_number_box_formatted(label, min, max, "%d", result);
}

uint32_t imgui_text_box(char* label, char* buf, int bufsize)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    float size_array[100] = {0.0};
    int array_size = 0;
    gfx_string_get_size_array(theme.text_scale, &size_array[1],99,&array_size, buf);

    if(ctx->focused_text_id == hash)
    {
        window_controls_set_text_buf(buf,bufsize);
        window_controls_set_key_mode(KEY_MODE_TEXT);

        if(ctx->text_box_props.text_cursor_index >= bufsize)
            ctx->text_box_props.text_cursor_index = bufsize-1;

        ctx->text_box_props.text_cursor_x = size_array[ctx->text_box_props.text_cursor_index];
        ctx->text_box_props.text_cursor_x_held_from = size_array[ctx->text_box_props.text_cursor_index_held_from];

        // handle cursor blink state
        if(ctx->text_box_props.cursor_blink_time >= CURSOR_BLINK_TIME)
        {
            ctx->text_box_props.cursor_blink_time = 0.0;
            ctx->text_box_props.cursor_show = !ctx->text_box_props.cursor_show;
        }

        ctx->text_box_props.cursor_blink_time += (timer_get_time() - ctx->text_box_props.cursor_blink_t0);
        ctx->text_box_props.cursor_blink_t0 = timer_get_time();
    }

    if(is_highlighted(hash))
    {
        ctx->text_box_props.highlighted = true;
        window_mouse_set_cursor_ibeam();

        bool mouse_went_down = window_mouse_left_went_down();

        if(mouse_went_down || ctx->text_box_props.text_click_held)
        {
            float min_dist = 10000.0;
            int selected_index = 0;

            int box_x_start = ctx->curr.x + theme.text_padding;

            if(ctx->mouse_x >= box_x_start)
            {
                int click_x = (int)(ctx->mouse_x - box_x_start);
                for(int i = array_size; i >= 0; --i)
                {
                    float dist = ABS(size_array[i] - (float)click_x);
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        selected_index = i;
                    }
                }
            }

            ctx->focused_text_id = hash;
            ctx->text_box_props.cursor_show = true;
            ctx->text_box_props.cursor_blink_time = 0.0;
            ctx->text_box_props.cursor_blink_t0 = timer_get_time();
            ctx->text_box_props.text_cursor_index = selected_index;
            ctx->text_box_props.text_cursor_x = size_array[selected_index];

            if(mouse_went_down)
            {
                ctx->text_box_props.text_click_held = true;
                ctx->text_box_props.text_cursor_x_held_from = ctx->text_box_props.text_cursor_x;
                ctx->text_box_props.text_cursor_index_held_from = ctx->text_box_props.text_cursor_index;

            }
        }
    }

    if(ctx->focused_text_id == hash)
    {
        if(ctx->text_box_props.text_click_held && window_mouse_left_went_up())
        {
            ctx->text_box_props.text_click_held = false;
        }

        int str_size = strlen(buf);
        if(ctx->text_box_props.text_cursor_index > str_size)
        {
            ctx->text_box_props.text_cursor_index = str_size;
            ctx->text_box_props.text_cursor_x = size_array[str_size];
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, theme.text_box_width, theme.text_box_height};
    handle_highlighting(hash, &interactive);

    draw_text_box(hash, new_label, &interactive, buf);

    ctx->curr.w = theme.text_box_width + text_size.x + theme.text_padding + theme.spacing;
    ctx->curr.h = theme.text_box_height + theme.spacing;

    progress_pos();

    return hash;
}

bool imgui_text_set_ctrl_enter()
{
    _ctrl_enter = true;
}

bool imgui_text_get_ctrl_enter()
{
    bool ret = _ctrl_enter;
    _ctrl_enter = false;
    return ret;
}

bool imgui_text_set_enter()
{
    _enter = true;
}

bool imgui_text_get_enter()
{
    bool ret = _enter;
    _enter = false;
    return ret;
}

uint32_t imgui_text_box_sized(char* label, char* buf, int bufsize, int width, int height)
{
    int width_prior = theme.text_box_width;
    int height_prior = theme.text_box_height;

    theme.text_box_width = width;
    theme.text_box_height = height;

    uint32_t hash = imgui_text_box(label, buf, bufsize);

    theme.text_box_width = width_prior;
    theme.text_box_height = height_prior;

    return hash;
}

void imgui_focus_text_box(uint32_t hash)
{
    ctx->focused_text_id = hash;
    ctx->text_box_props.cursor_show = true;
    ctx->text_box_props.cursor_blink_time = 0.0;
    ctx->text_box_props.cursor_blink_t0 = timer_get_time();
}

void imgui_reset_cursor_blink()
{
    ImGuiContext* active_ctx = get_active_textbox_context();

    active_ctx->text_box_props.cursor_blink_time = 0.0;
    active_ctx->text_box_props.cursor_show = true;
    active_ctx->text_box_props.cursor_blink_t0 = timer_get_time();
}

// demo vars
static int num_clicks = 0;
static uint32_t color1 = 0x00FE2225;
static uint32_t color2 = 0x002468F2;
static float v1 = 0.0;
static float v2 = 0.0;
static char name[32] = {0};
static char something[20] = {0};
static char something2[30] = {0};
static bool my_check = true;
static int ri = 10;
static bool toggle = false;
static bool thing1 = false, thing2 = false;

static char* colors[] = {"Red","Green","Blue","Orange","Purple","Cyan","Pink","Yellow","White", "Whatever", "Junk", "More Shit",
"Red","Green","Blue","Orange","Purple","Cyan","Pink","Yellow","White", "Whatever", "Junk", "More Shit","Cool"
};

static char* shapes[] = {"Square","Rectangle","Circle"};

static char color_filter_str[32] = {0};
static int color_select = 0;
static int shape_select = 0;
static int color_list_select = 0;

Rect imgui_draw_demo(int x, int y)
{
    imgui_begin_panel("Demo", x,y,true);

        imgui_text_colored(0x00FF00FF, "My name is %s", "Chris");
        imgui_text_colored(0x0000FFFF, "My name is %s", "Kam");

        if(imgui_button("Test Button"))
        {
            num_clicks++;
        }
        imgui_text_colored(0xFFFFFFFF, "Num clicks: %d", num_clicks);

        imgui_button("Dumb Button");
        imgui_slider_float("Slider 1", 0.0,1.0,&v1);
        imgui_slider_float("Slider 2", 0.0,1.0,&v2);

        imgui_checkbox("Checkbox 1",&my_check);

        imgui_indent_begin(12);
            imgui_checkbox("whatever",&thing1);
            imgui_checkbox("dawg",&thing2);
        imgui_indent_end();

        imgui_number_box("Some Int##haha",0, 100, &ri);

        imgui_color_picker("Color 1", &color1);
        imgui_color_picker("Color 2", &color2);
        imgui_text_colored(0xFFFFFFFF, "Test");
        imgui_text_box("Name##demo_name",name,IM_ARRAYSIZE(name));
        imgui_text_box("Another Thing",something,IM_ARRAYSIZE(something));
        imgui_text_box("Another One",something2,IM_ARRAYSIZE(something2));
        imgui_toggle_button(&toggle, "Toggle me");

        char* buttons[] = {"Apples", "Bananas", "Oranges"};
        static int selection = 0;
        imgui_button_select(3, buttons, "Best Fruit", &selection);

        imgui_text_colored(0x00FF00FF,buttons[selection]);

        imgui_dropdown(colors, IM_ARRAYSIZE(colors), "Select Color", &color_select, NULL);
        imgui_dropdown(shapes, IM_ARRAYSIZE(shapes), "Select Shape", &shape_select, NULL);

        imgui_text_box("Filter",color_filter_str,IM_ARRAYSIZE(color_filter_str));

        imgui_listbox(colors, IM_ARRAYSIZE(colors), "Colors", &color_list_select, 4);

   return imgui_end();
}

static void load_themes()
{
    num_themes = io_get_files_in_dir("src/themes/", ".theme", theme_files);

    for(int i = 0; i < num_themes; ++i)
    {
        theme_file_ptrs[i] = (char*)theme_files[i];
    }
}

void imgui_theme_selector()
{
    load_themes();

    int prior_selected_theme = theme_index;
    if(theme_index == -1)
    {
        theme_index = 0;
    }

    imgui_dropdown(theme_file_ptrs, num_themes, "Theme", &theme_index, NULL);
    if(theme_index != prior_selected_theme)
    {
        imgui_load_theme(theme_file_ptrs[theme_index]);
    }
}

static bool save_theme(char* file_name)
{
    theme.version = IMGUI_THEME_VERSION;

    char file_path[64]= {0};
    snprintf(file_path,63,"src/themes/%s.theme",file_name);
    FILE* fp = fopen(file_path,"wb");
    if(fp)
    {
        size_t n = fwrite(&theme,sizeof(ImGuiTheme),1,fp);
        fclose(fp);
        return (n > 0);
    }

    return false;
}

void imgui_theme_editor()
{
    imgui_theme_selector();

    int header_size = 12;
    imgui_text_sized(header_size,"Colors");
    imgui_indent_begin(10);
    imgui_color_picker("Text", &theme.color_text);
    imgui_color_picker("Background", &theme.color_background);
    imgui_color_picker("Highlight", &theme.color_highlight);
    imgui_color_picker("Highlight Subtle", &theme.color_highlight_subtle);
    imgui_color_picker("Active", &theme.color_active);
    imgui_color_picker("Panel", &theme.color_panel);
    imgui_color_picker("Slider", &theme.color_slider);

    if(imgui_button("Randomize Colors"))
    {
        theme.color_text             = COLOR(RAND8(),RAND8(),RAND8());
        theme.color_background       = COLOR(RAND8(),RAND8(),RAND8());
        theme.color_highlight        = COLOR(RAND8(),RAND8(),RAND8());
        theme.color_highlight_subtle = COLOR(RAND8(),RAND8(),RAND8());
        theme.color_active           = COLOR(RAND8(),RAND8(),RAND8());
        theme.color_panel            = COLOR(RAND8(),RAND8(),RAND8());
        theme.color_slider           = COLOR(RAND8(),RAND8(),RAND8());
    }

    imgui_indent_end();

    imgui_text_sized(header_size,"Settings");
    imgui_indent_begin(10);
    imgui_number_box("Spacing", 0, 100, &theme.spacing);
    imgui_slider_float("Text Size", 0.0, 100.0, &theme.text_size_px);
    imgui_set_text_size(theme.text_size_px);
    imgui_number_box("Text Padding", 0, 20, &theme.text_padding);
    imgui_slider_float("Button Opacity", 0.0,1.0,&theme.button_opacity);
    imgui_number_box("Button Min Width", 1,200,&theme.button_min_width);
    imgui_number_box("Checkbox Size", 0, 100, &theme.checkbox_size);
    imgui_slider_float("Slider Handle Width", 0.0,50.0,&theme.slider_handle_width);
    imgui_slider_float("Slider Opacity", 0.0,1.0,&theme.slider_opacity);
    imgui_number_box("Number box width", 0, 100, &theme.number_box_width);
    imgui_slider_float("Panel Opacity", 0.0,1.0,&theme.panel_opacity);
    imgui_number_box("Panel min width", 0, 1000, &theme.panel_min_width);
    imgui_number_box("Panel Spacing", 0, 100, &theme.panel_spacing);
    imgui_number_box("Panel Header Height", 0, 100, &theme.panel_header_height);
    imgui_indent_end();

    if(imgui_button("Defaults"))
    {
        set_default_theme();
    }

    imgui_horizontal_begin();
    imgui_text_box("File Name##file_name",_editor_file_name,IM_ARRAYSIZE(_editor_file_name));

    if(imgui_button("Save"))
    {
        saved = true;
        save_result = save_theme(_editor_file_name);
    }
    imgui_horizontal_end();

    if(saved)
    {
        if(save_result)
            imgui_text_colored(0x0000FF00, "File saved!");
        else
            imgui_text_colored(0x00FF0000, "Failed to Save file!");
    }
}

void imgui_deselect_text_box()
{
    for(int i = 0; i < MAX_CONTEXTS; ++i)
    {
        contexts[i].focused_text_id = 0x0;
    }

    window_controls_set_key_mode(KEY_MODE_NORMAL);
}

static ImGuiContext* get_active_textbox_context()
{
    ImGuiContext* active_ctx = NULL;
    for(int i = 0; i < MAX_CONTEXTS; ++i)
    {
        if(contexts[i].focused_text_id != 0x0)
        {
            return &contexts[i];
        }
    }
    return &default_context;
}

int imgui_get_text_cursor_index()
{
    ImGuiContext* active_ctx = get_active_textbox_context();
    return active_ctx->text_box_props.text_cursor_index;
}

void imgui_set_text_cursor_indices(int i0, int i1)
{
    ImGuiContext* active_ctx = get_active_textbox_context();
    active_ctx->text_box_props.text_cursor_index = i0;
    active_ctx->text_box_props.text_cursor_index_held_from = i1;
}

void imgui_get_text_cursor_indices(int* i0, int* i1)
{
    ImGuiContext* active_ctx = get_active_textbox_context();
    if(active_ctx->text_box_props.text_cursor_index <= active_ctx->text_box_props.text_cursor_index_held_from)
    {
        *i0 = active_ctx->text_box_props.text_cursor_index;
        *i1 = active_ctx->text_box_props.text_cursor_index_held_from;
    }
    else
    {
        *i0 = active_ctx->text_box_props.text_cursor_index_held_from;
        *i1 = active_ctx->text_box_props.text_cursor_index;
    }
}

void imgui_text_cursor_inc(int val)
{
    ImGuiContext* active_ctx = get_active_textbox_context();

    active_ctx->text_box_props.text_cursor_index += val;
    active_ctx->text_box_props.text_cursor_index = MAX(0,active_ctx->text_box_props.text_cursor_index);
    active_ctx->text_box_props.text_cursor_index_held_from = active_ctx->text_box_props.text_cursor_index;
}

Rect imgui_end()
{
    ctx->panel_props.panel_width = MAX(theme.panel_min_width,ctx->max_width+2.0*theme.spacing+theme.panel_spacing);
    ctx->panel_props.panel_height = (ctx->curr.y-ctx->start_y-ctx->panel_props.offset_y);

    bool text_highlighted = false;
    for(int i = 0; i < MAX_CONTEXTS; ++i)
    {
        if(contexts[i].text_box_props.highlighted)
        {
            text_highlighted = true;
            break;
        }
    }

    if(!text_highlighted)
    {
        window_mouse_set_cursor_normal();
    }

    if(ctx->dropdown_props.options && is_active(ctx->dropdown_props.hash))
    {
        imgui_set_text_size(ctx->dropdown_props.text_size_px);
        draw_dropdown(ctx->dropdown_props.hash, ctx->dropdown_props.label, ctx->dropdown_props.options, ctx->dropdown_props.num_options, ctx->dropdown_props.selected_index, &ctx->dropdown_props.size);
    }

    if(ctx->has_tooltip && is_highlighted(ctx->tooltip_hash))
    {
        draw_tooltip();
    }

    Rect size = {0};
    size.x = ctx->start_x + ctx->panel_props.offset_x;
    size.y = ctx->start_y + ctx->panel_props.offset_y;
    size.w = ctx->panel_props.panel_width;
    size.h = ctx->panel_props.panel_height;

    memcpy(&ctx->prior_size,&size,sizeof(Rect));

    size.x += size.w/2.0;
    size.y += size.h/2.0;

    return size;
}

bool imgui_clicked()
{
    return (ctx->active_id != 0x0);
}

bool imgui_active()
{
    return (ctx->highlighted_id != 0x0);
}

bool imgui_is_mouse_inside()
{
    return is_inside(&ctx->prior_size);
}

// ============================
// Static Functions
// ============================

static inline bool is_mouse_inside(int x, int y, int width, int height)
{
    return (ctx->mouse_x >= x && ctx->mouse_x <= (x+width) &&
                   ctx->mouse_y >= y && ctx->mouse_y <= (y+height));
}
static inline bool is_highlighted(uint32_t hash)
{
    return (ctx->highlighted_id == hash);
}
static inline bool is_active(uint32_t hash)
{
    return (ctx->active_id == hash);
}
static inline bool is_inside(Rect* r)
{
    return is_mouse_inside(r->x, r->y, r->w, r->h);
}
static inline void set_highlighted(uint32_t hash)
{
    ctx->highlighted_id = hash;
}
static inline void set_active(uint32_t hash)
{
    ctx->active_id = hash;
}

static inline void clear_highlighted()
{
    ctx->highlighted_id = 0x0;
}

static inline void clear_active()
{
    ctx->active_id = 0x0;
}

static void set_default_theme()
{
    theme_index = -1;
    load_themes();
    imgui_load_theme("default.theme");

    if(theme_initialized)
        return;

    // colors
    theme.color_text             = 0xFFFFFFFF;
    theme.color_background       = 0x55555555;
    theme.color_highlight        = 0xAAAAAAAA;
    theme.color_highlight_subtle = 0x66666666;
    theme.color_active           = 0x0000AAAA;
    theme.color_panel            = 0x20202020;
    theme.color_slider           = 0x88888888;

    // settings
    theme.spacing = 8;
    theme.text_size_px = 12.0;
    theme.text_scale = theme.text_size_px / NOMINAL_FONT_SIZE;
    theme.text_padding = 4;
    theme.text_box_width = 150;
    theme.text_box_height = 22;
    theme.number_box_width = 40;
    theme.slider_width = 132.0;
    theme.slider_handle_width = 16;
    theme.slider_opacity = 0.8;
    theme.button_opacity = 0.8;
    theme.button_min_width = 72;
    theme.checkbox_size = 10;
    theme.panel_opacity = 0.85;
    theme.panel_min_width = 200;
    theme.panel_spacing = 8;
    theme.panel_header_height = 20;

    theme.dropdown_max_height = 1000;
    theme.listbox_width = 200;
    theme.listbox_height = 300;
    theme.listbox_gutter_width = 12;

    theme_initialized = true;
    theme_index = -1;
}

static void progress_pos()
{

#if DRAW_DEBUG_BOXES
    gfx_draw_rect_xywh(ctx->curr.x + ctx->curr.w/2.0, ctx->curr.y + ctx->curr.h/2.0, ctx->curr.w, ctx->curr.h, 0x0000FFFF, 1.0, 0.0, 1.0, false,false);
#endif

    ctx->accum_width += ctx->curr.w;

    if(ctx->is_horizontal)
    {
        ctx->curr.x += ctx->curr.w;

        if(ctx->curr.h > ctx->horiontal_max_height)
            ctx->horiontal_max_height = ctx->curr.h;
    }
    else
    {
        ctx->curr.y += ctx->curr.h;

        if(ctx->accum_width + ctx->indent_amount > ctx->max_width)
            ctx->max_width = ctx->accum_width + ctx->indent_amount;
        ctx->accum_width = 0;
    }

    ctx->curr.w = 0;
    ctx->curr.h = 0;
}

static void assign_context(uint32_t hash)
{
    for(int i = 0; i < MAX_CONTEXTS;++i)
    {
        if(contexts[i].hash == hash || contexts[i].hash == 0x0)
        {
            ctx = &contexts[i];
            ctx->hash = hash;
            return;
        }
    }
    ctx = &default_context;
}

static void mask_off_hidden(char* label, char* new_label, int max_size)
{
    memcpy(new_label, label, MIN(strlen(label),max_size - 1));

    char* p = new_label;
    bool end = false;
    for(;;)
    {
        if((*p) == '\0') break;
        if(!end && *p == '#' && *(p+1) == '#') end = true;
        if(end) *p = '\0';
        p++;
    }
}

static IntLookup* get_int_lookup(uint32_t hash)
{
    for(int i = 0; i < MAX_INT_LOOKUPS; ++i)
    {
        if(ctx->int_lookups[i].hash == 0x0 || ctx->int_lookups[i].hash == hash)
        {
            IntLookup* lookup = &ctx->int_lookups[i];
            lookup->used = (lookup->hash != 0x0);
            lookup->hash = hash;
            return lookup;
        }
    }

    return NULL;
}

static FloatLookup* get_float_lookup(uint32_t hash)
{
    for(int i = 0; i < MAX_FLOAT_LOOKUPS; ++i)
    {
        if(ctx->float_lookups[i].hash == 0x0 || ctx->float_lookups[i].hash == hash)
        {
            FloatLookup* lookup = &ctx->float_lookups[i];
            lookup->used = (lookup->hash != 0x0);
            lookup->hash = hash;
            return lookup;
        }
    }

    return NULL;
}

static void imgui_slider_float_internal(char* label, float min, float max, float* result, char* format)
{
    if(min > max)
        return;

    float inp_val = *result;


    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    // get slider index
    int first_empty = -1;
    int slider_index = -1;

    IntLookup* lookup = get_int_lookup(hash);
    if (!lookup)
        return;

    ctx->curr.w = theme.slider_width;
    ctx->curr.h = text_size.y + 2*theme.text_padding;

    float val = 0.0;
    val = *result;
    val -= min;
    val /= (max-min);
    val *= (float)(ctx->curr.w - theme.slider_handle_width);
    
    lookup->val = roundf(val);

    int *slider_x = &lookup->val;

    bool active = false;

    if(is_active(hash))
    {
        active = true;
        if(window_mouse_left_went_up())
        {
            clear_active();
        }

        *slider_x = ctx->mouse_x-ctx->curr.x-theme.slider_handle_width/2.0;
        *slider_x = RANGE(*slider_x, 0, ctx->curr.w - theme.slider_handle_width);

    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    handle_highlighting(hash, &ctx->curr);

    float slider_val = (*slider_x / (ctx->curr.w - theme.slider_handle_width));
    slider_val *= (max-min);
    slider_val += min;

    draw_slider(hash,new_label, *slider_x, format, slider_val);

    ctx->curr.w = theme.slider_width + text_size.x + 2.0*theme.text_padding;
    ctx->curr.h = text_size.y + 2.0*theme.text_padding + theme.spacing;

    if(active)
        *result = slider_val;
    else
        *result = inp_val;
}

static void handle_highlighting(uint32_t hash, Rect* r)
{
    if(is_inside(r) && !dropdown_on_top(hash, r))
    {
        set_highlighted(hash);
    }
    else
    {
        if(is_highlighted(hash))
        {
            clear_highlighted();
        }
    }

    handle_tooltip(hash);
}

static bool dropdown_on_top(uint32_t hash, Rect* r)
{
    if(is_active(ctx->dropdown_props.hash))
    {
        if(ctx->dropdown_props.hash == hash)
        {
            return false;
        }

        if(rectangles_colliding2(&ctx->dropdown_props.size,r))
        {
            return true;
        }
    }
    return false;
}

static void handle_tooltip(uint32_t hash)
{
    if(ctx->has_tooltip)
    {
        //if(is_highlighted(hash) && !ctx->tooltip_hash)
        if(!ctx->tooltip_hash)
            ctx->tooltip_hash = hash;
    }
}

static void draw_button(uint32_t hash, char* str, Rect* r)
{
    // draw button
    uint32_t button_color = theme.color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.color_highlight;
    }
    if(is_active(hash))
    {
        button_color = theme.color_active;
    }

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, button_color, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    Vector2f val_size = gfx_string_get_size(theme.text_scale, str); // for centering text on button
    gfx_draw_string(r->x + (r->w - val_size.x)/2.0, r->y + (r->h - val_size.y)/2.0, theme.color_text, theme.text_scale, 0.0, 1.0, false, false, 0, str);
}

static void draw_image_button(uint32_t hash, char* str, Rect* r, int img_index, int sprite_index,float scale)
{
    // draw button
    uint32_t button_color = theme.color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.color_highlight;
    }
    if(is_active(hash))
    {
        button_color = theme.color_active;
    }

    gfx_draw_image_ignore_light(img_index, sprite_index, r->x + r->w/2.0, r->y + r->h/2.0, COLOR_TINT_NONE, scale, 0.0, 1.0, true,false);
    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, button_color, 1.0, 0.0, 0.3*ctx->global_opacity_scale, true,false);

    gfx_draw_string(r->x + r->w + theme.text_padding, r->y, theme.color_text, theme.text_scale, 0.0, 1.0, false, false, 0, str);
}

static void draw_toggle_button(uint32_t hash, char* str, Rect* r, bool toggled)
{
    // draw button
    uint32_t button_color = theme.color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.color_highlight;
    }
    if(toggled)
    {
        button_color = theme.color_active;
    }

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, button_color, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding, theme.color_text, theme.text_scale, 0.0, 1.0, false, false, 0, str);
}

static void draw_slider(uint32_t hash, char* str, int slider_x, char* val_format, float val)
{
    // draw bar
    gfx_draw_rect_xywh(ctx->curr.x + ctx->curr.w/2.0, ctx->curr.y + ctx->curr.h/2.0, ctx->curr.w, ctx->curr.h, theme.color_background, 1.0, 0.0, theme.slider_opacity*ctx->global_opacity_scale, true,false);

    // draw handle
    uint32_t handle_color = theme.color_slider;

    if(is_highlighted(hash))
    {
        handle_color = theme.color_highlight;
    }
    if(is_active(hash))
    {
        handle_color = theme.color_active;
    }

    gfx_draw_rect_xywh(ctx->curr.x + theme.slider_handle_width/2.0 + slider_x, ctx->curr.y + ctx->curr.h/2.0, theme.slider_handle_width-4, ctx->curr.h-4, handle_color, 1.0, 0.0, theme.slider_opacity*ctx->global_opacity_scale, true,false);

    // draw value
    char val_str[16] = {0};
    snprintf(val_str,15,val_format,val);
    Vector2f val_size = gfx_string_get_size(theme.text_scale, val_str);
    gfx_draw_string(ctx->curr.x + (ctx->curr.w-val_size.x)/2.0, ctx->curr.y + theme.text_padding, theme.color_text, theme.text_scale, 0.0, 1.0, false, false, 0, val_str);

    // draw label
    gfx_draw_string(ctx->curr.x + ctx->curr.w + theme.text_padding, ctx->curr.y + theme.text_padding, theme.color_text, theme.text_scale, 0.0, 1.0, false, false, 0, str);
}

static void draw_panel(uint32_t hash, bool moveable)
{
    gfx_draw_rect_xywh_tl(ctx->curr.x, ctx->curr.y, ctx->panel_props.panel_width, ctx->panel_props.panel_height, theme.color_panel, 1.0, 0.0, theme.panel_opacity*ctx->global_opacity_scale,true,false);

    if(moveable)
    {
        // draw header rect
        uint32_t color = is_highlighted(hash) ? theme.color_highlight : theme.color_highlight_subtle;
        gfx_draw_rect_xywh(ctx->curr.x+ctx->panel_props.panel_width/2.0, ctx->curr.y+theme.panel_header_height/2.0, ctx->panel_props.panel_width, theme.panel_header_height, color, 1.0, 0.0, theme.panel_opacity*ctx->global_opacity_scale,true,false);
    }
}

static void draw_checkbox(uint32_t hash, char* label, bool result)
{
    // draw button
    uint32_t check_color = theme.color_background;

    if(is_highlighted(hash))
    {
        check_color = theme.color_highlight;
    }
    if(is_active(hash))
    {
        check_color = theme.color_active;
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    // draw check
    float x = ctx->curr.x + theme.checkbox_size/2.0;
    float y = ctx->curr.y + theme.checkbox_size/2.0;

    gfx_draw_rect_xywh(x,y,theme.checkbox_size, theme.checkbox_size, check_color, 1.0, 0.0, 1.0*ctx->global_opacity_scale, false,false);
    gfx_draw_rect_xywh(x,y,0.65*theme.checkbox_size, 0.65*theme.checkbox_size, check_color, 1.0, 0.0, 1.0*ctx->global_opacity_scale, result,false);

    gfx_draw_string(ctx->curr.x + theme.checkbox_size + theme.text_padding, ctx->curr.y + (theme.checkbox_size - text_size.y)/2.0, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, label);

    ctx->curr.w = theme.checkbox_size + 2.0*theme.text_padding + text_size.x;
    ctx->curr.h = MAX(theme.checkbox_size, text_size.y)+2.0*theme.text_padding;
}

static void draw_number_box(uint32_t hash, char* label, Rect* r, int val, int max, char* format)
{
    uint32_t box_color = theme.color_background;

    if(is_highlighted(hash))
    {
        box_color = theme.color_highlight;
    }
    if(is_active(hash))
    {
        box_color = theme.color_active;
    }

    float pct = val/(float)max;

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, box_color, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);
    gfx_draw_rect_xywh(r->x + (r->w*pct)/2.0, r->y + r->h/2.0, r->w*pct, r->h, 0x00FFFFFF, 1.0, 0.0, 0.4*ctx->global_opacity_scale,true,false);

    char val_str[16] = {0};
    snprintf(val_str,15,format,val);
    Vector2f val_size = gfx_string_get_size(theme.text_scale, val_str);

    draw_label(r->x + (r->w - val_size.x)/2.0 , r->y + (r->h-val_size.y)/2.0, theme.color_text,val_str);
    draw_label(r->x + r->w + theme.text_padding, r->y + (r->h-val_size.y)/2.0, theme.color_text, label);
}

static void draw_text_box(uint32_t hash, char* label, Rect* r, char* text)
{
    uint32_t box_color = theme.color_background;

    if(is_highlighted(hash))
    {
        box_color = theme.color_highlight_subtle;
    }

    Vector2f label_size = gfx_string_get_size(theme.text_scale, label);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, text);

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, box_color, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    float min_x = ctx->text_box_props.text_cursor_x > ctx->text_box_props.text_cursor_x_held_from ? ctx->text_box_props.text_cursor_x_held_from : ctx->text_box_props.text_cursor_x;
    float sx = r->x + theme.text_padding + min_x;
    float sy = r->y;
    float sw = ABS(ctx->text_box_props.text_cursor_x_held_from - ctx->text_box_props.text_cursor_x);
    float sh = r->h;

    if(ctx->focused_text_id == hash)
        gfx_draw_rect_xywh(sx + sw/2.0, sy + sh/2.0, sw, sh, 0x000055CC, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    gfx_draw_string(r->x+theme.text_padding, r->y-(label_size.y-r->h)/2.0, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, text);

    if(ctx->focused_text_id == hash)
    {
        float x = r->x+theme.text_padding+ctx->text_box_props.text_cursor_x;
        float y = r->y-(theme.text_size_px-r->h)/2.0f;

        // line on text box
        if(ctx->text_box_props.cursor_show)
        {
            gfx_draw_rect_xywh(x,y+(theme.text_size_px*1.3)/2.0, 1, theme.text_size_px*1.3, theme.color_text, 1.0, 0.0, 1.0*ctx->global_opacity_scale, true,false);
        }
    }

    draw_label(r->x + r->w+theme.text_padding, r->y-(label_size.y-r->h)/2.0, theme.color_text, label);
}

static void draw_listbox(uint32_t hash, char* str, char* options[], int num_options, int selected_index, float scrollbar_offset, Rect* r, int max_display)
{
    if(!options)
        return;

    float box_height = NOMINAL_FONT_SIZE*theme.text_scale + 2*theme.text_padding;
    float listbox_height = box_height*max_display;

    // box
    bool show_scrollbar = num_options*box_height > listbox_height;
    int scrollbar_width = show_scrollbar ? theme.listbox_gutter_width : 0;

    int width = r->w - scrollbar_width;

    gfx_draw_rect_xywh(r->x + width/2.0, r->y + r->h/2.0, width, r->h, theme.color_background, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    bool is_active_scroll = (ctx->scroll_props.hash == hash);

    if(show_scrollbar)
    {
        // scrollbar gutter
        gfx_draw_rect_xywh(r->w+r->x-(theme.listbox_gutter_width/2.0), r->y + r->h/2.0, theme.listbox_gutter_width, r->h, theme.color_highlight_subtle, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

        // scrollbar
        int scrollbar_height = listbox_height*(listbox_height / (num_options*box_height));
        uint32_t color_slider = theme.color_slider;
        if(is_active_scroll && ctx->scroll_props.in_gutter)
        {
            color_slider = theme.color_highlight;
        }
        gfx_draw_rect_xywh(r->w+r->x-(theme.listbox_gutter_width/2.0), r->y + scrollbar_offset + scrollbar_height/2.0, theme.listbox_gutter_width, scrollbar_height, color_slider, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    }

    float ratio = (num_options*box_height) / listbox_height;
    float offset = ratio*scrollbar_offset;

    float y_diff = ctx->mouse_y + offset - r->y;
    int highlighted_index = floor(y_diff / box_height);

    if(ctx->mouse_x < ctx->curr.x || ctx->mouse_x > ctx->curr.x+theme.listbox_width)
        highlighted_index = -1;

    for(int i = 0; i < num_options; ++i)
    {
        if((i+1)*box_height < offset)
        {
            //printf("i*box_height: %f, offset: %f\n",i*box_height, offset);
            continue;
        }

        if(i*box_height - offset >= listbox_height)
            break;

        if(i == selected_index)
        {
            gfx_draw_rect_xywh(r->x + width/2.0, r->y + i*box_height + box_height/2.0 - offset, width, box_height, theme.color_active, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);
        }
        else if(i == highlighted_index && is_active_scroll && !ctx->scroll_props.in_gutter)
        {
            gfx_draw_rect_xywh(r->x + width/2.0, r->y + i*box_height + box_height/2.0 - offset, width, box_height, theme.color_highlight, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);
        }

        if(options[i])
        {
            gfx_draw_string(r->x+theme.text_padding, r->y+i*box_height - offset, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, options[i]);
        }
    }

    // masking rects

    gfx_draw_rect_xywh_tl(r->x, r->y-box_height, r->w, box_height, theme.color_panel, 1.0, 0.0, theme.panel_opacity*ctx->global_opacity_scale,true,false);
    gfx_draw_rect_xywh_tl(r->x, r->y+listbox_height, r->w, box_height, theme.color_panel, 1.0, 0.0, theme.panel_opacity*ctx->global_opacity_scale,true,false);

    // label
    int label_height = (NOMINAL_FONT_SIZE*theme.text_scale+theme.text_padding);
    gfx_draw_string(r->x, r->y-label_height, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, str);

    // selected / num options
    char stats[10] = {0};
    snprintf(stats,10,"%d/%d",selected_index+1,num_options);

    Vector2f stats_size = gfx_string_get_size(theme.text_scale, stats);

    gfx_draw_string(r->x+theme.listbox_width-stats_size.x, r->y-(NOMINAL_FONT_SIZE*theme.text_scale+theme.text_padding), theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, stats);


}

static void draw_dropdown(uint32_t hash, char* str, char* options[], int num_options, int selected_index, Rect* r)
{
    if(!options)
        return;

    // draw button
    uint32_t button_color = theme.color_background;

    if(is_active(hash))
    {
        button_color = theme.color_active;
    }
    else if(is_highlighted(hash))
    {
        button_color = theme.color_highlight;
    }

    float box_height = NOMINAL_FONT_SIZE*theme.text_scale + 2*theme.text_padding;

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + box_height/2.0, r->w, box_height, button_color, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

    int selection = 0;
    if(selected_index >= 0 && selected_index < num_options)
        selection = selected_index;

    bool active = is_active(hash);

    char* selected_text = options[selection];
    gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, selected_text);
    gfx_draw_string(r->x + theme.text_padding + r->w - 16, r->y + theme.text_padding, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, active ? "-" : "+");

    if(active)
    {
        float y_diff = ctx->mouse_y - r->y;
        int highlighted_index = floor(y_diff / box_height) - 1;

        gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, ctx->dropdown_props.size.h, theme.color_background, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);

        for(int i = 0; i < num_options; ++i)
        {
            float max_height = (i+1.5)*box_height;
            if(max_height > ctx->dropdown_props.size.h)
                break;

            if(is_highlighted(hash) && i == highlighted_index)
            {
                gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + (i+1.5)*box_height, r->w, box_height, theme.color_highlight, 1.0, 0.0, theme.button_opacity*ctx->global_opacity_scale, true,false);
            }

            if(options[i])
            {
                gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding + (i+1)*box_height, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, options[i]);
            }
        }
    }

    // label
    gfx_draw_string(r->x + r->w + theme.text_padding, r->y + theme.text_padding, theme.color_text, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, str);
}

static void draw_label(int x, int y, uint32_t color, char* label)
{
    gfx_draw_string(x, y, color, theme.text_scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, label);
}

static void draw_color_box(Rect* r, uint32_t color)
{
    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, color, 1.0, 0.0, 1.0*ctx->global_opacity_scale, true,false);
}

static void draw_tooltip()
{
    const float scale = 0.18;
    Vector2f text_size = gfx_string_get_size(scale,ctx->tooltip);

    Rect r = {ctx->mouse_x,ctx->mouse_y,text_size.x+2.0*theme.text_padding,text_size.y+2.0*theme.text_padding};

    r.y -= r.h;

    gfx_draw_rect_xywh(r.x + r.w/2.0, r.y + r.h/2.0, r.w, r.h, 0x00323232, 1.0, 0.0, 0.75*ctx->global_opacity_scale, true,false);
    gfx_draw_string(r.x+theme.text_padding, r.y-(text_size.y-r.h)/2.0, theme.color_text, scale, 0.0, 1.0*ctx->global_opacity_scale, false, false, 0, ctx->tooltip);
}
