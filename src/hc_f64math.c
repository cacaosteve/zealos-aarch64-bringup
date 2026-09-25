/* Freestanding F64 trig for HolyC Cos()/Sin() — compiled WITH FP (not +nofp).
 * No libm: range-reduce + Taylor. Bits in/out as uint64_t (IEEE754).
 *
 * Accepted domain: finite |x| < 2^20 (~1e6). Outside → quiet NaN.
 * Accurate enough for Lattice-scale angles after fold to [-pi/2, pi/2].
 */
#include <stdint.h>

#define HC_PI 3.14159265358979323846
#define HC_TWO_PI (2.0 * HC_PI)
#define HC_HALF_PI (0.5 * HC_PI)
#define HC_QNAN_BITS 0x7ff8000000000000ULL

static double hc_bits_to_d(uint64_t b) {
    union {
        uint64_t u;
        double d;
    } v;
    v.u = b;
    return v.d;
}

static uint64_t hc_d_to_bits(double d) {
    union {
        uint64_t u;
        double d;
    } v;
    v.d = d;
    return v.u;
}

static int hc_exp_bits(double x) {
    union {
        uint64_t u;
        double d;
    } v;
    v.d = x;
    return (int)((v.u >> 52) & 0x7ff);
}

/* Reduce to [-pi, pi]. Non-finite or |x| >= 2^20 → quiet NaN. */
static double hc_reduce_pi(double x) {
    int exp = hc_exp_bits(x);
    if (exp == 0x7ff) {
        return hc_bits_to_d(HC_QNAN_BITS);
    }
    /* |x| >= 2^20: long-long cast of x/(2π) is unsafe; Lattice stays far below. */
    if (exp >= 1023 + 20) {
        return hc_bits_to_d(HC_QNAN_BITS);
    }
    if (x > HC_PI || x < -HC_PI) {
        double n = x / HC_TWO_PI;
        long long k = (long long)n;
        x = x - (double)k * HC_TWO_PI;
        if (x > HC_PI) {
            x -= HC_TWO_PI;
        } else if (x < -HC_PI) {
            x += HC_TWO_PI;
        }
    }
    return x;
}

static double hc_cos_taylor(double x) {
    /* cos x = 1 - x^2/2! + …  (x in [-pi/2, pi/2]) */
    double x2 = x * x;
    double t = 1.0;
    double s = 1.0;
    int i;
    for (i = 1; i <= 12; i++) {
        t = t * (-x2) / (double)((2 * i - 1) * (2 * i));
        s += t;
    }
    return s;
}

static double hc_sin_taylor(double x) {
    double x2 = x * x;
    double t = x;
    double s = x;
    int i;
    for (i = 1; i <= 12; i++) {
        t = t * (-x2) / (double)((2 * i) * (2 * i + 1));
        s += t;
    }
    return s;
}

uint64_t hc_builtin_cos(uint64_t xbits) {
    double x = hc_reduce_pi(hc_bits_to_d(xbits));
    if (hc_exp_bits(x) == 0x7ff) {
        return HC_QNAN_BITS;
    }
    /* Fold to [-pi/2, pi/2]: cos(x) = -cos(pi - x) for x > pi/2;
     * cos(x) = -cos(-pi - x) for x < -pi/2 (even around ±pi). */
    if (x > HC_HALF_PI) {
        return hc_d_to_bits(-hc_cos_taylor(HC_PI - x));
    }
    if (x < -HC_HALF_PI) {
        return hc_d_to_bits(-hc_cos_taylor(-HC_PI - x));
    }
    return hc_d_to_bits(hc_cos_taylor(x));
}

uint64_t hc_builtin_sin(uint64_t xbits) {
    double x = hc_reduce_pi(hc_bits_to_d(xbits));
    if (hc_exp_bits(x) == 0x7ff) {
        return HC_QNAN_BITS;
    }
    /* Fold: sin(x) = sin(pi - x) for x > pi/2;
     * sin(x) = sin(-pi - x) for x < -pi/2 (odd; no extra minus). */
    if (x > HC_HALF_PI) {
        return hc_d_to_bits(hc_sin_taylor(HC_PI - x));
    }
    if (x < -HC_HALF_PI) {
        return hc_d_to_bits(hc_sin_taylor(-HC_PI - x));
    }
    return hc_d_to_bits(hc_sin_taylor(x));
}

/* aarch64 FSQRT (this TU is built with FP). Neg/NaN → qNaN. No libm. */
uint64_t hc_builtin_sqrt(uint64_t xbits) {
    double x = hc_bits_to_d(xbits);
    double y;
    int exp = hc_exp_bits(x);
    if (exp == 0x7ff) {
        return HC_QNAN_BITS;
    }
    if (x < 0.0) {
        return HC_QNAN_BITS;
    }
    __asm__ volatile("fsqrt %d0, %d1" : "=w"(y) : "w"(x));
    return hc_d_to_bits(y);
}

