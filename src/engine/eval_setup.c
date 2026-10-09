/* Evaluation set-up of FRITZ2.BIN, 0x6a0b..0x6e9f.

   Before each search the engine rebuilds its piece-square tables
   (0x6d0..0xe1f) from templates and the current position, sets the file
   masks 0x646..0x64f and the endgame weights, and computes the starting
   evaluation [0x11f0]. White's tables are computed on a colour-flipped
   board, then the board is flipped back for the other side (0x6cc3,
   0x6d03). Labels in the comments are the original addresses. */
#include "engine.h"

#define B(off) eng_rd8(e, (uint16_t)(off))
#define W(off) eng_rd16(e, (uint16_t)(off))

static void orb(Engine *e, uint16_t off, uint8_t v)
{
    eng_wr8(e, off, (uint8_t)(B(off) | v));
}

/* 0x6d9b: table bp, square si += value × 8 (value signed byte). */
static void add_entry(Engine *e, uint16_t bp, uint16_t si, uint8_t al)
{
    uint16_t v = (uint16_t)((int16_t)(int8_t)al * 8);
    eng_wr16(e, (uint16_t)(bp + si), (uint16_t)(W(bp + si) + v));
}

/* 0x6d6a: adds an 8×8 template of signed bytes (×8) to table bp. */
static void add_template(Engine *e, uint16_t bx, uint16_t bp)
{
    uint16_t di = BOARD_A1;
    for (int rank = 0; rank < 8; rank++, di += 4)
        for (int file = 0; file < 8; file++, di += 2, bx++)
            add_entry(e, bp, di, B(bx));
}

/* 0x6da5 (step 1) / 0x6daa (step 10): value along a line from si to the
   border. */
static void add_line(Engine *e, uint16_t bp, uint16_t si, uint8_t al, uint16_t cx)
{
    do {
        add_entry(e, bp, si, al);
        si = (uint16_t)(si + 2 * cx);
    } while (!(B(si) & 0x80));
}

/* 0x6e16: pattern list at bx, relative to square di: entries (offset,
   value) until 0. The offset byte holds the rank step in the high nibble
   (signed) and the file step in the low nibble (bit 3 = left, decoded as
   in the original: (n ^ 7) + 1 steps). Entries off the board are skipped. */
static void add_pattern(Engine *e, uint16_t bx, uint16_t bp, uint16_t di)
{
    for (;;) {
        uint8_t al = B(bx);
        if (!al)
            return;
        bx++;
        uint16_t si = di;
        uint8_t ah = al & 0xf0;
        int off = 0;
        if (ah) {
            int16_t cx = 0x14;
            if (ah & 0x80) {
                ah = (uint8_t)-ah;
                cx = -0x14;
            }
            do {
                si = (uint16_t)(si + cx);
                if (B(si) & 0x80) {
                    off = 1;
                    break;
                }
                ah = (uint8_t)(ah - 0x10);
            } while (ah);
        }
        if (!off) {
            al &= 0xf;
            if (al) {
                int16_t cx = 2;
                if (al & 8) {
                    al = (uint8_t)((al ^ 7) + 1);
                    cx = -2;
                }
                do {
                    si = (uint16_t)(si + cx);
                    if (B(si) & 0x80) {
                        off = 1;
                        break;
                    }
                } while (--al);
            }
        }
        if (!off)
            add_entry(e, bp, si, B(bx));
        bx++;
    }
}

/* 0x6e1d / 0x6e60: next passed pawn after di (CF set = found). */
static int next_passed(Engine *e, uint16_t *di, uint8_t own, uint8_t enemy, int dir)
{
    static const int16_t ahead[10] = {0x14, 0x28, 0x3c, 0x50, 0x26, 0x2a, 0x3a, 0x3e, 0x4e, 0x52};
    for (;;) {
        *di += 2;
        if (*di >= 0x1cc)
            return 0;
        if (B(*di) != own)
            continue;
        int blocked = 0;
        for (int i = 0; i < 10 && !blocked; i++)
            blocked = B(*di + dir * ahead[i]) == enemy;
        if (!blocked)
            return 1;
    }
}

/* 0x6d40: mirrors a square vertically (kings' squares). */
static uint16_t mirror_square(uint16_t ax)
{
    if ((int16_t)ax <= 0)
        return ax;
    ax = (uint16_t)(ax - BOARD_A1);
    uint8_t q = (uint8_t)((ax & 0xff) / 0x14), rem = (uint8_t)((ax & 0xff) % 0x14);
    if (ax >> 8) {                      /* div bl on a 16-bit AX */
        q = (uint8_t)(ax / 0x14);
        rem = (uint8_t)(ax % 0x14);
    }
    q ^= 7;
    ax = (uint16_t)(q * 0x14);
    ax = (uint16_t)(ax + rem);
    return (uint16_t)(ax + BOARD_A1);
}

