#pragma once

/* Virtqueue sizes must be powers of two and no greater than the device's
 * advertised maximum. Keep the buffer bounded by the driver's DMA region. */
static inline unsigned vk_queue_size(unsigned device_max) {
    if (!device_max) return 0;
    unsigned n = 1;
    while (n < 32 && n * 2 <= device_max) n *= 2;
    return n;
}
