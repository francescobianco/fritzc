#include "gfx.h"

/* DS:7033..706A, pointed to by the table at DS:7023. */
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

const GfxScale gfx_scale_vga = {16, 18, 9, 1, 48, 35, 17, 1};

static GfxScale current;

void gfx_set_scale(const GfxScale *scale)
{
    current = *scale;
}

/* mul word / add ax / div word: the addition does not carry into DX. */
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

/* (v * [6F56] + 1) / [6F4E], skipped when the two are equal. */
uint16_t gfx_scale_line(uint16_t v)
{
    if (current.line_div == current.line_mul)
        return v;
    uint32_t product = (uint32_t)v * current.line_mul;
    uint32_t sum = (product & 0xffff0000u) | (uint16_t)(product + 1);
    return (uint16_t)(sum / current.line_div);
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

/* VGA driver 293b:9ec4 + 293b:9b2c: on each row the bit mask is the pattern
   byte indexed by y & 7; zero bits leave the pixel unchanged. */
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

static void plot(Screen *screen, unsigned x, unsigned y, uint8_t color, uint8_t rop)
{
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT)
        screen->pixels[y][x] = apply_rop(screen->pixels[y][x], color, rop) & 15;
}

/* VGA row driver 293b:9b2c with an all-ones mask (DS:7012 = ff). */
static void hline_rop(Screen *screen, uint16_t x, uint16_t y, uint16_t w,
                      uint8_t color, uint8_t rop)
{
    for (unsigned col = x; col < (unsigned)x + w; col++)
        plot(screen, col, y, color, rop);
}

/* VGA column driver 293b:9d16: mask 0x80 >> (x & 7), [7009] rows. */
static void vline_rop(Screen *screen, uint16_t x, uint16_t y, uint16_t h,
                      uint8_t color, uint8_t rop)
{
    for (unsigned row = y; row < (unsigned)y + h; row++)
        plot(screen, x, row, color, rop);
}

void gfx_hline(Screen *screen, uint16_t x, uint16_t y, uint16_t w, uint8_t color)
{
    if (w != 0)
        hline_rop(screen, x, y, w, color, GFX_ROP_COPY);
}

void gfx_vline(Screen *screen, uint16_t x, uint16_t y, uint16_t h, uint8_t color)
{
    if (h != 0)
        vline_rop(screen, x, y, h, color, GFX_ROP_COPY);
}

/* Same order as the original: left, top, right, bottom. With XOR the
   corners are therefore inverted twice. */
void gfx_draw_rect(Screen *screen, uint16_t x, uint16_t y, uint16_t w,
                   uint16_t h, uint8_t color, uint8_t rop)
{
    if (w == 0 || h == 0)
        return;
    vline_rop(screen, x, y, h, color, rop);
    hline_rop(screen, x, y, w, color, rop);
    vline_rop(screen, (uint16_t)(x + w - 1), y, h, color, rop);
    hline_rop(screen, x, (uint16_t)(y + h - 1), w, color, rop);
}
