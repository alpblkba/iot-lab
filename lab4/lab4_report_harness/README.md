# Lab 4 Benchmark Report Harness

Place these two files in the `lab4/` root directory, next to `task1/` and `task2/`:

- `run_lab4_report.sh`
- `generate_lab4_markdown.py`

Then run:

```bash
cd ~/Documents/GitHub/iot-lab/lab4
./run_lab4_report.sh
```

The harness will:

1. Build Task 1.
2. Flash Task 1.
3. Reset the board while `screen` is already logging UART.
4. Capture the Task 1 UART log.
5. Build Task 2 in DFT mode.
6. Flash Task 2 DFT firmware.
7. Capture at least 5 DFT measurement entries.
8. Build Task 2 in FFT mode.
9. Flash Task 2 FFT firmware.
10. Capture at least 20 FFT measurement entries.
11. Generate a Markdown report.

Reports are written to:

```text
lab4/reports/lab4_full/<timestamp>/lab4_benchmark_report.md
```

## Optional settings

```bash
PORT=/dev/cu.usbmodem21403 CONFIG=Release ./run_lab4_report.sh
```

Capture durations can be extended if needed:

```bash
TASK1_SECONDS=10 DFT_SECONDS=25 FFT_SECONDS=10 ./run_lab4_report.sh
```

## Requirements

- `screen`
- `cmake`
- STM32CubeProgrammer CLI
- Python 3 for Markdown generation

The script assumes Task 2 supports this CMake option:

```cmake
-DLAB4_TASK2_MODE=DFT
-DLAB4_TASK2_MODE=FFT
```
