#include "game_board.h"

void game_board_setup(GameBoard *board)
{
    static const uint8_t back_rank[8] = {
        PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN,
        PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK,
    };
    uint8_t *p = board->squares;

    for (int file = 1; file <= 8; file++) {
        p[(file - 1) * 8 + 0] = back_rank[file - 1];
        p[(file - 1) * 8 + 1] = PIECE_PAWN;
        p[(file - 1) * 8 + 6] = PIECE_PAWN | PIECE_BLACK;
        p[(file - 1) * 8 + 7] = back_rank[file - 1] | PIECE_BLACK;
        for (int rank = 3; rank <= 6; rank++)
            p[(file - 1) * 8 + rank - 1] = PIECE_NONE;
    }
}
