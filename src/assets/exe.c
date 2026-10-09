#include "exe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

static int fail(char *error, size_t size, const char *msg)
{
    snprintf(error, size, "%s", msg);
    return -1;
}

/* Same algorithm as tools/inspect/unexepack.py: the EXEPACK header sits at
   CS:0000 of the packed image; data is expanded backwards. */
static int unpack(const uint8_t *file, size_t file_size, Exe *exe,
                  char *error, size_t error_size)
{
    if (file_size < 0x1c || rd16(file) != 0x5a4d)
        return fail(error, error_size, "not an MZ executable");
    uint16_t last = rd16(file + 2), pages = rd16(file + 4);
    uint16_t header = rd16(file + 8), cs = rd16(file + 0x16);
    size_t size = (size_t)pages * 512 - (last ? 512 - last : 0);
    if (size > file_size || (size_t)header * 16 > size)
        return fail(error, error_size, "truncated MZ executable");

    const uint8_t *image = file + header * 16;
    size_t image_size = size - header * 16;
    size_t hdr = (size_t)cs * 16;
    if (hdr + 18 > image_size)
        return fail(error, error_size, "EXEPACK header out of range");

    uint16_t dest_paras = rd16(image + hdr + 12);
    uint16_t skip = 1;
    if (rd16(image + hdr + 14) != 0x4252) {
        skip = rd16(image + hdr + 14);
        if (rd16(image + hdr + 16) != 0x4252)
            return fail(error, error_size, "EXEPACK signature not found");
    }
    size_t packed_size = hdr - (size_t)(skip - 1) * 16;
    if (packed_size > hdr)
        return fail(error, error_size, "bad EXEPACK skip length");

    size_t out_size = (size_t)dest_paras * 16;
    uint8_t *out = calloc(out_size ? out_size : 1, 1);
    if (!out)
        return fail(error, error_size, "out of memory");

    long src = (long)packed_size - 1;
    long dst = (long)out_size - 1;
    while (src >= 0 && image[src] == 0xff)
        src--;
    for (;;) {
        if (src < 2) {
            free(out);
            return fail(error, error_size, "corrupt EXEPACK data");
        }
        uint8_t cmd = image[src];
        long count = image[src - 1] << 8 | image[src - 2];
        src -= 3;
        if ((cmd & 0xfe) == 0xb0) {
            uint8_t fill = image[src--];
            if (dst + 1 < count)
                break;
            for (long i = 0; i < count; i++)
                out[dst--] = fill;
        } else if ((cmd & 0xfe) == 0xb2) {
            if (dst + 1 < count || src + 1 < count)
                break;
            for (long i = 0; i < count; i++)
                out[dst--] = image[src--];
        } else {
            break;
        }
        if (cmd & 1) {
            memcpy(out, image, (size_t)(src + 1));
            exe->image = out;
            exe->size = out_size;
            return 0;
        }
    }
    free(out);
    return fail(error, error_size, "corrupt EXEPACK data");
}

int exe_load(Exe *exe, const char *path, char *error, size_t error_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(error, error_size, "cannot open %s", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *file = n > 0 ? malloc((size_t)n) : NULL;
    if (!file || fread(file, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(file);
        snprintf(error, error_size, "cannot read %s", path);
        return -1;
    }
    fclose(f);
    int rc = unpack(file, (size_t)n, exe, error, error_size);
    free(file);
    return rc;
}

void exe_free(Exe *exe)
{
    free(exe->image);
    exe->image = NULL;
    exe->size = 0;
}

const uint8_t *exe_at(const Exe *exe, uint16_t seg, uint16_t off, size_t len)
{
    size_t pos = (size_t)seg * 16 + off;
    if (!exe->image || pos + len > exe->size)
        return NULL;
    return exe->image + pos;
}
