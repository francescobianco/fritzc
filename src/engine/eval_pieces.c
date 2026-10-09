/* Incremental evaluation terms of FRITZ2.BIN, 0x637c..0x6a06.

   One routine per piece code, reached through the tables at 0x632a
   (piece leaving `di`) and 0x634a (piece arriving on `di`). They add the
   piece's terms to AX. Registers in: AX running score, DI square, SI the
   from-square (pawn arrivals), BP piece code, CL move flags, DX piece mask.
   Out: AX, and for arrivals BP (rooks and kings change code when they move
   for the first time, promotions turn the pawn into the new piece), CL
   (promotion and double-push flags) and the material sets / counters at
   0x67c..0x69b. The comments give the original labels. */
#include "engine.h"

#define B(off) eng_rd8(e, (uint16_t)(off))
#define W(off) eng_rd16(e, (uint16_t)(off))

static void xorw(Engine *e, uint16_t off, uint16_t v)
{
    eng_wr16(e, off, W(off) ^ v);
}

static void addb(Engine *e, uint16_t off, int v)
{
    eng_wr8(e, off, (uint8_t)(B(off) + v));
}

/* Knights: 0x637c white, 0x63b8 black. */
static void white_knight(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    r->ax += W(di + 0x6d6);
    if (di == 0x15e && B(0x14a) == 0x10 && B(0x188) == 0x20)
        r->ax -= 0x28;               /* Nc3 blocking c2 with ...d5 */
    if (W(di + 0x16) == 0x24 || W(di + 0x2c) == 0x24)
        r->ax -= 0x10;               /* black bishop on the diagonal */
    if (W(di + 0x12) == 0x24 || W(di + 0x24) == 0x24)
        r->ax -= 0x10;
}

static void black_knight(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    r->ax += W(di + 0x772);
    if (di == 0x19a && B(0x1ae) == 0x20 && B(0x174) == 0x10)
        r->ax -= 0x28;
    if (W((uint16_t)(di - 0x16)) == 0x14 || W((uint16_t)(di - 0x2c)) == 0x14)
        r->ax -= 0x10;
    if (W((uint16_t)(di - 0x12)) == 0x14 || W((uint16_t)(di - 0x24)) == 0x14)
        r->ax -= 0x10;
}

/* Bishops: 0x681c white, 0x68a8 black. `pawn` is the own pawn code,
   `knight` the enemy knight code, `d` the direction sign. */
static void bishop(Engine *e, EngRegs *r, uint16_t table, uint16_t sq1, uint16_t sq2,
                   uint8_t pawn, uint8_t knight, int d)
{
    uint16_t di = r->di;
    r->ax += W(di + table);
    if ((di == sq1 || di == sq2) && B(di - d * 0x14) == pawn)
        r->ax -= 0x38;               /* bishop in front of its centre pawn */
    for (int side = 0; side < 2; side++) {
        int near = side ? 0x16 : 0x12, far = near == 0x12 ? 0x24 : 0x2c;
        uint8_t bl = B(di + d * near);
        if (bl == knight)
            r->ax += 0x10;
        if (bl == pawn) {
            r->ax -= 0x10;
            if (B(di + d * (near + 0x14)))
                r->ax -= 0x10;
            continue;
        }
        bl = B(di + d * far);
        if (bl == knight)
            r->ax += 0x10;
        if (bl == pawn) {
            r->ax -= 8;
            if (B(di + d * (far + 0x14)))
                r->ax -= 0x10;
        }
    }
}

/* Rooks: 0x6936 white (0x6934: first move, code 0x16 -> 0x18). */
static void white_rook(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    if (di >= 0x1aa && di <= 0x1b8 && W(0x620) >= 0x1be)
        r->ax += 0x50;               /* seventh rank with the king on the eighth */
    r->ax += W(di + 0x946);
    uint8_t bl = B(di + 0x6090);
    if (B(0x648) & bl) {
        r->ax += 0x10;
        if (B(0x646) & bl)
            r->ax += 0x10;
    }
    if (B(0x647) & bl)
        r->ax += 0x10;
}

static void black_rook(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    if (di <= 0x154 && di >= 0x146 && W(0x600) <= 0x140)
        r->ax += 0x50;
    r->ax += W(di + 0x9e2);
    uint8_t bl = B(di + 0x6090);
    if (B(0x649) & bl) {
        r->ax += 0x10;
        if (B(0x647) & bl)
            r->ax += 0x10;
    }
    if (B(0x646) & bl)
        r->ax += 0x10;
}

/* Queens: 0x69b7 white, 0x69cf black. */
static void queen(Engine *e, EngRegs *r, uint16_t table, uint16_t files)
{
    r->ax += W(r->di + table);
    if (B(files) & B(r->di + 0x6090))
        r->ax -= 0x10;
}

