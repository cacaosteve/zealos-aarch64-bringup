/* Platform-neutral absolute block I/O and ZealOS volume geometry. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "disk_layout.h"
#include "block_partition.h"
#include "virtio_blk.h"

struct zeal_block_backend {
    int ready;
    int flush_supported;
    uint32_t max_sectors;
    uint64_t sectors;          /* logical capacity exposed to ZealOS */
    uint64_t device_sectors;   /* physical capacity of the backing device */
    uint64_t lba_offset;       /* logical sector zero -> physical sector */
    uint64_t redsea_lba;
    uint64_t redsea_sectors;
    uint64_t source_a_lba;
    uint32_t source_a_sectors;
    uint64_t source_b_lba;
    uint32_t source_b_sectors;
    int (*read_abs)(void *buf, uint64_t sector, uint32_t count);
    int (*write_abs)(const void *buf, uint64_t sector, uint32_t count);
    int (*flush)(void);
};

static struct zeal_block_backend g_zbd;

static int zbd_virtio_read_abs(void *buf, uint64_t sector, uint32_t count) {
    return virtio_blk_read_abs_many(buf, sector, count);
}

static int zbd_virtio_write_abs(const void *buf, uint64_t sector,
                                uint32_t count) {
    return virtio_blk_write_abs_many(buf, sector, count);
}

static int zbd_virtio_flush(void) {
    return virtio_blk_flush_abs();
}

static int zbd_range_inside(uint64_t base, uint64_t sectors,
                            uint64_t capacity) {
    return sectors && base <= capacity && sectors <= capacity - base;
}

static int zbd_bind(const struct zeal_block_backend *backend) {
    if (!backend || !backend->sectors || !backend->device_sectors ||
        backend->lba_offset > backend->device_sectors ||
        backend->sectors > backend->device_sectors - backend->lba_offset ||
        !backend->max_sectors ||
        !backend->read_abs || !backend->write_abs ||
        (backend->flush_supported && !backend->flush) ||
        !zbd_range_inside(backend->redsea_lba, backend->redsea_sectors,
                          backend->sectors) ||
        !zbd_range_inside(backend->source_a_lba, backend->source_a_sectors,
                          backend->sectors) ||
        !zbd_range_inside(backend->source_b_lba, backend->source_b_sectors,
                          backend->sectors))
        return 0;
    /* Keep each published region disjoint, so filesystem writes cannot
     * overwrite the source-store banks or vice versa. */
    uint64_t rs_end = backend->redsea_lba + backend->redsea_sectors;
    uint64_t a_end = backend->source_a_lba + backend->source_a_sectors;
    uint64_t b_end = backend->source_b_lba + backend->source_b_sectors;
    if (!(rs_end <= backend->source_a_lba ||
          a_end <= backend->redsea_lba) ||
        !(rs_end <= backend->source_b_lba ||
          b_end <= backend->redsea_lba) ||
        !(a_end <= backend->source_b_lba ||
          b_end <= backend->source_a_lba))
        return 0;
    g_zbd = *backend;
    g_zbd.ready = 1;
    return 1;
}

/* Keep the UTM layout identical while allowing a later Pi SD backend to
 * publish the same logical volumes at different absolute card LBAs. */
static int zbd_bind_virtio(void) {
    if (!virtio_blk_absolute_ready() || g_vb.xport != VB_XPORT_PCI)
        return 0;
    const struct zeal_block_backend backend = {
        .flush_supported = g_vb.flush_supported,
        .max_sectors = virtio_blk_max_sectors(),
        .sectors = virtio_blk_full_capacity(),
        .device_sectors = virtio_blk_full_capacity(),
        .lba_offset = 0,
        .redsea_lba = g_vb.lba_base,
        .redsea_sectors = g_vb.capacity,
        .source_a_lba = ZEAL_SRC_LBA_BASE,
        .source_a_sectors = ZEAL_SRC_SECTS,
        .source_b_lba = ZEAL_SRC_SHADOW_LBA,
        .source_b_sectors = ZEAL_SRC_SHADOW_SECTS,
        .read_abs = zbd_virtio_read_abs,
        .write_abs = zbd_virtio_write_abs,
        .flush = zbd_virtio_flush,
    };
    return zbd_bind(&backend);
}

static int zbd_partition_scan_read(void *context, uint64_t lba,
                                   uint8_t sector[ZEAL_PARTITION_SECTOR_SIZE]) {
    const struct zeal_block_backend *device = context;
    return device && device->read_abs && device->device_sectors &&
           lba < device->device_sectors &&
           device->read_abs(sector, lba, 1);
}

/* Bind a dedicated data partition as the entire ZealOS-visible block device.
 * The firmware FAT partition is therefore outside the logical LBA range used
 * by RedSea, the source banks, and user-facing block operations. */
