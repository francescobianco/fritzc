#include "board.h"

#include <stdbool.h>

#include "gfx.h"
#include "layout.h"
#include "sprites.h"

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

/* Colour table entries used in colour mode (DS:0F74..0F82). */
enum {
    COLOR_LIGHT_SQUARE = 20,   /* [0F74] */
    COLOR_DARK_SQUARE = 21,    /* [0F76] */
    COLOR_DARK_HATCH = 22,     /* [0F78] */
    COLOR_WHITE_OUTLINE = 24,  /* [0F7C] */
    COLOR_WHITE_BODY = 25,     /* [0F7E] */
    COLOR_BLACK_OUTLINE = 26,  /* [0F80] */
    COLOR_BLACK_BODY = 27      /* [0F82] */
};

/* Colour mode needs [ADCF] & 1 and two different sprite sets. */
static bool colour_mode(const Layout *l)
{
    return (l->color_mode & 1) && l->piece_set != l->mask_set;
}

static uint8_t table_color(const Settings *settings, int index)
{
    return (uint8_t)settings->colors_0f4c[index];
}

/* Both routines flip ranks for the normal view and files for the flipped
   one ([B154]). Calls of 0a1f:73b0, made only when [4256] != 0, are not
   translated. */
void board_draw_empty_square(Screen *screen, const Settings *settings, int rank, int file)
{
    const Layout *l = &layout_vga;
    if (!(settings->flip_board & 1))
        rank = 9 - rank;
    else
        file = 9 - file;
    unsigned dark_parity = !(settings->swap_colors & 1);
    uint16_t x = l->square_w * (file - 1) + l->board_x;
    uint16_t y = l->square_h * (rank - 1) + l->board_y;
    bool dark = ((unsigned)(rank + file) & 1) == dark_parity;

    if (!colour_mode(l)) {
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         (uint8_t)settings->color_b112, GFX_PATTERN_SOLID, GFX_ROP_COPY);
        if (dark)
            gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                             (uint8_t)settings->color_ac30, l->dark_pattern, GFX_ROP_COPY);
    } else if (dark) {
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         table_color(settings, COLOR_DARK_SQUARE), GFX_PATTERN_SOLID,
                         GFX_ROP_COPY);
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         table_color(settings, COLOR_DARK_HATCH), l->dark_pattern,
                         GFX_ROP_COPY);
    } else {
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         table_color(settings, COLOR_LIGHT_SQUARE), GFX_PATTERN_SOLID,
                         GFX_ROP_COPY);
    }
}

void board_draw_piece_square(Screen *screen, const Settings *settings, int rank,
                             int file, int color, int type)
{
    const Layout *l = &layout_vga;
    unsigned parity;
    if (!(settings->flip_board & 1))
        rank = 9 - rank;
    else
        file = 9 - file;
    if (!(settings->swap_colors & 1)) {
        parity = (unsigned)(rank + file);
    } else {
        color = 1 - color;
        parity = (unsigned)(rank + file + 1);
    }
    parity &= 1;
    uint16_t x = l->square_w * (file - 1) + l->board_x;
    uint16_t y = l->square_h * (rank - 1) + l->board_y;

    if (!colour_mode(l)) {
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         (uint8_t)settings->color_b112, GFX_PATTERN_SOLID, GFX_ROP_COPY);
        gfx_draw_piece(screen, x, y, l->piece_set, (uint16_t)(color * 8 + type), parity,
                       (uint8_t)settings->color_ac30);
        return;
    }
    if (parity == 1) {
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         table_color(settings, COLOR_DARK_SQUARE), GFX_PATTERN_SOLID,
                         GFX_ROP_COPY);
        /* Variant 1: the piece cut out of a pre-hatched square. */
        gfx_draw_piece(screen, x, y, l->piece_set, (uint16_t)(color * 8 + type), 1,
                       table_color(settings, COLOR_DARK_HATCH));
    } else {
        gfx_fill_pattern(screen, x, y, l->square_w, l->square_h,
                         table_color(settings, COLOR_LIGHT_SQUARE), GFX_PATTERN_SOLID,
                         GFX_ROP_COPY);
    }
    if (color == 0) {
        gfx_draw_piece(screen, x, y, l->mask_set, (uint16_t)type, 1,
                       table_color(settings, COLOR_WHITE_BODY));
        gfx_draw_piece(screen, x, y, l->piece_set, (uint16_t)type, 0,
                       table_color(settings, COLOR_WHITE_OUTLINE));
    } else {
        gfx_draw_piece(screen, x, y, l->mask_set, (uint16_t)type, 1,
                       table_color(settings, COLOR_BLACK_BODY));
        gfx_draw_piece(screen, x, y, l->piece_set, (uint16_t)(type + 8), 0,
                       table_color(settings, COLOR_BLACK_OUTLINE));
    }
}

/* The opening-book check done first when [4256] != 0 is not translated. */
void board_draw(Screen *screen, const Settings *settings, const GameBoard *board)
{
    for (int file = 1; file <= 8; file++)
        for (int rank = 1; rank <= 8; rank++) {
            uint8_t code = game_board_at(board, file, rank);
            int type = code & 7;
            int color = (int8_t)code >> 3;
            if (type == PIECE_NONE)
                board_draw_empty_square(screen, settings, rank, file);
            else
                board_draw_piece_square(screen, settings, rank, file, color, type);
        }
}
