#ifndef PET_MACHINE_H
#define PET_MACHINE_H

#include "cpu.h"

#include <stddef.h>
#include <stdint.h>

#define PET_MEM_SIZE 65536
#define PET_SCREEN_ADDR 0x8000
#define PET_SCREEN_COLS 40
#define PET_SCREEN_ROWS 25
#define PET_SCREEN_BYTES (PET_SCREEN_COLS * PET_SCREEN_ROWS)
#define PET_CHARGEN_SIZE 2048

typedef enum pet_model_id {
    PET_MODEL_2001 = 0,
    PET_MODEL_4032 = 1,
    PET_MODEL_8032 = 2
} pet_model_id;

typedef struct pet {
    cpu_t cpu;
    uint8_t ram[PET_MEM_SIZE];
    uint8_t basic_rom[0x2000];
    uint8_t editor_rom[0x0800];
    uint8_t kernal_rom[0x1000];
    uint8_t chargen[PET_CHARGEN_SIZE];
    uint8_t key_matrix[10];
    uint8_t pia1_port_a;
    uint8_t pia1_cra;
    uint8_t pia1_crb;
    uint8_t via[16];
    uint8_t crtc_index;
    uint8_t crtc[32];
    char key_queue[256];
    int key_head;
    int key_tail;
    int queued_row;
    int queued_col;
    unsigned int last_load_addr;
    pet_model_id model;
    char error[160];
} pet_t;

void pet_init(pet_t *pet);
int pet_load_roms(pet_t *pet, const char *rom_dir);
void pet_reset(pet_t *pet);
void pet_execute(pet_t *pet, int instructions);
uint8_t pet_read(pet_t *pet, uint16_t addr);
void pet_write(pet_t *pet, uint16_t addr, uint8_t value);
void pet_keyboard_press(pet_t *pet, int row, int col);
void pet_keyboard_release(pet_t *pet, int row, int col);
void pet_queue_text(pet_t *pet, const char *text);
int pet_load_prg(pet_t *pet, const char *path);
int pet_set_model(pet_t *pet, int model_id);
const char *pet_error(const pet_t *pet);

#endif
