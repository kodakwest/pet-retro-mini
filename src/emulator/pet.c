#include "pet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASIC_ADDR 0xc000
#define EDITOR_ADDR 0xe000
#define IO_ADDR 0xe800
#define KERNAL_ADDR 0xf000

static const struct {
    char ch;
    int row;
    int col;
} ASCII_KEYS[] = {
    {'1',0,0},{'2',0,1},{'3',0,2},{'4',0,3},{'5',0,4},{'6',0,5},{'7',0,6},{'8',0,7},
    {'Q',1,0},{'W',1,1},{'E',1,2},{'R',1,3},{'T',1,4},{'Y',1,5},{'U',1,6},{'I',1,7},
    {'A',2,0},{'S',2,1},{'D',2,2},{'F',2,3},{'G',2,4},{'H',2,5},{'J',2,6},{'K',2,7},
    {'Z',3,0},{'X',3,1},{'C',3,2},{'V',3,3},{'B',3,4},{'N',3,5},{'M',3,6},{',',3,7},
    {'9',4,0},{'0',4,1},{'-',4,2},{'=',4,3},{'\r',4,4},{'\n',4,4},{'\b',4,5},{' ',4,6},
    {'O',5,0},{'P',5,1},{'[',5,2},{']',5,3},
    {'L',6,0},{';',6,1},{':',6,2},
    {'.',7,0},{'/',7,1}
};

static void set_error(pet_t *pet, const char *message)
{
    strncpy(pet->error, message, sizeof(pet->error) - 1);
    pet->error[sizeof(pet->error) - 1] = '\0';
}

static uint8_t cpu_read_cb(void *user, uint16_t addr)
{
    return pet_read((pet_t *)user, addr);
}

static void cpu_write_cb(void *user, uint16_t addr, uint8_t value)
{
    pet_write((pet_t *)user, addr, value);
}

static int load_file_window(const char *path, uint8_t *dst, size_t dst_size, size_t required_min)
{
    FILE *file = fopen(path, "rb");
    size_t n;
    if (file == NULL) return 0;
    memset(dst, 0xea, dst_size);
    n = fread(dst, 1, dst_size, file);
    fclose(file);
    return n >= required_min;
}

static int load_rom_file(const char *dir, const char *name, uint8_t *dst, size_t dst_size, size_t min_size)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    return load_file_window(path, dst, dst_size, min_size);
}

static void clear_keyboard(pet_t *pet)
{
    int i;
    for (i = 0; i < 10; i++) {
        pet->key_matrix[i] = 0xff;
    }
    pet->key_head = 0;
    pet->key_tail = 0;
    pet->queued_row = -1;
    pet->queued_col = -1;
}

void pet_init(pet_t *pet)
{
    memset(pet, 0, sizeof(*pet));
    pet->model = PET_MODEL_4032;
    clear_keyboard(pet);
    cpu_init(&pet->cpu, cpu_read_cb, cpu_write_cb, pet);
}

int pet_load_roms(pet_t *pet, const char *rom_dir)
{
    const char *basic_name = pet->model == PET_MODEL_2001 ? "basic1.bin" : "basic2.bin";
    if (!load_rom_file(rom_dir, basic_name, pet->basic_rom, sizeof(pet->basic_rom), 0x1000)) {
        set_error(pet, "Could not load BASIC ROM");
        return 0;
    }
    if (!load_rom_file(rom_dir, "editor.bin", pet->editor_rom, sizeof(pet->editor_rom), 0x0800)) {
        set_error(pet, "Could not load editor ROM");
        return 0;
    }
    if (!load_rom_file(rom_dir, "kernal.bin", pet->kernal_rom, sizeof(pet->kernal_rom), 0x1000)) {
        set_error(pet, "Could not load kernal ROM");
        return 0;
    }
    if (!load_rom_file(rom_dir, "chargen.bin", pet->chargen, sizeof(pet->chargen), 0x0400)) {
        set_error(pet, "Could not load chargen ROM");
        return 0;
    }
    pet->error[0] = '\0';
    return 1;
}

