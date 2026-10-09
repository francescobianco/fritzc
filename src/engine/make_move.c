/* Make move, FRITZ2.BIN 0x6f81.

   In: SI from, DI to, CX move flags (CL = captured piece code plus flag
   bits: 0x40 en passant, 1 with code 0 castling, 0x41 double push), DL side
   to move, BP ply × 2. Out: AX = evaluation after the move (negamax
   perspective), CX is changed only by the final clamp. Per-ply arrays
   (offset + BP): 0x1650 / 0x16a0 hash key, 0xf1c moving piece, 0x11f0
   evaluation, 0x1012 flags, 0x10b2 captured piece index, 0xe7a en passant
   victim square. */
#include "engine.h"

#define B(off) eng_rd8(e, (uint16_t)(off))
#define W(off) eng_rd16(e, (uint16_t)(off))

enum {
    PLY_HASH_LO = 0x1650,
    PLY_HASH_HI = 0x16a0,
    PLY_MOVER = 0xf1c,
    PLY_EVAL = 0x11f0,
    PLY_FLAGS = 0x1012,
    PLY_CAPTURED = 0x10b2,
    PLY_EP_SQUARE = 0xe7a,
    CAPTURE_VALUE = 0x710e,   /* + piece code: material value */
    COUNT_ADDRESS = 0x714e,   /* + piece code: material counter address */
    COUNT_AMOUNT = 0x716e     /* + piece code: amount removed */
};

/* Call one of the register routines with the current registers. */
static void eval_remove(Engine *e, EngRegs *r) { eng_eval_remove(e, r); }
static void eval_add(Engine *e, EngRegs *r) { eng_eval_add(e, r); }

static void toggle(Engine *e, uint16_t code, uint16_t di, uint16_t dx)
{
    eng_piece_toggle(e, code, di, dx);
}

/* 0x7009: after a capture that emptied a material counter, the score is
   kept inside bounds that depend on the material left. */
static void clamp(Engine *e, EngRegs *r, uint8_t dl)
{
    uint16_t bx = 0xfff8, cx = 8;
    if (B(0x68a) == 0 && B(0x68b) < 5)
        bx = 0x8000;
    if (B(0x69a) == 0 && B(0x69b) < 5)
        cx = 0x8000;
    if (!(dl & 0x10)) {
        uint16_t t = cx;
        cx = (uint16_t)-bx;
        bx = (uint16_t)-t;
    }
    if (r->ax >= bx)
        r->ax = bx;
    if (r->ax < cx)
        r->ax = cx;
    r->cx = cx;
}

