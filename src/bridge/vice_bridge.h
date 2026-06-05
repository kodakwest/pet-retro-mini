#ifndef PET_VICE_BRIDGE_H
#define PET_VICE_BRIDGE_H

#include <SDL.h>

#define VICE_BRIDGE_SCREEN_W 640
#define VICE_BRIDGE_SCREEN_H 400

typedef enum vice_bridge_model_id {
    VICE_BRIDGE_MODEL_2001 = 0,
    VICE_BRIDGE_MODEL_4032 = 1,
    VICE_BRIDGE_MODEL_8032 = 2
} vice_bridge_model_id;

int vice_bridge_init(SDL_Renderer *renderer, int model_id, const char *rom_dir);
void vice_bridge_shutdown(void);
void vice_bridge_reset(void);
SDL_Texture *vice_bridge_frame(void);
void vice_bridge_keyboard(SDL_Keycode key, int down);
int vice_bridge_load_prg(const char *path);
int vice_bridge_model(int model_id);
const char *vice_bridge_error(void);
unsigned int vice_bridge_last_load_addr(void);

#endif
