/* Portable SDHCI register helpers shared by the Pi driver and host tests. */
#pragma once

#include <stdint.h>

#define PI4_SDHCI_REG_BLOCK_SIZE_COUNT 0x04u
#define PI4_SDHCI_REG_TRANSFER_COMMAND 0x0cu
#define PI4_SDHCI_REG_RESPONSE0        0x10u
#define PI4_SDHCI_REG_DATA             0x20u
#define PI4_SDHCI_REG_HOST_CONTROL     0x28u
#define PI4_SDHCI_REG_CLOCK_CONTROL   0x2cu
#define PI4_SDHCI_REG_SOFTWARE_RESET   0x2fu
#define PI4_SDHCI_REG_INT_STATUS      0x30u

#define PI4_SDHCI_CLOCK_INTERNAL_ENABLE 0x0001u
#define PI4_SDHCI_CLOCK_INTERNAL_STABLE 0x0002u
#define PI4_SDHCI_CLOCK_CARD_ENABLE     0x0004u
#define PI4_SDHCI_CLOCK_DIV_MASK        0xffc0u

#define PI4_SDHCI_INT_COMMAND_COMPLETE 0x00000001u
#define PI4_SDHCI_INT_TRANSFER_COMPLETE 0x00000002u
#define PI4_SDHCI_INT_BUFFER_READ_READY 0x00000020u
#define PI4_SDHCI_INT_ERROR             0x00008000u

#define PI4_SDHCI_PRESENT_CMD_INHIBIT  0x00000001u
#define PI4_SDHCI_PRESENT_DATA_INHIBIT 0x00000002u
#define PI4_SDHCI_PRESENT_CARD_INSERTED 0x00100000u

#define PI4_SDHCI_TRANSFER_BLOCK_COUNT_ENABLE 0x0002u
#define PI4_SDHCI_TRANSFER_READ              0x0010u
#define PI4_SDHCI_TRANSFER_MULTI_BLOCK       0x0020u

enum pi4_sdhci_response {
    PI4_SDHCI_RESPONSE_NONE = 0,
    PI4_SDHCI_RESPONSE_136 = 1,
    PI4_SDHCI_RESPONSE_48 = 2,
    PI4_SDHCI_RESPONSE_48_BUSY = 3,
};

/* Return the SDHCI clock-divider field for a requested maximum clock.
 * `host_version` is the low byte of SDHCI_HOST_VERSION (0=1.0, 1=2.0,
 * 2=3.0). SDHCI 3.0 adds two high divider bits; earlier versions allow only
 * even divisors through 256. `actual_hz` reports the resulting integer rate.
 */
static inline int pi4_sdhci_clock_divisor(uint32_t base_hz,
                                          uint32_t target_hz,
                                          uint8_t host_version,
                                          uint16_t *clock_bits,
                                          uint32_t *actual_hz) {
    uint64_t real_divisor;
    uint64_t encoded_divisor;
    uint32_t actual;

    if (!base_hz || !target_hz || !clock_bits || !actual_hz)
        return 0;

    if (base_hz <= target_hz) {
        real_divisor = 1;
    } else if (host_version < 2) {
        /* SDHCI 1.x/2.0 only permits real divisors that are powers of two. */
        for (real_divisor = 2; real_divisor <= 256; real_divisor <<= 1) {
            if ((uint64_t)base_hz / real_divisor <= target_hz)
                break;
        }
        if (real_divisor > 256) return 0;
    } else {
        /* SDHCI 3.0 permits even real divisors through 2046. */
        for (real_divisor = 2; real_divisor <= 2046; real_divisor += 2) {
            if ((uint64_t)base_hz / real_divisor <= target_hz)
                break;
        }
        if (real_divisor > 2046) return 0;
    }

    /* The register stores half the real divisor; zero represents divisor 1. */
    encoded_divisor = real_divisor == 1 ? 0 : real_divisor >> 1;
    if (host_version < 2) {
        *clock_bits = (uint16_t)((encoded_divisor & 0xffu) << 8);
    } else {
        *clock_bits = (uint16_t)(((encoded_divisor & 0xffu) << 8) |
                                 ((encoded_divisor & 0x300u) >> 2));
    }
    actual = (uint32_t)((uint64_t)base_hz / real_divisor);
    *actual_hz = actual;
    return actual <= target_hz;
}

