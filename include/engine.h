#ifndef FRITZ_ENGINE_H
#define FRITZ_ENGINE_H

#include <stdint.h>

/* C reconstruction of the FRITZ2.BIN engine (docs/engine-analysis.md).

   The engine works on one 64 KB segment: code, tables and variables share
   it, and every routine addresses its data by fixed offsets. The C version
   keeps that segment as `mem`, with the same offsets, so that each routine
   can be checked against the original one on the same memory image
   (tests/engine). Names for the offsets are introduced as their meaning
   is established. */
typedef struct Engine {
    uint8_t mem[0x10000];
} Engine;

static inline uint8_t eng_rd8(const Engine *e, uint16_t off)
{
    return e->mem[off];
}

static inline uint16_t eng_rd16(const Engine *e, uint16_t off)
{
    return (uint16_t)(e->mem[off] | e->mem[(uint16_t)(off + 1)] << 8);
}

static inline void eng_wr8(Engine *e, uint16_t off, uint8_t v)
{
    e->mem[off] = v;
}

static inline void eng_wr16(Engine *e, uint16_t off, uint16_t v)
{
    e->mem[off] = (uint8_t)v;
    e->mem[(uint16_t)(off + 1)] = (uint8_t)(v >> 8);
}

/* Board: 10x12 mailbox of words, a1 at BOARD_A1, ranks 20 bytes apart,
   border squares 0xFFFE. Piece codes: white pawn 0x10, knight 0x12,
   bishop 0x14, rook 0x16/0x18, queen 0x1A, king 0x1C/0x1E; black +0x10. */
enum {
    BOARD_A1 = 0x132,
    BOARD_H8 = 0x1cc - 2,
    BOARD_RANK = 0x14,
    SQ_INFO = 0x100,      /* + square: per-square data, piece index ×2 */
    ATTACK_WHITE = 0x200, /* + square: attack mask word of white pieces */
    ATTACK_BLACK = 0x300, /* + square: same for black */
    PAWN_PUSH = 0x400,    /* + square: pawn advance bytes */
    PIECE_MASKS = 0x42f0  /* word per piece index: ~(1 << n) */
};

/* 0x46b7: clears the attack tables (border 0xFFFE, squares 0). */
void eng_attacks_clear(Engine *e);

/* 0x46f8: adds every piece on the board to the attack tables. */
void eng_attacks_build(Engine *e);

/* 0x45d4 dispatch: toggles the attacks of the piece `code` on square `di`
   with mask `dx` (the same call adds and removes). */
void eng_piece_attacks(Engine *e, uint16_t code, uint16_t di, uint16_t dx);

/* 0x8692: static exchange evaluation of the piece on `si` capturing on
   `di`, side chosen by bit 0x10 of `dx`. Returns AX (AH = gain estimate,
   AL = attacker balance). */
uint16_t eng_see(Engine *e, uint16_t si, uint16_t di, uint16_t dx);

#endif
