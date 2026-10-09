#include "ui_text.h"

#include <string.h>

#include "font.h"
#include "gfx.h"
#include "layout.h"

/* 0a1f:64b5 */
static void clear_text_area(Screen *screen, const Settings *settings,
                            uint8_t length, uint16_t y, uint16_t column)
{
    const Layout *l = &layout_vga;
    gfx_fill_pattern(screen, l->char_width * column, gfx_scale_y(y),
                     l->char_width * length, l->menu_bar_height,
                     (uint8_t)settings->color_ad6e, GFX_PATTERN_SOLID, GFX_ROP_COPY);
}

void ui_text_line(Screen *screen, const Settings *settings, const char *text,
                  uint16_t color, uint16_t y, uint16_t column)
{
    const Layout *l = &layout_vga;
    uint8_t pstring[256];
    size_t n = strlen(text);

    /* 293b:1dee copies the C string and stores the length in one byte. */
    memcpy(pstring + 1, text, n > 255 ? 255 : n);
    pstring[0] = (uint8_t)n;

    uint8_t mode = settings->color_b0c6 == color ? TEXT_INVERSE : 0;
    clear_text_area(screen, settings, pstring[0], y, column);
    gfx_draw_text_p(screen, l->char_width * column, gfx_scale_y(y), l->text_font,
                    pstring, (uint8_t)settings->color_b0c6, mode);
}

void ui_message(Screen *screen, const Settings *settings, const Language *lan,
                uint16_t index, uint16_t color, uint16_t y, uint16_t column)
{
    ui_text_line(screen, settings,
                 language_string(lan, LAN_BLOCK_MESSAGES, index), color, y, column);
}