static int zbd_bind_data_partition(const struct zeal_block_backend *device,
                                   const struct zeal_partition *partition) {
    const uint64_t source_sectors = ZEAL_SRC_SECTS + ZEAL_SRC_SHADOW_SECTS;
    const uint64_t required = ZEAL_RS_SECTS + source_sectors;
    struct zeal_block_backend view;
    if (!device || !partition || !device->device_sectors ||
        !device->read_abs || !device->write_abs ||
        !zeal_partition_range_valid(partition->first_lba,
                                    partition->sector_count,
                                    device->device_sectors) ||
        partition->sector_count < required)
        return 0;
    view = *device;
    view.sectors = partition->sector_count;
    view.lba_offset = partition->first_lba;
    view.redsea_lba = 0;
    /* Give the RedSea volume the full data partition except for the two
     * append-only source banks, which stay fixed at the end of the partition. */
    view.redsea_sectors = partition->sector_count - source_sectors;
    view.source_a_lba = view.redsea_sectors;
    view.source_a_sectors = ZEAL_SRC_SECTS;
    view.source_b_lba = view.source_a_lba + ZEAL_SRC_SECTS;
    view.source_b_sectors = ZEAL_SRC_SHADOW_SECTS;
    return zbd_bind(&view);
}

/* Read the raw MBR/GPT through the supplied device callbacks, locate the
 * unique ZealOS data type, and bind it as a partition-bounded logical device. */
static int zbd_discover_and_bind_data_partition(
    const struct zeal_block_backend *device) {
    struct zeal_partition parts[ZEAL_PARTITION_MAX];
    struct zeal_partition data;
    size_t count = 0;
    if (!device || !device->device_sectors || !device->read_abs ||
        zeal_partition_scan(zbd_partition_scan_read, (void *)device,
                            device->device_sectors, parts,
                            ZEAL_PARTITION_MAX, &count) != ZEAL_PARTITION_OK ||
        zeal_partition_find_data(parts, count, &data) != ZEAL_PARTITION_OK)
        return 0;
    return zbd_bind_data_partition(device, &data);
}

static int zbd_ready(void) {
    return g_zbd.ready && g_zbd.read_abs && g_zbd.write_abs;
}

static uint64_t zbd_capacity(void) {
    return zbd_ready() ? g_zbd.sectors : 0;
}

static int zbd_read_abs_many(void *buf, uint64_t sector, uint32_t count) {
    if (!buf || !count || !zbd_ready() || !g_zbd.max_sectors ||
        sector >= g_zbd.sectors ||
        count > g_zbd.sectors - sector)
        return 0;
    uint8_t *dst = (uint8_t *)buf;
    while (count) {
        uint32_t batch = count > g_zbd.max_sectors ?
                         g_zbd.max_sectors : count;
        if (!g_zbd.read_abs(dst, g_zbd.lba_offset + sector, batch)) return 0;
        dst += (size_t)batch * 512u;
        sector += batch;
        count -= batch;
    }
    return 1;
}

static int zbd_write_abs_many(const void *buf, uint64_t sector,
                             uint32_t count) {
    if (!buf || !count || !zbd_ready() || !g_zbd.max_sectors ||
        sector >= g_zbd.sectors ||
        count > g_zbd.sectors - sector)
        return 0;
    const uint8_t *src = (const uint8_t *)buf;
    while (count) {
        uint32_t batch = count > g_zbd.max_sectors ?
                         g_zbd.max_sectors : count;
        if (!g_zbd.write_abs(src, g_zbd.lba_offset + sector, batch)) return 0;
        src += (size_t)batch * 512u;
        sector += batch;
        count -= batch;
    }
    return 1;
}

static int zbd_volume_range_valid(uint64_t sector, uint32_t count) {
    return zbd_ready() && count && sector < g_zbd.redsea_sectors &&
           count <= g_zbd.redsea_sectors - sector;
}

static int zbd_read_volume(void *buf, uint64_t sector, uint32_t count) {
    if (!buf || !zbd_volume_range_valid(sector, count) ||
        g_zbd.redsea_lba > g_zbd.sectors ||
        sector > g_zbd.sectors - g_zbd.redsea_lba ||
        count > g_zbd.sectors - g_zbd.redsea_lba - sector)
        return 0;
    return zbd_read_abs_many(buf, g_zbd.redsea_lba + sector, count);
}

static int zbd_write_volume(const void *buf, uint64_t sector, uint32_t count) {
    if (!buf || !zbd_volume_range_valid(sector, count) ||
        g_zbd.redsea_lba > g_zbd.sectors ||
        sector > g_zbd.sectors - g_zbd.redsea_lba ||
        count > g_zbd.sectors - g_zbd.redsea_lba - sector)
        return 0;
    return zbd_write_abs_many(buf, g_zbd.redsea_lba + sector, count);
}

static int zbd_flush(void) {
    return zbd_ready() && g_zbd.flush && g_zbd.flush();
}
