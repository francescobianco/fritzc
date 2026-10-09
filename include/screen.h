#ifndef FRITZ_SCREEN_H
#define FRITZ_SCREEN_H

#include <stdint.h>

/* Superficie virtuale con la geometria della modalità VGA usata da Fritz 2.51
   (640x480, 16 colori, pixel quadrati). Ogni pixel è un indice di palette. */
enum {
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 480
};

/* Indici della palette EGA/VGA standard. */
enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_LIGHT_GRAY = 7,
    COLOR_WHITE = 15
};

typedef struct Screen {
    uint8_t pixels[SCREEN_HEIGHT][SCREEN_WIDTH];
} Screen;

void screen_clear(Screen *screen, uint8_t color);
void screen_put_pixel(Screen *screen, int x, int y, uint8_t color);
void screen_fill_rect(Screen *screen, int x, int y, int w, int h, uint8_t color);

#endif