void pet_reset(pet_t *pet)
{
    memset(pet->ram, 0, sizeof(pet->ram));
    memset(pet->via, 0, sizeof(pet->via));
    memset(pet->crtc, 0, sizeof(pet->crtc));
    pet->pia1_port_a = 0xff;
    pet->pia1_cra = 0;
    pet->pia1_crb = 0;
    clear_keyboard(pet);
    cpu_reset(&pet->cpu);
}

static int queued_to_matrix(pet_t *pet, int *row, int *col)
{
    char ch;
    size_t i;
    if (pet->queued_row >= 0) {
        *row = pet->queued_row;
        *col = pet->queued_col;
        return 1;
    }
    if (pet->key_head == pet->key_tail) {
        return 0;
    }
    ch = pet->key_queue[pet->key_head];
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
    for (i = 0; i < sizeof(ASCII_KEYS) / sizeof(ASCII_KEYS[0]); i++) {
        if (ASCII_KEYS[i].ch == ch) {
            pet->queued_row = ASCII_KEYS[i].row;
            pet->queued_col = ASCII_KEYS[i].col;
            *row = pet->queued_row;
            *col = pet->queued_col;
            pet->key_head = (pet->key_head + 1) & 255;
            return 1;
        }
    }
    pet->key_head = (pet->key_head + 1) & 255;
    return 0;
}

static uint8_t read_keyboard_port(pet_t *pet)
{
    uint8_t out = 0xff;
    int row;
    for (row = 0; row < 10; row++) {
        if ((pet->pia1_port_a & (1u << row)) == 0 || pet->pia1_port_a == 0) {
            out &= pet->key_matrix[row];
        }
    }
    if (out == 0xff) {
        int qrow;
        int qcol;
        if (queued_to_matrix(pet, &qrow, &qcol)) {
            if ((pet->pia1_port_a & (1u << qrow)) == 0 || pet->pia1_port_a == 0) {
                out &= (uint8_t)~(1u << qcol);
                pet->queued_row = -1;
                pet->queued_col = -1;
            }
        }
    }
    return out;
}

uint8_t pet_read(pet_t *pet, uint16_t addr)
{
    if (addr >= BASIC_ADDR && addr <= 0xdfff) return pet->basic_rom[addr - BASIC_ADDR];
    if (addr >= EDITOR_ADDR && addr <= 0xe7ff) return pet->editor_rom[addr - EDITOR_ADDR];
    if (addr >= IO_ADDR && addr <= 0xe8ff) {
        if (addr == 0xe810) return pet->pia1_port_a;
        if (addr == 0xe811) return pet->pia1_cra;
        if (addr == 0xe812) return read_keyboard_port(pet);
        if (addr == 0xe813) return pet->pia1_crb;
        if (addr >= 0xe840 && addr <= 0xe84f) {
            if (addr == 0xe844) pet->via[0x0d] &= (uint8_t)~0x40;
            return pet->via[addr & 0x0f];
        }
        if (addr == 0xe880) return pet->crtc_index;
        if (addr == 0xe881) return pet->crtc[pet->crtc_index & 31];
        return 0xff;
    }
    if (addr >= KERNAL_ADDR) return pet->kernal_rom[addr - KERNAL_ADDR];
    return pet->ram[addr];
}

void pet_write(pet_t *pet, uint16_t addr, uint8_t value)
{
    if (addr >= BASIC_ADDR && addr <= 0xdfff) return;
    if (addr >= EDITOR_ADDR && addr <= 0xe7ff) return;
    if (addr >= KERNAL_ADDR) return;
    if (addr >= IO_ADDR && addr <= 0xe8ff) {
        if (addr == 0xe810) pet->pia1_port_a = value;
        else if (addr == 0xe811) pet->pia1_cra = value;
        else if (addr == 0xe813) pet->pia1_crb = value;
        else if (addr >= 0xe840 && addr <= 0xe84f) pet->via[addr & 0x0f] = value;
        else if (addr == 0xe880) pet->crtc_index = value & 31;
        else if (addr == 0xe881) pet->crtc[pet->crtc_index & 31] = value;
        return;
    }
    pet->ram[addr] = value;
}

