#ifndef FRITZ_EXE_H
#define FRITZ_EXE_H

#include <stddef.h>
#include <stdint.h>

/* The user's original FRITZ2.EXE, unpacked in memory. The port reads its
   artwork (fonts, bitmaps) from here instead of shipping copies of it. */
typedef struct Exe {
    uint8_t *image;   /* load image, relocations not applied */
    size_t size;
} Exe;

enum { EXE_DGROUP = 0x3464 };

/* Loads and unpacks an EXEPACK-compressed MZ file. Returns 0 on success,
   or -1 with a message in `error`. */
int exe_load(Exe *exe, const char *path, char *error, size_t error_size);
void exe_free(Exe *exe);

/* Pointer to SEG:OFF in the image (segment relative to the load address),
   or NULL if [SEG:OFF, +len) is out of range. */
const uint8_t *exe_at(const Exe *exe, uint16_t seg, uint16_t off, size_t len);

#endif
