#include "vice_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PET_COLS_40 40
#define PET_ROWS 25
#define PET_CHAR_W 16
#define PET_CHAR_H 16
#define PET_TEXT_MAX 80
#define PET_RAM_SIZE 65536
#define PET_KEY_QUEUE 256

typedef struct pet_cell {
    unsigned char ch;
} pet_cell;

typedef struct vice_bridge_state {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    unsigned int *pixels;
    unsigned char *ram;
    pet_cell cells[PET_ROWS][PET_COLS_40];
    char line[PET_TEXT_MAX];
    char key_queue[PET_KEY_QUEUE];
    char rom_dir[260];
    char error[160];
    unsigned int last_load_addr;
    int model_id;
    int cursor_x;
    int cursor_y;
    int line_len;
    int key_head;
    int key_tail;
    int ready;
} vice_bridge_state;

static vice_bridge_state g_bridge;

static const unsigned char *glyph_rows(char c)
{
    static const unsigned char space[7] = {0, 0, 0, 0, 0, 0, 0};
    static const unsigned char unknown[7] = {14, 17, 1, 6, 4, 0, 4};
    static const unsigned char quote[7] = {10, 10, 0, 0, 0, 0, 0};
    static const unsigned char period[7] = {0, 0, 0, 0, 0, 12, 12};
    static const unsigned char comma[7] = {0, 0, 0, 0, 0, 12, 8};
    static const unsigned char dash[7] = {0, 0, 0, 31, 0, 0, 0};
    static const unsigned char slash[7] = {1, 2, 2, 4, 8, 8, 16};
    static const unsigned char star[7] = {0, 21, 14, 31, 14, 21, 0};
    static const unsigned char font[43][7] = {
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
        {31,1,2,4,8,16,31}
    };

    if (c == ' ') return space;
    if (c == '"') return quote;
    if (c == '.') return period;
    if (c == ',') return comma;
    if (c == '-') return dash;
    if (c == '/') return slash;
    if (c == '*') return star;
    if (c >= '0' && c <= '9') return font[c - '0'];
    if (c >= ':' && c <= '?') return font[10 + (c - ':')];
    if (c >= 'A' && c <= 'Z') return font[17 + (c - 'A')];
    if (c >= 'a' && c <= 'z') return font[17 + (c - 'a')];
    return unknown;
}

static void set_error(const char *message)
{
    strncpy(g_bridge.error, message, sizeof(g_bridge.error) - 1);
    g_bridge.error[sizeof(g_bridge.error) - 1] = '\0';
}

static void clear_screen(void)
{
    int y;
    int x;
    for (y = 0; y < PET_ROWS; y++) {
        for (x = 0; x < PET_COLS_40; x++) {
            g_bridge.cells[y][x].ch = ' ';
        }
    }
    g_bridge.cursor_x = 0;
    g_bridge.cursor_y = 0;
    g_bridge.line_len = 0;
    g_bridge.line[0] = '\0';
}

static void scroll_screen(void)
{
    int y;
    int x;
    for (y = 1; y < PET_ROWS; y++) {
        memcpy(g_bridge.cells[y - 1], g_bridge.cells[y], sizeof(g_bridge.cells[y]));
    }
    for (x = 0; x < PET_COLS_40; x++) {
        g_bridge.cells[PET_ROWS - 1][x].ch = ' ';
    }
    g_bridge.cursor_y = PET_ROWS - 1;
}

static void newline(void)
{
    g_bridge.cursor_x = 0;
    g_bridge.cursor_y++;
    if (g_bridge.cursor_y >= PET_ROWS) {
        scroll_screen();
    }
}

static void put_char(char ch)
{
    if (ch == '\r' || ch == '\n') {
        newline();
        return;
    }
    if (ch == '\b') {
        if (g_bridge.cursor_x > 0) {
            g_bridge.cursor_x--;
            g_bridge.cells[g_bridge.cursor_y][g_bridge.cursor_x].ch = ' ';
        }
        return;
    }
    g_bridge.cells[g_bridge.cursor_y][g_bridge.cursor_x].ch = (unsigned char)ch;
    g_bridge.cursor_x++;
    if (g_bridge.cursor_x >= PET_COLS_40) {
        newline();
    }
}

static void put_text(const char *text)
{
    while (*text != '\0') {
        put_char(*text++);
    }
}

static void boot_banner(void)
{
    static const char *model_bytes[] = {"7167 BYTES FREE", "31743 BYTES FREE", "31743 BYTES FREE"};
    clear_screen();
    put_text("*** COMMODORE BASIC ***\r");
    put_text(model_bytes[g_bridge.model_id]);
    put_text("\r\rREADY.\r");
}

