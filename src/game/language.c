#include "language.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { LAN_MAGIC = 0x4d2 };

static unsigned rd16(const Language *lan, long pos)
{
    if (pos < 0 || pos + 2 > lan->size)
        return 0xffff;
    return (unsigned)(lan->data[pos] | lan->data[pos + 1] << 8);
}

bool language_load(Language *lan, const char *path)
{
    FILE *f = fopen(path, "rb");
    lan->data = NULL;
    lan->size = 0;
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = n > 6 ? malloc((size_t)n + 1) : NULL;
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(data);
        return false;
    }
    fclose(f);
    data[n] = 0;   /* keeps a damaged last string terminated */
    lan->data = data;
    lan->size = n;
    if (rd16(lan, 2) != LAN_MAGIC) {
        language_free(lan);
        return false;
    }
    return true;
}

void language_free(Language *lan)
{
    free(lan->data);
    lan->data = NULL;
    lan->size = 0;
}

const char *language_string(const Language *lan, uint16_t block, uint16_t index)
{
    if (!lan->data || block >= rd16(lan, 4))
        return "";
    long dir = (long)rd16(lan, 0) + 4L * block;
    long off = (long)(rd16(lan, dir) | (unsigned long)rd16(lan, dir + 2) << 16);
    unsigned header = rd16(lan, off);
    if (rd16(lan, off + 2) != LAN_MAGIC || rd16(lan, off + 4) != block ||
        index >= rd16(lan, off + 6))
        return "";
    long strings = off + header + 2L * rd16(lan, off + 6);
    long pos = strings + rd16(lan, off + header + 2L * index);
    return pos < lan->size ? (const char *)lan->data + pos : "";
}
