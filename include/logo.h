#ifndef FRITZ_LOGO_H
#define FRITZ_LOGO_H

#include <stdbool.h>
#include <stdint.h>

#include "screen.h"
#include "settings.h"

/* Monochrome bitmap built by 19e7:eb38 from a GEM IMG file (FRITZ2.IMG),
   pointed to by [AFA4]: width, height, words per row, rows of
   2*words_per_row bytes, bit 7 leftmost, 1 = ink. */
typedef struct Logo {
    uint16_t width;
    uint16_t height;
    uint16_t words_per_row;
    uint8_t *rows;
} Logo;

/* 19e7:e91e + 19e7:eb38. */
bool logo_load(Logo *logo, const char *path);
void logo_free(Logo *logo);

/* 19e7:a994: the logo box at device coordinates (x, y). */
void logo_draw(Screen *screen, const Settings *settings, const Logo *logo,
               uint16_t x, uint16_t y);

#endif
