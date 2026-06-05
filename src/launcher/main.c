#include "config.h"
#include "game_launcher.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#define WINDOW_W 800
#define WINDOW_H 600
#define MENU_H 24
#define STATUS_H 28
#define LAUNCHER_H 60
#define PET_W 640
#define PET_H 400
#define PET_BORDER 8
#define MAX_MENU_ITEMS 8
#define MAX_TOAST 160

typedef struct color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} color;

typedef struct ui_rect {
    int x;
    int y;
    int w;
    int h;
} ui_rect;

typedef struct menu_state {
    int open_menu;
    ui_rect menu_tabs[4];
    ui_rect dropdown;
    ui_rect items[MAX_MENU_ITEMS];
    int item_count;
} menu_state;

typedef struct app_state {
    SDL_Window *window;
    SDL_Renderer *renderer;
    app_config config;
    game_manifest manifest;
    menu_state menu;
    ui_rect screen_rect;
    ui_rect chip_rects[MAX_GAMES + 1];
    int active_game;
    int hover_chip;
    int fullscreen;
    int aspect_lock;
    int dragging;
    char current_title[GAME_TEXT];
    char screen_line_1[96];
    char screen_line_2[96];
    char screen_line_3[96];
    char toast[MAX_TOAST];
    Uint32 toast_until;
    Uint32 start_ticks;
} app_state;

static const color C_BG = {0x1a, 0x1a, 0x1a, 0xff};
static const color C_PANEL = {0x22, 0x22, 0x22, 0xff};
static const color C_SURFACE = {0x2a, 0x2a, 0x2a, 0xff};
static const color C_SURFACE_HOVER = {0x33, 0x33, 0x33, 0xff};
static const color C_TEXT = {0xe0, 0xe0, 0xe0, 0xff};
static const color C_MUTED = {0x88, 0x88, 0x88, 0xff};
static const color C_ACCENT = {0x33, 0xff, 0x33, 0xff};
static const color C_BORDER = {0x3a, 0x3a, 0x3a, 0xff};
static const color C_BLACK = {0x0a, 0x0a, 0x0a, 0xff};

static const char *MODEL_NAMES[] = {"PET 2001", "CBM 4032", "CBM 8032"};
static const char *SPEED_NAMES[] = {"100%", "200%", "400%"};