/* 0x6d03: flips the board vertically and swaps the colours. */
static void flip_board(Engine *e)
{
    uint16_t si = BOARD_A1, di = 0x1be;
    for (;;) {
        uint16_t ax = W(si);
        if ((int16_t)ax < 0) {          /* border: next rank (0x6d23) */
            si += 4;
            di = (uint16_t)(di - 0x24);
            if (si >= 0x17c)
                break;
            continue;
        }
        if (ax)
            ax ^= 0x30;
        uint16_t old = W(di);
        eng_wr16(e, di, ax);
        if (old)
            old ^= 0x30;
        eng_wr16(e, si, old);
        si += 2;
        di += 2;
    }
    uint16_t white = mirror_square(W(0x600));
    uint16_t black = W(0x620);
    eng_wr16(e, 0x620, white);
    eng_wr16(e, 0x600, mirror_square(black));
}

/* 0x6cc3: swaps the white and black tables (vertically mirrored), the
   masks 0x64a/0x64b and 0x64c/0x64d, then flips the board (0x6d03). */
static void swap_sides(Engine *e)
{
    uint16_t si = 0x6d0, di = 0x7f8;
    for (int t = 0; t < 6; t++) {
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++, si += 2, di += 2) {
                uint16_t a = W(si);
                eng_wr16(e, si, W(di));
                eng_wr16(e, di, a);
            }
            si += 4;
            di = (uint16_t)(di - 0x24);
        }
        si += 0x98;
        di += 0x1d8;
    }
    uint16_t m = W(0x64a);
    eng_wr16(e, 0x64a, (uint16_t)(m << 8 | m >> 8));
    m = W(0x64c);
    eng_wr16(e, 0x64c, (uint16_t)(m << 8 | m >> 8));
    flip_board(e);
}

/* 0x6bd8: king tables. */
static void king_tables(Engine *e, uint8_t dh)
{
    uint16_t di = W(0x600), bp = 0xbb6;
    if (dh & 0x80) {
        add_template(e, 0x5c1a, bp);
        eng_wr8(e, 0x64c, 0xff);
        add_pattern(e, 0x6f1e, bp, W(0x620));
        if (di <= 0x154) {
            for (int16_t bx = 0x0e; bx >= 0; bx -= 2)
                if (B(bx + 0x146) == 0x28) {
                    add_line(e, bp, BOARD_A1, 0xf6, 1);
                    break;
                }
        }
        uint16_t p = 0x144;
        while (next_passed(e, &p, 0x10, 0x20, 1))
            add_pattern(e, 0x6ea3, bp, p);
        p = 0x144;
        while (next_passed(e, &p, 0x20, 0x10, -1))
            add_pattern(e, 0x6ebd, bp, p);
    } else {
        add_template(e, 0x5bda, bp);
        uint8_t bl = B(di + 0x5e20);
        uint8_t al = 0xe0, ah = 0xbf;
        if (bl < 6) {
            al = 7;
            ah = 0xf8;
            if (bl >= 3) {
                al = 0xc3;
                ah = 0xff;
            }
        }
        if (B(0x64e) & 0x20)
            al &= 0xdf;
        eng_wr8(e, 0x64a, al);
        eng_wr8(e, 0x64c, ah);
    }
    /* 0x6c6a: short castled king with the rook still on h1. */
    if (B(0x140) == 0x16 && !(B(0x648) & 0x80) &&
        (di == 0x13c || di == 0x13e || di == 0x150 || di == 0x152)) {
        add_entry(e, bp, 0x150, 8);
        add_entry(e, bp, 0x152, 8);
        add_entry(e, bp, 0x154, 8);
    }
}

/* 0x6b93: pawn, knight, bishop, rook and queen tables, then the kings. */
static void piece_tables(Engine *e, uint8_t dh)
{
    add_template(e, 0x5a9a, 0x59e);
    add_template(e, 0x5ada, 0x6d6);
    add_template(e, 0x5b1a, 0x80e);
    if (dh & 0x80) {
        add_entry(e, 0x80e, 0x152, 0xfe);   /* inline data at 0x6bb5 */
        add_entry(e, 0x80e, 0x148, 0xfe);   /* inline data at 0x6bba */
    }
    add_template(e, 0x5b5a, 0x946);
    add_template(e, 0x5b9a, 0xa7e);
    add_pattern(e, 0x6f1e, 0xa7e, W(0x620));
    king_tables(e, dh);
}

/* 0x6ca0: material and terms of side dl's pieces, added to si. */
static uint16_t side_total(Engine *e, uint16_t si, uint16_t dx)
{
    for (uint16_t di = BOARD_A1; di <= 0x1cc; di += 2) {
        uint8_t own = B(di) & (uint8_t)dx;
        if (own == 0 || (own & 0x80))
            continue;
        uint16_t bp = W(di);
        si = (uint16_t)(si + W(bp + 0x710e));
        EngRegs r = {0};
        r.ax = 0;
        r.cx = 0x30;
        r.di = di;
        r.bp = bp;
        r.dx = dx;
        eng_eval_remove(e, &r);
        si = (uint16_t)(si + (uint16_t)((int16_t)r.ax >> 1));
    }
    return si;
}

