# IoT Lab 04 Benchmark Report

Generated: 2026-07-02 16:06:52

## Run metadata

| Field | Value |
|---|---|
| Run ID | 20260702_160612 |
| CMake configuration | Release |
| UART port | /dev/cu.usbmodem21403 |
| Baud rate | 115200 |
| Lab 4 root | /Users/alpblkba/Documents/GitHub/iot-lab/lab4 |

## Task 1 — Kernel microbenchmarks

Task 1 benchmarks the scalar and optimized implementations of the AXPY, convolution, and SAD kernels. The firmware reports DWT cycle-counter measurements over UART. Correctness is checked against golden reference outputs before interpreting the cycle counts.

| Kernel | Variant | Status | Cycles | Speedup | Value |
|---|---|---|---|---|---|
| AXPY | base | PASS | 41,460 | baseline |  |
| AXPY | unrolled | PASS | 28,962 | 1.43 |  |
| CONV | base | PASS | 1,222,017 | baseline |  |
| CONV | SIMD | PASS | 766,740 | 1.59 |  |
| SAD | base bonus | PASS | 45,781 | baseline | 348396 |
| SAD | SIMD bonus | PASS | 11,516 | 3.97 | 348396 |

### Task 1 observations

- AXPY unrolled measured a speedup of **1.43x** over the corresponding baseline.
- CONV SIMD measured a speedup of **1.59x** over the corresponding baseline.
- SAD SIMD bonus measured a speedup of **3.97x** over the corresponding baseline.
- The AXPY optimization mainly reduces loop overhead through unrolling.
- The convolution optimization uses packed 16-bit multiply-accumulate style operations, reducing the cost of the inner loop.
- The SAD optimization uses SIMD-style byte absolute-difference accumulation, which is well matched to the `uint8_t` data layout.

## Task 2 — Audio spectral benchmark

The audio benchmark processes 512-sample blocks from the microphone stream at 16 kHz. This gives a block deadline of `512 / 16000 = 32 ms`. At a 160 MHz CPU clock, the corresponding budget is `160e6 × 0.032 = 5,120,000 cycles`.

| Mode | N | Avg cycles | Min cycles | Max cycles | Avg time ms | Avg load % | Overrun first→last | Max errors | Avg peak Hz |
|---|---|---|---|---|---|---|---|---|---|
| DFT | 5 | 343,496,629 | 343,496,469 | 343,497,120 | 2146.853 | 6708.91 | 68→337 | 0 | 31 |
| FFT | 20 | 46,018 | 45,984 | 46,379 | 0.287 | 0.89 | 0→0 | 0 | 31 |

Measured average speedup of CMSIS-DSP FFT over naive DFT: **7464.4x**.

### DFT raw entries

| Block | Half | Cycles | Time ms | Load % | Overrun | Errors | Peak Hz |
|---|---|---|---|---|---|---|---|
| 0 | 1 | 343,497,120 | 2146.857 | 6708.92 | 68 | 0 | 31 |
| 1 | 1 | 343,496,469 | 2146.852 | 6708.91 | 135 | 0 | 31 |
| 2 | 1 | 343,496,517 | 2146.853 | 6708.91 | 202 | 0 | 31 |
| 3 | 0 | 343,496,563 | 2146.853 | 6708.91 | 270 | 0 | 31 |
| 4 | 0 | 343,496,478 | 2146.852 | 6708.91 | 337 | 0 | 31 |

### FFT raw entries

| Block | Half | Cycles | Time ms | Load % | Overrun | Errors | Peak Hz |
|---|---|---|---|---|---|---|---|
| 0 | 1 | 46,379 | 0.289 | 0.90 | 0 | 0 | 31 |
| 1 | 0 | 45,984 | 0.287 | 0.89 | 0 | 0 | 31 |
| 2 | 1 | 46,029 | 0.287 | 0.89 | 0 | 0 | 31 |
| 3 | 0 | 45,984 | 0.287 | 0.89 | 0 | 0 | 31 |
| 4 | 1 | 46,026 | 0.287 | 0.89 | 0 | 0 | 31 |
| 5 | 0 | 45,984 | 0.287 | 0.89 | 0 | 0 | 31 |
| 6 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 7 | 0 | 45,984 | 0.287 | 0.89 | 0 | 0 | 31 |
| 8 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 9 | 0 | 45,984 | 0.287 | 0.89 | 0 | 0 | 31 |
| 10 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 11 | 0 | 46,036 | 0.287 | 0.89 | 0 | 0 | 31 |
| 12 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 13 | 0 | 46,026 | 0.287 | 0.89 | 0 | 0 | 31 |
| 14 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 15 | 0 | 46,027 | 0.287 | 0.89 | 0 | 0 | 31 |
| 16 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 17 | 0 | 46,022 | 0.287 | 0.89 | 0 | 0 | 31 |
| 18 | 1 | 45,988 | 0.287 | 0.89 | 0 | 0 | 31 |
| 19 | 0 | 45,984 | 0.287 | 0.89 | 0 | 0 | 31 |

### Task 2 observations

- The naive DFT is far above the real-time budget: average `343,496,629` cycles versus `5,120,000` cycles available per block.
- The DFT overrun counter increases during the capture, confirming that audio blocks arrive faster than they are processed.
- The CMSIS-DSP FFT is comfortably inside the real-time budget: average `46,018` cycles.
- The FFT overrun counter remained stable during the captured entries.
- The FFT result demonstrates why an algorithmic change from O(N²) DFT to O(N log N) FFT is more important than small loop-level optimizations for this workload.

## Captured files

| File | Size |
|---|---|
| metadata.env | 314 bytes |
| task1_build.log | 6327 bytes |
| task1_flash.log | 7682 bytes |
| task1_uart.log | 336 bytes |
| task2_dft_build.log | 8417 bytes |
| task2_dft_flash.log | 9916 bytes |
| task2_dft_uart.log | 957 bytes |
| task2_fft_build.log | 8417 bytes |
| task2_fft_flash.log | 9916 bytes |
| task2_fft_uart.log | 18172 bytes |