/* Form the command half of the paired TRANSFER_MODE/COMMAND register. */
static inline int pi4_sdhci_command_word(uint8_t index,
                                          enum pi4_sdhci_response response,
                                          int check_crc, int check_index,
                                          int data_present,
                                          uint16_t *command_word) {
    uint16_t value;
    if (!command_word || index > 63 || response > PI4_SDHCI_RESPONSE_48_BUSY)
        return 0;
    value = (uint16_t)((uint16_t)index << 8) | (uint16_t)response;
    if (check_crc) value |= 1u << 3;
    if (check_index) value |= 1u << 4;
    if (data_present) value |= 1u << 5;
    *command_word = value;
    return 1;
}

/* The BCM2711 eMMC2 host requires the adjacent 16-bit transfer mode and
 * command to be written as one 32-bit access. */
static inline uint32_t pi4_sdhci_transfer_command(uint16_t transfer_mode,
                                                   uint16_t command_word) {
    return ((uint32_t)command_word << 16) | transfer_mode;
}

/* Likewise, block size and block count occupy one paired 32-bit register. */
static inline int pi4_sdhci_block_size_count(uint16_t block_size,
                                              uint16_t block_count,
                                              uint32_t *word) {
    if (!word || !block_size || block_size > 0x0fffu || !block_count)
        return 0;
    *word = ((uint32_t)block_count << 16) | block_size;
    return 1;
}

/* Build a 32-bit read/modify/write value for a byte-wide field. */
static inline int pi4_sdhci_replace_byte(uint32_t old_word, unsigned offset,
                                          uint8_t value, uint32_t *new_word) {
    unsigned shift;
    uint32_t mask;
    if (!new_word || offset > 3) return 0;
    shift = offset * 8u;
    mask = 0xffu << shift;
    *new_word = (old_word & ~mask) | ((uint32_t)value << shift);
    return 1;
}

/* Build a 32-bit read/modify/write value for a halfword field. */
static inline int pi4_sdhci_replace_halfword(uint32_t old_word,
                                              unsigned offset,
                                              uint16_t value,
                                              uint32_t *new_word) {
    unsigned shift;
    uint32_t mask;
    if (!new_word || (offset != 0 && offset != 2)) return 0;
    shift = offset * 8u;
    mask = 0xffffu << shift;
    *new_word = (old_word & ~mask) | ((uint32_t)value << shift);
    return 1;
}

/* SDHCI exposes an R2 response in response3..response0 register order, with
 * the transmitted CRC/start bits omitted. Return a canonical 128-bit
 * MSW-first value containing the aligned CID/CSD bits. */
static inline void pi4_sdhci_unpack_response136(uint32_t response0,
                                                 uint32_t response1,
                                                 uint32_t response2,
                                                 uint32_t response3,
                                                 uint32_t out[4]) {
    uint32_t raw[4] = { response3, response2, response1, response0 };
    if (!out) return;
    for (unsigned i = 0; i < 4; i++)
        out[i] = (raw[i] << 8) | (i < 3 ? raw[i + 1] >> 24 : 0);
}

static inline uint32_t pi4_sdhci_response_bits(const uint32_t words[4],
                                                unsigned most,
                                                unsigned least) {
    uint32_t value = 0;
    if (!words || most > 127 || least > most) return 0;
    for (int bit = (int)most; bit >= (int)least; bit--) {
        unsigned from_msb = 127u - (unsigned)bit;
        unsigned word = from_msb / 32u;
        unsigned position = 31u - (from_msb & 31u);
        value = (value << 1) | ((words[word] >> position) & 1u);
    }
    return value;
}

static inline int pi4_sdhci_csd_sector_count(const uint32_t csd[4],
                                             uint64_t *sectors) {
    uint32_t structure, c_size;
    if (!csd || !sectors) return 0;
    structure = pi4_sdhci_response_bits(csd, 127, 126);
    if (structure == 1) {
        c_size = pi4_sdhci_response_bits(csd, 69, 48);
        *sectors = ((uint64_t)c_size + 1u) * 1024u;
        return 1;
    }
    if (structure == 0) {
        uint32_t read_bl_len = pi4_sdhci_response_bits(csd, 83, 80);
        uint32_t c_size_mult = pi4_sdhci_response_bits(csd, 49, 47);
        c_size = pi4_sdhci_response_bits(csd, 73, 62);
        if (read_bl_len > 31 || c_size_mult > 7) return 0;
        uint64_t blocks = ((uint64_t)c_size + 1u) << (c_size_mult + 2u);
        uint64_t bytes = blocks << read_bl_len;
        if (bytes < 512u || (bytes & 511u)) return 0;
        *sectors = bytes / 512u;
        return *sectors != 0;
    }
    return 0;
}
