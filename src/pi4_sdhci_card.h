/* Polled SD-card identification and bounded PIO block I/O. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pi4_sdhci_core.h"

#define PI4_SDHCI_CARD_REG_ARGUMENT       0x08u
#define PI4_SDHCI_CARD_REG_PRESENT_STATE  0x24u
#define PI4_SDHCI_CARD_REG_POWER_WORD     0x28u
#define PI4_SDHCI_CARD_REG_TIMEOUT_WORD   0x2cu
#define PI4_SDHCI_CARD_REG_CAPABILITIES   0x40u
#define PI4_SDHCI_CARD_REG_HOST_VERSION   0xfcu

#define PI4_SDHCI_CARD_INT_CMD_COMPLETE   0x00000001u
#define PI4_SDHCI_CARD_INT_XFER_COMPLETE  0x00000002u
#define PI4_SDHCI_CARD_INT_WRITE_READY    0x00000010u
#define PI4_SDHCI_CARD_INT_READ_READY     0x00000020u
#define PI4_SDHCI_CARD_INT_ERROR          0x00008000u
#define PI4_SDHCI_CARD_INT_ERROR_DETAIL   0xffff0000u
#define PI4_SDHCI_CARD_PRESENT_CMD_BUSY   0x00000001u
#define PI4_SDHCI_CARD_PRESENT_DATA_BUSY  0x00000002u
#define PI4_SDHCI_CARD_PRESENT_WRITE_PROTECT 0x00080000u
#define PI4_SDHCI_CARD_PRESENT_INSERTED   0x00100000u

#define PI4_SDHCI_CARD_ERR_IO       -1
#define PI4_SDHCI_CARD_ERR_TIMEOUT  -2
#define PI4_SDHCI_CARD_ERR_HOST     -3
#define PI4_SDHCI_CARD_ERR_COMMAND  -4
#define PI4_SDHCI_CARD_ERR_UNSUPPORTED -5
#define PI4_SDHCI_CARD_ERR_RANGE    -6

#define PI4_SDHCI_CARD_R1_READY_FOR_DATA (1u << 8)
#define PI4_SDHCI_CARD_R1_STATE_MASK     (15u << 9)
#define PI4_SDHCI_CARD_R1_STATE_TRAN     (4u << 9)

struct pi4_sdhci_card_io {
    void *context;
    uint64_t counter_hz;
    uint64_t (*counter)(void *context);
    int (*read32)(void *context, unsigned offset, uint32_t *value);
    int (*write32)(void *context, unsigned offset, uint32_t value);
};

struct pi4_sdhci_card {
    struct pi4_sdhci_card_io io;
    uint16_t rca;
    uint32_t ocr;
    uint64_t sectors;
    uint32_t identification_clock_hz;
    uint8_t host_version;
    uint8_t high_capacity;
    uint8_t ready;
};

static uint64_t pi4_sdhci_card_deadline(const struct pi4_sdhci_card *card,
                                        uint32_t milliseconds) {
    uint64_t ticks = (card->io.counter_hz * milliseconds + 999u) / 1000u;
    return card->io.counter(card->io.context) + (ticks ? ticks : 1u);
}

static int pi4_sdhci_card_expired(const struct pi4_sdhci_card *card,
                                  uint64_t deadline) {
    return (int64_t)(card->io.counter(card->io.context) - deadline) >= 0;
}

static int pi4_sdhci_card_wait_reg(struct pi4_sdhci_card *card,
                                   unsigned offset, uint32_t mask,
                                   uint32_t expected, uint64_t deadline,
                                   uint32_t *last) {
    uint32_t value;
    for (;;) {
        if (!card->io.read32(card->io.context, offset, &value))
            return PI4_SDHCI_CARD_ERR_IO;
        if (last) *last = value;
        if ((value & mask) == expected) return 0;
        if (pi4_sdhci_card_expired(card, deadline))
            return PI4_SDHCI_CARD_ERR_TIMEOUT;
        __asm__ volatile("yield");
    }
}

static int pi4_sdhci_card_wait_status(struct pi4_sdhci_card *card,
                                      unsigned offset, uint32_t mask,
                                      uint64_t deadline, uint32_t *status) {
    uint32_t value;
    for (;;) {
        if (!card->io.read32(card->io.context, offset, &value))
            return PI4_SDHCI_CARD_ERR_IO;
        if (value & mask) {
            if (status) *status = value;
            return 0;
        }
        if (pi4_sdhci_card_expired(card, deadline))
            return PI4_SDHCI_CARD_ERR_TIMEOUT;
        __asm__ volatile("yield");
    }
}

/* Perform byte updates as aligned 32-bit accesses. BCM2711 eMMC2 is
 * sensitive to narrow/closely spaced accesses to adjacent SDHCI registers. */
