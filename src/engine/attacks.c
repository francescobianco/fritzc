/* Attack tables of FRITZ2.BIN: 0x4380..0x4594 (per piece) and 0x46b7,
   0x46f8 (whole board). Each routine XORs the piece mask into the attack
   word of every square the piece attacks, so the same call adds or removes
   the piece. */
#include "engine.h"

static void xor16(Engine *e, uint16_t off, uint16_t dx)
{
    eng_wr16(e, off, eng_rd16(e, off) ^ dx);
}

static void xor8(Engine *e, uint16_t off, uint8_t v)
{
    eng_wr8(e, off, eng_rd8(e, off) ^ v);
}

/* 0x4380 white pawn: attacks up-left and up-right, pushes one square and,
   from the second rank with the square ahead holding dl, two squares. */
static void white_pawn(Engine *e, uint16_t di, uint16_t dx)
{
    xor16(e, di + 0x212, dx);
    xor16(e, di + 0x216, dx);
    xor8(e, di + 0x414, (uint8_t)(dx >> 8));
    if (di < 0x15a && (uint8_t)dx == eng_rd8(e, di + 0x14))
        xor8(e, di + 0x428, (uint8_t)(dx >> 8));
}

/* 0x43ac black pawn: mirror of the white one, black tables (+0x300). */
static void black_pawn(Engine *e, uint16_t di, uint16_t dx)
{
    xor16(e, di + 0x2ee, dx);
    xor16(e, di + 0x2ea, dx);
    xor8(e, di + 0x3ed, (uint8_t)(dx >> 8));
    if (di >= 0x1aa && (uint8_t)dx == eng_rd8(e, (uint16_t)(di - 0x14)))
        xor8(e, di + 0x3d9, (uint8_t)(dx >> 8));
}

/* Offsets of the eight neighbours in table order, for kings (0x43d8 white
   at +0x200, 0x440a black at +0x300) and knights (0x443c, 0x446e). */
static const int16_t king_steps[8] = {2, -2, 0x14, -0x14, 0x12, -0x12, 0x16, -0x16};
static const int16_t knight_steps[8] = {0x10, -0x10, 0x18, -0x18, 0x26, -0x26, 0x2a, -0x2a};

static void leaper(Engine *e, uint16_t table, const int16_t *steps, uint16_t di, uint16_t dx)
{
    for (int i = 0; i < 8; i++)
        xor16(e, (uint16_t)(di + table + steps[i]), dx);
}

/* Slider ray: walks while the square byte is 0, the first occupied (or
   border) square included. */
static void ray(Engine *e, uint16_t table, int16_t step, uint16_t di, uint16_t dx)
{
    uint16_t bx = di;
    do {
        bx = (uint16_t)(bx + step);
        xor16(e, (uint16_t)(bx + table), dx);
    } while (eng_rd8(e, bx) == 0);
}

static void rook(Engine *e, uint16_t table, uint16_t di, uint16_t dx)
{
    ray(e, table, 2, di, dx);
    ray(e, table, -2, di, dx);
    ray(e, table, 0x14, di, dx);
    ray(e, table, -0x14, di, dx);
}

static void bishop(Engine *e, uint16_t table, uint16_t di, uint16_t dx)
{
    ray(e, table, 0x12, di, dx);
    ray(e, table, -0x12, di, dx);
    ray(e, table, 0x16, di, dx);
    ray(e, table, -0x16, di, dx);
}

void eng_piece_attacks(Engine *e, uint16_t code, uint16_t di, uint16_t dx)
{
    switch (code) {
    case 0x10: white_pawn(e, di, dx); break;
    case 0x12: leaper(e, ATTACK_WHITE, knight_steps, di, dx); break;
    case 0x14: bishop(e, ATTACK_WHITE, di, dx); break;
    case 0x16: case 0x18: rook(e, ATTACK_WHITE, di, dx); break;
    case 0x1a: rook(e, ATTACK_WHITE, di, dx); bishop(e, ATTACK_WHITE, di, dx); break;
    case 0x1c: case 0x1e: leaper(e, ATTACK_WHITE, king_steps, di, dx); break;
    case 0x20: black_pawn(e, di, dx); break;
    case 0x22: leaper(e, ATTACK_BLACK, knight_steps, di, dx); break;
    case 0x24: bishop(e, ATTACK_BLACK, di, dx); break;
    case 0x26: case 0x28: rook(e, ATTACK_BLACK, di, dx); break;
    case 0x2a: rook(e, ATTACK_BLACK, di, dx); bishop(e, ATTACK_BLACK, di, dx); break;
    case 0x2c: case 0x2e: leaper(e, ATTACK_BLACK, king_steps, di, dx); break;
    default: break;
    }
}