static int point_in_rect(int x, int y, ui_rect r)
{
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static void set_color(SDL_Renderer *r, color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static SDL_Rect sdl_rect(ui_rect r)
{
    SDL_Rect out;
    out.x = r.x;
    out.y = r.y;
    out.w = r.w;
    out.h = r.h;
    return out;
}

static void fill_rect(SDL_Renderer *r, ui_rect rect, color c)
{
    SDL_Rect sr = sdl_rect(rect);
    set_color(r, c);
    SDL_RenderFillRect(r, &sr);
}

static void draw_rect(SDL_Renderer *r, ui_rect rect, color c)
{
    SDL_Rect sr = sdl_rect(rect);
    set_color(r, c);
    SDL_RenderDrawRect(r, &sr);
}

static void draw_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, color c)
{
    set_color(r, c);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

static const unsigned char *glyph_rows(char c)
{
    static const unsigned char space[7] = {0, 0, 0, 0, 0, 0, 0};
    static const unsigned char unknown[7] = {14, 17, 1, 6, 4, 0, 4};
    static const unsigned char period[7] = {0, 0, 0, 0, 0, 12, 12};
    static const unsigned char comma[7] = {0, 0, 0, 0, 0, 12, 8};
    static const unsigned char dash[7] = {0, 0, 0, 31, 0, 0, 0};
    static const unsigned char star[7] = {0, 21, 14, 31, 14, 21, 0};
    static const unsigned char dollar[7] = {4, 15, 20, 14, 5, 30, 4};
    static const unsigned char apostrophe[7] = {4, 4, 8, 0, 0, 0, 0};
    static const unsigned char lbracket[7] = {14, 8, 8, 8, 8, 8, 14};
    static const unsigned char rbracket[7] = {14, 2, 2, 2, 2, 2, 14};
    static const unsigned char font[59][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,30,1,1,17,14},
        {6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,2,12},{0,4,4,0,4,4,0},{0,4,4,0,4,4,8},
        {2,4,8,16,8,4,2},{0,0,31,0,31,0,0},{8,4,2,1,2,4,8},
        {14,17,1,2,4,0,4},{14,17,23,21,23,16,14},{14,17,17,31,17,17,17},
        {30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},
        {17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},
        {17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},
        {31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},
        {31,1,2,4,8,16,31},{14,8,8,8,8,8,14},{16,8,4,2,1,0,0},
        {14,2,2,2,2,2,14},{4,10,17,0,0,0,0},{0,0,0,0,0,0,31},
        {8,4,2,0,0,0,0},{0,0,14,1,15,17,15},{16,16,30,17,17,17,30},
        {0,0,14,16,16,17,14},{1,1,15,17,17,17,15},{0,0,14,17,31,16,14},
        {6,8,8,28,8,8,8}
    };

    if (c == ' ') return space;
    if (c == '.') return period;
    if (c == ',') return comma;
    if (c == '-') return dash;
    if (c == '*') return star;
    if (c == '$') return dollar;
    if (c == '\'') return apostrophe;
    if (c == '[') return lbracket;
    if (c == ']') return rbracket;
    if (c >= '0' && c <= '9') return font[c - '0'];
    if (c >= ':' && c <= '`') return font[10 + (c - ':')];
    if (c >= 'a' && c <= 'f') return font[49 + (c - 'a')];
    if (c >= 'A' && c <= 'Z') return font[17 + (c - 'A')];
    return unknown;
}

static void draw_text(SDL_Renderer *r, int x, int y, const char *text, int scale, color c)
{
    int cx = x;
    size_t i;
    for (i = 0; text[i] != '\0'; i++) {
        const unsigned char *rows;
        int row;
        char ch = text[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - 32);
        }
        rows = glyph_rows(ch);
        for (row = 0; row < 7; row++) {
            int col;
            for (col = 0; col < 5; col++) {
                if ((rows[row] & (1 << (4 - col))) != 0) {
                    fill_rect(r, (ui_rect){cx + col * scale, y + row * scale, scale, scale}, c);
                }
            }
        }
        cx += 6 * scale;
    }
}

static int text_width(const char *text, int scale)
{
    return (int)strlen(text) * 6 * scale;
}

static void toast(app_state *app, const char *message)
{
    strncpy(app->toast, message, sizeof(app->toast) - 1);
    app->toast[sizeof(app->toast) - 1] = '\0';
    app->toast_until = SDL_GetTicks() + 3500;
}

static void set_basic_screen(app_state *app)
{
    strcpy(app->screen_line_1, "*** COMMODORE BASIC ***");
    strcpy(app->screen_line_2, "31743 BYTES FREE");
    strcpy(app->screen_line_3, "READY.");
}

static void set_running_screen(app_state *app, const char *title, unsigned int load_addr)
{
    snprintf(app->screen_line_1, sizeof(app->screen_line_1), "LOAD \"%s\",8", title);
    snprintf(app->screen_line_2, sizeof(app->screen_line_2), "LOADED AT $%04X", load_addr);
    strcpy(app->screen_line_3, "RUN");
}

