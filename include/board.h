#ifndef FRITZ_BOARD_VIEW_H
#define FRITZ_BOARD_VIEW_H

#include <stdbool.h>

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

/* 0a1f:719e: square cursor (three nested rectangles in [AC30]). */
void board_cursor_draw(Screen *screen, const Settings *settings, int rank, int file);

/* 0a1f:7843: repaint one square from the board array. */
void board_redraw_square(Screen *screen, const Settings *settings,
                         const GameBoard *board, int rank, int file);

/* 0000:6c2a..6cc2: square under the pointer, false when outside the board. */
bool board_square_at(const Settings *settings, int x, int y, int *rank, int *file);

#endif
