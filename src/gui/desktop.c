#include "desktop.h"

#include "gfx.h"

/* FRITZ2.EXE globals used to draw the desktop.
   The colours come from the configuration (origin not reconstructed yet):
   the values are those observed in DOSBox with the distribution's FRITZ2.PAR. */
static const uint16_t menu_bar_height = 14;     /* DS:0BAC, from DS:73CE */
static const uint16_t display_flags = 1;        /* DS:0BB0, from DS:74A6 */
static const uint8_t color_background = 15;     /* DS:AD6E [observed] */
static const uint8_t color_desktop = 1;         /* DS:0F4E [observed] */
static const uint8_t color_desktop_mono = 1;    /* DS:B0C6 [unknown] */

/* 0000:2512 up to 0000:259d. The rest (call 0000:05f3 onwards: frames
   and panels) is not reconstructed yet. */
void desktop_draw(Screen *screen)
{
    uint16_t top = menu_bar_height + 1;
    uint16_t width = gfx_scale_x(720);
    uint16_t height = gfx_scale_y(348);

    gfx_fill_pattern(screen, 0, 0, width, height, color_background,
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_fill_pattern(screen, 0, top, width, height - top,
                     (display_flags & 1) ? color_desktop : color_desktop_mono,
                     GFX_PATTERN_DESKTOP, GFX_ROP_COPY);
}