void eng_make_move(Engine *e, EngRegs *r)
{
    uint16_t ply = r->bp, from = r->si, to = r->di, dx_in = r->dx;
    uint8_t dl = (uint8_t)dx_in;

    eng_wr16(e, HASH_KEY, W(ply + PLY_HASH_LO));
    eng_wr16(e, HASH_KEY + 2, W(ply + PLY_HASH_HI));
    uint16_t mover = W(from);
    eng_wr16(e, from, 0);
    eng_wr16(e, ply + PLY_MOVER, mover);

    EngRegs t = *r;
    t.ax = W(ply + PLY_EVAL);
    uint16_t captured_index = W(to + SQ_INFO);
    uint16_t index = W(from + SQ_INFO);
    eng_wr16(e, to + SQ_INFO, index);
    eng_wr16(e, index + 0x5e0, to);
    t.dx = W(index + PIECE_MASKS);
    t.bp = mover;

    /* Leave the from-square (DI = from, SI = to). */
    t.di = from;
    t.si = to;
    eval_remove(e, &t);
    toggle(e, t.bp, from, t.dx);
    eng_square_update(e, from);
    t.ax = (uint16_t)(-t.ax - 0x10);

    /* Arrive on the to-square (DI = to, SI = from). */
    t.di = to;
    t.si = from;
    eval_add(e, &t);
    eng_wr16(e, to, t.bp);
    toggle(e, t.bp, to, t.dx);

    uint16_t cx = t.cx;
    uint16_t victim = cx & 0x3e;
    r->ax = t.ax;
    r->cx = cx;
    if (victim) {
        /* Capture. */
        t.ax += W(victim + CAPTURE_VALUE);
        t.bp = victim;
        t.dx = W(captured_index + PIECE_MASKS);
        eval_remove(e, &t);
        toggle(e, victim, to, t.dx);
        uint16_t counter = W(victim + COUNT_ADDRESS);
        uint8_t amount = B(victim + COUNT_AMOUNT);
        cx = (uint16_t)((cx & 0xff) | amount << 8);   /* loaded into CH */
        uint8_t left = (uint8_t)(B(counter) - amount);
        eng_wr8(e, counter, left);
        eng_wr16(e, ply + PLY_FLAGS, cx);
        eng_wr16(e, ply + PLY_CAPTURED, captured_index);
        eng_wr16(e, captured_index + 0x5e0, 0);
        r->ax = t.ax;
        r->cx = cx;
        if (left == 0)
            clamp(e, r, dl);
        return;
    }

    eng_square_update(e, to);
    uint8_t cl = (uint8_t)cx;
    uint8_t test = cl & 0xc1;
    if (!(test != 0 && !(test & 0x80))) {            /* jg not taken */
        eng_wr16(e, ply + PLY_FLAGS, cx);
        return;
    }
    if (cl > 0x40) {                                 /* double push */
        eng_wr16(e, ply + PLY_FLAGS, cx);
        return;
    }
    if (cl == 0x40) {                                /* en passant, 0x7067 */
        uint16_t sq = W(ply + PLY_EP_SQUARE);
        uint16_t bx = W(sq + SQ_INFO);
        eng_wr16(e, bx + 0x5e0, 0);
        eng_wr16(e, ply + PLY_CAPTURED, bx);
        uint16_t pawn = W(sq);
        eng_wr16(e, sq, 0);
        t.dx = W(bx + PIECE_MASKS);
        t.ax += 0x100;
        t.bp = pawn;
        t.di = sq;
        eval_remove(e, &t);
        toggle(e, pawn, sq, t.dx);
        eng_slider_update(e, sq);
        eng_wr8(e, pawn + 0x67a, (uint8_t)(B(pawn + 0x67a) - 1));
        eng_wr16(e, ply + PLY_FLAGS, cx);
        r->ax = t.ax;
        return;
    }
    /* Castling, 0x709f: the rook jumps over the king (hash and attacks
       only; the evaluation gets a constant). */
    uint16_t rook_from, rook_to;
    if (from > to) {
        rook_from = (uint16_t)(to - 4);
        rook_to = (uint16_t)(to + 2);
    } else {
        rook_from = (uint16_t)(to + 2);
        rook_to = (uint16_t)(to - 2);
    }
    uint16_t bx = W(rook_from + SQ_INFO);
    uint16_t rook = W(rook_from);
    uint16_t mask = W(bx + PIECE_MASKS);
    toggle(e, rook, rook_from, mask);
    eng_wr8(e, rook_from, 0);
    eng_wr16(e, rook_to + SQ_INFO, bx);
    eng_wr16(e, bx + 0x5e0, rook_to);
    eng_wr16(e, rook_to, rook);
    toggle(e, rook, rook_to, mask);
    eng_slider_update(e, rook_to);
    t.ax = (uint16_t)(t.ax + 0x18 - W(0x650) - W(0x650));
    eng_wr16(e, ply + PLY_FLAGS, cx);
    r->ax = t.ax;
}

/* Unmake move, FRITZ2.BIN 0x719e: same registers as eng_make_move. Only
   the attack tables (0x45d4, no hash), board, piece list and material sets
   are restored: hash key and evaluation are kept per ply. Out: DX with bit
   0x40 of DH cleared, CX = the ply's flags (CH changed for captures). */