static int pi4_sdhci_card_write_byte(struct pi4_sdhci_card *card,
                                     unsigned offset, uint8_t value) {
    unsigned aligned = offset & ~3u;
    uint32_t old_word, new_word;
    if (!card->io.read32(card->io.context, aligned, &old_word) ||
        !pi4_sdhci_replace_byte(old_word, offset & 3u, value, &new_word) ||
        !card->io.write32(card->io.context, aligned, new_word))
        return PI4_SDHCI_CARD_ERR_IO;
    return 0;
}

static int pi4_sdhci_card_set_clock(struct pi4_sdhci_card *card,
                                    uint32_t base_hz, uint32_t target_hz) {
    uint16_t divider;
    uint32_t actual, word;
    if (!pi4_sdhci_clock_divisor(base_hz, target_hz, card->host_version,
                                  &divider, &actual))
        return PI4_SDHCI_CARD_ERR_UNSUPPORTED;
    if (!card->io.read32(card->io.context,
                         PI4_SDHCI_CARD_REG_TIMEOUT_WORD, &word))
        return PI4_SDHCI_CARD_ERR_IO;
    word = (word & 0xffff0000u) | divider |
           PI4_SDHCI_CLOCK_INTERNAL_ENABLE;
    if (!card->io.write32(card->io.context,
                          PI4_SDHCI_CARD_REG_TIMEOUT_WORD, word))
        return PI4_SDHCI_CARD_ERR_IO;
    uint64_t deadline = pi4_sdhci_card_deadline(card, 1000);
    int rc = pi4_sdhci_card_wait_reg(card,
                 PI4_SDHCI_CARD_REG_TIMEOUT_WORD,
                 PI4_SDHCI_CLOCK_INTERNAL_STABLE,
                 PI4_SDHCI_CLOCK_INTERNAL_STABLE, deadline, &word);
    if (rc) return rc;
    word |= PI4_SDHCI_CLOCK_CARD_ENABLE;
    if (!card->io.write32(card->io.context,
                          PI4_SDHCI_CARD_REG_TIMEOUT_WORD, word))
        return PI4_SDHCI_CARD_ERR_IO;
    card->identification_clock_hz = actual;
    return 0;
}

static int pi4_sdhci_card_command(struct pi4_sdhci_card *card,
                                  uint8_t index, uint32_t argument,
                                  enum pi4_sdhci_response response,
                                  int check_crc, int check_index,
                                  int data_present, uint16_t transfer_mode,
                                  uint32_t response_words[4]) {
    uint32_t present, status;
    uint16_t command_word;
    uint64_t deadline;
    int rc;

    deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_reg(card, PI4_SDHCI_CARD_REG_PRESENT_STATE,
             PI4_SDHCI_CARD_PRESENT_CMD_BUSY |
             (data_present || response == PI4_SDHCI_RESPONSE_48_BUSY ?
              PI4_SDHCI_CARD_PRESENT_DATA_BUSY : 0),
             0, deadline, &present);
    if (rc) return rc;
    if (!card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                          0xffffffffu) ||
        !card->io.write32(card->io.context, PI4_SDHCI_CARD_REG_ARGUMENT,
                          argument) ||
        !pi4_sdhci_command_word(index, response, check_crc, check_index,
                                data_present, &command_word))
        return PI4_SDHCI_CARD_ERR_IO;
    uint32_t paired = pi4_sdhci_transfer_command(transfer_mode, command_word);
    if (!card->io.write32(card->io.context,
                          PI4_SDHCI_REG_TRANSFER_COMMAND, paired))
        return PI4_SDHCI_CARD_ERR_IO;

    deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_status(card, PI4_SDHCI_REG_INT_STATUS,
             PI4_SDHCI_CARD_INT_CMD_COMPLETE |
             PI4_SDHCI_CARD_INT_ERROR |
             PI4_SDHCI_CARD_INT_ERROR_DETAIL,
             deadline, &status);
    if (rc) return rc;
    if (status & (PI4_SDHCI_CARD_INT_ERROR |
                  PI4_SDHCI_CARD_INT_ERROR_DETAIL)) {
        (void)card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                               status);
        return PI4_SDHCI_CARD_ERR_COMMAND;
    }
    if (!(status & PI4_SDHCI_CARD_INT_CMD_COMPLETE))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    if (!card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                          PI4_SDHCI_CARD_INT_CMD_COMPLETE))
        return PI4_SDHCI_CARD_ERR_IO;
    if (response_words && response == PI4_SDHCI_RESPONSE_136) {
        uint32_t registers[4];
        for (unsigned i = 0; i < 4; i++) {
            if (!card->io.read32(card->io.context,
                                 PI4_SDHCI_REG_RESPONSE0 + i * 4u,
                                 &registers[i]))
                return PI4_SDHCI_CARD_ERR_IO;
        }
        pi4_sdhci_unpack_response136(registers[0], registers[1],
                                     registers[2], registers[3],
                                     response_words);
    } else if (response_words && response != PI4_SDHCI_RESPONSE_NONE) {
        if (!card->io.read32(card->io.context, PI4_SDHCI_REG_RESPONSE0,
                             &response_words[0]))
            return PI4_SDHCI_CARD_ERR_IO;
        response_words[1] = response_words[2] = response_words[3] = 0;
    }
    return 0;
}

