#include "emulator_runtime.h"

#include "pet.h"
#include "render.h"

#include <stdlib.h>
#include <string.h>

typedef struct runtime_key_map {
    SDL_Keycode key;
    int row;
    int col;
} runtime_key_map;

static const runtime_key_map SDL_KEYS[] = {
    {SDLK_1,0,0},{SDLK_2,0,1},{SDLK_3,0,2},{SDLK_4,0,3},{SDLK_5,0,4},{SDLK_6,0,5},{SDLK_7,0,6},{SDLK_8,0,7},
    {SDLK_q,1,0},{SDLK_w,1,1},{SDLK_e,1,2},{SDLK_r,1,3},{SDLK_t,1,4},{SDLK_y,1,5},{SDLK_u,1,6},{SDLK_i,1,7},
    {SDLK_a,2,0},{SDLK_s,2,1},{SDLK_d,2,2},{SDLK_f,2,3},{SDLK_g,2,4},{SDLK_h,2,5},{SDLK_j,2,6},{SDLK_k,2,7},
    {SDLK_z,3,0},{SDLK_x,3,1},{SDLK_c,3,2},{SDLK_v,3,3},{SDLK_b,3,4},{SDLK_n,3,5},{SDLK_m,3,6},{SDLK_COMMA,3,7},
    {SDLK_9,4,0},{SDLK_0,4,1},{SDLK_MINUS,4,2},{SDLK_EQUALS,4,3},{SDLK_RETURN,4,4},{SDLK_BACKSPACE,4,5},{SDLK_SPACE,4,6},
    {SDLK_o,5,0},{SDLK_p,5,1},{SDLK_LEFTBRACKET,5,2},{SDLK_RIGHTBRACKET,5,3},
    {SDLK_l,6,0},{SDLK_SEMICOLON,6,1},{SDLK_QUOTE,6,2},
    {SDLK_PERIOD,7,0},{SDLK_SLASH,7,1},
    {SDLK_KP_ENTER,4,4},{SDLK_DELETE,4,5}
};

typedef struct pet_runtime_state {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    pet_t pet;
    char rom_dir[260];
    int ready;
} pet_runtime_state;

static pet_runtime_state g_runtime;

int pet_runtime_init(SDL_Renderer *renderer, int model_id, const char *rom_dir)
{
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.renderer = renderer;
    if (rom_dir != NULL) {
        strncpy(g_runtime.rom_dir, rom_dir, sizeof(g_runtime.rom_dir) - 1);
    } else {
        strcpy(g_runtime.rom_dir, "roms/open");
    }
    pet_init(&g_runtime.pet);
    pet_set_model(&g_runtime.pet, model_id);
    if (!pet_load_roms(&g_runtime.pet, g_runtime.rom_dir)) {
        return 0;
    }
    g_runtime.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                         PET_RUNTIME_SCREEN_W, PET_RUNTIME_SCREEN_H);
    if (g_runtime.texture == NULL) {
        return 0;
    }
    SDL_SetTextureBlendMode(g_runtime.texture, SDL_BLENDMODE_NONE);
    pet_reset(&g_runtime.pet);
    g_runtime.ready = 1;
    return 1;
}

void pet_runtime_shutdown(void)
{
    if (g_runtime.texture != NULL) {
        SDL_DestroyTexture(g_runtime.texture);
    }
    memset(&g_runtime, 0, sizeof(g_runtime));
}

void pet_runtime_reset(void)
{
    if (g_runtime.ready) {
        pet_reset(&g_runtime.pet);
    }
}

SDL_Texture *pet_runtime_frame(void)
{
    if (!g_runtime.ready) {
        return NULL;
    }
    pet_execute(&g_runtime.pet, 5000);
    render_screen(&g_runtime.pet, g_runtime.texture);
    return g_runtime.texture;
}

void pet_runtime_keyboard(SDL_Keycode key, int down)
{
    size_t i;
    if (g_runtime.ready) {
        for (i = 0; i < sizeof(SDL_KEYS) / sizeof(SDL_KEYS[0]); i++) {
            if (SDL_KEYS[i].key == key) {
                if (down) {
                    pet_keyboard_press(&g_runtime.pet, SDL_KEYS[i].row, SDL_KEYS[i].col);
                } else {
                    pet_keyboard_release(&g_runtime.pet, SDL_KEYS[i].row, SDL_KEYS[i].col);
                }
                return;
            }
        }
    }
}

int pet_runtime_load_prg(const char *path)
{
    if (!g_runtime.ready) {
        return 0;
    }
    return pet_load_prg(&g_runtime.pet, path);
}

int pet_runtime_model(int model_id)
{
    if (!g_runtime.ready) {
        return 0;
    }
    if (!pet_set_model(&g_runtime.pet, model_id)) {
        return 0;
    }
    if (!pet_load_roms(&g_runtime.pet, g_runtime.rom_dir)) {
        return 0;
    }
    pet_reset(&g_runtime.pet);
    return 1;
}

const char *pet_runtime_error(void)
{
    return pet_error(&g_runtime.pet);
}

unsigned int pet_runtime_last_load_addr(void)
{
    return g_runtime.pet.last_load_addr;
}
