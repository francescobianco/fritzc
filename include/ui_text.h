#ifndef FRITZ_UI_TEXT_H
#define FRITZ_UI_TEXT_H

#include <stdint.h>

#include "language.h"
#include "screen.h"
#include "settings.h"

/* 0a1f:65bb: text line at character column `column` and logical row `y`
   (720x348 space). The area is first cleared with [AD6E] (0a1f:64b5);
   the text is drawn in [B0C6], inverted when `color` equals [B0C6]. */
void ui_text_line(Screen *screen, const Settings *settings, const char *text,
                  uint16_t color, uint16_t y, uint16_t column);

/* 0a1f:67c3: same, with string `index` of the messages block (DS:77AE). */
void ui_message(Screen *screen, const Settings *settings, const Language *lan,
                uint16_t index, uint16_t color, uint16_t y, uint16_t column);

#endif