static int pi4_sdhci_card_init(struct pi4_sdhci_card *card,
                               const struct pi4_sdhci_card_io *io) {
    uint32_t caps, version, word, response[4], ocr = 0;
    uint32_t base_hz;
    uint64_t deadline;
    int rc;
    if (!card || !io || !io->counter || !io->counter_hz ||
        !io->read32 || !io->write32)
        return PI4_SDHCI_CARD_ERR_IO;
    card->io = *io;
    card->rca = 0;
    card->ocr = 0;
    card->sectors = 0;
    card->identification_clock_hz = 0;
    card->ready = 0;

    if (!io->read32(io->context, PI4_SDHCI_CARD_REG_CAPABILITIES, &caps) ||
        !io->read32(io->context, PI4_SDHCI_CARD_REG_HOST_VERSION, &version))
        return PI4_SDHCI_CARD_ERR_IO;
    card->host_version = (uint8_t)(version >> 16);
    base_hz = ((caps >> 8) & 0xffu) * 1000000u;
    if (!base_hz || !(caps & (1u << 24)))
        return PI4_SDHCI_CARD_ERR_UNSUPPORTED;

    /* Software reset, power at 3.3 V, and the conservative identification
     * clock. */
    rc = pi4_sdhci_card_write_byte(card, PI4_SDHCI_REG_SOFTWARE_RESET, 1u);
    if (rc) return rc;
    deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_reg(card, PI4_SDHCI_CARD_REG_TIMEOUT_WORD,
             0x01000000u, 0, deadline, &word);
    if (rc) return rc;
    rc = pi4_sdhci_card_write_byte(card, PI4_SDHCI_REG_HOST_CONTROL,
                                    0u); /* 1-bit, PIO, normal speed */
    if (rc) return rc;
    rc = pi4_sdhci_card_write_byte(card, PI4_SDHCI_REG_HOST_CONTROL + 1u,
                                    0x0fu); /* power on, 3.3 V */
    if (rc) return rc;
    deadline = pi4_sdhci_card_deadline(card, 10);
    while (!pi4_sdhci_card_expired(card, deadline))
        __asm__ volatile("yield");
    rc = pi4_sdhci_card_write_byte(card, PI4_SDHCI_CARD_REG_TIMEOUT_WORD + 2u,
                                    0x0eu);
    if (rc) return rc;
    rc = pi4_sdhci_card_set_clock(card, base_hz, 400000u);
    if (rc) return rc;
    if (!io->read32(io->context, PI4_SDHCI_CARD_REG_PRESENT_STATE, &word))
        return PI4_SDHCI_CARD_ERR_IO;
    if (!(word & PI4_SDHCI_CARD_PRESENT_INSERTED))
        return PI4_SDHCI_CARD_ERR_UNSUPPORTED;

    rc = pi4_sdhci_card_command(card, 0, 0, PI4_SDHCI_RESPONSE_NONE,
                                 0, 0, 0, 0, 0);
    if (rc) return rc;
    rc = pi4_sdhci_card_command(card, 8, 0x1aau, PI4_SDHCI_RESPONSE_48,
                                 1, 1, 0, 0, response);
    int v2_card = rc == 0 && (response[0] & 0xfffu) == 0x1aau;
    if (rc && rc != PI4_SDHCI_CARD_ERR_COMMAND) return rc;

    deadline = pi4_sdhci_card_deadline(card, 1000);
    do {
        rc = pi4_sdhci_card_command(card, 55, 0,
                 PI4_SDHCI_RESPONSE_48, 1, 1, 0, 0, response);
        if (rc) return rc;
        rc = pi4_sdhci_card_command(card, 41,
                 0x00ff8000u | (v2_card ? (1u << 30) : 0),
                 PI4_SDHCI_RESPONSE_48, 0, 0, 0, 0, response);
        if (rc) return rc;
        ocr = response[0];
        if (ocr & (1u << 31)) break;
        if (pi4_sdhci_card_expired(card, deadline))
            return PI4_SDHCI_CARD_ERR_TIMEOUT;
        uint64_t retry_deadline = pi4_sdhci_card_deadline(card, 10);
        while (!pi4_sdhci_card_expired(card, retry_deadline))
            __asm__ volatile("yield");
    } while (1);
    card->ocr = ocr;
    card->high_capacity = !!(ocr & (1u << 30));

    rc = pi4_sdhci_card_command(card, 2, 0, PI4_SDHCI_RESPONSE_136,
                                 0, 0, 0, 0, response);
    if (rc) return rc;
    rc = pi4_sdhci_card_command(card, 3, 0, PI4_SDHCI_RESPONSE_48,
                                 1, 1, 0, 0, response);
    if (rc) return rc;
    card->rca = (uint16_t)(response[0] >> 16);
    if (!card->rca) return PI4_SDHCI_CARD_ERR_COMMAND;

    rc = pi4_sdhci_card_command(card, 9, (uint32_t)card->rca << 16,
                                 PI4_SDHCI_RESPONSE_136, 0, 0, 0, 0,
                                 response);
    if (rc || !pi4_sdhci_csd_sector_count(response, &card->sectors))
        return rc ? rc : PI4_SDHCI_CARD_ERR_UNSUPPORTED;
    rc = pi4_sdhci_card_command(card, 7, (uint32_t)card->rca << 16,
                                 PI4_SDHCI_RESPONSE_48_BUSY,
                                 1, 1, 0, 0, response);
    if (rc) return rc;
    deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_reg(card, PI4_SDHCI_CARD_REG_PRESENT_STATE,
             PI4_SDHCI_CARD_PRESENT_DATA_BUSY, 0, deadline, &word);
    if (rc) return rc;
    if (!card->high_capacity) {
        rc = pi4_sdhci_card_command(card, 16, 512,
                 PI4_SDHCI_RESPONSE_48, 1, 1, 0, 0, response);
        if (rc) return rc;
    }
    card->ready = 1;
    return 0;
}

