#include "board.h"

#include "gfx.h"
#include "layout.h"

void board_frame_draw(Screen *screen, const Settings *settings)
{
    const Layout *l = &layout_vga;
    uint8_t outer = (uint8_t)settings->color_ac30;
    uint16_t bw = l->square_w * 8;
    uint16_t bh = l->square_h * 8;
    uint16_t i;

    gfx_draw_rect(screen, l->board_x - 1, l->board_y - 1, bw + 2, bh + 2, outer,
                  GFX_ROP_COPY);
    gfx_draw_rect(screen, l->board_x - 2, l->board_y - 2, bw + 4, bh + 4,
                  (uint8_t)settings->color_b112, GFX_ROP_COPY);
    uint16_t n = gfx_scale_line(3) + 2;
    for (i = 3; i <= n; i++)
        gfx_draw_rect(screen, l->board_x - i, l->board_y - i, bw + 2 * i, bh + 2 * i,
                      outer, GFX_ROP_COPY);
    /* Pads the frame to 5 pixels when the scaled thickness is smaller. */
    uint16_t h = bh + 2 * n;
    for (; i < 6; i++) {
        gfx_vline(screen, l->board_x - i, l->board_y - n, h, outer);
        gfx_vline(screen, l->board_x + bw + i - 1, l->board_y - n, h, outer);
    }
}