/* C-like fmod: x - trunc(x/y)*y (toward zero). */
static double hc_fmod(double x, double y) {
    double n;
    long long k;
    if (y == 0.0 || hc_exp_bits(x) == 0x7ff || hc_exp_bits(y) == 0x7ff) {
        return hc_bits_to_d(HC_QNAN_BITS);
    }
    n = x / y;
    if (hc_exp_bits(n) >= 1023 + 20) {
        return hc_bits_to_d(HC_QNAN_BITS);
    }
    k = (long long)n;
    return x - (double)k * y;
}

/* ZealOS Wrap(θ, base=-π): angle in [base, base+2π). */
uint64_t hc_builtin_wrap2(uint64_t th_bits, uint64_t base_bits) {
    double th = hc_bits_to_d(th_bits);
    double base = hc_bits_to_d(base_bits);
    double res;
    if (hc_exp_bits(th) == 0x7ff || hc_exp_bits(base) == 0x7ff ||
        hc_exp_bits(th) >= 1023 + 20) {
        return HC_QNAN_BITS;
    }
    res = hc_fmod(th, HC_TWO_PI);
    if (res >= base + HC_TWO_PI) {
        res -= HC_TWO_PI;
    } else if (res < base) {
        res += HC_TWO_PI;
    }
    return hc_d_to_bits(res);
}

uint64_t hc_builtin_wrap1(uint64_t th_bits) {
    return hc_builtin_wrap2(th_bits, hc_d_to_bits(-HC_PI));
}

/* atan for |z| <= 1 (odd Taylor). */
static double hc_atan_unit(double z) {
    double z2 = z * z;
    double t = z;
    double s = z;
    int i;
    for (i = 1; i <= 12; i++) {
        t *= -z2;
        s += t / (double)(2 * i + 1);
    }
    return s;
}

static double hc_atan(double z) {
    if (z > 1.0) {
        return HC_HALF_PI - hc_atan_unit(1.0 / z);
    }
    if (z < -1.0) {
        return -HC_HALF_PI - hc_atan_unit(1.0 / z);
    }
    return hc_atan_unit(z);
}

/* ZealOS Arg(x, y) = atan2(y, x), range (-π, π]. */
uint64_t hc_builtin_arg(uint64_t x_bits, uint64_t y_bits) {
    double x = hc_bits_to_d(x_bits);
    double y = hc_bits_to_d(y_bits);
    double a;
    if (hc_exp_bits(x) == 0x7ff || hc_exp_bits(y) == 0x7ff) {
        return HC_QNAN_BITS;
    }
    if (x > 0.0) {
        a = hc_atan(y / x);
    } else if (x < 0.0) {
        a = hc_atan(y / x);
        if (y >= 0.0) {
            a += HC_PI;
        } else {
            a -= HC_PI;
        }
    } else if (y > 0.0) {
        a = HC_HALF_PI;
    } else if (y < 0.0) {
        a = -HC_HALF_PI;
    } else {
        a = 0.0;
    }
    return hc_d_to_bits(a);
}

/* Format IEEE F64 bits into *dp (no libm snprintf). prec = digits after '.'. */
void hc_fmt_f64_bits(char **dp, uint64_t bits, int prec) {
    double v = hc_bits_to_d(bits);
    char *d = *dp;
    int neg = 0;
    long long ip;
    long long frac = 0;
    long long scale = 1;
    int i;
    if (prec < 0) {
        prec = 0;
    }
    if (prec > 6) {
        prec = 6;
    }
    if (v != v) { /* NaN */
        *d++ = 'n';
        *d++ = 'a';
        *d++ = 'n';
        *dp = d;
        return;
    }
    if (v < 0.0) {
        neg = 1;
        v = -v;
    }
    for (i = 0; i < prec; i++) {
        scale *= 10;
    }
    /* Round half up at prec. */
    v = v + 0.5 / (double)scale;
    ip = (long long)v;
    if (prec > 0) {
        frac = (long long)((v - (double)ip) * (double)scale);
        if (frac >= scale) {
            frac = 0;
            ip++;
        }
    }
    if (neg) {
        *d++ = '-';
    }
    {
        char tmp[24];
        int n = 0;
        unsigned long long u = (unsigned long long)ip;
        if (u == 0) {
            tmp[n++] = '0';
        } else {
            while (u && n < 20) {
                tmp[n++] = (char)('0' + (u % 10));
                u /= 10;
            }
        }
        while (n--) {
            *d++ = tmp[n];
        }
    }
    if (prec > 0) {
        *d++ = '.';
        {
            char tmp[8];
            int n = prec;
            long long f = frac;
            while (n--) {
                tmp[n] = (char)('0' + (f % 10));
                f /= 10;
            }
            for (i = 0; i < prec; i++) {
                *d++ = tmp[i];
            }
        }
    }
    *dp = d;
}