static int pi4_sdhci_card_read_sector(struct pi4_sdhci_card *card,
                                      uint64_t lba, uint8_t sector[512]) {
    uint32_t word, status;
    uint64_t address;
    int rc;
    if (!card || !card->ready || !sector)
        return PI4_SDHCI_CARD_ERR_IO;
    if (lba >= card->sectors) return PI4_SDHCI_CARD_ERR_RANGE;
    address = card->high_capacity ? lba : lba * 512u;
    if (address > 0xffffffffu) return PI4_SDHCI_CARD_ERR_RANGE;
    if (!pi4_sdhci_block_size_count(512, 1, &word) ||
        !card->io.write32(card->io.context,
                          PI4_SDHCI_REG_BLOCK_SIZE_COUNT, word))
        return PI4_SDHCI_CARD_ERR_IO;
    rc = pi4_sdhci_card_command(card, 17, (uint32_t)address,
             PI4_SDHCI_RESPONSE_48, 1, 1, 1,
             PI4_SDHCI_TRANSFER_BLOCK_COUNT_ENABLE | PI4_SDHCI_TRANSFER_READ,
             0);
    if (rc) return rc;
    uint64_t deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_status(card, PI4_SDHCI_REG_INT_STATUS,
             PI4_SDHCI_CARD_INT_READ_READY |
             PI4_SDHCI_CARD_INT_ERROR |
             PI4_SDHCI_CARD_INT_ERROR_DETAIL,
             deadline, &status);
    if (rc) return rc;
    if (status & (PI4_SDHCI_CARD_INT_ERROR |
                  PI4_SDHCI_CARD_INT_ERROR_DETAIL))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    if (!(status & PI4_SDHCI_CARD_INT_READ_READY))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    for (unsigned i = 0; i < 128; i++) {
        if (!card->io.read32(card->io.context, PI4_SDHCI_REG_DATA, &word))
            return PI4_SDHCI_CARD_ERR_IO;
        sector[i * 4u + 0] = (uint8_t)word;
        sector[i * 4u + 1] = (uint8_t)(word >> 8);
        sector[i * 4u + 2] = (uint8_t)(word >> 16);
        sector[i * 4u + 3] = (uint8_t)(word >> 24);
    }
    if (!card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                          PI4_SDHCI_CARD_INT_READ_READY))
        return PI4_SDHCI_CARD_ERR_IO;
    deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_status(card, PI4_SDHCI_REG_INT_STATUS,
             PI4_SDHCI_CARD_INT_XFER_COMPLETE |
             PI4_SDHCI_CARD_INT_ERROR |
             PI4_SDHCI_CARD_INT_ERROR_DETAIL,
             deadline, &status);
    if (rc) return rc;
    if (status & (PI4_SDHCI_CARD_INT_ERROR |
                  PI4_SDHCI_CARD_INT_ERROR_DETAIL))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    if (!(status & PI4_SDHCI_CARD_INT_XFER_COMPLETE))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    return card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                            PI4_SDHCI_CARD_INT_XFER_COMPLETE) ? 0 :
           PI4_SDHCI_CARD_ERR_IO;
}

