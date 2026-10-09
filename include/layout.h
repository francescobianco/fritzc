#ifndef FRITZ_LAYOUT_H
#define FRITZ_LAYOUT_H

#include <stdint.h>

/* Per-card screen layout globals copied by 0000:7eaf (VGA branch) from
   initialised data: DS:0B8E..0B9B from DS:748A, DS:0B9C..0BA9 from DS:7498,
   DS:0BAA..0BAF from DS:73CC, DS:0BB0..0BB7 from DS:74A6. Names follow the
   addresses until the meaning of each value is known. */
typedef struct Layout {
    uint16_t w0b8e, w0b90, w0b92;
    uint16_t board_x;            /* [0B94] */
    uint16_t board_y;            /* [0B96] */
    uint16_t square_w;           /* [0B98] */
    uint16_t square_h;           /* [0B9A] */
    uint16_t w0b9c[7];
    uint16_t w0baa;
    uint16_t menu_bar_height;    /* [0BAC] */
    uint16_t w0bae;
    uint16_t display_flags;      /* [0BB0] */
    uint16_t w0bb2, w0bb4, w0bb6;
} Layout;

extern const Layout layout_vga;

#endif
