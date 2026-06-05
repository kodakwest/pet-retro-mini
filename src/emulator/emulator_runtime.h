#ifndef PET_EMULATOR_RUNTIME_H
#define PET_EMULATOR_RUNTIME_H

#include <SDL.h>

#define PET_RUNTIME_SCREEN_W 640
#define PET_RUNTIME_SCREEN_H 400

typedef enum pet_runtime_model_id {
    PET_RUNTIME_MODEL_2001 = 0,
    PET_RUNTIME_MODEL_4032 = 1,
    PET_RUNTIME_MODEL_8032 = 2
} pet_runtime_model_id;

int pet_runtime_init(SDL_Renderer *renderer, int model_id, const char *rom_dir);
void pet_runtime_shutdown(void);
void pet_runtime_reset(void);
SDL_Texture *pet_runtime_frame(void);
void pet_runtime_keyboard(SDL_Keycode key, int down);
int pet_runtime_load_prg(const char *path);
int pet_runtime_model(int model_id);
const char *pet_runtime_error(void);
unsigned int pet_runtime_last_load_addr(void);

#endif