static void layout(app_state *app)
{
    int w;
    int h;
    int content_h;
    int display_h;
    int screen_total_w = PET_W + PET_BORDER * 2;
    int screen_total_h = PET_H + PET_BORDER * 2;
    int chip_y;
    int chip_w;
    int gap = 10;
    int i;

    SDL_GetWindowSize(app->window, &w, &h);
    content_h = h - MENU_H - LAUNCHER_H - STATUS_H;
    display_h = content_h;
    app->screen_rect.w = screen_total_w;
    app->screen_rect.h = screen_total_h;
    if (app->screen_rect.w > w - 32) {
        app->screen_rect.w = w - 32;
    }
    if (app->screen_rect.h > display_h - 24) {
        app->screen_rect.h = display_h - 24;
    }
    app->screen_rect.x = (w - app->screen_rect.w) / 2;
    app->screen_rect.y = MENU_H + (display_h - app->screen_rect.h) / 2;

    app->menu.menu_tabs[0] = (ui_rect){8, 0, 44, MENU_H};
    app->menu.menu_tabs[1] = (ui_rect){58, 0, 76, MENU_H};
    app->menu.menu_tabs[2] = (ui_rect){140, 0, 46, MENU_H};
    app->menu.menu_tabs[3] = (ui_rect){192, 0, 48, MENU_H};

    chip_y = h - STATUS_H - LAUNCHER_H + 10;
    chip_w = (w - 32 - gap * 5) / 6;
    if (chip_w < 82) {
        chip_w = 82;
    }
    for (i = 0; i < MAX_GAMES + 1; i++) {
        app->chip_rects[i] = (ui_rect){16 + i * (chip_w + gap), chip_y, chip_w, 40};
    }
}

static void open_program(app_state *app, const char *path)
{
    unsigned int load_addr;
    char message[MAX_TOAST];

    if (!prg_read_load_addr(path, &load_addr)) {
        snprintf(message, sizeof(message), "Could not read '%s' as a .prg", path_basename(path));
        toast(app, message);
        return;
    }

    app->active_game = -1;
    strncpy(app->current_title, path_basename(path), sizeof(app->current_title) - 1);
    app->current_title[sizeof(app->current_title) - 1] = '\0';
    set_running_screen(app, app->current_title, load_addr);
    config_add_recent(&app->config, path);
    config_save(&app->config, "config.ini");
    snprintf(message, sizeof(message), "Loaded '%s' at $%04X. Running...", path_basename(path), load_addr);
    toast(app, message);
}

static void launch_game(app_state *app, int index)
{
    char message[MAX_TOAST];
    if (index < 0 || index >= app->manifest.count) {
        return;
    }

    app->active_game = index;
    strncpy(app->current_title, app->manifest.games[index].title, sizeof(app->current_title) - 1);
    app->current_title[sizeof(app->current_title) - 1] = '\0';
    set_running_screen(app, app->current_title, app->manifest.games[index].load_addr);
    snprintf(message, sizeof(message), "Loading %s...", app->current_title);
    toast(app, message);
}

static int browse_for_prg(char *out_path, size_t out_size)
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char file_name[MAX_PATH] = "";
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Commodore PRG (*.prg)\0*.prg\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "prg";
    if (GetOpenFileNameA(&ofn)) {
        strncpy(out_path, file_name, out_size - 1);
        out_path[out_size - 1] = '\0';
        return 1;
    }
    return 0;
#else
    (void)out_path;
    (void)out_size;
    return 0;
#endif
}

static void choose_program(app_state *app)
{
    char path[MAX_PATH_TEXT];
    if (browse_for_prg(path, sizeof(path))) {
        open_program(app, path);
    } else {
        toast(app, "File dialog is available in the Windows build. Drag a .prg here.");
    }
}

static void reset_machine(app_state *app)
{
    app->active_game = -1;
    strcpy(app->current_title, "BASIC");
    set_basic_screen(app);
    toast(app, "Machine reset.");
}