/* Pawn structure terms shared by arrivals and departures of a white pawn,
   0x64f0..0x6606 (`passed` selects the 0x64f0 entry). */
static void white_pawn_common(Engine *e, EngRegs *r, int passed)
{
    uint16_t di = r->di, bp = r->bp;
    if (passed)
        r->ax += W(di + 0x5ce0);                                   /* 64f0 */
    r->ax += W(di + 0x59e);                                        /* 64f4 */
    if (B(di - 0x24) == 0x14 || B(di - 0x12) == 0x14 || B(di - 0x2c) == 0x14 ||
        B(di - 0x16) == 0x14) {
        if (!(B(di - 0x24) == 0x14) && B(di - 0x12) == 0x14)
            r->ax -= 8;                                            /* 650f */
        else if (!(B(di - 0x24) == 0x14) && !(B(di - 0x2c) == 0x14))
            r->ax -= 8;
        r->ax -= 8;                                                /* 6512 */
        if (B(di + 0x14))
            r->ax -= 0x10;
    }
    /* 651d: neighbours on the files left and right. */
    if (W(di - 2) == bp) {
        if (W(di - 4) == bp)
            goto l6589;
        if (W(di - 0x18) != bp && W(di - 0x14) != bp) {            /* 6544 */
            r->ax += W(di + 0x5b9e);
            if (B(0x647) & B(di + 0x608e))
                r->ax += 0x10;
        }
    } else if (W(di + 2) != bp && W(di - 0x16) != bp && W(di - 0x12) != bp) {
        r->ax -= W(di + 0x5ba0);                                   /* isolated */
        if (B(0x647) & B(di + 0x6090))
            r->ax -= 0x10;
    }
    if (W(di + 0x12) == bp && W(di + 0x10) != bp && W(di + 0x14) != bp &&
        W(di - 4) != bp) {                                         /* 6564 */
        r->ax += W(di + 0x5bb2);
        if (B(0x647) & B(di + 0x60a2))
            r->ax += 0x10;
    }
l6589:
    if (W(di + 2) == bp) {
        if (W(di + 4) == bp)
            goto l65d3;
        if (W(di - 0x10) != bp && W(di - 0x14) != bp) {
            r->ax += W(di + 0x5ba2);
            if (B(0x647) & B(di + 0x6092))
                r->ax += 0x10;
        }
    }
    if (W(di + 0x16) == bp && W(di + 0x18) != bp && W(di + 0x14) != bp &&
        W(di + 4) != bp) {                                         /* 65ae */
        r->ax += W(di + 0x5bb6);
        if (B(0x647) & B(di + 0x60a6))
            r->ax += 0x10;
    }
l65d3:
    if (r->cx & 0x30) {
        if (W(di + 0x14) == bp || W(di + 0x28) == bp || W(di + 0x3c) == bp ||
            W(di - 0x14) == bp || W(di - 0x28) == bp || W(di - 0x3c) == bp)
            r->ax -= 0x40;                                         /* doubled */
        if (B(0x64e) & B(di + 0x6090))
            r->ax += 0xffe0;
    }
}

/* Is a black pawn in front of `di` or on the adjacent files ahead? */
static int white_pawn_blocked(Engine *e, uint16_t di)
{
    static const int16_t ahead[7] = {0x14, 0x28, 0x3c, 0x26, 0x3a, 0x2a, 0x3e};
    for (int i = 0; i < 7; i++)
        if (B(di + ahead[i]) == 0x20)
            return 1;
    return 0;
}

/* White promotion, 0x63f1: the new piece follows CL (table DS:7198 of the
   EXE: queen 0x80, rook 0x81, bishop 0xC0, knight 0xC1). */
static void white_promotion(Engine *e, EngRegs *r)
{
    r->cx |= 0x80;
    xorw(e, 0x67c, r->dx);
    addb(e, 0x68a, -1);
    if (!(r->cx & 0x41)) {
        r->bp = 0x1a;
        xorw(e, 0x684, r->dx);
        xorw(e, 0x688, r->dx);
        r->ax += 0x880;
        addb(e, 0x68b, 9);
    } else if (!(r->cx & 0x40)) {
        r->bp = 0x16;
        xorw(e, 0x682, r->dx);
        xorw(e, 0x688, r->dx);
        r->ax += 0x440;
        addb(e, 0x68b, 5);
    } else {
        addb(e, 0x68b, 3);
        if (!(r->cx & 1)) {
            r->bp = 0x14;
            xorw(e, 0x680, r->dx);
            xorw(e, 0x688, r->dx);
            r->ax += 0x280;
        } else {
            xorw(e, 0x67e, r->dx);
            r->bp = 0x12;
            r->ax += 0x260;
        }
    }
}