void eng_unmake_move(Engine *e, EngRegs *r)
{
    uint16_t ply = r->bp, from = r->si, to = r->di;
    uint16_t dx = r->dx & 0xbfff;

    uint16_t bx = W(to + SQ_INFO);
    eng_wr16(e, from + SQ_INFO, bx);
    eng_wr16(e, bx + 0x5e0, from);
    uint16_t cx = W(ply + PLY_FLAGS);
    uint16_t mask = W(bx + PIECE_MASKS);
    eng_piece_attacks(e, W(to), to, mask);
    uint16_t victim = cx & 0x3e;
    eng_wr16(e, to, victim);
    if (!victim) {
        eng_square_update(e, to);
    } else {
        uint16_t vb = W(ply + PLY_CAPTURED);
        eng_wr16(e, to + SQ_INFO, vb);
        eng_wr16(e, vb + 0x5e0, to);
        eng_piece_attacks(e, victim, to, W(vb + PIECE_MASKS));
        uint16_t counter = W(victim + COUNT_ADDRESS);
        uint8_t amount = B(victim + COUNT_AMOUNT);
        cx = (uint16_t)((cx & 0xff) | amount << 8);
        eng_wr8(e, counter, (uint8_t)(B(counter) + amount));
    }
    uint16_t mover = W(ply + PLY_MOVER);
    eng_wr16(e, from, mover);
    eng_piece_attacks(e, mover, from, mask);
    eng_square_update(e, from);
    r->cx = cx;
    r->dx = dx;

    uint8_t cl = (uint8_t)cx;
    uint8_t test = cl & 0xc1;
    if (test == 0)
        return;
    if (test & 0x80) {
        /* Promotion: the pawn comes back, the new piece's sets go. */
        eng_wr16(e, mover + 0x66c, W(mover + 0x66c) ^ mask);
        eng_wr8(e, mover + 0x67a, (uint8_t)(B(mover + 0x67a) + 1));
        if (!(cl & 0x41)) {
            eng_wr16(e, mover + 0x674, W(mover + 0x674) ^ mask);
            eng_wr16(e, mover + 0x678, W(mover + 0x678) ^ mask);
            eng_wr8(e, mover + 0x67b, (uint8_t)(B(mover + 0x67b) - 9));
        } else if (!(cl & 0x40)) {
            eng_wr16(e, mover + 0x672, W(mover + 0x672) ^ mask);
            eng_wr16(e, mover + 0x678, W(mover + 0x678) ^ mask);
            eng_wr8(e, mover + 0x67b, (uint8_t)(B(mover + 0x67b) - 5));
        } else {
            eng_wr8(e, mover + 0x67b, (uint8_t)(B(mover + 0x67b) - 3));
            if (!(cl & 1)) {
                eng_wr16(e, mover + 0x670, W(mover + 0x670) ^ mask);
                eng_wr16(e, mover + 0x678, W(mover + 0x678) ^ mask);
            } else {
                eng_wr16(e, mover + 0x66e, W(mover + 0x66e) ^ mask);
            }
        }
        return;
    }
    if (cl > 0x40)
        return;
    if (cl == 0x40) {                                 /* en passant, 0x72c0 */
        uint16_t sq = W(ply + PLY_EP_SQUARE);
        uint16_t pb = W(ply + PLY_CAPTURED);
        eng_wr16(e, sq + SQ_INFO, pb);
        eng_wr16(e, pb + 0x5e0, sq);
        uint16_t pawn = (uint16_t)((dx ^ 0x30) & 0x30);
        eng_wr16(e, sq, pawn);
        eng_piece_attacks(e, pawn, sq, W(pb + PIECE_MASKS));
        eng_slider_update(e, sq);
        eng_wr8(e, pawn + 0x67a, (uint8_t)(B(pawn + 0x67a) + 1));
        return;
    }
    /* Castling, 0x725f: the rook goes back. */
    uint16_t rook_at, rook_home;
    if (from > to) {
        rook_at = (uint16_t)(to + 2);
        rook_home = (uint16_t)(to - 4);
    } else {
        rook_at = (uint16_t)(to - 2);
        rook_home = (uint16_t)(to + 2);
    }
    uint16_t rook = W(rook_at);
    uint16_t rb = W(rook_at + SQ_INFO);
    uint16_t rmask = W(rb + PIECE_MASKS);
    eng_piece_attacks(e, rook, rook_at, rmask);
    eng_slider_update(e, rook_at);
    eng_wr8(e, rook_at, 0);
    eng_wr16(e, rook_home + SQ_INFO, rb);
    eng_wr16(e, rb + 0x5e0, rook_home);
    eng_wr16(e, rook_home, rook);
    eng_piece_attacks(e, rook, rook_home, rmask);
}