static void draw_menu_dropdown(app_state *app)
{
    SDL_Renderer *r = app->renderer;
    const char *items[MAX_MENU_ITEMS];
    char recent_labels[MAX_RECENT_FILES][GAME_TEXT];
    int count = 0;
    int menu = app->menu.open_menu;
    int i;
    int x;
    int w = 190;

    if (menu < 0) {
        return;
    }

    if (menu == 0) {
        items[count++] = "Load .prg    Ctrl+O";
        if (app->config.recent_count == 0) {
            items[count++] = "Recent: none";
        } else {
            for (i = 0; i < app->config.recent_count && count < MAX_MENU_ITEMS - 2; i++) {
                snprintf(recent_labels[i], sizeof(recent_labels[i]), "Recent: %s",
                         path_basename(app->config.recent_files[i]));
                items[count++] = recent_labels[i];
            }
        }
        items[count++] = "Reset        Ctrl+R";
        items[count++] = "Exit";
    } else if (menu == 1) {
        items[count++] = "PET 2001";
        items[count++] = "CBM 4032";
        items[count++] = "CBM 8032";
        items[count++] = "Speed 100%";
        items[count++] = "Speed 200%";
        items[count++] = "Speed 400%";
    } else if (menu == 2) {
        items[count++] = app->config.crt_enabled ? "CRT Effect [x]" : "CRT Effect [ ]";
        items[count++] = app->fullscreen ? "Fullscreen [x]" : "Fullscreen [ ]";
        items[count++] = app->aspect_lock ? "Aspect Lock [x]" : "Aspect Lock [ ]";
    } else {
        items[count++] = "About PET Retro Mini";
        items[count++] = "PET Keyboard Reference";
    }

    x = app->menu.menu_tabs[menu].x;
    app->menu.dropdown = (ui_rect){x, MENU_H, w, count * 26 + 8};
    app->menu.item_count = count;
    fill_rect(r, app->menu.dropdown, C_SURFACE);
    draw_rect(r, app->menu.dropdown, C_BORDER);
    for (i = 0; i < count; i++) {
        app->menu.items[i] = (ui_rect){x + 4, MENU_H + 4 + i * 26, w - 8, 24};
        draw_text(r, x + 10, MENU_H + 10 + i * 26, items[i], 1, C_TEXT);
    }
}

