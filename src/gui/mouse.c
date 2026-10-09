#include "mouse.h"

enum { SHAPE_TABLE = 0x69f8 };

static MouseShape shapes[MOUSE_SHAPE_COUNT];
static bool loaded;

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

bool mouse_shapes_load(const Exe *exe)
{
    const uint8_t *table = exe_at(exe, EXE_DGROUP, SHAPE_TABLE, 2 * MOUSE_SHAPE_COUNT);
    if (!table)
        return false;
    for (int i = 0; i < MOUSE_SHAPE_COUNT; i++) {
        const uint8_t *s = exe_at(exe, EXE_DGROUP, rd16(table + 2 * i), 0x44);
        if (!s)
            return false;
        for (int r = 0; r < 16; r++) {
            shapes[i].screen_mask[r] = rd16(s + 2 * r);
            shapes[i].cursor_mask[r] = rd16(s + 32 + 2 * r);
        }
        shapes[i].hot_x = (int16_t)rd16(s + 0x40);
        shapes[i].hot_y = (int16_t)rd16(s + 0x42);
    }
    loaded = true;
    return true;
}

const MouseShape *mouse_shape(int index)
{
    return loaded && index >= 0 && index < MOUSE_SHAPE_COUNT ? &shapes[index] : NULL;
}

void mouse_draw_pointer(Screen *screen, int x, int y, const MouseShape *shape)
{
    if (!shape)
        return;
    for (int r = 0; r < 16; r++) {
        int py = y - shape->hot_y + r;
        if (py < 0 || py >= SCREEN_HEIGHT)
            continue;
        for (int c = 0; c < 16; c++) {
            int px = x - shape->hot_x + c;
            if (px < 0 || px >= SCREEN_WIDTH)
                continue;
            uint16_t bit = (uint16_t)(0x8000u >> c);
            uint8_t p = screen->pixels[py][px];
            if (!(shape->screen_mask[r] & bit))
                p = 0;
            if (shape->cursor_mask[r] & bit)
                p ^= 15;
            screen->pixels[py][px] = p;
        }
    }
}