static void black_promotion(Engine *e, EngRegs *r)   /* 0x6607 */
{
    r->cx |= 0x80;
    xorw(e, 0x68c, r->dx);
    addb(e, 0x69a, -1);
    if (!(r->cx & 0x41)) {
        r->bp = 0x2a;
        xorw(e, 0x694, r->dx);
        xorw(e, 0x698, r->dx);
        r->ax += 0x880;
        addb(e, 0x69b, 9);
    } else if (!(r->cx & 0x40)) {
        r->bp = 0x26;
        xorw(e, 0x692, r->dx);
        xorw(e, 0x698, r->dx);
        r->ax += 0x440;
        addb(e, 0x69b, 5);
    } else {
        addb(e, 0x69b, 3);
        if (!(r->cx & 1)) {
            r->bp = 0x24;
            xorw(e, 0x690, r->dx);
            xorw(e, 0x698, r->dx);
            r->ax += 0x280;
        } else {
            xorw(e, 0x68e, r->dx);
            r->bp = 0x22;
            r->ax += 0x260;
        }
    }
}

/* 0x6454: white pawn arriving on `di`. */
static void white_pawn_add(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    if (di >= 0x1be) {
        white_promotion(e, r);
        return;
    }
    uint8_t bh = B(di + 0x6090);
    if (!(B(0x64c) & bh))
        r->ax -= 0x10;
    if (di < 0x16e) {
        white_pawn_common(e, r, 0);
        return;
    }
    if (di < 0x182 && r->si < 0x15a)
        r->cx |= 0x41;               /* double push */
    if (white_pawn_blocked(e, di)) {
        white_pawn_common(e, r, 0);
        return;
    }
    if (B(di + 0x14) == 0)
        r->ax += 0x10;
    if (!(B(0x64e) & bh))
        r->ax -= 0x10;
    white_pawn_common(e, r, 1);
}

/* 0x64b8: white pawn leaving `di`. */
static void white_pawn_remove(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    if (B(0x64a) & B(di + 0x6090))
        r->ax += 0x20;
    white_pawn_common(e, r, di >= 0x16e && !white_pawn_blocked(e, di));
}

/* Black pawn: mirror of the white one (0x6668 / 0x66cc / 0x6704..0x681a). */
static void black_pawn_common(Engine *e, EngRegs *r, int passed)
{
    uint16_t di = r->di, bp = r->bp;
    if (passed)
        r->ax += W(di + 0x5d80);                                   /* 6704 */
    r->ax += W(di + 0x63a);                                        /* 6708 */
    if (B(di + 0x24) == 0x24 || B(di + 0x12) == 0x24 || B(di + 0x2c) == 0x24 ||
        B(di + 0x16) == 0x24) {
        if (!(B(di + 0x24) == 0x24) && B(di + 0x12) == 0x24)
            r->ax -= 8;                                            /* 6723 */
        else if (!(B(di + 0x24) == 0x24) && !(B(di + 0x2c) == 0x24))
            r->ax -= 8;
        r->ax -= 8;                                                /* 6726 */
        if (B(di - 0x14))
            r->ax -= 0x10;
    }
    if (W(di + 2) == bp) {                                         /* 6731 */
        if (W(di + 4) == bp)
            goto l679d;
        if (W(di + 0x18) != bp && W(di + 0x14) != bp) {            /* 6758 */
            r->ax += W(di + 0x5c42);
            if (B(0x646) & B(di + 0x6092))
                r->ax += 0x10;
        }
    } else if (W(di - 2) != bp && W(di + 0x16) != bp && W(di + 0x12) != bp) {
        r->ax -= W(di + 0x5c40);
        if (B(0x646) & B(di + 0x6090))
            r->ax -= 0x10;
    }
    if (W(di - 0x12) == bp && W(di - 0x10) != bp && W(di - 0x14) != bp &&
        W(di + 4) != bp) {                                         /* 6778 */
        r->ax += W(di + 0x5c2e);
        if (B(0x646) & B(di + 0x607e))
            r->ax += 0x10;
    }
l679d:
    if (W(di - 2) == bp) {
        if (W(di - 4) == bp)
            goto l67e7;
        if (W(di + 0x10) != bp && W(di + 0x14) != bp) {
            r->ax += W(di + 0x5c3e);
            if (B(0x646) & B(di + 0x608e))
                r->ax += 0x10;
        }
    }
    if (W(di - 0x16) == bp && W(di - 0x18) != bp && W(di - 0x14) != bp &&
        W(di - 4) != bp) {                                         /* 67c2 */
        r->ax += W(di + 0x5c2a);
        if (B(0x646) & B(di + 0x607a))
            r->ax += 0x10;
    }
l67e7:
    if (r->cx & 0x30) {
        if (W(di + 0x14) == bp || W(di + 0x28) == bp || W(di + 0x3c) == bp ||
            W(di - 0x14) == bp || W(di - 0x28) == bp || W(di - 0x3c) == bp)
            r->ax -= 0x40;
        if (B(0x64f) & B(di + 0x6090))
            r->ax += 0xffe0;
    }
}

