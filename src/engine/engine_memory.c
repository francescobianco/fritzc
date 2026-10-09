#include "engine_memory.h"

/* Index of the highest set bit, as the `inc bx / shr ax, 1` loops do;
   -1 for zero. */
static int floor_log2(unsigned v)
{
    int n = -1;
    for (; v; v >>= 1)
        n++;
    return n;
}

int engine_hash_exponent(const EngineMemory *mem)
{
    if (mem->use_extended && mem->cpu_class >= 2) {
        unsigned blocks = mem->extended_kb >> 6;   /* 64 KB units */
        if (blocks != 0)
            return floor_log2(blocks);
        /* no extended memory: fall back to conventional memory */
    }
    unsigned paras = mem->largest_paragraphs;
    if (paras <= 0x1000)
        return -1;
    paras = paras > 0x2800 ? paras - 0x1800 : 0x1000;   /* keeps 96 KB free */
    unsigned blocks = paras >> 12;
    if (blocks == 0)
        return -1;
    /* The original then allocates 1 << (n + 12) paragraphs and fails (-1)
       if DOS refuses; that cannot happen within the computed size. */
    return floor_log2(blocks);
}
