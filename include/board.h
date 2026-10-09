#ifndef FRITZ_BOARD_VIEW_H
#define FRITZ_BOARD_VIEW_H

#include "screen.h"
#include "settings.h"

/* 0a1f:7d72: frame around the board. */
void board_frame_draw(Screen *screen, const Settings *settings);

#endif
