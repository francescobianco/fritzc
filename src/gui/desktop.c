#include "desktop.h"

#include "gfx.h"
#include "layout.h"
#include "settings.h"

/* 0000:05f3 up to the message box background (0000:0820). Not yet
   translated: the board coordinates (0000:0413, only with [B17C] & 1) and the
   message box text that follows. */
static void desktop_frame(Screen *screen, const Settings *settings)
{
    const Layout *l = &layout_vga;
    uint8_t outer = (uint8_t)settings->color_ad6e;
    uint8_t inner = (uint8_t)settings->color_b0c6;
    uint16_t top = l->menu_bar_height + 1;
    uint16_t width = gfx_scale_x(720);
    uint16_t height = gfx_scale_y(348) - top;
    uint16_t n = gfx_scale_line(4);

    gfx_draw_rect(screen, 0, top, width, height, outer, GFX_ROP_COPY);
    for (uint16_t i = 1; (int16_t)n > 0 && i <= n; i++)
        gfx_draw_rect(screen, i, top + i, width - 2 * i, height - 2 * i, inner,
                      GFX_ROP_COPY);
    /* Pads the border to 4 pixels when the scaled thickness is smaller. */
    for (uint16_t i = gfx_scale_line(4) + 1; (int16_t)i < 5; i++) {
        gfx_vline(screen, i, l->menu_bar_height + 2, height - 2, inner);
        gfx_vline(screen, width - i - 1, l->menu_bar_height + 2, height - 2, inner);
    }

    /* Message box below the board. */
    uint16_t x = l->board_x - 4;
    uint16_t y = gfx_scale_y(282);
    uint16_t w = gfx_scale_x(720) - 2 * x;
    uint16_t h = gfx_scale_y(52);
    gfx_draw_rect(screen, x, y, w, h, inner, GFX_ROP_COPY);
    gfx_draw_rect(screen, l->board_x - 3, y + 1, w - 2, h - 2, inner, GFX_ROP_COPY);
    gfx_vline(screen, l->board_x - 2, y + 2, h - 4, inner);
    gfx_vline(screen, w + x - 3, y + 2, h - 4, inner);
    gfx_fill_pattern(screen, l->board_x - 1, y + 2, w - 6, h - 4, outer,
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
}

/* 0000:2512 up to the call of 0000:05f3. */
void desktop_draw(Screen *screen, const Settings *settings)
{
    const Layout *l = &layout_vga;
    uint16_t top = l->menu_bar_height + 1;
    uint16_t width = gfx_scale_x(720);
    uint16_t height = gfx_scale_y(348);

    gfx_fill_pattern(screen, 0, 0, width, height, (uint8_t)settings->color_ad6e,
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_fill_pattern(screen, 0, top, width, height - top,
                     (uint8_t)((l->display_flags & 1) ? settings->colors_0f4c[1]  /* [0F4E] */
                                                      : settings->color_b0c6),
                     GFX_PATTERN_DESKTOP, GFX_ROP_COPY);
    desktop_frame(screen, settings);
}
