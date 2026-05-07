# IoT Lab

This repository contains the tasks for the IoT lab using the STM32U5 IoT Discovery Kit B-U585I-IOT02A.

## Structure

- `task1/`: installation, GPIO, LED blink, UART, and first board bring-up
- `task2/`: second lab task
- `task3/`: third lab task
- `IOTLIB/`: shared helper code used across tasks
- `external/`: external libraries or third-party code
- `doc/`: notes, documentation, and lab references

## Target Board

- Board: B-U585I-IOT02A
- MCU family: STM32U575 / STM32U585
- CPU: Arm Cortex-M33
- Debug interface: onboard ST-LINK V3 over SWD
- UART interface: ST-LINK virtual COM port

## Local Toolchain

The lab is developed on Apple Silicon macOS using:

- STM32CubeMX
- STM32CubeCLT
- STM32CubeProgrammer
- Visual Studio Code
- CMake
- Ninja
