#include <assert.h>
#include <stdint.h>
#include "block_range.h"

int main(void) {
    assert(zeal_block_range_valid(0, 128, 128));
    assert(zeal_block_range_valid(127, 1, 128));
    assert(!zeal_block_range_valid(0, 0, 128));
    assert(!zeal_block_range_valid(128, 1, 128));
    assert(!zeal_block_range_valid(127, 2, 128));
    assert(!zeal_block_range_valid(UINT64_MAX, 2, 128));
    assert(!zeal_block_range_valid(1, UINT64_MAX, 128));
    assert(!zeal_block_range_valid(0, 1, 0));
    return 0;
}