static int pi4_sdhci_card_write_sector(struct pi4_sdhci_card *card,
                                       uint64_t lba,
                                       const uint8_t sector[512]) {
    uint32_t word, status, present;
    uint64_t address;
    int rc;
    if (!card || !card->ready || !sector)
        return PI4_SDHCI_CARD_ERR_IO;
    if (lba >= card->sectors) return PI4_SDHCI_CARD_ERR_RANGE;
    if (!card->io.read32(card->io.context,
                         PI4_SDHCI_CARD_REG_PRESENT_STATE, &present))
        return PI4_SDHCI_CARD_ERR_IO;
    if (present & PI4_SDHCI_CARD_PRESENT_WRITE_PROTECT)
        return PI4_SDHCI_CARD_ERR_UNSUPPORTED;
    address = card->high_capacity ? lba : lba * 512u;
    if (address > 0xffffffffu) return PI4_SDHCI_CARD_ERR_RANGE;
    if (!pi4_sdhci_block_size_count(512, 1, &word) ||
        !card->io.write32(card->io.context,
                          PI4_SDHCI_REG_BLOCK_SIZE_COUNT, word))
        return PI4_SDHCI_CARD_ERR_IO;
    rc = pi4_sdhci_card_command(card, 24, (uint32_t)address,
             PI4_SDHCI_RESPONSE_48, 1, 1, 1,
             PI4_SDHCI_TRANSFER_BLOCK_COUNT_ENABLE, 0);
    if (rc) return rc;
    uint64_t deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_status(card, PI4_SDHCI_REG_INT_STATUS,
             PI4_SDHCI_CARD_INT_WRITE_READY |
             PI4_SDHCI_CARD_INT_ERROR |
             PI4_SDHCI_CARD_INT_ERROR_DETAIL,
             deadline, &status);
    if (rc) return rc;
    if (status & (PI4_SDHCI_CARD_INT_ERROR |
                  PI4_SDHCI_CARD_INT_ERROR_DETAIL))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    if (!(status & PI4_SDHCI_CARD_INT_WRITE_READY))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    for (unsigned i = 0; i < 128; i++) {
        word = (uint32_t)sector[i * 4u + 0] |
               ((uint32_t)sector[i * 4u + 1] << 8) |
               ((uint32_t)sector[i * 4u + 2] << 16) |
               ((uint32_t)sector[i * 4u + 3] << 24);
        if (!card->io.write32(card->io.context, PI4_SDHCI_REG_DATA, word))
            return PI4_SDHCI_CARD_ERR_IO;
    }
    if (!card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                          PI4_SDHCI_CARD_INT_WRITE_READY))
        return PI4_SDHCI_CARD_ERR_IO;
    deadline = pi4_sdhci_card_deadline(card, 1000);
    rc = pi4_sdhci_card_wait_status(card, PI4_SDHCI_REG_INT_STATUS,
             PI4_SDHCI_CARD_INT_XFER_COMPLETE |
             PI4_SDHCI_CARD_INT_ERROR |
             PI4_SDHCI_CARD_INT_ERROR_DETAIL,
             deadline, &status);
    if (rc) return rc;
    if (status & (PI4_SDHCI_CARD_INT_ERROR |
                  PI4_SDHCI_CARD_INT_ERROR_DETAIL))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    if (!(status & PI4_SDHCI_CARD_INT_XFER_COMPLETE))
        return PI4_SDHCI_CARD_ERR_COMMAND;
    return card->io.write32(card->io.context, PI4_SDHCI_REG_INT_STATUS,
                            PI4_SDHCI_CARD_INT_XFER_COMPLETE) ? 0 :
           PI4_SDHCI_CARD_ERR_IO;
}

