/* Static exchange evaluation, FRITZ2.BIN 0x8692.

   The piece on `si` captures on `di`. The routine counts, in `al`, the
   balance of attackers minus defenders of `di`, x-ray pieces included, and
   keeps in `cl` / `ch` the values of the cheapest pieces met on each side;
   from those it returns in `ah` a gain estimate. Two mirrored branches,
   chosen by bit 0x10 of `dl` (side to move), differ only in which attack
   table and piece list come first. */
#include "engine.h"

enum {
    DIRECTION_STEP = 0x82cc,   /* + (from - to): ray step, word, 0 if none */
    DIRECTION_BITS = 0x8404,   /* + (from - to): direction class bits */
    PIECE_RAYS = 0x81c0,       /* + piece code: directions the piece moves in */
    PIECE_FLAGS = 0x81e0,      /* + piece code: 0x24 = no x-ray behind it */
    PIECE_VALUE = 0x8662,      /* + piece code: exchange value, byte */
    MOVER_MASK = 0x42b0,       /* + piece index: mask removing the mover */
    PIECE_SQUARES = 0x5e0      /* + 0x20 + 2·bit: square of white piece bit,
                                  + 0x40 + 2·bit: black */
};

typedef struct Side {
    uint16_t first, second;    /* attack tables counted with +1 and −1 */
    uint16_t first_list, second_list;
    uint8_t first_bit, second_bit;  /* piece code bit of each side */
    int first_is_low;          /* first side has codes below 0x20 */
} Side;

static const Side sides[2] = {
    /* dl & 0x10 set */
    {ATTACK_WHITE, ATTACK_BLACK, 0x1e, 0x3e, 0x10, 0x20, 1},
    /* dl & 0x10 clear */
    {ATTACK_BLACK, ATTACK_WHITE, 0x3e, 0x1e, 0x20, 0x10, 0},
};

/* Walks back from `si` (away from `di`) over the pieces lined up behind an
   attacker, as 0x872f / 0x877e do. */
static void xray(Engine *e, uint16_t si, uint16_t di, uint8_t *al, int sign,
                 uint8_t own_bit)
{
    uint16_t bx = (uint16_t)(di - si);
    for (;;) {
        si = (uint16_t)(si - eng_rd16(e, (uint16_t)(bx + DIRECTION_STEP)));
        uint16_t bp = eng_rd16(e, si);
        if (bp == 0)
            continue;
        if ((int16_t)bp < 0)
            return;
        bp = eng_rd16(e, (uint16_t)(bp + PIECE_RAYS));
        if (!(eng_rd16(e, (uint16_t)(bx + DIRECTION_BITS)) & bp))
            return;
        *al = (uint8_t)(*al + sign);
        if (!(eng_rd8(e, si) & own_bit))
            *al = (uint8_t)(*al - 2 * sign);
    }
}

/* One attack table: every piece whose bit is set, cheapest-first by bit
   order, adjusts `al` and stores its value in `*value`. */
static void attackers(Engine *e, uint16_t dx, uint16_t list, uint16_t di,
                      uint8_t *al, uint8_t *value, int sign, uint8_t own_bit)
{
    uint16_t bp = list;
    while (dx) {
        do {
            bp += 2;
            int carry = dx & 1;
            dx >>= 1;
            if (carry)
                break;
        } while (1);
        uint16_t si = eng_rd16(e, (uint16_t)(bp + PIECE_SQUARES));
        uint16_t code = eng_rd16(e, si);
        *al = (uint8_t)(*al + sign);
        *value = eng_rd8(e, (uint16_t)(code + PIECE_VALUE));
        if (!(eng_rd8(e, (uint16_t)(code + PIECE_FLAGS)) & 0x24))
            xray(e, si, di, al, sign, own_bit);
    }
}

uint16_t eng_see(Engine *e, uint16_t si, uint16_t di, uint16_t dx_in)
{
    const Side *s = &sides[(dx_in & 0x10) ? 0 : 1];

    if (!(eng_rd16(e, (uint16_t)(si + s->second)) | eng_rd16(e, (uint16_t)(di + s->second))))
        return 0;

    uint8_t al = 0, cl = 0x80, ch = 0x80;
    uint16_t delta = (uint16_t)(si - di);
    uint16_t step = eng_rd16(e, (uint16_t)(delta + DIRECTION_STEP));

    /* Pieces lined up behind the mover, beyond `si` (0x86b9..0x86fb). */
    if (step) {
        uint8_t dir = eng_rd8(e, (uint16_t)(delta + DIRECTION_BITS));
        uint16_t bx = si;
        uint16_t bp;
        do {
            bx = (uint16_t)(bx + step);
            bp = eng_rd16(e, bx);
        } while (bp == 0);
        if ((int16_t)bp >= 0 && (eng_rd8(e, (uint16_t)(bp + PIECE_RAYS)) & dir)) {
            int first = (bp < 0x20) == s->first_is_low;
            if (first) {
                al++;
                cl = eng_rd8(e, (uint16_t)(bp + PIECE_VALUE));
            } else {
                al--;
                ch = eng_rd8(e, (uint16_t)(bp + PIECE_VALUE));
            }
            for (;;) {
                do {
                    bx = (uint16_t)(bx + step);
                    bp = eng_rd16(e, bx);
                } while (bp == 0);
                if ((int16_t)bp < 0 || !(eng_rd8(e, (uint16_t)(bp + PIECE_RAYS)) & dir))
                    break;
                al++;
                if ((bp < 0x20) != s->first_is_low)
                    al = (uint8_t)(al - 2);
            }
        }
    }

    /* Attackers of the target, without the mover (0x86fd..0x87a2). */
    uint16_t mover = eng_rd16(e, (uint16_t)(si + SQ_INFO));
    uint16_t dx = eng_rd16(e, (uint16_t)(di + s->first)) &
                  eng_rd16(e, (uint16_t)(mover + MOVER_MASK));
    attackers(e, dx, s->first_list, di, &al, &cl, +1, s->first_bit);
    attackers(e, eng_rd16(e, (uint16_t)(di + s->second)), s->second_list, di, &al, &ch,
              -1, s->second_bit);

    /* Result (0x87a7..0x87c2). */
    uint8_t ah = eng_rd8(e, (uint16_t)(eng_rd16(e, si) + PIECE_VALUE));
    if ((int8_t)al >= 0) {
        ah = ah > ch ? (uint8_t)(ah - ch) : 0;
    } else if (cl < ch) {
        cl = (uint8_t)(cl - ch);
        unsigned sum = (unsigned)ah + cl;
        ah = sum > 0xff ? (uint8_t)sum : 0;
    }
    return (uint16_t)(ah << 8 | al);
}
