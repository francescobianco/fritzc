#ifndef FRITZ_SCREEN_H
#define FRITZ_SCREEN_H

#include <stdint.h>

/* Virtual surface with the geometry of the VGA mode used by Fritz 2.51
   (640x480, 16 colours, square pixels). Each pixel is a palette index. */
enum {
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 480
};

/* Standard EGA/VGA palette indices. */
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
