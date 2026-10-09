#include "logo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"

enum {
    IMG_MAX_READ = 0x513c,     /* bytes read by 19e7:e91e */
    IMG_DATA = 16,             /* eb38 ignores the header length field */
    LOGO_MAX_BYTES = 0x9c30    /* height limit: 40000 / row bytes */
};

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)(p[0] << 8 | p[1]);
}

/* The three run types of 19e7:eb38; counts are signed bytes, so a count of
   0x80 or more writes nothing and returns a negative width. */
static int pattern_run(const uint8_t **src, uint8_t **dst, int pattern_len)
{
    int count = (int8_t)*(*src)++;   /* 19e7:ea1e */
    const uint8_t *pattern = *src;
    for (int i = 0; i < count; i++)
        for (int k = 0; k < pattern_len; k++)
            *(*dst)++ = pattern[k];
    if (count > 0)
        *src += pattern_len;
    return count * pattern_len * 8;
}

static int bit_string(const uint8_t **src, uint8_t **dst)
{
    int count = (int8_t)*(*src)++;   /* 19e7:eaa2 */
    for (int i = 0; i < count; i++)
        *(*dst)++ = *(*src)++;
    return count * 8;
}

static int solid_run(const uint8_t **src, uint8_t **dst)
{
    uint8_t b = *(*src)++;           /* 19e7:eaea */
    for (int i = 0; i < (b & 0x7f); i++)
        *(*dst)++ = (b & 0x80) ? 0xff : 0x00;
    return (b & 0x7f) * 8;
}

bool logo_load(Logo *logo, const char *path)
{
    uint8_t *file = malloc(IMG_MAX_READ + 8);   /* +8: runs may overread */
    FILE *f = fopen(path, "rb");
    memset(logo, 0, sizeof *logo);
    if (!file || !f) {
        free(file);
        if (f)
            fclose(f);
        return false;
    }
    memset(file, 0, IMG_MAX_READ + 8);
    size_t n = fread(file, 1, IMG_MAX_READ, f);
    fclose(f);
    if (n <= IMG_DATA) {
        free(file);
        return false;
    }

    uint16_t pattern_len = be16(file + 6);
    uint16_t width = be16(file + 12);
    uint16_t words = (uint16_t)((width + 15u) >> 4);
    uint16_t height = words ? (uint16_t)(LOGO_MAX_BYTES / (words * 2u)) : 0;
    if (be16(file + 14) < height)
        height = be16(file + 14);

    size_t stride = (size_t)words * 2;
    /* Rows may be overrun by a run that crosses the width, as in the
       original; one spare row absorbs it. */
    uint8_t *rows = calloc((size_t)height + 1, stride ? stride : 1);
    if (!rows) {
        free(file);
        return false;
    }

    const uint8_t *src = file + IMG_DATA;
    const uint8_t *end = file + n + 7;   /* `<= base + size + 11` with base+4 */
    for (unsigned row = 0; src <= end && row < height;) {
        unsigned repeat = 0;
        uint8_t *dst = rows + row * stride;
        if (src[0] == 0 && src[1] == 0 && src[2] == 0xff) {
            repeat = src[3];
            src += 4;
        }
        for (uint16_t done = 0; done < width;) {
            int w;
            if (*src == 0x00) {
                src++;
                w = pattern_run(&src, &dst, pattern_len);
            } else if (*src == 0x80) {
                src++;
                w = bit_string(&src, &dst);
            } else {
                w = solid_run(&src, &dst);
            }
            done = (uint16_t)(done + w);   /* unsigned compare in eb38 */
            if (src > end)
                break;
        }
        /* eb38 copies the line `repeat` more times (n + 1 in total). */
        uint8_t *first = rows + row * stride;
        for (row++; repeat > 0 && row < height; repeat--, row++)
            memcpy(rows + row * stride, first, stride);
    }
    free(file);
    logo->width = width;
    logo->height = height;
    logo->words_per_row = words;
    logo->rows = rows;
    return true;
}

void logo_free(Logo *logo)
{
    free(logo->rows);
    memset(logo, 0, sizeof *logo);
}

/* 293b:ab09 with the VGA driver 293b:abb2: one bitmap row at byte-aligned
   x; 1 bits are painted, the last partial byte is cut by DS:701B. */
static void bitmap_row(Screen *screen, uint16_t x, uint16_t y, uint16_t w,
                       const uint8_t *bits, uint8_t color)
{
    unsigned whole = w >> 3, rest = w & 7;
    unsigned x0 = x & ~7u;
    for (unsigned i = 0; i < whole + (rest ? 1 : 0); i++) {
        uint8_t mask = bits[i];
        if (i == whole)
            mask &= (uint8_t)(0xff00 >> rest);
        for (unsigned b = 0; b < 8; b++) {
            unsigned px = x0 + i * 8 + b;
            if ((mask & (0x80 >> b)) && px < SCREEN_WIDTH && y < SCREEN_HEIGHT)
                screen->pixels[y][px] = color & 15;
        }
    }
}

void logo_draw(Screen *screen, const Settings *settings, const Logo *logo,
               uint16_t x, uint16_t y)
{
    uint8_t ink = (uint8_t)settings->colors_0f4c[0];      /* [0F4C] */
    uint8_t paper = (uint8_t)settings->colors_0f4c[15];   /* [0F6A] */
    uint16_t w = logo->width + 0x1f;
    uint16_t h = logo->height;
    size_t stride = (size_t)logo->words_per_row * 2;

    gfx_fill_pattern(screen, x + 3, y + 2, w, h + 4, paper, GFX_PATTERN_SOLID,
                     GFX_ROP_COPY);
    for (uint16_t row = 0; row < h; row++)
        bitmap_row(screen, x + 0x13, y + 2 + row, logo->width,
                   logo->rows + row * stride, ink);
    gfx_draw_rect(screen, x + 2, y + 1, w + 2, h + 6, ink, GFX_ROP_COPY);
    gfx_draw_rect(screen, x + 1, y, w + 4, h + 8, ink, GFX_ROP_COPY);
    gfx_vline(screen, x, y, h + 8, ink);
    gfx_vline(screen, w + x + 3 + 2, y, h + 8, ink);
}
