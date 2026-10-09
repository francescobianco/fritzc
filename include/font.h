#ifndef FRITZ_FONT_H
#define FRITZ_FONT_H

#include <stdbool.h>
#include <stdint.h>

#include "exe.h"
#include "screen.h"

/* Font descriptor pointed to by the table at DS:714F: glyph data in
   segment 293b, rows per glyph, bytes per glyph. Glyphs are 8 pixels wide. */
typedef struct Font {
    const uint8_t *glyphs;
    uint8_t rows;
    uint8_t bytes_per_glyph;
} Font;

enum { FONT_COUNT = 4 };

/* Text effects: bits of DS:715A, the last argument of 293b:9fb1/a044. */
enum {
    TEXT_BOLD = 0x01,
    TEXT_UNDERLINE = 0x02,
    TEXT_HALFTONE_3 = 0x04,
    TEXT_HALFTONE = 0x08,
    TEXT_INVERSE = 0x10
};

/* Reads the four fonts from FRITZ2.EXE. */
bool fonts_load(const Exe *exe);
const Font *font_get(uint16_t index);

/* 293b:9fb1: Pascal string (length byte first). */
void gfx_draw_text_p(Screen *screen, uint16_t x, uint16_t y, uint16_t font,
                     const uint8_t *pstring, uint8_t color, uint8_t mode);
/* 293b:a044: zero-terminated string. */
void gfx_draw_text(Screen *screen, uint16_t x, uint16_t y, uint16_t font,
                   const char *string, uint8_t color, uint8_t mode);

#endif
