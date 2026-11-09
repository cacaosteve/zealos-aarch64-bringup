#ifndef ZEAL_BLOCK_RANGE_H
#define ZEAL_BLOCK_RANGE_H

#include <stdint.h>

/* Validate a nonempty sector range without overflowing start + count. */
static inline int zeal_block_range_valid(uint64_t start, uint64_t count,
                                         uint64_t capacity) {
    return count && start < capacity && count <= capacity - start;
}

#endif
