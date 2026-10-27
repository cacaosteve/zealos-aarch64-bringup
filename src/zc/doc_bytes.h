/* Bounded DolDoc resource recovery. Pinned ZealOS documents encode their
 * original CP437 binary tail as UTF-8. Native documents already have packed
 * bytes and must pass through unchanged. Only the tail after the text NUL is
 * eligible; executable source text is never transcoded. */
#ifndef ZC_DOC_BYTES_H
#define ZC_DOC_BYTES_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
static const uint16_t doc_cp437_high[128] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
    0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
    0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
    0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
    0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
    0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
    0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
    0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
    0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
    0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
    0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
    0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
    0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
};
static uint32_t doc_u32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int doc_binary_valid(const unsigned char *data, size_t size) {
    while (size) {
        if (size < 16) return 0;
        uint32_t bytes = doc_u32(data+8);
        if (bytes > size-16) return 0;
        data += 16+bytes;
        size -= 16+bytes;
    }
    return 1;
}
/* Return 1 with an owned decoded buffer, 0 for native/plain input, -1 for an
 * invalid resource tail. The caller keeps ownership of the original buffer. */
static int doc_bytes_normalize(const unsigned char *data, size_t size,
                               unsigned char **owned, size_t *result_size) {
    *owned = NULL;
    *result_size = size;
    if (!size) return 0;
    const unsigned char *nul = memchr(data, 0, size);
    if (!nul) return 0;
    size_t prefix = (size_t)(nul-data)+1, tail = size-prefix;
    if (doc_binary_valid(data+prefix, tail)) return 0;
    unsigned char *copy = malloc(size+1);
    if (!copy) return -1;
    memcpy(copy, data, prefix);
    size_t used = prefix;
    for (size_t i=prefix; i<size;) {
        uint32_t cp = data[i++];
        unsigned count = 0;
        if (cp < 128) { copy[used++]=(unsigned char)cp; continue; }
        if (cp >= 0xc2 && cp <= 0xdf) { cp &= 0x1f; count=1; }
        else if (cp >= 0xe0 && cp <= 0xef) { cp &= 15; count=2; }
        else goto invalid;
        unsigned width = count;
        while (count--) {
            if (i >= size || (data[i]&0xc0) != 0x80) goto invalid;
            cp = cp<<6 | (data[i++]&63);
        }
        if ((width==2 && cp<0x800) || (cp>=0xd800 && cp<=0xdfff)) goto invalid;
        unsigned byte;
        for (byte=0; byte<128; byte++) if (doc_cp437_high[byte]==cp) break;
        if (byte==128) goto invalid;
        copy[used++]=(unsigned char)(byte+128);
    }
    if (!doc_binary_valid(copy+prefix, used-prefix)) goto invalid;
    copy[used]=0;
    *owned=copy;
    *result_size=used;
    return 1;
invalid:
    free(copy);
    return -1;
}
#endif
