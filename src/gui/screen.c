#include "screen.h"

#include <string.h>

void screen_clear(Screen *screen, uint8_t color)
{
    memset(screen->pixels, color, sizeof screen->pixels);
}

void screen_put_pixel(Screen *screen, int x, int y, uint8_t color)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        screen->pixels[y][x] = color;
}

void screen_fill_rect(Screen *screen, int x, int y, int w, int h, uint8_t color)
{
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCREEN_WIDTH ? SCREEN_WIDTH : x + w;
    int y1 = y + h > SCREEN_HEIGHT ? SCREEN_HEIGHT : y + h;

    for (int row = y0; row < y1; row++)
        if (x1 > x0)
            memset(&screen->pixels[row][x0], color, (size_t)(x1 - x0));
}