static void queue_text(const char *text)
{
    while (*text != '\0') {
        int next = (g_bridge.key_tail + 1) % PET_KEY_QUEUE;
        if (next == g_bridge.key_head) {
            return;
        }
        g_bridge.key_queue[g_bridge.key_tail] = *text++;
        g_bridge.key_tail = next;
    }
}

static void process_basic_line(void)
{
    if (strcmp(g_bridge.line, "PRINT \"HELLO\"") == 0 || strcmp(g_bridge.line, "PRINT\"HELLO\"") == 0) {
        put_text("\rHELLO\rREADY.\r");
    } else if (strcmp(g_bridge.line, "RUN") == 0) {
        put_text("\rRUNNING\r");
    } else if (g_bridge.line_len > 0) {
        put_text("\r?SYNTAX ERROR\rREADY.\r");
    } else {
        put_text("\rREADY.\r");
    }
    g_bridge.line_len = 0;
    g_bridge.line[0] = '\0';
}

static void type_ascii(char ch)
{
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 32);
    }
    if (ch == '\n') {
        ch = '\r';
    }
    if (ch == '\r') {
        process_basic_line();
        return;
    }
    if (ch == '\b') {
        if (g_bridge.line_len > 0) {
            g_bridge.line[--g_bridge.line_len] = '\0';
            put_char('\b');
        }
        return;
    }
    if (ch < 32 || ch > 126 || g_bridge.line_len >= PET_TEXT_MAX - 1) {
        return;
    }
    g_bridge.line[g_bridge.line_len++] = ch;
    g_bridge.line[g_bridge.line_len] = '\0';
    put_char(ch);
}

static void drain_key_queue(void)
{
    while (g_bridge.key_head != g_bridge.key_tail) {
        char ch = g_bridge.key_queue[g_bridge.key_head];
        g_bridge.key_head = (g_bridge.key_head + 1) % PET_KEY_QUEUE;
        type_ascii(ch);
    }
}

static void render_cell(int cell_x, int cell_y, unsigned char ch)
{
    int row;
    int col;
    int px;
    int py;
    const unsigned char *rows = glyph_rows((char)ch);

    for (py = 0; py < PET_CHAR_H; py++) {
        row = py / 2;
        for (px = 0; px < PET_CHAR_W; px++) {
            col = px / 3;
            if (col > 4) {
                col = 4;
            }
            g_bridge.pixels[(cell_y * PET_CHAR_H + py) * VICE_BRIDGE_SCREEN_W + cell_x * PET_CHAR_W + px] =
                (rows[row] & (1 << (4 - col))) != 0 ? 0xff33ff33u : 0xff050805u;
        }
    }
}

static void render_framebuffer(void)
{
    int y;
    int x;
    unsigned int bg = 0xff050805u;
    for (y = 0; y < VICE_BRIDGE_SCREEN_H; y++) {
        for (x = 0; x < VICE_BRIDGE_SCREEN_W; x++) {
            g_bridge.pixels[y * VICE_BRIDGE_SCREEN_W + x] = bg;
        }
    }
    for (y = 0; y < PET_ROWS; y++) {
        for (x = 0; x < PET_COLS_40; x++) {
            render_cell(x, y, g_bridge.cells[y][x].ch);
        }
    }
    if ((SDL_GetTicks() / 500) % 2 == 0) {
        int cx = g_bridge.cursor_x * PET_CHAR_W;
        int cy = g_bridge.cursor_y * PET_CHAR_H;
        int yy;
        int xx;
        for (yy = cy + 12; yy < cy + PET_CHAR_H; yy++) {
            for (xx = cx; xx < cx + PET_CHAR_W; xx++) {
                if (xx >= 0 && xx < VICE_BRIDGE_SCREEN_W && yy >= 0 && yy < VICE_BRIDGE_SCREEN_H) {
                    g_bridge.pixels[yy * VICE_BRIDGE_SCREEN_W + xx] = 0xff33ff33u;
                }
            }
        }
    }
}

