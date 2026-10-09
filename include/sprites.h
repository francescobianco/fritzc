#ifndef FRITZ_SPRITES_H
#define FRITZ_SPRITES_H

#include <stdbool.h>
#include <stdint.h>

#include "exe.h"
#include "screen.h"

/* Bitmap sets of segment 293b, described at DS:7073 -> DS:7087.. :
   word offset table pointer (one bitmap offset per piece type), word scratch,
   word bitmap size, byte width in bytes, byte height. */
enum { SPRITE_SET_COUNT = 6 };

bool sprites_load(const Exe *exe);

/* 293b:a649 + VGA driver 293b:a6b0: bitmap of `piece` (type in bits 0-2,
   bit 3 selects the black variants) from `set`, at byte-aligned x. When
   `variant` is even the next bitmap is used. Pixels whose bit is 0 are
   painted with `color`; the others are left unchanged. */
void gfx_draw_piece(Screen *screen, uint16_t x, uint16_t y, uint16_t set,
                    uint16_t piece, uint16_t variant, uint8_t color);

#endif
