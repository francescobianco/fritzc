#ifndef FRITZ_PANELS_H
#define FRITZ_PANELS_H

#include <stdint.h>

#include "language.h"
#include "screen.h"
#include "settings.h"
#include "ui_state.h"

/* 0a1f:6356: framed panel with a shadow, logical coordinates. */
void panel_box(Screen *screen, const Settings *settings, uint16_t x, uint16_t y,
               uint16_t w, uint16_t h);

/* 0000:24ee: notation panel frame. */
void notation_panel_draw(Screen *screen, const Settings *settings);

/* 0000:21c5: the two clocks. */
void clocks_draw(Screen *screen, const Settings *settings, const UiState *ui);

/* 0000:2372: opening book line. */
void book_line_draw(Screen *screen, const Settings *settings, const UiState *ui);

/* 0a1f:00a4: status panel (game label). */
void status_panel_draw(Screen *screen, const Settings *settings, const Language *lan,
                       const UiState *ui);

/* Free memory text written by the main loop 0000:7a39. */
void memory_line_draw(Screen *screen, const Settings *settings, const UiState *ui);

/* 0000:31e5: notation text (first move number). */
void notation_text_draw(Screen *screen, const Settings *settings, UiState *ui);

/* Move input field of the main loop: 0000:6a31 and the cursor (0000:7961). */
void move_input_draw(Screen *screen, const Settings *settings, UiState *ui);

/* 0000:056d: first line of the message box. */
void message_line_draw(Screen *screen, const Settings *settings, const UiState *ui);

#endif