void pet_execute(pet_t *pet, int instructions)
{
    int i;
    for (i = 0; i < instructions; i++) {
        cpu_step(&pet->cpu);
        if ((i & 4095) == 0 && (pet->via[0x0e] & 0x40) != 0) {
            pet->via[0x0d] |= 0x40;
            cpu_irq(&pet->cpu);
        }
    }
}

void pet_keyboard_press(pet_t *pet, int row, int col)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 8) {
        pet->key_matrix[row] &= (uint8_t)~(1u << col);
    }
}

void pet_keyboard_release(pet_t *pet, int row, int col)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 8) {
        pet->key_matrix[row] |= (uint8_t)(1u << col);
    }
}

void pet_queue_text(pet_t *pet, const char *text)
{
    while (*text != '\0') {
        int next = (pet->key_tail + 1) & 255;
        if (next == pet->key_head) return;
        pet->key_queue[pet->key_tail] = *text++;
        pet->key_tail = next;
    }
}

static uint16_t scan_basic_end(pet_t *pet, uint16_t start)
{
    uint16_t ptr = start;
    int guard = 0;
    while (ptr + 1 < PET_MEM_SIZE && guard++ < 4096) {
        uint16_t next = (uint16_t)(pet->ram[ptr] | ((uint16_t)pet->ram[ptr + 1] << 8));
        if (next == 0 || next <= ptr) {
            return (uint16_t)(ptr + 2);
        }
        ptr = next;
    }
    return start;
}

int pet_load_prg(pet_t *pet, const char *path)
{
    FILE *file;
    long size;
    uint8_t header[2];
    size_t payload_size;
    uint16_t end;
    file = fopen(path, "rb");
    if (file == NULL) {
        set_error(pet, "Could not open PRG");
        return 0;
    }
    if (fread(header, 1, 2, file) != 2 || fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        set_error(pet, "PRG is invalid");
        return 0;
    }
    size = ftell(file);
    if (size < 2) {
        fclose(file);
        set_error(pet, "PRG is empty");
        return 0;
    }
    pet->last_load_addr = (unsigned int)header[0] | ((unsigned int)header[1] << 8);
    payload_size = (size_t)(size - 2);
    if (pet->last_load_addr + payload_size > PET_MEM_SIZE || pet->last_load_addr >= BASIC_ADDR) {
        fclose(file);
        set_error(pet, "PRG does not fit in writable PET RAM");
        return 0;
    }
    if (fseek(file, 2, SEEK_SET) != 0 ||
        fread(pet->ram + pet->last_load_addr, 1, payload_size, file) != payload_size) {
        fclose(file);
        set_error(pet, "Could not read PRG payload");
        return 0;
    }
    fclose(file);
    end = scan_basic_end(pet, (uint16_t)pet->last_load_addr);
    pet->ram[0x28] = (uint8_t)pet->last_load_addr;
    pet->ram[0x29] = (uint8_t)(pet->last_load_addr >> 8);
    pet->ram[0x2a] = (uint8_t)end;
    pet->ram[0x2b] = (uint8_t)(end >> 8);
    pet->ram[0x2c] = pet->ram[0x2a];
    pet->ram[0x2d] = pet->ram[0x2b];
    pet->ram[0x2e] = pet->ram[0x2a];
    pet->ram[0x2f] = pet->ram[0x2b];
    pet_queue_text(pet, "RUN\r");
    pet->error[0] = '\0';
    return 1;
}

int pet_set_model(pet_t *pet, int model_id)
{
    if (model_id < PET_MODEL_2001 || model_id > PET_MODEL_8032) {
        set_error(pet, "Unknown PET model");
        return 0;
    }
    pet->model = (pet_model_id)model_id;
    return 1;
}

const char *pet_error(const pet_t *pet)
{
    return pet->error[0] == '\0' ? "No PET emulator error" : pet->error;
}
