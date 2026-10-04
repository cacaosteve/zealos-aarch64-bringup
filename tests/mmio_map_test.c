#include <stdint.h>
#include <stdio.h>
#include "../src/mmio_map.h"

int main(void) {
    const uint64_t block_phys = 0x0000000040000000ull;
    const uint64_t attrs = (5ull << 2) | (1ull << 6) | (3ull << 8) |
                           (1ull << 10) | (1ull << 11) | (1ull << 52) |
                           (1ull << 53) | (1ull << 54);
    const uint64_t input = block_phys | 0x1ull | attrs;
    const uint64_t preserved = attrs & ~(1ull << 52);
    const unsigned indexes[] = {0, 1, 15, 255, 511};

    /* The supported regime is explicitly 4KiB/48-bit TTBR1. */
    uint64_t valid_tcr = (16ull << 16) | (2ull << 30) | (5ull << 32);
    if (!mmio_pt_tcr_supported(valid_tcr) ||
        mmio_pt_tcr_supported(valid_tcr ^ (1ull << 30)) ||
        mmio_pt_tcr_supported(valid_tcr ^ (1ull << 16)) ||
        mmio_pt_tcr_supported((valid_tcr & ~(7ull << 32)) | (6ull << 32))) {
        fprintf(stderr, "TTBR1 geometry contract mismatch\n");
        return 3;
    }

    /* Misaligned or absent pools fail closed instead of rounding into RAM. */
    mmio_pt_set_pool(0x2003);
    if (g_pt_pool_phys != 0) {
        fprintf(stderr, "misaligned page-table pool was accepted\n");
        return 4;
    }
    mmio_pt_set_pool(0x2000);
    if (g_pt_pool_phys != 0x2000) {
        fprintf(stderr, "aligned page-table pool was rejected\n");
        return 5;
    }

    for (unsigned n = 0; n < sizeof(indexes) / sizeof(indexes[0]); n++) {
        unsigned i = indexes[n];
        uint64_t got = mmio_split_l1_block(input, i);
        uint64_t expected_phys = block_phys + ((uint64_t)i << 21);
        if ((got & 0x0000ffffffe00000ull) != expected_phys ||
            (got & 3ull) != 1ull ||
            (got & (0x0000000000000ffCull | (1ull << 53) |
                    (1ull << 54))) != preserved) {
            fprintf(stderr, "bad L1 split descriptor at index %u: %016llx\n",
                    i, (unsigned long long)got);
            return 1;
        }
        if (got & (1ull << 52)) {
            fprintf(stderr, "contiguous hint survived L1 split at %u\n", i);
            return 2;
        }
    }
    puts("PASS TTBR1 contract, reserved pool alignment, and 1GiB-to-2MiB split");
    return 0;
}
