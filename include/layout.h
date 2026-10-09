#ifndef FRITZ_LAYOUT_H
#define FRITZ_LAYOUT_H

#include <stdint.h>

/* Per-card screen layout globals copied by 0000:7eaf (VGA branch) from
   initialised data: DS:0B8E..0B9B from DS:748A, DS:0B9C..0BA9 from DS:7498,
   DS:0BAA..0BAF from DS:73CC, DS:0BB0..0BB7 from DS:74A6. Names follow the
   addresses until the meaning of each value is known. */
typedef struct Layout {
    /* Board geometry block passed as `param_1` to 0a1f:74ac/7632. */
    uint16_t piece_set;          /* [0B8E]: sprite set of the pieces */
    uint16_t mask_set;           /* [0B90]: sprite set of the silhouettes */
    uint16_t dark_pattern;       /* [0B92]: fill pattern of dark squares */
    uint16_t board_x;            /* [0B94] */
    uint16_t board_y;            /* [0B96] */
    uint16_t square_w;           /* [0B98] */
    uint16_t square_h;           /* [0B9A] */
    uint16_t w0b9c[7];
    uint16_t text_font;          /* [0BAA] */
    uint16_t menu_bar_height;    /* [0BAC]: also the text line height */
    uint16_t char_width;         /* [0BAE] */
    uint16_t display_flags;      /* [0BB0] */
    uint16_t w0bb2, w0bb4, w0bb6;
    uint8_t color_mode;          /* [ADCF]: set to 1 by 0000:7eaf */
} Layout;

extern const Layout layout_vga;

#endif
