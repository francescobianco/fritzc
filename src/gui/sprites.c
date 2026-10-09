#include "sprites.h"

enum {
    GFX_SEGMENT = 0x293b,
    SET_TABLE = 0x7073
};

typedef struct SpriteSet {
    uint16_t offsets[8];   /* bitmap offsets in segment 293b */
    uint16_t size;
    uint8_t width_bytes;
    uint8_t height;
} SpriteSet;

static SpriteSet sets[SPRITE_SET_COUNT];
static const Exe *source;

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

bool sprites_load(const Exe *exe)
{
    const uint8_t *table = exe_at(exe, EXE_DGROUP, SET_TABLE, 2 * SPRITE_SET_COUNT);
    if (!table)
        return false;
    for (int i = 0; i < SPRITE_SET_COUNT; i++) {
        const uint8_t *d = exe_at(exe, EXE_DGROUP, rd16(table + 2 * i), 8);
        if (!d)
            return false;
        const uint8_t *offs = exe_at(exe, EXE_DGROUP, rd16(d), 16);
        if (!offs)
            return false;
        for (int k = 0; k < 8; k++)
            sets[i].offsets[k] = rd16(offs + 2 * k);
        sets[i].size = rd16(d + 4);
        sets[i].width_bytes = d[6];
        sets[i].height = d[7];
    }
    source = exe;
    return true;
}

void gfx_draw_piece(Screen *screen, uint16_t x, uint16_t y, uint16_t set,
                    uint16_t piece, uint16_t variant, uint8_t color)
{
    if (!source || set >= SPRITE_SET_COUNT)
        return;
    const SpriteSet *s = &sets[set];
    uint16_t off = s->offsets[(uint8_t)(piece - 1) & 7];
    if (!(variant & 1))
        off += s->size;
    if (piece & 8)
        off += 2 * s->size;
    size_t len = (size_t)s->width_bytes * s->height;
    const uint8_t *bits = exe_at(source, GFX_SEGMENT, off, len);
    if (!bits)
        return;

    unsigned x0 = x & ~7u;   /* the VGA driver addresses whole bytes */
    for (unsigned row = 0; row < s->height; row++) {
        unsigned py = (unsigned)y + row;
        for (unsigned col = 0; col < s->width_bytes; col++) {
            uint8_t mask = (uint8_t)~bits[row * s->width_bytes + col];
            for (unsigned b = 0; b < 8; b++) {
                unsigned px = x0 + col * 8 + b;
                if ((mask & (0x80 >> b)) && px < SCREEN_WIDTH && py < SCREEN_HEIGHT)
                    screen->pixels[py][px] = color & 15;
            }
        }
    }
}
