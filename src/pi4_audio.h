/*
 * BCM2711 Pi 4B analog-audio tone output through PWM1 on GPIO40/41.
 *
 * This is a simple square-wave tone backend for the stock ZealOS Sound/Beep
 * hooks. It deliberately does not claim PCM playback quality. On Pi 4B the
 * board routes PWM1 channel 0/1 to the 3.5 mm analog output; the line output
 * may need amplified headphones/speakers.
 */
#pragma once

#include <stdint.h>
#include "mmio_map.h"
#include "plat_pi4.h"

#define PI4_GPIO_PHYS       0xfe200000ull
#define PI4_PWM1_PHYS       0xfe20c800ull
#define PI4_CM_PHYS         0xfe101000ull

#define PI4_GPIO_FSEL4      0x10u
#define PI4_CM_PWMCTL       0xa0u
#define PI4_CM_PWMDIV       0xa4u
#define PI4_PWM_CTL         0x00u
#define PI4_PWM_STA         0x04u
#define PI4_PWM_RNG1        0x10u
#define PI4_PWM_DAT1        0x14u
#define PI4_PWM_RNG2        0x20u
#define PI4_PWM_DAT2        0x24u

#define PI4_CM_PASSWORD     0x5a000000u
#define PI4_CM_CTL_SRC_PLLD 6u
#define PI4_CM_CTL_ENAB     (1u << 4)
#define PI4_CM_CTL_KILL     (1u << 5)
#define PI4_CM_CTL_BUSY     (1u << 7)
#define PI4_CM_DIV_PLLD     (6u << 12) /* BCM2711 PLLD peripheral clock: 750 MHz / 6 */
#define PI4_PWM_CLOCK_HZ    125000000ull

#define PI4_PWM_CTL_PWEN1   (1u << 0)
#define PI4_PWM_CTL_MSEN1   (1u << 7)
#define PI4_PWM_CTL_PWEN2   (1u << 8)
#define PI4_PWM_CTL_MSEN2   (1u << 15)
#define PI4_PWM_STA_BERR    (1u << 8)
#define PI4_PWM_STA_ERRORS  (PI4_PWM_STA_BERR | (0xfu << 2)) /* W1C */
#define PI4_PWM_STA_ACTIVE  ((1u << 9) | (1u << 10))

static volatile uint32_t *g_pi4_audio_gpio;
static volatile uint32_t *g_pi4_audio_pwm;
static volatile uint32_t *g_pi4_audio_cm;
static uint64_t g_pi4_audio_cntfrq;
static uint64_t g_pi4_audio_beep_deadline;
static int g_pi4_audio_ready;
static uint32_t g_pi4_audio_beep_calls;
static uint32_t g_pi4_audio_tone_calls;
static int8_t g_pi4_audio_last_note;
static uint32_t g_pi4_audio_last_hz;
static int8_t g_pi4_audio_last_beep_note;
static uint32_t g_pi4_audio_last_beep_hz;
static uint32_t g_pi4_audio_init_stage;
static uint32_t g_pi4_audio_last_pwm_sta;
static uint32_t g_pi4_audio_active_ctl;
static uint32_t g_pi4_audio_active_dat1;
static uint32_t g_pi4_audio_active_dat2;
static uint32_t g_pi4_audio_clear_sta;
static uint32_t g_pi4_audio_enable_sta;
static uint32_t g_pi4_audio_end_sta;

static inline void pi4_audio_dsb(void) {
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline uint64_t pi4_audio_counter(void) {
    uint64_t count;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(count));
    return count;
}

static void pi4_audio_write(volatile uint32_t *regs, unsigned offset, uint32_t value) {
    regs[offset / 4u] = value;
    pi4_audio_dsb();
    /* PWM's APB synchronizer needs time between consecutive writes. */
    uint64_t start = pi4_audio_counter();
    uint64_t ticks = (g_pi4_audio_cntfrq + 99999u) / 100000u;
    while (pi4_audio_counter() - start < ticks)
        __asm__ volatile("yield");
}

static uint32_t pi4_audio_note_hz(int8_t ona) {
    /* Match Kernel/KMisc.ZC: ona=60 is concert A, 440 Hz. */
    int delta = (int)ona - 60;
    uint32_t hz = 440;
    if (delta > 0)
        while (delta--) hz = (uint32_t)(((uint64_t)hz * 1059463u + 500000u) / 1000000u);
    else
        while (delta++) hz = (uint32_t)(((uint64_t)hz * 943874u + 500000u) / 1000000u);
    return hz;
}

