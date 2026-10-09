#ifndef FRITZ_GAME_BOARD_H
#define FRITZ_GAME_BOARD_H

#include <stdint.h>

/* Board array of the user interface (DS:048C and the start position at
   DS:AEF6): one byte per square, file-major, index (file-1)*8 + (rank-1).
   Byte: piece type in bits 0-2, colour in bit 3 (0 white, 1 black). */
enum {
    PIECE_NONE = 0,
    PIECE_KING = 1,
    PIECE_QUEEN = 2,
    PIECE_KNIGHT = 3,
    PIECE_BISHOP = 4,
    PIECE_ROOK = 5,
    PIECE_PAWN = 6,
    PIECE_BLACK = 8
};

typedef struct GameBoard {
    uint8_t squares[64];
} GameBoard;

static inline uint8_t game_board_at(const GameBoard *b, int file, int rank)
{
    return b->squares[(file - 1) * 8 + (rank - 1)];
}

/* 0a1f:70ed: initial position. */
void game_board_setup(GameBoard *board);

#endif
