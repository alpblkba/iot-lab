#include "iot_kernel.h"

#include <stddef.h>
#include <stdint.h>

/* ========================================================================== */
/*  Baselines (provided -- do not change)                                     */
/* ========================================================================== */

void axpy_u8(size_t n, uint8_t a, const uint8_t *x, uint8_t *y)
{
    for (size_t i = 0; i < n; ++i) {
        y[i] = (uint8_t)(a * x[i] + y[i]);
    }
}

void conv1d_i16_valid(const int16_t *x,
                      size_t x_len,
                      const int16_t *h,
                      size_t k_len,
                      int32_t *y)
{
    size_t out_len = x_len - k_len + 1;

    for (size_t t = 0; t < out_len; ++t) {
        int32_t acc = 0;

        for (size_t k = 0; k < k_len; ++k) {
            acc += (int32_t)x[t + k] * (int32_t)h[k];
        }

        y[t] = acc;
    }
}

uint32_t sad_u8(size_t n, const uint8_t *a, const uint8_t *b)
{
    uint32_t acc = 0;
    for (size_t i = 0; i < n; ++i) {
        int d = (int)a[i] - (int)b[i];
        acc += (uint32_t)(d < 0 ? -d : d);
    }
    return acc;
}

/* ========================================================================== */
/*  Your work starts here.                                                    */
/*  The stubs below just call the baseline so the project builds and the      */
/*  correctness check passes before you start. Replace the bodies.            */
/* ========================================================================== */

void axpy_u8_unrolled(size_t n, uint8_t a, const uint8_t *x, uint8_t *y)
{
    /* TODO (Task 1.3): manual loop unrolling, factor 4, handle the remainder. */
    axpy_u8(n, a, x, y);
}

void conv1d_i16_valid_opt(const int16_t *x,
                          size_t x_len,
                          const int16_t *h,
                          size_t k_len,
                          int32_t *y)
{
    /* TODO (Task 1.4): SIMD with __SMLAD. Mind 32-bit load alignment and the
     * scalar tail. Output must match conv1d_i16_valid. */
    conv1d_i16_valid(x, x_len, h, k_len, y);
}

void conv1d_i16_valid_opt_unrolled(const int16_t *x,
                                   size_t x_len,
                                   const int16_t *h,
                                   size_t k_len,
                                   int32_t *y)
{
    /* TODO (Bonus): deeper unrolling of the SIMD convolution. */
    conv1d_i16_valid(x, x_len, h, k_len, y);
}

uint32_t sad_u8_opt(size_t n, const uint8_t *a, const uint8_t *b)
{
    /* TODO (Bonus): use the __USADA8 SIMD intrinsic (4 byte lanes / instr).
     * Mind 32-bit load alignment and the scalar tail. Must equal sad_u8. */
    return sad_u8(n, a, b);
}