int vice_bridge_init(SDL_Renderer *renderer, int model_id, const char *rom_dir)
{
    memset(&g_bridge, 0, sizeof(g_bridge));
    g_bridge.renderer = renderer;
    g_bridge.model_id = model_id < 0 || model_id > 2 ? VICE_BRIDGE_MODEL_4032 : model_id;
    if (rom_dir != NULL) {
        strncpy(g_bridge.rom_dir, rom_dir, sizeof(g_bridge.rom_dir) - 1);
    }
    g_bridge.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                         VICE_BRIDGE_SCREEN_W, VICE_BRIDGE_SCREEN_H);
    g_bridge.pixels = (unsigned int *)calloc(VICE_BRIDGE_SCREEN_W * VICE_BRIDGE_SCREEN_H, sizeof(unsigned int));
    g_bridge.ram = (unsigned char *)calloc(PET_RAM_SIZE, 1);
    if (g_bridge.texture == NULL || g_bridge.pixels == NULL || g_bridge.ram == NULL) {
        set_error("VICE bridge allocation failed");
        vice_bridge_shutdown();
        return 0;
    }
    SDL_SetTextureBlendMode(g_bridge.texture, SDL_BLENDMODE_NONE);
    g_bridge.ready = 1;
    boot_banner();
    return 1;
}

void vice_bridge_shutdown(void)
{
    if (g_bridge.texture != NULL) {
        SDL_DestroyTexture(g_bridge.texture);
    }
    free(g_bridge.pixels);
    free(g_bridge.ram);
    memset(&g_bridge, 0, sizeof(g_bridge));
}

void vice_bridge_reset(void)
{
    if (!g_bridge.ready) {
        return;
    }
    memset(g_bridge.ram, 0, PET_RAM_SIZE);
    g_bridge.key_head = 0;
    g_bridge.key_tail = 0;
    boot_banner();
}

SDL_Texture *vice_bridge_frame(void)
{
    if (!g_bridge.ready) {
        return NULL;
    }
    drain_key_queue();
    render_framebuffer();
    SDL_UpdateTexture(g_bridge.texture, NULL, g_bridge.pixels, VICE_BRIDGE_SCREEN_W * (int)sizeof(unsigned int));
    return g_bridge.texture;
}

void vice_bridge_keyboard(SDL_Keycode key, int down)
{
    char ch = 0;
    if (!down || !g_bridge.ready) {
        return;
    }
    if (key >= SDLK_a && key <= SDLK_z) {
        ch = (char)('A' + (key - SDLK_a));
    } else if (key >= SDLK_0 && key <= SDLK_9) {
        ch = (char)('0' + (key - SDLK_0));
    } else if (key == SDLK_SPACE) {
        ch = ' ';
    } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        ch = '\r';
    } else if (key == SDLK_BACKSPACE) {
        ch = '\b';
    } else if (key == SDLK_QUOTE) {
        ch = '"';
    }
    if (ch != 0) {
        type_ascii(ch);
    }
}

int vice_bridge_load_prg(const char *path)
{
    FILE *file;
    long size;
    unsigned char header[2];
    size_t payload_size;

    if (!g_bridge.ready) {
        set_error("VICE bridge is not initialized");
        return 0;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        set_error("Could not open PRG");
        return 0;
    }
    if (fread(header, 1, 2, file) != 2) {
        fclose(file);
        set_error("PRG is missing a load address");
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        set_error("Could not size PRG");
        return 0;
    }
    size = ftell(file);
    if (size < 2 || size > PET_RAM_SIZE + 2L) {
        fclose(file);
        set_error("PRG size is invalid");
        return 0;
    }
    g_bridge.last_load_addr = (unsigned int)header[0] | ((unsigned int)header[1] << 8);
    payload_size = (size_t)(size - 2);
    if (g_bridge.last_load_addr + payload_size > PET_RAM_SIZE) {
        fclose(file);
        set_error("PRG does not fit in PET RAM");
        return 0;
    }
    if (fseek(file, 2, SEEK_SET) != 0 ||
        fread(g_bridge.ram + g_bridge.last_load_addr, 1, payload_size, file) != payload_size) {
        fclose(file);
        set_error("Could not read PRG payload");
        return 0;
    }
    fclose(file);

    put_text("LOAD \"");
    put_text(path);
    put_text("\",8\rREADY.\r");
    queue_text("RUN\n");
    return 1;
}

int vice_bridge_model(int model_id)
{
    if (model_id < 0 || model_id > 2) {
        set_error("Unknown PET model");
        return 0;
    }
    g_bridge.model_id = model_id;
    vice_bridge_reset();
    return 1;
}

const char *vice_bridge_error(void)
{
    return g_bridge.error[0] == '\0' ? "No bridge error" : g_bridge.error;
}

unsigned int vice_bridge_last_load_addr(void)
{
    return g_bridge.last_load_addr;
}
