#include "panels.h"

#include "font.h"
#include "gfx.h"
#include "layout.h"
#include "ui_text.h"

#include <stdio.h>
#include <string.h>

/* Colour table entries (DS:0F4C + 2·n). */
enum {
    COLOR_INK = 0,      /* [0F4C] */
    COLOR_SHADOW = 7,   /* [0F5A], ANDed onto the background */
    COLOR_PAPER = 15    /* [0F6A] */
};

static uint8_t table_color(const Settings *settings, int index)
{
    return (uint8_t)settings->colors_0f4c[index];
}

void panel_box(Screen *screen, const Settings *settings, uint16_t x, uint16_t y,
               uint16_t w, uint16_t h)
{
    const Layout *l = &layout_vga;
    uint16_t dx = gfx_scale_x(x), dy = gfx_scale_y(y);
    uint16_t dw = gfx_scale_x(w), dh = gfx_scale_y(h);
    uint16_t sx = gfx_scale_x(x + 8), sy = gfx_scale_y(y + 5);

    if (!(l->display_flags & 1))
        gfx_fill_pattern(screen, sx, sy, dw, dh, (uint8_t)settings->color_b0c6,
                         GFX_PATTERN_SOLID, GFX_ROP_COPY);
    else
        gfx_fill_pattern(screen, sx, sy, dw, dh, table_color(settings, COLOR_SHADOW),
                         GFX_PATTERN_SOLID, GFX_ROP_AND);
    gfx_fill_pattern(screen, dx, dy, dw, dh, (uint8_t)settings->color_ad6e,
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_draw_rect(screen, dx, dy, dw, dh, (uint8_t)settings->color_b0c6, GFX_ROP_COPY);
}

void notation_panel_draw(Screen *screen, const Settings *settings)
{
    panel_box(screen, settings, 0x18e, 0xbe, 0x110, 0x37);
}

/* Clock widget 293b:2a12 (set-up) + 293b:2abb (draw): background of
   8 characters × one text line, then the time string. */
static void clock_widget(Screen *screen, uint16_t x, uint16_t y, uint8_t paper,
                         uint8_t ink, const char *text)
{
    const Layout *l = &layout_vga;
    gfx_fill_pattern(screen, x, y, (uint16_t)((l->char_width & 0xff) * 8),
                     l->menu_bar_height, paper, GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_draw_text(screen, x, y, l->text_font, text, ink, 0);
}

void clocks_draw(Screen *screen, const Settings *settings, const UiState *ui)
{
    const Layout *l = &layout_vga;
    uint8_t paper, ink;
    if (!(ui->side_flag & 1)) {
        paper = (uint8_t)settings->color_ad6e;
        ink = (uint8_t)settings->color_b0c6;
    } else {
        paper = (uint8_t)settings->color_b0c6;
        ink = (uint8_t)settings->color_ad6e;
    }

    uint16_t px = gfx_scale_x(0x18e), pw = gfx_scale_x(0x110);
    uint16_t y = gfx_scale_y(0x84), h = gfx_scale_y(0x23);
    uint16_t w = l->char_width * 9 + 10;
    uint16_t x = px + pw - w;
    gfx_fill_pattern(screen, x, y, w, h, paper, GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_draw_rect(screen, x, y, w, h, (uint8_t)settings->color_b0c6, GFX_ROP_COPY);

    x += 5;
    if ((ui->hide_marker[0] & 1) && (ui->hide_marker[1] & 1))
        x -= (int16_t)l->char_width / 2;
    uint16_t row0, row1;   /* clock 0 = white, clock 1 = black */
    if (!(settings->flip_board & 1)) {
        row0 = y - l->menu_bar_height + h - 1;
        row1 = y + 2;
    } else {
        row0 = y + 2;
        row1 = y - l->menu_bar_height + h - 1;
    }
    clock_widget(screen, l->char_width + x, row0, paper, ink, ui->clock_text[0]);
    clock_widget(screen, l->char_width + x, row1, paper, ink, ui->clock_text[1]);

    /* Marker (character 0x19) beside the clock whose flag is clear. */
    static const uint8_t marker[2] = {1, 0x19};
    if (!(ui->hide_marker[0] & 1))
        gfx_draw_text_p(screen, x, row0, l->text_font, marker, ink, 0);
    if (!(ui->hide_marker[1] & 1))
        gfx_draw_text_p(screen, x, row1, l->text_font, marker, ink, 0);
}

/* The number at column 25 is 1 << p[0xFE], p = [7190] being the engine
   module block (docs/engine-analysis.md); not available until the engine
   interface is reconstructed. */
void book_line_draw(Screen *screen, const Settings *settings, const UiState *ui)
{
    const Layout *l = &layout_vga;
    uint8_t ink = table_color(settings, COLOR_INK);
    uint16_t x = gfx_scale_x(0x18e), w = gfx_scale_x(0x110);
    uint16_t y = gfx_scale_y(0xfe), h = l->menu_bar_height + 2;

    gfx_fill_pattern(screen, x, y, w, h, table_color(settings, COLOR_PAPER),
                     GFX_PATTERN_SOLID, GFX_ROP_COPY);
    gfx_draw_rect(screen, x, y, w, h, ink, GFX_ROP_COPY);
    gfx_draw_text(screen, l->char_width + x, y + 1, l->text_font, ui->book_line_left, ink, 0);
    gfx_draw_text(screen, l->char_width * 16 + x, y + 1, l->text_font, ui->book_name, ink, 0);
    message_line_draw(screen, settings, ui);
}

/* The text (built by 19e7:c9f2) is only drawn when [AE6A] & 1. */
void message_line_draw(Screen *screen, const Settings *settings, const UiState *ui)
{
    const Layout *l = &layout_vga;
    if (ui->message_busy & 1)
        return;
    gfx_fill_pattern(screen, gfx_scale_x(0x28), gfx_scale_y(0x11e), l->char_width * 0x44,
                     l->menu_bar_height, (uint8_t)settings->color_ad6e, GFX_PATTERN_SOLID,
                     GFX_ROP_COPY);
}

/* 0a1f:6432 and 0a1f:6475: fill and frame in logical coordinates. */
static void fill_logical(Screen *screen, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         uint16_t pattern, uint8_t color)
{
    gfx_fill_pattern(screen, gfx_scale_x(x), gfx_scale_y(y), gfx_scale_x(w),
                     gfx_scale_y(h), color, pattern, GFX_ROP_COPY);
}

static void rect_logical(Screen *screen, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         uint8_t color)
{
    gfx_draw_rect(screen, gfx_scale_x(x), gfx_scale_y(y), gfx_scale_x(w), gfx_scale_y(h),
                  color, GFX_ROP_COPY);
}

/* The branch for a loaded database game ([ADD4] >= 2: game number and
   messages built with 0a1f:690d / 0000:9a0a) is not translated yet. */
void status_panel_draw(Screen *screen, const Settings *settings, const Language *lan,
                       const UiState *ui)
{
    fill_logical(screen, 0x18e, 0xa9, 0x110, 0x12, GFX_PATTERN_SOLID,
                 (uint8_t)settings->color_ad6e);
    rect_logical(screen, 0x18e, 0xa9, 0x110, 0x12, (uint8_t)settings->color_b0c6);
    if (ui->game_number <= 0)
        ui_message(screen, settings, lan, 2, settings->color_ad6e, 0xac, 0x3c);
    else if (ui->game_number == 1)
        ui_message(screen, settings, lan, 0, settings->color_ad6e, 0xac, 0x3a);
}

/* 0a1f:690d formats the number, 0000:9a0a appends the Pascal string
   DS:7473 " kB  ". */
void memory_line_draw(Screen *screen, const Settings *settings, const UiState *ui)
{
    char text[32];
    snprintf(text, sizeof text, "%ld kB  ", ui->free_memory_kb);
    ui_text_line(screen, settings, text, settings->color_ad6e, 0xac, 0x2e);
}

/* (ply + parity) / 2 + offset, with long arithmetic as in 0000:9088. */
static long move_number(const UiState *ui)
{
    return ((long)ui->ply + ui->move_parity) / 2 + ui->move_offset;
}

/* 0a1f:6546: one character at a text column, inverted when the colour
   equals [B0C6]. */
static void ui_char(Screen *screen, const Settings *settings, char ch, uint16_t color,
                    uint16_t y, uint16_t column)
{
    char text[2] = {ch, '\0'};
    ui_text_line(screen, settings, text, color, y, column);
}

void notation_text_draw(Screen *screen, const Settings *settings, UiState *ui)
{
    char text[32] = "";
    if (!(ui->side_flag & 1))
        snprintf(text, sizeof text, "%ld. ...", move_number(ui) - 1);   /* DS:7441 */
    text[15] = '\0';   /* 0000:9a0a limit */
    clear_text_line(screen, settings, 0x1d, 0xdc, 0x2d);
    ui_text_line(screen, settings, text, settings->color_ad6e, 0xdc, 0x2f);
    /* With [B156] > 1 the moves are replayed (0000:1c36, 0000:1a3c): not
       translated yet. */
}

void move_input_draw(Screen *screen, const Settings *settings, UiState *ui)
{
    char text[32];
    snprintf(text, sizeof text, "%ld.", move_number(ui));   /* DS:7471 */
    ui_text_line(screen, settings, text, settings->color_ad6e, 0xdc, 0x2f);
    ui->input_column = (ui->side_flag & 1) ? 0xf : (int16_t)strlen(text);
    clear_text_line(screen, settings, 0xb, 0xdc, (uint16_t)(ui->input_column + 0x2f));
    /* Cursor at the first input position ([047B] == [047D] at start). */
    ui_char(screen, settings, ' ', settings->color_b0c6, 0xdc,
            (uint16_t)(ui->input_column + 1 + 0x2e));
}
