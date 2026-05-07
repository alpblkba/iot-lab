# Task 1

Task 1 covers the initial STM32U5 IoT lab setup and first board bring-up.

The goal is to verify that the development environment works on Apple Silicon macOS and then implement the first basic embedded programs on the B-U585I-IOT02A board.

## Board

- Board: B-U585I-IOT02A
- MCU family: STM32U575 / STM32U585
- CPU: Arm Cortex-M33
- Debug interface: onboard ST-LINK V3 over SWD
- UART interface: ST-LINK virtual COM port

## Directory Structure

```text
task1/
├── gpio_example/
├── blink_register/
├── blink_hal/
└── uart_test/
```

## Subtasks

### gpio_example

Generated GPIO toggle example from STM32CubeMX.

This is the first sanity test. If this builds, flashes, and blinks an LED, the toolchain and board connection are working.

### blink_register

LED blink using direct register access.

This task uses the STM32U5 reference manual and board schematic directly. The purpose is to understand RCC, GPIO peripheral clocks, GPIO mode configuration, and output control registers.

### blink_hal

LED blink using STM32 HAL.

This task uses CubeMX-generated initialization code and HAL functions such as `HAL_GPIO_TogglePin()` and `HAL_Delay()`.

### uart_test

UART transmission through the ST-LINK virtual COM port.

This task verifies serial communication between the STM32 board and the Mac.

## Useful Commands

Check detected ST-LINK probes:

```bash
STM32_Programmer_CLI -l
```

Connect to the board through SWD:

```bash
STM32_Programmer_CLI -c port=SWD
```

Build a generated CMake project:

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Flash firmware:

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/<firmware>.elf -s
```

Open UART terminal on macOS:

```bash
screen /dev/cu.usbmodem21203 115200
```

Exit `screen`:

```text
Ctrl + A
Ctrl + \\
```

## Current Verified Setup

The following setup has been verified on Apple Silicon macOS:

- STM32CubeMX
- STM32CubeCLT
- STM32CubeProgrammer
- Visual Studio Code
- CMake
- Ninja
- ST-LINK connection over SWD
- B-U585I-IOT02A board detection