static void draw_ui(app_state *app)
{
    SDL_Renderer *r = app->renderer;
    int w;
    int h;
    int i;
    Uint32 now = SDL_GetTicks();
    int cursor_on = ((now / 500) % 2) == 0;
    ui_rect pet_inner;
    char status[160];

    SDL_GetWindowSize(app->window, &w, &h);
    layout(app);

    fill_rect(r, (ui_rect){0, 0, w, h}, C_BG);
    fill_rect(r, (ui_rect){0, 0, w, MENU_H}, C_BG);
    draw_line(r, 0, MENU_H - 1, w, MENU_H - 1, C_BORDER);

    draw_text(r, 14, 7, "File", 1, C_TEXT);
    draw_text(r, 64, 7, "Machine", 1, C_TEXT);
    draw_text(r, 146, 7, "View", 1, C_TEXT);
    draw_text(r, 198, 7, "Help", 1, C_TEXT);

    if (app->dragging) {
        draw_rect(r, (ui_rect){2, MENU_H + 2, w - 4, h - MENU_H - 4}, C_ACCENT);
    }

    fill_rect(r, app->screen_rect, (color){0x11, 0x11, 0x11, 0xff});
    draw_rect(r, app->screen_rect, C_BORDER);
    pet_inner = (ui_rect){app->screen_rect.x + PET_BORDER, app->screen_rect.y + PET_BORDER,
                          app->screen_rect.w - PET_BORDER * 2, app->screen_rect.h - PET_BORDER * 2};
    fill_rect(r, pet_inner, C_BLACK);

    if (app->config.crt_enabled) {
        int ring;
        for (ring = 0; ring < 56; ring++) {
            color glow = {0x10, (Uint8)(0x22 + ring / 3), 0x10, 0xff};
            ui_rect gr = {pet_inner.x + ring, pet_inner.y + ring,
                          pet_inner.w - ring * 2, pet_inner.h - ring * 2};
            if (gr.w <= 0 || gr.h <= 0) {
                break;
            }
            draw_rect(r, gr, glow);
        }
    }

    draw_text(r, pet_inner.x + 22, pet_inner.y + 24, app->screen_line_1, 3, C_ACCENT);
    draw_text(r, pet_inner.x + 22, pet_inner.y + 64, app->screen_line_2, 2, C_ACCENT);
    draw_text(r, pet_inner.x + 22, pet_inner.y + 104, app->screen_line_3, 3, C_ACCENT);
    if (cursor_on) {
        int cx = pet_inner.x + 22 + text_width(app->screen_line_3, 3) + 8;
        fill_rect(r, (ui_rect){cx, pet_inner.y + 104, 18, 22}, C_ACCENT);
    }

    if (app->config.crt_enabled) {
        int y;
        for (y = pet_inner.y; y < pet_inner.y + pet_inner.h; y += 4) {
            fill_rect(r, (ui_rect){pet_inner.x, y + 2, pet_inner.w, 2}, (color){0, 0, 0, 0x55});
        }
    }

    if (now < app->toast_until && app->toast[0] != '\0') {
        int tw = text_width(app->toast, 1) + 32;
        ui_rect tr = {(w - tw) / 2, h - STATUS_H - LAUNCHER_H - 48, tw, 32};
        fill_rect(r, tr, (color){0x2a, 0x2a, 0x2a, 0xff});
        draw_rect(r, tr, C_BORDER);
        draw_text(r, tr.x + 16, tr.y + 10, app->toast, 1, C_TEXT);
    }

    fill_rect(r, (ui_rect){0, h - STATUS_H - LAUNCHER_H, w, LAUNCHER_H}, C_PANEL);
    draw_line(r, 0, h - STATUS_H - LAUNCHER_H, w, h - STATUS_H - LAUNCHER_H, C_BORDER);
    for (i = 0; i < app->manifest.count; i++) {
        ui_rect chip = app->chip_rects[i];
        color bg = i == app->hover_chip ? C_SURFACE_HOVER : C_SURFACE;
        color fg = i == app->active_game ? C_ACCENT : C_TEXT;
        if (i == app->hover_chip) {
            chip.y -= 2;
        }
        fill_rect(r, chip, bg);
        draw_rect(r, chip, i == app->active_game ? C_ACCENT : C_BORDER);
        draw_text(r, chip.x + (chip.w - text_width(app->manifest.games[i].title, 1)) / 2,
                  chip.y + 14, app->manifest.games[i].title, 1, fg);
    }

    i = app->manifest.count;
    fill_rect(r, app->chip_rects[i], C_PANEL);
    draw_rect(r, app->chip_rects[i], C_MUTED);
    draw_text(r, app->chip_rects[i].x + 12, app->chip_rects[i].y + 14, "Load .prg...", 1, C_MUTED);

    fill_rect(r, (ui_rect){0, h - STATUS_H, w, STATUS_H}, C_PANEL);
    draw_line(r, 0, h - STATUS_H, w, h - STATUS_H, C_BORDER);
    draw_text(r, 12, h - 18, MODEL_NAMES[app->config.model_index], 1, C_MUTED);
    draw_text(r, (w - text_width(app->current_title, 1)) / 2, h - 18, app->current_title, 1, C_TEXT);
    snprintf(status, sizeof(status), "Speed %s", SPEED_NAMES[app->config.speed_index]);
    draw_text(r, w - text_width(status, 1) - 12, h - 18, status, 1, C_MUTED);

    draw_menu_dropdown(app);
}