void eng_attacks_clear(Engine *e)
{
    for (uint16_t di = 0x300; di < 0x300 + 2 * 0x180; di += 2)
        eng_wr16(e, di, 0xfffe);
    for (uint16_t di = BOARD_A1; di < 0x1cc; di += 4)
        for (int file = 0; file < 8; file++, di += 2) {
            eng_wr16(e, di + 0x200, 0);
            eng_wr16(e, di + 0x300, 0);
            eng_wr16(e, di + 0x400, 0);
        }
    eng_wr16(e, 0x100 + 0x200, 0);
    eng_wr16(e, 0x100 + 0x300, 0);
}

void eng_attacks_build(Engine *e)
{
    for (uint16_t di = BOARD_A1; di <= 0x1cc; di += 2) {
        int16_t code = (int16_t)eng_rd16(e, di);
        if (code <= 0)
            continue;
        uint16_t bx = eng_rd16(e, di + SQ_INFO);
        eng_piece_attacks(e, (uint16_t)code, di, eng_rd16(e, (uint16_t)(bx + PIECE_MASKS)));
    }
}

/* Hash key: two words at HASH_KEY, XORed with a per-square random word of
   each piece type. Low tables per piece code (0x45f4 dispatch); the high
   word table is 0x9c bytes further. */
static uint16_t hash_table(uint16_t code)
{
    switch (code) {
    case 0x10: return 0x45e8;
    case 0x12: return 0x4720;
    case 0x14: return 0x4858;
    case 0x16: case 0x18: return 0x4ac8;
    case 0x1a: return 0x4c00;
    case 0x1c: case 0x1e: return 0x4e70;
    case 0x20: return 0x4fa8;
    case 0x22: return 0x50e0;
    case 0x24: return 0x5218;
    case 0x26: case 0x28: return 0x5350;
    case 0x2a: return 0x55c0;
    case 0x2c: case 0x2e: return 0x5830;
    default: return 0;
    }
}

void eng_piece_toggle(Engine *e, uint16_t code, uint16_t di, uint16_t dx)
{
    uint16_t t = hash_table(code);
    if (t) {
        eng_wr16(e, HASH_KEY, eng_rd16(e, HASH_KEY) ^ eng_rd16(e, (uint16_t)(di + t)));
        eng_wr16(e, HASH_KEY + 2,
                 eng_rd16(e, HASH_KEY + 2) ^ eng_rd16(e, (uint16_t)(di + t + 0x9c)));
    }
    eng_piece_attacks(e, code, di, dx);
}

/* 0x4646: the square `di` changed occupancy; every slider of either side
   that attacks it (masks [0x688] white, [0x698] black) has its ray beyond
   `di` toggled up to the next non-empty square. */
static void slider_rays(Engine *e, uint16_t di, uint16_t sliders, uint16_t table,
                        uint16_t list)
{
    uint16_t dx = eng_rd16(e, sliders) & eng_rd16(e, (uint16_t)(di + table));
    uint16_t bx = list;
    while (dx) {
        do {
            bx += 2;
            int carry = dx & 1;
            dx >>= 1;
            if (carry)
                break;
        } while (1);
        uint16_t from = eng_rd16(e, (uint16_t)(bx + 0x5e0));
        uint16_t step = eng_rd16(e, (uint16_t)((uint16_t)(di - from) + 0x82cc));
        uint16_t mask = eng_rd16(e, (uint16_t)(bx + PIECE_MASKS));
        uint16_t si = di;
        do {
            si = (uint16_t)(si + step);
            eng_wr16(e, (uint16_t)(si + table), eng_rd16(e, (uint16_t)(si + table)) ^ mask);
        } while (eng_rd8(e, si) == (uint8_t)(bx >> 8));
    }
}

void eng_slider_update(Engine *e, uint16_t di)
{
    slider_rays(e, di, 0x688, ATTACK_WHITE, 0x1e);
    slider_rays(e, di, 0x698, ATTACK_BLACK, 0x3e);
}

/* 0x4624: as 0x4646, after toggling the double pawn push through `di` when
   it lies on the third rank (class 2, white) or sixth (class 5, black). */
void eng_square_update(Engine *e, uint16_t di)
{
    uint8_t rank_class = eng_rd8(e, (uint16_t)(di + 0x5ebc));
    if (rank_class == 2)
        eng_wr8(e, (uint16_t)(di + 0x414),
                eng_rd8(e, (uint16_t)(di + 0x414)) ^ eng_rd8(e, (uint16_t)(di + 0x400)));
    else if (rank_class == 5)
        eng_wr8(e, (uint16_t)(di + 0x3ed),
                eng_rd8(e, (uint16_t)(di + 0x3ed)) ^ eng_rd8(e, (uint16_t)(di + 0x401)));
    eng_slider_update(e, di);
}
