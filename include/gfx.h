#ifndef FRITZ_GFX_H
#define FRITZ_GFX_H

#include <stdint.h>

#include "screen.h"

/* Fritz 2.51 graphics library: segment 293b of FRITZ2.UNP.EXE.
   Only the VGA driver (640x480x16) is reconstructed. */

/* Raster operations: VGA Data Rotate/Function Select values passed as the
   last argument of 293b:9e44. */
enum {
    GFX_ROP_COPY = 0x00,
    GFX_ROP_AND = 0x08,
    GFX_ROP_OR = 0x10,
    GFX_ROP_XOR = 0x18
};

/* Indices into the pattern table at DS:7023. */
enum {
    GFX_PATTERN_HALFTONE = 0,  /* DS:7033 55 aa ... */
    GFX_PATTERN_DESKTOP = 1,   /* DS:703B 88 22 ... */
    GFX_PATTERN_SOLID = 2      /* DS:7043 ff ff ... */
};

/* Scale parameters set by 293b:aa8b (DS:6F48..6F54). */
typedef struct GfxScale {
    uint16_t x_mul, x_div, x_add;
    uint16_t unknown_6f4e;
    uint16_t y_mul, y_div, y_add;
} GfxScale;

/* VGA table at DS:7478, copied to DS:0B7C by 0000:7eaf. */
extern const GfxScale gfx_scale_vga;

void gfx_set_scale(const GfxScale *scale);
uint16_t gfx_scale_x(uint16_t x);   /* 293b:aa5f */
uint16_t gfx_scale_y(uint16_t y);   /* 293b:aa75 */

/* 293b:9e44: pattern-filled rectangle, in device coordinates. */
void gfx_fill_pattern(Screen *screen, uint16_t x, uint16_t y, uint16_t w,
                      uint16_t h, uint8_t color, uint16_t pattern, uint8_t rop);

#endif