static void activate_menu_item(app_state *app, int index)
{
    int menu = app->menu.open_menu;
    if (menu == 0) {
        if (index == 0) {
            choose_program(app);
        }
        if (index > 0 && index <= app->config.recent_count) {
            open_program(app, app->config.recent_files[index - 1]);
        }
        if (index == app->config.recent_count + 1 || (app->config.recent_count == 0 && index == 2)) {
            reset_machine(app);
        }
        if (index == app->config.recent_count + 2 || (app->config.recent_count == 0 && index == 3)) {
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_QUIT;
            SDL_PushEvent(&ev);
        }
    } else if (menu == 1) {
        if (index >= 0 && index <= 2) {
            app->config.model_index = index;
            config_save(&app->config, "config.ini");
        } else if (index >= 3 && index <= 5) {
            app->config.speed_index = index - 3;
            config_save(&app->config, "config.ini");
        }
    } else if (menu == 2) {
        if (index == 0) {
            app->config.crt_enabled = !app->config.crt_enabled;
            config_save(&app->config, "config.ini");
        } else if (index == 1) {
            app->fullscreen = !app->fullscreen;
            SDL_SetWindowFullscreen(app->window, app->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        } else if (index == 2) {
            app->aspect_lock = !app->aspect_lock;
        }
    } else if (menu == 3) {
        toast(app, index == 0 ? "PET Retro Mini Phase 1 shell." : "Keyboard reference arrives in Phase 2.");
    }
    app->menu.open_menu = -1;
}

static void handle_key(app_state *app, SDL_KeyboardEvent *key)
{
    int ctrl = (key->keysym.mod & KMOD_CTRL) != 0;
    if (key->type != SDL_KEYDOWN) {
        return;
    }
    if (ctrl && key->keysym.sym == SDLK_o) {
        choose_program(app);
    } else if (ctrl && key->keysym.sym == SDLK_r) {
        reset_machine(app);
    } else if (key->keysym.sym == SDLK_F11) {
        app->fullscreen = !app->fullscreen;
        SDL_SetWindowFullscreen(app->window, app->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    } else if (key->keysym.sym == SDLK_ESCAPE) {
        app->menu.open_menu = -1;
    }
}

static int handle_event(app_state *app, SDL_Event *event)
{
    int i;

    if (event->type == SDL_QUIT) {
        return 0;
    }
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
        handle_key(app, &event->key);
    } else if (event->type == SDL_MOUSEMOTION) {
        app->hover_chip = -1;
        for (i = 0; i < app->manifest.count + 1; i++) {
            if (point_in_rect(event->motion.x, event->motion.y, app->chip_rects[i])) {
                app->hover_chip = i;
            }
        }
    } else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        int x = event->button.x;
        int y = event->button.y;
        for (i = 0; i < 4; i++) {
            if (point_in_rect(x, y, app->menu.menu_tabs[i])) {
                app->menu.open_menu = app->menu.open_menu == i ? -1 : i;
                return 1;
            }
        }
        if (app->menu.open_menu >= 0) {
            for (i = 0; i < app->menu.item_count; i++) {
                if (point_in_rect(x, y, app->menu.items[i])) {
                    activate_menu_item(app, i);
                    return 1;
                }
            }
            app->menu.open_menu = -1;
        }
        for (i = 0; i < app->manifest.count; i++) {
            if (point_in_rect(x, y, app->chip_rects[i])) {
                launch_game(app, i);
                return 1;
            }
        }
        if (point_in_rect(x, y, app->chip_rects[app->manifest.count])) {
            choose_program(app);
        }
    } else if (event->type == SDL_DROPFILE) {
        open_program(app, event->drop.file);
        SDL_free(event->drop.file);
        app->dragging = 0;
    } else if (event->type == SDL_DROPBEGIN) {
        app->dragging = 1;
    } else if (event->type == SDL_DROPCOMPLETE) {
        app->dragging = 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    app_state app;
    int running = 1;
    (void)argc;
    (void)argv;

    memset(&app, 0, sizeof(app));
    app.active_game = -1;
    app.hover_chip = -1;
    app.menu.open_menu = -1;
    app.aspect_lock = 1;
    strcpy(app.current_title, "BASIC");
    set_basic_screen(&app);

    config_load(&app.config, "config.ini");
    manifest_load(&app.manifest, "games/manifest.json");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    app.window = SDL_CreateWindow("PET Retro Mini", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_W, WINDOW_H, SDL_WINDOW_RESIZABLE);
    if (app.window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (app.renderer == NULL) {
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (app.renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    app.start_ticks = SDL_GetTicks();
    toast(&app, "Welcome to PET Retro Mini. Pick a game below, or explore BASIC.");

    while (running) {
        SDL_Event event;
        Uint32 frame_start = SDL_GetTicks();
        while (SDL_PollEvent(&event)) {
            running = handle_event(&app, &event);
        }
        draw_ui(&app);
        SDL_RenderPresent(app.renderer);
        if (SDL_GetTicks() - frame_start < 16) {
            SDL_Delay(16 - (SDL_GetTicks() - frame_start));
        }
    }

    config_save(&app.config, "config.ini");
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 0;
}
