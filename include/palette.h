#ifndef FRITZ_PALETTE_H
#define FRITZ_PALETTE_H

#include <stdint.h>

typedef struct Rgb {
    uint8_t r, g, b;
} Rgb;

/* 16-colour palette as it appears in native DOSBox screenshots. */
extern const Rgb palette_vga16[16];

#endif
