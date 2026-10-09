#include "gfx.h"

/* DS:7033..706A, puntati dalla tabella DS:7023. */
static const uint8_t patterns[8][8] = {
    {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa},
    {0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81},
    {0xcc, 0x99, 0x33, 0x66, 0xcc, 0x99, 0x33, 0x66},
    {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88},
    {0xee, 0xdd, 0xbb, 0x77, 0xee, 0xdd, 0xbb, 0x77},
    {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55},
};

const GfxScale gfx_scale_vga = {16, 18, 9, 1, 48, 35, 17};

static GfxScale current;

void gfx_set_scale(const GfxScale *scale)
{
    current = *scale;
}

/* mul word / add ax / div word: la somma non propaga il riporto in DX. */
static uint16_t scale16(uint16_t v, uint16_t mul, uint16_t add, uint16_t div)
{
    uint32_t product = (uint32_t)v * mul;
    uint32_t sum = (product & 0xffff0000u) | (uint16_t)(product + add);
    return (uint16_t)(sum / div);
}

uint16_t gfx_scale_x(uint16_t x)
{
    return scale16(x, current.x_mul, current.x_add, current.x_div);
}

uint16_t gfx_scale_y(uint16_t y)
{
    return scale16(y, current.y_mul, current.y_add, current.y_div);
}

static uint8_t apply_rop(uint8_t dst, uint8_t color, uint8_t rop)
{
    switch (rop & 0x18) {
    case GFX_ROP_AND: return dst & color;
    case GFX_ROP_OR: return dst | color;
    case GFX_ROP_XOR: return dst ^ color;
    default: return color;
    }
}

/* Driver VGA 293b:9ec4 + 293b:9b2c: per ogni riga la bit mask è il byte
   del retino indicizzato da y & 7; i bit a 0 lasciano il pixel invariato. */
void gfx_fill_pattern(Screen *screen, uint16_t x, uint16_t y, uint16_t w,
                      uint16_t h, uint8_t color, uint16_t pattern, uint8_t rop)
{
    if (w == 0 || h == 0)
        return;
    const uint8_t *mask = patterns[pattern & 7];
    for (unsigned row = y; row < (unsigned)y + h; row++) {
        uint8_t bits = mask[row & 7];
        for (unsigned col = x; col < (unsigned)x + w; col++) {
            if (row >= SCREEN_HEIGHT || col >= SCREEN_WIDTH)
                continue;
            if (bits & (0x80 >> (col & 7)))
                screen->pixels[row][col] =
                    apply_rop(screen->pixels[row][col], color, rop) & 15;
        }
    }
}
