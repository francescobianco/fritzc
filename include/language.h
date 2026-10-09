#ifndef FRITZ_LANGUAGE_H
#define FRITZ_LANGUAGE_H

#include <stdbool.h>
#include <stdint.h>

/* .LAN text files (FENGLISH.LAN, FDEUTSCH.LAN), read by 19e7:b0c4.

   Header: word directory offset, word 0x4D2, word block count.
   Directory: one dword file offset per block.
   Block: word header size (10), word 0x4D2, word block number,
          word string count, word string data size; then one word offset
          per string (relative to the string data) and the zero-terminated
          strings. */
typedef struct Language {
    uint8_t *data;
    long size;
} Language;

/* Blocks loaded into string tables by 19e7:2354. */
enum {
    LAN_BLOCK_7882 = 0x17,     /* 0x4b strings -> DS:7882 */
    LAN_BLOCK_7922 = 0x18,     /* 0x10 strings -> DS:7922 */
    LAN_BLOCK_7918 = 0x19,     /* 5 strings -> DS:7918 */
    LAN_BLOCK_MESSAGES = 0x1a  /* 0x6a strings -> DS:77AE, used by 0a1f:67c3 */
};

bool language_load(Language *lan, const char *path);
void language_free(Language *lan);

/* String `index` of `block`, or "" if missing. */
const char *language_string(const Language *lan, uint16_t block, uint16_t index);

#endif