/* SD cards have no general-purpose cache-flush command. Wait for DAT0 to
 * leave its programming-busy state, then confirm TRAN/READY_FOR_DATA with
 * CMD13 before reporting the backend flush complete. */
static int pi4_sdhci_card_flush(struct pi4_sdhci_card *card) {
    uint32_t present, response[4];
    uint64_t deadline;
    int rc;
    if (!card || !card->ready)
        return PI4_SDHCI_CARD_ERR_IO;
    deadline = pi4_sdhci_card_deadline(card, 3000);
    rc = pi4_sdhci_card_wait_reg(card, PI4_SDHCI_CARD_REG_PRESENT_STATE,
             PI4_SDHCI_CARD_PRESENT_DATA_BUSY, 0, deadline, &present);
    if (rc) return rc;
    rc = pi4_sdhci_card_command(card, 13, (uint32_t)card->rca << 16,
             PI4_SDHCI_RESPONSE_48, 1, 1, 0, 0, response);
    if (rc) return rc;
    if (!(response[0] & PI4_SDHCI_CARD_R1_READY_FOR_DATA) ||
        (response[0] & PI4_SDHCI_CARD_R1_STATE_MASK) !=
        PI4_SDHCI_CARD_R1_STATE_TRAN)
        return PI4_SDHCI_CARD_ERR_HOST;
    return 0;
}

/* Use conservative single-block commands until multi-block termination and
 * error recovery have been validated on the physical BCM2711 controller. */
static int pi4_sdhci_card_read_many(struct pi4_sdhci_card *card,
                                    uint64_t first_lba, uint32_t count,
                                    void *buffer) {
    uint8_t *out = buffer;
    if (!card || !buffer) return PI4_SDHCI_CARD_ERR_IO;
    if (!count || (uint64_t)count > SIZE_MAX / 512u ||
        first_lba >= card->sectors ||
        count > card->sectors - first_lba)
        return PI4_SDHCI_CARD_ERR_RANGE;
    for (uint32_t i = 0; i < count; i++) {
        int rc = pi4_sdhci_card_read_sector(card, first_lba + i,
                                             out + (size_t)i * 512u);
        if (rc) return rc;
    }
    return 0;
}

static int pi4_sdhci_card_write_many(struct pi4_sdhci_card *card,
                                     uint64_t first_lba, uint32_t count,
                                     const void *buffer) {
    const uint8_t *in = buffer;
    if (!card || !buffer) return PI4_SDHCI_CARD_ERR_IO;
    if (!count || (uint64_t)count > SIZE_MAX / 512u ||
        first_lba >= card->sectors ||
        count > card->sectors - first_lba)
        return PI4_SDHCI_CARD_ERR_RANGE;
    for (uint32_t i = 0; i < count; i++) {
        int rc = pi4_sdhci_card_write_sector(card, first_lba + i,
                                              in + (size_t)i * 512u);
        if (rc) return rc;
    }
    return 0;
}
