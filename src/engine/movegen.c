/* Legal move generation of FRITZ2.BIN, 0x72fe..0x7325 with its generators.

   The original generators are coroutines (they return the next move with
   AX = resume address and CF clear). Between two moves their state lives in
   SI, DI and [BP+0x1152], which the move processing (0x7326) leaves intact,
   so here they are plain loops that call legal_move() for each move with the
   same registers the original passes. */
#include "engine.h"

#define B(off) eng_rd8(e, (uint16_t)(off))
#define W(off) eng_rd16(e, (uint16_t)(off))

enum {
    PLY_REMAINING = 0x1152,   /* + BP: attacker bits still to try */
    PLY_VICTIM_VALUE = 0xfbd, /* + BP: value of the captured piece */
    PLY_RESUME = 0x1102,      /* + BP: generator resume address */
    MOVE_COUNT = 0x640,       /* count × 2 */
    LIST_FROM = 0x2218,
    LIST_TO = 0x22b8,
    LIST_SCORE = 0x2358,
    LIST_FLAGS = 0x23f8,
    SQUARE_ORDER = 0x616c,    /* + square: next square of the quiet scan */
    WHITE_PAWNS = 0x67c,
    BLACK_PAWNS = 0x68c
};

/* 0x81a3 / 0x81b8: attack word of the opponent on the own king (non-zero
   = in check). */
uint16_t eng_in_check(Engine *e, uint8_t dl)
{
    return (dl & 0x10) ? W(W(0x600) + ATTACK_BLACK) : W(W(0x620) + ATTACK_WHITE);
}

/* 0x81c3: attack word of the opponent on square `bx`. */
static uint16_t square_attacked(Engine *e, uint16_t bx, uint8_t dl)
{
    return (dl & 0x10) ? W(bx + ATTACK_BLACK) : W(bx + ATTACK_WHITE);
}

/* 0x7326: makes the move, keeps it if the own king is safe (with its SEE
   and evaluation in the score), unmakes it. Promotions are tried as queen,
   rook, bishop and knight by stepping the flag bits in CL. */
static void legal_move(Engine *e, EngRegs *r, uint16_t resume)
{
    eng_wr16(e, r->bp + PLY_RESUME, resume);
    for (;;) {
        uint16_t see = eng_see(e, r->si, r->di, r->dx);
        see = (uint16_t)((see & 0xff00) | B(0xfbd));
        if (!((uint8_t)r->cx == 1 && (r->dx & 0x8000))) {   /* no castling in check */
            EngRegs m = *r;
            eng_make_move(e, &m);
            uint16_t ax = m.ax;
            if (!eng_in_check(e, (uint8_t)r->dx)) {
                ax = (uint16_t)(ax - (see & 0xff00));
                uint16_t n = W(MOVE_COUNT);
                eng_wr16(e, n + LIST_FROM, r->si);
                eng_wr16(e, n + LIST_TO, r->di);
                eng_wr16(e, n + LIST_SCORE, ax);
                eng_wr16(e, n + LIST_FLAGS, (uint16_t)(B(r->bp + 0x1012) | B(r->bp + 0xf1c) << 8));
                eng_wr16(e, MOVE_COUNT, (uint16_t)(n + 2));
            }
            EngRegs u = *r;
            u.cx = see;
            eng_unmake_move(e, &u);
            r->cx = u.cx;
            r->dx = u.dx;
            r->ax = ax;
        } else {
            r->ax = see;
            return;
        }
        uint8_t cl = (uint8_t)r->cx;
        if (!(cl & 0x80))
            return;
        if (!(cl & 1)) {
            r->cx = (uint16_t)((r->cx & 0xff00) | (uint8_t)(cl + 1));
            continue;
        }
        unsigned sum = (unsigned)cl + 0x3f;
        r->cx = (uint16_t)((r->cx & 0xff00) | (uint8_t)sum);
        if (sum > 0xff)
            return;
    }
}

/* 0x7408: captures. Victims in the opponent's piece list order (king
   excluded), attackers from the highest bit down (pawns first). */
static void captures(Engine *e, EngRegs *r)
{
    int white = r->dx & 0x10;
    uint16_t list = white ? 0x40 : 0x20;          /* victims */
    uint16_t own = white ? 0x40 : 0x60;           /* attacker bit scan start */
    uint16_t table = white ? ATTACK_WHITE : ATTACK_BLACK;
    for (uint16_t bx = (uint16_t)(list + 2); bx & 0x1e; bx += 2) {
        uint16_t di = W(bx + 0x5e0);
        if (!di)
            continue;
        uint16_t cx = W(di + table);
        if (!cx)
            continue;
        eng_wr8(e, r->bp + PLY_VICTIM_VALUE, B(W(di) + 0x8662));
        uint16_t si = own;
        for (;;) {
            int carry;
            do {
                si -= 2;
                carry = cx & 0x8000;
                cx = (uint16_t)(cx << 1);
            } while (!carry);
            uint16_t from = W(si + 0x5e0);
            eng_wr16(e, r->bp + PLY_REMAINING, cx);
            r->si = from;
            r->di = di;
            r->cx = white ? W(di) : (uint16_t)((cx & 0xff00) | B(di));
            legal_move(e, r, white ? 0x744a : 0x7496);
            si = W(r->si + SQ_INFO);
            cx = W(r->bp + PLY_REMAINING);
            if (!cx)
                break;
        }
        bx = W(r->di + SQ_INFO);
    }
}