static int black_pawn_blocked(Engine *e, uint16_t di)
{
    static const int16_t ahead[7] = {0x14, 0x28, 0x3c, 0x26, 0x3a, 0x2a, 0x3e};
    for (int i = 0; i < 7; i++)
        if (B(di - ahead[i]) == 0x10)
            return 1;
    return 0;
}

static void black_pawn_add(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    if (di < 0x146) {
        black_promotion(e, r);
        return;
    }
    uint8_t bh = B(di + 0x6090);
    if (!(B(0x64d) & bh))
        r->ax -= 0x10;
    if (di >= 0x196) {
        black_pawn_common(e, r, 0);
        return;
    }
    if (di >= 0x182 && r->si >= 0x1aa)
        r->cx |= 0x41;
    if (black_pawn_blocked(e, di)) {
        black_pawn_common(e, r, 0);
        return;
    }
    if (B(di - 0x14) == 0)
        r->ax += 0x10;
    if (!(B(0x64f) & bh))
        r->ax -= 0x10;
    black_pawn_common(e, r, 1);
}

static void black_pawn_remove(Engine *e, EngRegs *r)
{
    uint16_t di = r->di;
    if (B(0x64b) & B(di + 0x6090))
        r->ax += 0x20;
    black_pawn_common(e, r, di < 0x196 && !black_pawn_blocked(e, di));
}

/* 0x634a dispatch: piece `r->bp` arriving on `r->di`. */
void eng_eval_add(Engine *e, EngRegs *r)
{
    switch (r->bp) {
    case 0x10: white_pawn_add(e, r); break;
    case 0x12: white_knight(e, r); break;
    case 0x14: bishop(e, r, 0x80e, 0x160, 0x162, 0x10, 0x22, 1); break;
    case 0x16: r->bp += 2; white_rook(e, r); break;          /* 0x6934 */
    case 0x18: white_rook(e, r); break;
    case 0x1a: r->ax -= 8; queen(e, r, 0xa7e, 0x647); break;  /* 0x69b4 */
    case 0x1c:                                                /* 0x69e4 */
        r->bp += 2;
        r->ax += W(0x650) + W(0x650);
        /* fall through */
    case 0x1e: r->ax += W(0x650); r->ax += W(r->di + 0xbb6); break;
    case 0x20: black_pawn_add(e, r); break;
    case 0x22: black_knight(e, r); break;
    case 0x24: bishop(e, r, 0x8aa, 0x19c, 0x19e, 0x20, 0x12, -1); break;
    case 0x26: r->bp += 2; black_rook(e, r); break;           /* 0x6974 */
    case 0x28: black_rook(e, r); break;
    case 0x2a: r->ax -= 8; queen(e, r, 0xb1a, 0x646); break;  /* 0x69cc */
    case 0x2c:                                                /* 0x69f8 */
        r->bp += 2;
        r->ax += W(0x650) + W(0x650);
        /* fall through */
    case 0x2e: r->ax += W(0x650); r->ax += W(r->di + 0xc52); break;
    default: break;
    }
}

/* 0x632a dispatch: piece `r->bp` leaving `r->di`. */
void eng_eval_remove(Engine *e, EngRegs *r)
{
    switch (r->bp) {
    case 0x10: white_pawn_remove(e, r); break;
    case 0x12: white_knight(e, r); break;
    case 0x14: bishop(e, r, 0x80e, 0x160, 0x162, 0x10, 0x22, 1); break;
    case 0x16: case 0x18: white_rook(e, r); break;
    case 0x1a: queen(e, r, 0xa7e, 0x647); break;
    case 0x1c: case 0x1e: r->ax += W(r->di + 0xbb6); break;
    case 0x20: black_pawn_remove(e, r); break;
    case 0x22: black_knight(e, r); break;
    case 0x24: bishop(e, r, 0x8aa, 0x19c, 0x19e, 0x20, 0x12, -1); break;
    case 0x26: case 0x28: black_rook(e, r); break;
    case 0x2a: queen(e, r, 0xb1a, 0x646); break;
    case 0x2c: case 0x2e: r->ax += W(r->di + 0xc52); break;
    default: break;
    }
}