static int pi4_audio_init(uint64_t hhdm) {
    uint64_t frequency;
    uint32_t fsel;

    g_pi4_audio_init_stage = 1; /* HHDM/MMIO mapping */
    g_pi4_audio_ready = 0;
    g_pi4_audio_cntfrq = 0;
    g_pi4_audio_gpio = NULL;
    g_pi4_audio_pwm = NULL;
    g_pi4_audio_cm = NULL;
    if (!hhdm || mmio_map_2m(hhdm, PI4_GPIO_PHYS) != 0 ||
        mmio_map_2m(hhdm, PI4_CM_PHYS) != 0)
        return 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    if (!frequency) { g_pi4_audio_init_stage = 8; return 0; }
    g_pi4_audio_cntfrq = frequency;

    g_pi4_audio_gpio = (volatile uint32_t *)(uintptr_t)(hhdm + PI4_GPIO_PHYS);
    g_pi4_audio_pwm = (volatile uint32_t *)(uintptr_t)(hhdm + PI4_PWM1_PHYS);
    g_pi4_audio_cm = (volatile uint32_t *)(uintptr_t)(hhdm + PI4_CM_PHYS);

    /* GPIO40 and GPIO41 use ALT0 for PWM1 channel 0 and channel 1. */
    g_pi4_audio_init_stage = 2; /* GPIO40/41 mux */
    fsel = g_pi4_audio_gpio[PI4_GPIO_FSEL4 / 4u];
    fsel &= ~((7u << 0) | (7u << 3));
    fsel |= (4u << 0) | (4u << 3);
    pi4_audio_write(g_pi4_audio_gpio, PI4_GPIO_FSEL4, fsel);

    /* Stop PWM before changing its clock; wait boundedly for BUSY to clear. */
    g_pi4_audio_init_stage = 3; /* Stop PWM and kill clock */
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_CTL, 0);
    pi4_audio_write(g_pi4_audio_cm, PI4_CM_PWMCTL, PI4_CM_PASSWORD | PI4_CM_CTL_KILL);
    for (unsigned spin = 0; spin < 1000000u; spin++) {
        if (!(g_pi4_audio_cm[PI4_CM_PWMCTL / 4u] & PI4_CM_CTL_BUSY)) break;
        if (spin == 999999u) { g_pi4_audio_init_stage = 4; return 0; }
    }
    g_pi4_audio_init_stage = 5; /* Configure and start PWM clock */
    pi4_audio_write(g_pi4_audio_cm, PI4_CM_PWMDIV, PI4_CM_PASSWORD | PI4_CM_DIV_PLLD);
    pi4_audio_write(g_pi4_audio_cm, PI4_CM_PWMCTL, PI4_CM_PASSWORD | PI4_CM_CTL_SRC_PLLD);
    pi4_audio_write(g_pi4_audio_cm, PI4_CM_PWMCTL,
                    PI4_CM_PASSWORD | PI4_CM_CTL_SRC_PLLD | PI4_CM_CTL_ENAB);
    for (unsigned spin = 0; spin < 1000000u; spin++) {
        if (g_pi4_audio_cm[PI4_CM_PWMCTL / 4u] & PI4_CM_CTL_BUSY) break;
        if (spin == 999999u) { g_pi4_audio_init_stage = 6; return 0; }
    }

    g_pi4_audio_init_stage = 7; /* PWM ranges */
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_RNG1, 2);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_DAT1, 0);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_RNG2, 2);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_DAT2, 0);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_CTL, PI4_PWM_CTL_MSEN1 | PI4_PWM_CTL_MSEN2);
    g_pi4_audio_beep_deadline = 0;
    g_pi4_audio_ready = 1;
    g_pi4_audio_init_stage = 9;
    return 1;
}