/* 0x74a8: en passant after a double push in the previous ply: the pawn
   on the left (0x74af), then the one on the right (0x74d6). */
static int en_passant_from(Engine *e, EngRegs *r, uint16_t pawn, int side)
{
    uint8_t code = B(pawn + 2 * side);
    uint8_t own = code & (uint8_t)r->dx;
    if (own == 0 || (own & 0x80) || (code & 0xe))     /* jle / jne */
        return 0;
    r->di = (uint16_t)((uint16_t)(W(r->bp + 0xe24) + pawn) >> 1);
    r->si = (uint16_t)(pawn + 2 * side);
    eng_wr8(e, r->bp + PLY_VICTIM_VALUE, 1);
    r->cx = 0x40;
    legal_move(e, r, side < 0 ? 0x74d4 : 0x74f7);
    return 1;
}

static void en_passant(Engine *e, EngRegs *r)
{
    if (B(r->bp + 0x1010) != 0x41)
        return;
    uint16_t pawn = W(r->bp + 0xe7a);
    if (en_passant_from(e, r, pawn, -1))
        pawn = (uint16_t)(r->si + 2);                   /* 0x74d4: inc si ×2 */
    en_passant_from(e, r, pawn, +1);
}

/* 0x79c1: castling, then quiet moves square by square. */
static void quiet(Engine *e, EngRegs *r)
{
    int white = r->dx & 0x10;
    uint16_t king = W(white ? 0x600 : 0x620);
    uint8_t dl = (uint8_t)r->dx;
    if (B(king) == (white ? 0x1c : 0x2c)) {
        /* 0x7aa2 short, 0x7ac6 long (only the square next to the king is
           checked for attacks). */
        if (!(W(king + 2) | W(king + 4)) && (B(king + 6) & 0xcf) == 6 &&
            !square_attacked(e, (uint16_t)(king + 2), dl)) {
            r->si = king;
            r->di = (uint16_t)(king + 4);
            r->cx = 1;
            legal_move(e, r, white ? 0x79d8 : 0x7a46);
            king = r->si;
        }
        if (!(W(king - 2) | W(king - 4) | W(king - 6)) && (B(king - 8) & 0xcf) == 6 &&
            !square_attacked(e, (uint16_t)(king - 2), dl)) {
            r->si = king;
            r->di = (uint16_t)(king - 4);
            r->cx = 1;
            legal_move(e, r, white ? 0x79e1 : 0x7a4f);
        }
    }
    uint16_t pawns = white ? WHITE_PAWNS : BLACK_PAWNS;
    uint16_t table = white ? ATTACK_WHITE : ATTACK_BLACK;
    uint16_t push = white ? PAWN_PUSH : PAWN_PUSH + 1;
    uint16_t top = white ? 0x40 : 0x60;
    uint16_t di = 0x174;
    for (;;) {
        uint16_t ax = W(di);
        if (ax) {
            if ((int16_t)ax < 0)
                return;
            di = W(di + SQUARE_ORDER);
            continue;
        }
        uint16_t cx = (uint16_t)~W(pawns);
        ax = (uint16_t)(B(di + push) << 8);
        ax |= W(di + table) & cx;
        uint16_t bx = top;
        while (ax) {
            int carry;
            do {
                bx -= 2;
                carry = ax & 0x8000;
                ax = (uint16_t)(ax << 1);
            } while (!carry && ax);
            eng_wr16(e, r->bp + PLY_REMAINING, ax);
            r->si = W(bx + 0x5e0);
            r->di = di;
            r->cx = (uint16_t)(cx & 0xff00);
            legal_move(e, r, white ? 0x7a20 : 0x7a8e);
            di = r->di;
            bx = W(r->si + SQ_INFO);
            ax = W(r->bp + PLY_REMAINING);
        }
        di = W(di + SQUARE_ORDER);
    }
}

/* 0x72fe..0x7325: legal moves into the lists at 0x2218.. (count × 2 in
   [0x640]). In: DX side flags (DH bit 7 = in check), BP ply × 2. */
void eng_generate_legal_core(Engine *e, EngRegs *r)
{
    eng_wr16(e, MOVE_COUNT, 0);
    captures(e, r);
    en_passant(e, r);
    eng_wr8(e, 0xfbd, 0);
    quiet(e, r);
}

/* 0x2f2d: BP = 0, DH bit 7 = side to move in check. */
static void check_flag(Engine *e, EngRegs *r)
{
    r->bp = 0;
    r->dx &= 0x7fff;
    if (eng_in_check(e, (uint8_t)r->dx))
        r->dx |= 0x8000;
}

/* 0x72f2: evaluation set-up, check flag, attack tables from scratch, then
   the legal move lists. In: DL side to move, BP ply × 2. */
void eng_generate_legal(Engine *e, EngRegs *r)
{
    eng_eval_setup(e, r);
    check_flag(e, r);
    eng_attacks_clear(e);
    eng_attacks_build(e);
    eng_generate_legal_core(e, r);
}
