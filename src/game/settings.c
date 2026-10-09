#include "settings.h"

#include <stdio.h>
#include <string.h>

enum {
    PAR_MAGIC = 200,
    PAR_RECORD_SIZE = 0x212,   /* bytes read by 393b:20a3 */
    PAR_LANGUAGE = 0x004,
    PAR_COLOR_AD6E = 0x194,
    PAR_COLOR_B0C6 = 0x196,
    PAR_COLOR_B112 = 0x198,
    PAR_COLOR_AC30 = 0x19a,
    PAR_COLORS_0F4C = 0x19c,
    PAR_BOOK_DIR = 0x13a,
    PAR_BOOK_NAME = 0x180,
    PAR_FLIP_BOARD = 0x1d4,
    PAR_SWAP_COLORS = 0x1d6
};

/* VGA card descriptor at DS:0E28: flags, id, ?, default background. */
static const uint16_t card_vga[4] = {1, 18, 3, 15};

/* Initialised data at DS:0F4C. */
static const uint16_t default_colors_0f4c[28] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    10, 11, 12, 14, 15, 15, 0, 15, 0, 15, 0, 15,
};

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

void settings_defaults(Settings *s)
{
    s->card_id = card_vga[1];
    /* Without a PAR file the original asks for the language; English is the
       port's default. */
    strcpy(s->language_file, "FENGLISH.LAN");
    s->color_ad6e = card_vga[3];
    s->color_b0c6 = 0;
    s->color_b112 = card_vga[3];
    s->color_ac30 = 0;
    memcpy(s->colors_0f4c, default_colors_0f4c, sizeof s->colors_0f4c);
    s->flip_board = 0;
    s->swap_colors = 0;
    s->book_dir[0] = '\0';
    strcpy(s->book_name, "Fritz2");   /* initialised data at DS:4304 */
    /* With a monochrome descriptor (flags bit 0 clear) 19e7:4294 also sets
       DS:0F6C..0F73 to 15; the VGA descriptor is a colour one. */
    if (!(card_vga[0] & 1))
        for (int i = 0x10; i < 0x14; i++)
            s->colors_0f4c[i] = 15;
}

bool settings_load_par(Settings *s, const char *path)
{
    uint8_t rec[PAR_RECORD_SIZE];
    FILE *f = fopen(path, "rb");

    if (!f)
        return false;
    size_t n = fread(rec, 1, sizeof rec, f);
    fclose(f);
    if (n != sizeof rec || read16(rec) != PAR_MAGIC || read16(rec + 2) != s->card_id)
        return false;

    memcpy(s->language_file, rec + PAR_LANGUAGE, 16);
    s->language_file[16] = '\0';
    s->color_ad6e = read16(rec + PAR_COLOR_AD6E);
    s->color_b0c6 = read16(rec + PAR_COLOR_B0C6);
    s->color_b112 = read16(rec + PAR_COLOR_B112);
    s->color_ac30 = read16(rec + PAR_COLOR_AC30);
    for (int i = 0; i < 28; i++)
        s->colors_0f4c[i] = read16(rec + PAR_COLORS_0F4C + 2 * i);
    memcpy(s->book_dir, rec + PAR_BOOK_DIR, 70);
    s->book_dir[70] = '\0';
    memcpy(s->book_name, rec + PAR_BOOK_NAME, 14);
    s->book_name[14] = '\0';
    s->flip_board = rec[PAR_FLIP_BOARD];
    s->swap_colors = rec[PAR_SWAP_COLORS];
    return true;
}