static void pi4_audio_set_tone(int8_t ona) {
    const uint32_t mode = PI4_PWM_CTL_MSEN1 | PI4_PWM_CTL_MSEN2;
    uint32_t hz, range, duty;
    uint64_t numerator;
    if (!g_pi4_audio_ready || !g_pi4_audio_pwm) return;
    hz = ona ? pi4_audio_note_hz(ona) : 0;
    if (!hz && g_pi4_audio_last_hz)
        g_pi4_audio_end_sta = g_pi4_audio_pwm[PI4_PWM_STA / 4u];
    g_pi4_audio_tone_calls++;
    g_pi4_audio_last_note = ona;
    g_pi4_audio_last_hz = hz;
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_CTL, mode);
    if (!hz) {
        pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_DAT1, 0);
        pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_DAT2, 0);
        return;
    }
    numerator = PI4_PWM_CLOCK_HZ + hz / 2u;
    range = (uint32_t)(numerator / hz);
    if (range < 2u) range = 2u;
    if (range > 0x00ffffffu) range = 0x00ffffffu;
    duty = range / 2u;
    g_pi4_audio_end_sta = 0;
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_STA, PI4_PWM_STA_ERRORS);
    g_pi4_audio_clear_sta = g_pi4_audio_pwm[PI4_PWM_STA / 4u];
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_RNG1, range);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_RNG2, range);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_DAT1, duty);
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_DAT2, duty);
    g_pi4_audio_enable_sta = g_pi4_audio_pwm[PI4_PWM_STA / 4u];
    pi4_audio_write(g_pi4_audio_pwm, PI4_PWM_CTL, mode | PI4_PWM_CTL_PWEN1 | PI4_PWM_CTL_PWEN2);

    uint64_t start = pi4_audio_counter();
    do {
        g_pi4_audio_last_pwm_sta = g_pi4_audio_pwm[PI4_PWM_STA / 4u];
        if ((g_pi4_audio_last_pwm_sta & PI4_PWM_STA_ACTIVE) == PI4_PWM_STA_ACTIVE)
            break;
    } while (pi4_audio_counter() - start < g_pi4_audio_cntfrq / 200u);
    g_pi4_audio_active_ctl = g_pi4_audio_pwm[PI4_PWM_CTL / 4u];
    g_pi4_audio_active_dat1 = g_pi4_audio_pwm[PI4_PWM_DAT1 / 4u];
    g_pi4_audio_active_dat2 = g_pi4_audio_pwm[PI4_PWM_DAT2 / 4u];
}

static void pi4_audio_beep(int8_t ona) {
    g_pi4_audio_beep_calls++;
    g_pi4_audio_last_beep_note = ona;
    g_pi4_audio_last_beep_hz = ona ? pi4_audio_note_hz(ona) : 0;
    pi4_audio_set_tone(ona);
    g_pi4_audio_beep_deadline = pi4_audio_counter() + g_pi4_audio_cntfrq / 2u;
}

/* Read-only state for the interactive `zstatus` hardware diagnostic. */
static uint64_t pi4_audio_debug(unsigned index) {
    switch (index) {
    case 0: return (uint64_t)g_pi4_audio_ready;
    case 1: return g_pi4_audio_beep_calls;
    case 2: return g_pi4_audio_tone_calls;
    case 3: return (uint64_t)(uint8_t)g_pi4_audio_last_beep_note;
    case 4: return g_pi4_audio_last_beep_hz;
    case 5: return g_pi4_audio_gpio ? g_pi4_audio_gpio[PI4_GPIO_FSEL4 / 4u] : 0;
    case 6: return g_pi4_audio_cm ? g_pi4_audio_cm[PI4_CM_PWMCTL / 4u] : 0;
    case 7: return g_pi4_audio_cm ? g_pi4_audio_cm[PI4_CM_PWMDIV / 4u] : 0;
    case 8: return g_pi4_audio_pwm ? g_pi4_audio_pwm[PI4_PWM_CTL / 4u] : 0;
    case 9: return g_pi4_audio_pwm ? g_pi4_audio_pwm[PI4_PWM_RNG1 / 4u] : 0;
    case 10: return g_pi4_audio_pwm ? g_pi4_audio_pwm[PI4_PWM_DAT1 / 4u] : 0;
    case 11: return g_pi4_audio_pwm ? g_pi4_audio_pwm[PI4_PWM_RNG2 / 4u] : 0;
    case 12: return g_pi4_audio_pwm ? g_pi4_audio_pwm[PI4_PWM_DAT2 / 4u] : 0;
    case 13: return g_pi4_audio_init_stage;
    case 14: return g_pi4_audio_last_pwm_sta;
    case 15: return g_pi4_audio_active_ctl;
    case 16: return g_pi4_audio_active_dat1;
    case 17: return g_pi4_audio_active_dat2;
    case 18: return g_pi4_audio_clear_sta;
    case 19: return g_pi4_audio_enable_sta;
    case 20: return g_pi4_audio_end_sta;
    default: return 0;
    }
}

static int pi4_audio_beep_done(void) {
    return !g_pi4_audio_ready || !g_pi4_audio_beep_deadline ||
           (int64_t)(pi4_audio_counter() - g_pi4_audio_beep_deadline) >= 0;
}

static void pi4_audio_pump(void) { /* PWM is clocked autonomously by BCM2711. */ }
