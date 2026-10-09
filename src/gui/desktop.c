#include "desktop.h"

#include <stdbool.h>

#include "board.h"
#include "font.h"
#include "gfx.h"
#include "logo.h"
#include "panels.h"
#include "layout.h"
#include "settings.h"
#include "ui_text.h"

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

/* [AFB2], set by 0000:7d99 when a mouse driver answers int 33h. SDL always
   provides a pointer, so the port behaves as with a mouse. */
static const bool mouse_present = true;

/* 0000:2037(colour, h, w, y, x), logical coordinates: the box with the two
   move arrows (previous/next), drawn only when a mouse is present. The
   arrows are tiles of font 3: 2+3 left, 0+1 right. */
static void arrow_buttons(Screen *screen, const Settings *settings, uint8_t color,
                          uint16_t h, uint16_t w, uint16_t y, uint16_t x)
{
    if (!mouse_present)
        return;
    uint8_t inner = (uint8_t)settings->color_b0c6;
    x = gfx_scale_x(x);
    y = gfx_scale_y(y);
    w = gfx_scale_x(w);
    h = gfx_scale_y(h);
    gfx_fill_pattern(screen, x, y, w, h, (uint8_t)settings->color_ad6e,
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_draw_rect(screen, x, y, w, h, inner, GFX_ROP_COPY);
    gfx_draw_rect(screen, x + 2, y + 2, w - 4, h - 4, inner, GFX_ROP_COPY);
    gfx_vline(screen, (int16_t)w / 2 + x, y + 3, h - 6, inner);

    uint16_t ty = y + (int16_t)(h - 14) / 2;
    int16_t quarter = (int16_t)w / 4;
    static const struct { int16_t dx; uint8_t tile; } tiles[] = {
        {-4, 2}, {4, 3}, {-4, 0}, {4, 1},
    };
    for (int i = 0; i < 4; i++) {
        uint8_t pstring[2] = {1, tiles[i].tile};
        uint16_t tx = (uint16_t)((i < 2 ? quarter : quarter * 3) + x + tiles[i].dx);
        gfx_draw_text_p(screen, tx, ty, 3, pstring, color, 0);
    }
}

/* 0000:2512. Translated: the background, the frame (0000:05f3) and the
   arrow buttons (0000:2037) and the menu bar at the end. 0000:022f draws
   nothing on the start screen; 0a1f:7d72 is board_frame_draw and
   0a1f:78ac(DS:0B8E, DS:048C) is board_draw. Not yet
   translated: the 0a1f:7a98 group (drawn instead
   of the logo in some states). 19e7:a994 is
   logo_draw; 0000:21c5, 0000:2372, 0000:24ee are in panels.c. */
void desktop_draw(Screen *screen, const Settings *settings, const Language *lan,
                  const GameBoard *board, const Logo *logo, UiState *ui)
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
    board_frame_draw(screen, settings);
    board_draw(screen, settings, board);
    arrow_buttons(screen, settings, (uint8_t)settings->color_b0c6, 0x23, 0xa0, 0x84, 0x18e);
    status_panel_draw(screen, settings, lan, ui);
    if (ui->engine_active & 1)
        clocks_draw(screen, settings, ui);
    /* Drawn when !([B161] & [005E] & 1) && !([0052] & 1); the start screen
       satisfies both. */
    if (logo && logo->rows)
        logo_draw(screen, settings, logo, gfx_scale_x(0x18e), l->board_y - 4);

    /* Menu bar text and the line below it. */
    ui_message(screen, settings, lan, 0x1c, settings->color_ad6e, 0, 1);
    gfx_hline(screen, 0, l->menu_bar_height, width, (uint8_t)settings->color_b0c6);
    if (ui->engine_active & 1)
        book_line_draw(screen, settings, ui);
    notation_panel_draw(screen, settings);
    notation_text_draw(screen, settings, ui);

    /* First pass of the main loop 0000:7a39. */
    memory_line_draw(screen, settings, ui);
    move_input_draw(screen, settings, ui);
}
