#ifndef FRITZ_MOUSE_H
#define FRITZ_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

#include "exe.h"
#include "screen.h"

/* Graphics pointer shapes set with int 33h function 9 by 293b:1fb6: table
   DS:69F8 -> 16-word screen (AND) mask, 16-word cursor (XOR) mask, hot
   spot x/y at +0x40/+0x42. 0000:7dd3 selects shape 0 at start-up. */
enum { MOUSE_SHAPE_COUNT = 10 };

typedef struct MouseShape {
    uint16_t screen_mask[16];
    uint16_t cursor_mask[16];
    int16_t hot_x, hot_y;
} MouseShape;

bool mouse_shapes_load(const Exe *exe);
const MouseShape *mouse_shape(int index);

/* What the DOS mouse driver does in a 16-colour mode: each pixel of the
   16x16 block becomes (pixel AND screen) XOR cursor, per bit plane. */
void mouse_draw_pointer(Screen *screen, int x, int y, const MouseShape *shape);

#endif
