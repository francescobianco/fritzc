#ifndef FRITZ_SETTINGS_H
#define FRITZ_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

/* FRITZ2.EXE globals filled by the settings loader 19e7:4294.
   Field names follow their DGROUP addresses until their meaning is known. */
typedef struct Settings {
    uint16_t card_id;            /* [AFAE]+2: graphics card descriptor id */
    uint16_t color_ad6e;         /* [AD6E]: background, menu bar */
    uint16_t color_b0c6;         /* [B0C6] */
    uint16_t colors_0f4c[28];    /* DS:0F4C..0F83: colour table */
} Settings;

/* Defaults applied by 19e7:4294 when called with reload == 0, for the VGA
   card descriptor DS:0E28. */
void settings_defaults(Settings *settings);

/* Reads a FRITZ2.PAR record (magic 200, 0x212 bytes) over the defaults, as
   19e7:4294 does. Returns false and leaves the settings untouched if the
   file is missing, short, has a wrong magic or belongs to another card. */
bool settings_load_par(Settings *settings, const char *path);

#endif
