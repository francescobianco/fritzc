#ifndef FRITZ_GFX_H
#define FRITZ_GFX_H

#include <stdint.h>

#include "screen.h"

/* Libreria grafica di Fritz 2.51: segmento 293b di FRITZ2.UNP.EXE.
   Solo il driver VGA (640x480x16) è ricostruito. */

/* Operazioni raster: valori del registro VGA Data Rotate/Function Select
   passati come ultimo parametro a 293b:9e44. */
enum {
    GFX_ROP_COPY = 0x00,
    GFX_ROP_AND = 0x08,
    GFX_ROP_OR = 0x10,
    GFX_ROP_XOR = 0x18
};

/* Indici della tabella dei retini DS:7023. */
enum {
    GFX_PATTERN_HALFTONE = 0,  /* DS:7033 55 aa ... */
    GFX_PATTERN_DESKTOP = 1,   /* DS:703B 88 22 ... */
    GFX_PATTERN_SOLID = 2      /* DS:7043 ff ff ... */
};

/* Parametri di scala impostati da 293b:aa8b (DS:6F48..6F54). */
typedef struct GfxScale {
    uint16_t x_mul, x_div, x_add;
    uint16_t unknown_6f4e;
    uint16_t y_mul, y_div, y_add;
} GfxScale;

/* Tabella VGA DS:7478, copiata in DS:0B7C da 0000:7eaf. */
extern const GfxScale gfx_scale_vga;

void gfx_set_scale(const GfxScale *scale);
uint16_t gfx_scale_x(uint16_t x);   /* 293b:aa5f */
uint16_t gfx_scale_y(uint16_t y);   /* 293b:aa75 */

/* 293b:9e44: rettangolo riempito con retino, in coordinate dispositivo. */
void gfx_fill_pattern(Screen *screen, uint16_t x, uint16_t y, uint16_t w,
                      uint16_t h, uint8_t color, uint16_t pattern, uint8_t rop);

#endif
