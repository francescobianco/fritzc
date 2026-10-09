#ifndef FRITZ_ENGINE_MEMORY_H
#define FRITZ_ENGINE_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

/* Hash table sizing of the engine init command (FRITZ2.BIN 0x29d8).

   The original depends on the machine: with the /X switch ([B0FA], only
   when no XMS driver answers int 2Fh/4300h) and a 286 or better it uses
   raw extended memory (int 15h/88h); otherwise the largest free DOS block
   (int 21h/48h). The port has neither, so the caller supplies them. */
typedef struct EngineMemory {
    bool use_extended;          /* p[0xFA] */
    unsigned cpu_class;         /* 0x2ddc: 2 or more for a 286+ */
    uint16_t extended_kb;       /* int 15h/88h */
    uint16_t largest_paragraphs;/* int 21h/48h with BX = FFFF */
} EngineMemory;

/* Returns p[0xFE] (hash size = 64 KB << result), or -1 when the engine
   refuses to start (p[0x81] = 0xFF: "Fritz needs more RAM!"). */
int engine_hash_exponent(const EngineMemory *mem);

/* Largest free DOS block assumed by the port: inside the range
   0x3800..0x57FF that gives the 128 KB hash the original shows in the
   reference DOSBox setup. */
enum { ENGINE_DEFAULT_LARGEST_PARAGRAPHS = 0x4000 };

#endif
