#include "render.h"

#include <stdint.h>
#include <string.h>

#define OUT_W 640
#define OUT_H 400
#define FG 0xff33ff33u
#define BG 0xff050805u

void render_screen(const pet_t *pet, SDL_Texture *texture)
{
    uint32_t pixels[OUT_W * OUT_H];
    int row;
    int col;
    memset(pixels, 0, sizeof(pixels));
    for (row = 0; row < PET_SCREEN_ROWS; row++) {
        for (col = 0; col < PET_SCREEN_COLS; col++) {
            uint8_t code = pet->ram[PET_SCREEN_ADDR + row * PET_SCREEN_COLS + col];
            const uint8_t *glyph = &pet->chargen[(code & 0xff) * 8u % PET_CHARGEN_SIZE];
            int gy;
            for (gy = 0; gy < 8; gy++) {
                uint8_t bits = glyph[gy];
                int gx;
                for (gx = 0; gx < 8; gx++) {
                    uint32_t color = (bits & (uint8_t)(0x80u >> gx)) ? FG : BG;
                    int px = col * 16 + gx * 2;
                    int py = row * 16 + gy * 2;
                    pixels[py * OUT_W + px] = color;
                    pixels[py * OUT_W + px + 1] = color;
                    pixels[(py + 1) * OUT_W + px] = color;
                    pixels[(py + 1) * OUT_W + px + 1] = color;
                }
            }
        }
    }
    SDL_UpdateTexture(texture, NULL, pixels, OUT_W * (int)sizeof(uint32_t));
}
