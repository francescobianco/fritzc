#include "font.h"

enum {
    GFX_SEGMENT = 0x293b,
    FONT_TABLE = 0x714f   /* DS: word pointers to 4-byte descriptors */
};

static Font fonts[FONT_COUNT];

bool fonts_load(const Exe *exe)
{
    const uint8_t *table = exe_at(exe, EXE_DGROUP, FONT_TABLE, 2 * FONT_COUNT);
    if (!table)
        return false;
    for (int i = 0; i < FONT_COUNT; i++) {
        uint16_t desc_off = (uint16_t)(table[2 * i] | table[2 * i + 1] << 8);
        const uint8_t *desc = exe_at(exe, EXE_DGROUP, desc_off, 4);
        if (!desc)
            return false;
        uint16_t data = (uint16_t)(desc[0] | desc[1] << 8);
        fonts[i].rows = desc[2];
        fonts[i].bytes_per_glyph = desc[3];
        fonts[i].glyphs = exe_at(exe, GFX_SEGMENT, data, 256u * desc[3]);
        if (!fonts[i].glyphs)
            return false;
    }
    return true;
}

const Font *font_get(uint16_t index)
{
    return index < FONT_COUNT && fonts[index].glyphs ? &fonts[index] : NULL;
}

/* VGA glyph driver 293b:a0ce. Each row byte is shifted right by x & 7 over
   two bytes and used as the bit mask (set/reset = colour), so zero bits are
   transparent. `left` counts rows down from `rows` to 1, as CL does. */
static void draw_glyph(Screen *screen, uint16_t x, uint16_t y, const Font *font,
                       uint8_t ch, uint8_t color, uint8_t mode)
{
    const uint8_t *g = font->glyphs + (unsigned)ch * font->bytes_per_glyph;
    int third = 3;

    for (unsigned row = 0; row < font->rows; row++) {
        unsigned left = font->rows - row;
        uint8_t bits = g[row];
        if (mode) {
            uint8_t halftone = (left & 1) ? 0x55 : 0xaa;
            if (mode & TEXT_BOLD)
                bits |= bits >> 1;
            if ((mode & TEXT_UNDERLINE) && left == 1)
                bits = 0xff;
            if (mode & TEXT_HALFTONE)
                bits &= halftone;
            if ((mode & TEXT_HALFTONE_3) && --third == 0) {
                third = 3;
                bits &= halftone;
            }
            if (mode & TEXT_INVERSE)
                bits = (uint8_t)~bits;
        }
        unsigned py = (unsigned)y + row;
        for (unsigned b = 0; b < 8; b++) {
            unsigned px = (unsigned)x + b;
            if ((bits & (0x80 >> b)) && px < SCREEN_WIDTH && py < SCREEN_HEIGHT)
                screen->pixels[py][px] = color & 15;
        }
    }
}

/* The VGA loop advances the video address by one byte (8 pixels) per char. */
void gfx_draw_text_p(Screen *screen, uint16_t x, uint16_t y, uint16_t font,
                     const uint8_t *pstring, uint8_t color, uint8_t mode)
{
    const Font *f = font_get(font);
    if (!f)
        return;
    for (unsigned i = 1; i <= pstring[0]; i++, x += 8)
        draw_glyph(screen, x, y, f, pstring[i], color, mode);
}

void gfx_draw_text(Screen *screen, uint16_t x, uint16_t y, uint16_t font,
                   const char *string, uint8_t color, uint8_t mode)
{
    const Font *f = font_get(font);
    if (!f)
        return;
    for (; *string; string++, x += 8)
        draw_glyph(screen, x, y, f, (uint8_t)*string, color, mode);
}
