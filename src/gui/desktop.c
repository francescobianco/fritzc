#include "desktop.h"

#include "gfx.h"
#include "settings.h"

/* DS:0BAC and DS:0BB0, copied from DS:73CE and DS:74A6 by 0000:7eaf (VGA). */
static const uint16_t menu_bar_height = 14;
static const uint16_t display_flags = 1;

/* 0000:2512 up to 0000:259d. The rest (call 0000:05f3 onwards: frames
   and panels) is not reconstructed yet. */
void desktop_draw(Screen *screen, const Settings *settings)
{
    uint16_t top = menu_bar_height + 1;
    uint16_t width = gfx_scale_x(720);
    uint16_t height = gfx_scale_y(348);

    gfx_fill_pattern(screen, 0, 0, width, height, (uint8_t)settings->color_ad6e,
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_fill_pattern(screen, 0, top, width, height - top,
                     (uint8_t)((display_flags & 1) ? settings->colors_0f4c[1]  /* [0F4E] */
                                                   : settings->color_b0c6),
                     GFX_PATTERN_DESKTOP, GFX_ROP_COPY);
}
