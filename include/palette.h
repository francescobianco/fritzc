#ifndef FRITZ_PALETTE_H
#define FRITZ_PALETTE_H

#include <stdint.h>

typedef struct Rgb {
    uint8_t r, g, b;
} Rgb;

/* Palette a 16 colori come appare negli screenshot nativi di DOSBox. */
extern const Rgb palette_vga16[16];

#endif
