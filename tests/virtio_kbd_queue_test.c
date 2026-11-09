#include <assert.h>
#include "../src/virtio_kbd_queue.h"

int main(void) {
    assert(vk_queue_size(0) == 0);
    for (unsigned max = 1; max <= 512; ++max) {
        unsigned n = vk_queue_size(max);
        assert(n && n <= max && n <= 32 && !(n & (n - 1)));
        assert(n == 32 || n * 2 > max);
        /* The legacy ring's used and event areas must fit its 8 KiB DMA. */
        unsigned used = (n * 16 + 6 + n * 2 + 4095) & ~4095u;
        unsigned events = (used + 6 + n * 8 + 15) & ~15u;
        assert(events + n * 8 <= 8192);
    }
    return 0;
}
