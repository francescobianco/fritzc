#include "engine.h"

void eng_bind(Engine *e, uint16_t segment)
{
    e->segment = segment;
    e->mem = e->ram + (uint32_t)segment * 16;
}

unsigned long eng_size(void)
{
    return sizeof(Engine);
}
