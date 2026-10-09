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