/* 0x6a0b. In: DL side to move. Out: DH bit 7 = endgame. */
void eng_eval_setup(Engine *e, EngRegs *r)
{
    for (uint16_t a = 0x646; a <= 0x654; a++)
        eng_wr8(e, a, 0);
    eng_wr8(e, 0x654, 2);
    for (uint16_t a = 0x6d0; a < 0x6d0 + 2 * 0x3a8; a += 2)
        eng_wr16(e, a, 0);

    for (uint16_t si = BOARD_A1; si <= 0x1cc; si += 2) {
        uint8_t bl = B(si);
        if (bl == 0x16 || bl == 0x18) {
            bl = B(si + 0x6090);
            orb(e, 0x64e, bl);
        }
        if (bl == 0x26 || bl == 0x28)
            orb(e, 0x64f, B(si + 0x6090));
    }
    for (uint16_t cx = BOARD_A1; cx < 0x142; cx += 2) {
        uint16_t si = cx;
        uint8_t bl = B(si + 0x6090);
        for (;;) {
            si += 0x14;
            uint16_t ax = W(si);
            if ((int16_t)ax < 0) {
                orb(e, 0x646, bl);
                break;
            }
            if ((uint8_t)ax != 0x10)
                continue;
            if (B(si + 0x14) == 0x20)
                si -= 0x14;
            break;
        }
        if (si >= 0x16e)
            orb(e, 0x648, bl);
    }
    for (uint16_t cx = 0x1be; cx <= 0x1cc; cx += 2) {
        uint16_t si = cx;
        uint8_t bl = B(si + 0x6090);
        for (;;) {
            si -= 0x14;
            uint16_t ax = W(si);
            if ((int16_t)ax < 0) {
                orb(e, 0x647, bl);
                break;
            }
            if ((uint8_t)ax != 0x20)
                continue;
            if (B(si - 0x14) == 0x10)
                si += 0x14;
            break;
        }
        if (si < 0x196)
            orb(e, 0x649, bl);
    }

    uint8_t dh = 0, al = 0;
    for (uint16_t si = BOARD_A1; si <= 0x1cc; si += 2) {
        int16_t bx = (int16_t)W(si);
        if (bx > 0)
            al = (uint8_t)(al + B(bx + 0x716f));
    }
    if (al < 0x2e) {
        dh |= 0x80;
        eng_wr8(e, 0x654, 1);
        eng_wr16(e, 0x652, 0xf600);
        if (B(0x68b) != 0 && B(0x69b) != 0)
            eng_wr16(e, 0x652, 0xffd0);
    } else if (!((uint8_t)(al - 0x2e) & 0x80)) {
        eng_wr16(e, 0x650, 0xffe8);
    } else {
        /* cmp sets SF for al >= 0xae: js taken (not reachable with real
           material, kept for fidelity) */
        eng_wr16(e, 0x652, 0xf600);
        if (B(0x68b) != 0 && B(0x69b) != 0)
            eng_wr16(e, 0x652, 0xffd0);
    }

    int special = 0;
    uint16_t bp = 0;
    if (al == 5) {
        if (B(0x68b) == 6) {
            al = 0x14;
            bp = 0xc52;
            special = 1;
        } else if (B(0x69b) == 6) {
            al = 0x24;
            bp = 0xbb6;
            special = 1;
        }
    }
    if (special) {
        /* 0x6b2f: king, bishop and knight against king. */
        uint16_t si = BOARD_A1;
        while (si <= 0x1cc && B(si) != al)
            si += 2;
        uint8_t colour = (uint8_t)(B(si + 0x5e20) ^ B(si + 0x5ebc));
        add_template(e, (colour & 1) ? 0x5c92 : 0x5c5a, bp);
        static const uint16_t minor_tables[4] = {0x80e, 0x8aa, 0x6d6, 0x772};
        for (int i = 3; i >= 0; i--)
            add_template(e, 0x5c1a, minor_tables[i]);
        swap_sides(e);
        king_tables(e, dh);
        swap_sides(e);
        king_tables(e, dh);
    } else {
        flip_board(e);
        piece_tables(e, dh);
        swap_sides(e);
        piece_tables(e, dh);
    }

    uint16_t si = side_total(e, 0x8000, r->dx);
    si = (uint16_t)-si;
    si = side_total(e, si, (uint16_t)(r->dx ^ 0x30));
    eng_wr16(e, 0x11f0, (uint16_t)(si & 0xfff8));
    r->dx = (uint16_t)((r->dx & 0x00ff) | dh << 8);
}
