#include "iot_kernel.h"

#include <stddef.h>
#include <stdint.h>

#include "cmsis_gcc.h"
#include <string.h>

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
    size_t i = 0;

    for (; i + 7 < n; i += 8) {
        y[i + 0] = (uint8_t)(a * x[i + 0] + y[i + 0]);
        y[i + 1] = (uint8_t)(a * x[i + 1] + y[i + 1]);
        y[i + 2] = (uint8_t)(a * x[i + 2] + y[i + 2]);
        y[i + 3] = (uint8_t)(a * x[i + 3] + y[i + 3]);
        y[i + 4] = (uint8_t)(a * x[i + 4] + y[i + 4]);
        y[i + 5] = (uint8_t)(a * x[i + 5] + y[i + 5]);
        y[i + 6] = (uint8_t)(a * x[i + 6] + y[i + 6]);
        y[i + 7] = (uint8_t)(a * x[i + 7] + y[i + 7]);
    }

    for (; i < n; ++i) {
        y[i] = (uint8_t)(a * x[i] + y[i]);
    }
}

void conv1d_i16_valid_opt(const int16_t *x,
                          size_t x_len,
                          const int16_t *h,
                          size_t k_len,
                          int32_t *y)
{
    const size_t out_len = x_len - k_len + 1u;

    for (size_t t = 0; t < out_len; ++t) {
        int32_t acc = 0;
        size_t k = 0;

        for (; k + 1 < k_len; k += 2) {
            uint32_t x_pack = __UNALIGNED_UINT32_READ(&x[t + k]);
            uint32_t h_pack = __UNALIGNED_UINT32_READ(&h[k]);

            acc = __SMLAD(x_pack, h_pack, acc);
        }

        for (; k < k_len; ++k) {
            acc += (int32_t)x[t + k] * (int32_t)h[k];
        }

        y[t] = acc;
    }
}

void conv1d_i16_valid_opt_unrolled(const int16_t *x,
                                   size_t x_len,
                                   const int16_t *h,
                                   size_t k_len,
                                   int32_t *y)
{
    const size_t out_len = x_len - k_len + 1u;

    for (size_t t = 0; t < out_len; ++t) {
        int32_t acc = 0;
        size_t k = 0;

        for (; k + 7 < k_len; k += 8) {
            uint32_t x0 = __UNALIGNED_UINT32_READ(&x[t + k + 0]);
            uint32_t x1 = __UNALIGNED_UINT32_READ(&x[t + k + 2]);
            uint32_t x2 = __UNALIGNED_UINT32_READ(&x[t + k + 4]);
            uint32_t x3 = __UNALIGNED_UINT32_READ(&x[t + k + 6]);

            uint32_t h0 = __UNALIGNED_UINT32_READ(&h[k + 0]);
            uint32_t h1 = __UNALIGNED_UINT32_READ(&h[k + 2]);
            uint32_t h2 = __UNALIGNED_UINT32_READ(&h[k + 4]);
            uint32_t h3 = __UNALIGNED_UINT32_READ(&h[k + 6]);

            acc = __SMLAD(x0, h0, acc);
            acc = __SMLAD(x1, h1, acc);
            acc = __SMLAD(x2, h2, acc);
            acc = __SMLAD(x3, h3, acc);
        }

        for (; k + 1 < k_len; k += 2) {
            uint32_t x_pack = __UNALIGNED_UINT32_READ(&x[t + k]);
            uint32_t h_pack = __UNALIGNED_UINT32_READ(&h[k]);

            acc = __SMLAD(x_pack, h_pack, acc);
        }

        for (; k < k_len; ++k) {
            acc += (int32_t)x[t + k] * (int32_t)h[k];
        }

        y[t] = acc;
    }
}

uint32_t sad_u8_opt(size_t n, const uint8_t *a, const uint8_t *b)
{
    uint32_t acc = 0;
    size_t i = 0;

    for (; i + 3 < n; i += 4) {
        uint32_t a_pack = __UNALIGNED_UINT32_READ(&a[i]);
        uint32_t b_pack = __UNALIGNED_UINT32_READ(&b[i]);

        acc = __USADA8(a_pack, b_pack, acc);
    }

    for (; i < n; ++i) {
        int d = (int)a[i] - (int)b[i];
        acc += (uint32_t)(d < 0 ? -d : d);
    }

    return acc;
}