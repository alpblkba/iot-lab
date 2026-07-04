#ifndef IOT_KERNEL_H
#define IOT_KERNEL_H

#include <stddef.h>
#include <stdint.h>

/*
 * Baseline: y[i] = (uint8_t)(a * x[i] + y[i]) for i = 0 .. n-1.
 * Note: uint8_t arithmetic is modular (wraps mod 256).
 */
void axpy_u8(size_t n, uint8_t a, const uint8_t *x, uint8_t *y);

/*
 * Task 1.3: same operation as axpy_u8, optimized with manual loop unrolling
 * (unroll factor 4). Must produce identical output to axpy_u8, including
 * correct handling of the trailing (remainder) elements.
 */
void axpy_u8_unrolled(size_t n, uint8_t a, const uint8_t *x, uint8_t *y);

/*
 * Baseline valid 1D convolution:
 *   y[t] = sum_{i=0}^{k_len-1} x[t+i] * h[i]   for t = 0 .. x_len-k_len.
 * Output length is x_len - k_len + 1.
 */
void conv1d_i16_valid(const int16_t *x,
                      size_t x_len,
                      const int16_t *h,
                      size_t k_len,
                      int32_t *y);

/*
 * Task 1.4: SIMD-optimized convolution using Arm DSP intrinsics (e.g. __SMLAD).
 * Must produce the same output as conv1d_i16_valid for all valid t.
 */
void conv1d_i16_valid_opt(const int16_t *x,
                          size_t x_len,
                          const int16_t *h,
                          size_t k_len,
                          int32_t *y);

/*
 * Bonus: further-unrolled SIMD convolution (process several taps per iteration).
 * Must produce the same output as conv1d_i16_valid.
 */
void conv1d_i16_valid_opt_unrolled(const int16_t *x,
                                   size_t x_len,
                                   const int16_t *h,
                                   size_t k_len,
                                   int32_t *y);

/*
 * Baseline sum of absolute differences: sum_{i=0}^{n-1} |a[i] - b[i]|.
 */
uint32_t sad_u8(size_t n, const uint8_t *a, const uint8_t *b);

/*
 * Bonus: SAD optimized with the __USADA8 SIMD intrinsic (4 bytes / instruction).
 * Must return the same value as sad_u8.
 */
uint32_t sad_u8_opt(size_t n, const uint8_t *a, const uint8_t *b);

#endif /* IOT_KERNEL_H */
