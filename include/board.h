#ifndef FRITZ_BOARD_VIEW_H
#define FRITZ_BOARD_VIEW_H

#include "game_board.h"
#include "screen.h"
#include "settings.h"

/* 0a1f:7d72: frame around the board. */
void board_frame_draw(Screen *screen, const Settings *settings);

/* 0a1f:78ac: all 64 squares. */
void board_draw(Screen *screen, const Settings *settings, const GameBoard *board);

/* 0a1f:74ac and 0a1f:7632: one square, empty or with a piece. */
void board_draw_empty_square(Screen *screen, const Settings *settings, int rank, int file);
void board_draw_piece_square(Screen *screen, const Settings *settings, int rank,
                             int file, int color, int type);

#endif
