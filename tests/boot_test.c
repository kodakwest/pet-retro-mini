/* Headless boot test: boots the PET core with the bundled open ROMs and
 * asserts that BASIC comes up (the READY. prompt appears in screen RAM).
 *
 * Build/run:  make boot-test
 *
 * Status: RED as of June 2026 — see docs/REVIEW-AND-ROADMAP.md §1.1. The
 * committed BASIC ROM images are oversized and the kernal jumps to $C000,
 * which holds a pointer table rather than the cold-start entry, so the
 * machine wedges before printing anything. This test is the exit gate for
 * roadmap Phase 0 and should run in CI from then on.
 */
#include "pet.h"

#include <stdio.h>
#include <string.h>

#define FRAMES 600          /* ~10 emulated seconds at 5000 instructions/frame */
#define INSTR_PER_FRAME 5000

static char screen_to_ascii(uint8_t code)
{
    uint8_t c = code & 0x7f;
    if (c == 0x20) return ' ';
    if (c >= 1 && c <= 26) return (char)('A' + c - 1);
    if (c >= 0x30 && c <= 0x39) return (char)c;
    if (c >= 0x21 && c <= 0x3f) return (char)c;
    if (c == 0) return '@';
    return '.';
}

static void dump_screen(const pet_t *pet, char rows[PET_SCREEN_ROWS][PET_SCREEN_COLS + 1])
{
    int r;
    int c;
    for (r = 0; r < PET_SCREEN_ROWS; r++) {
        for (c = 0; c < PET_SCREEN_COLS; c++) {
            rows[r][c] = screen_to_ascii(pet->ram[PET_SCREEN_ADDR + r * PET_SCREEN_COLS + c]);
        }
        rows[r][PET_SCREEN_COLS] = '\0';
    }
}

int main(void)
{
    pet_t pet;
    char rows[PET_SCREEN_ROWS][PET_SCREEN_COLS + 1];
    int frame;
    int r;
    int found_ready = 0;

    pet_init(&pet);
    if (!pet_load_roms(&pet, "roms/open")) {
        fprintf(stderr, "FAIL: %s\n", pet_error(&pet));
        return 1;
    }
    pet_reset(&pet);
    printf("reset vector -> $%04X\n", pet.cpu.pc);

    for (frame = 0; frame < FRAMES && !found_ready; frame++) {
        pet_execute(&pet, INSTR_PER_FRAME);
        dump_screen(&pet, rows);
        for (r = 0; r < PET_SCREEN_ROWS; r++) {
            if (strstr(rows[r], "READY") != NULL) {
                found_ready = 1;
                break;
            }
        }
    }

    dump_screen(&pet, rows);
    printf("after %d frames, PC=$%04X. Screen:\n", frame, pet.cpu.pc);
    for (r = 0; r < PET_SCREEN_ROWS; r++) {
        printf("%2d|%s|\n", r, rows[r]);
    }

    if (!found_ready) {
        fprintf(stderr, "FAIL: BASIC never printed READY.\n");
        return 1;
    }
    printf("PASS: BASIC booted to READY.\n");
    return 0;
}
