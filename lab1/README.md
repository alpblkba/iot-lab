# IoT Lab 1

This lab introduces basic GPIO and serial communication workflows on the STM32 B-U585I-IOT02A board, moving from direct register-level control toward HAL-based embedded software development.

## Task 1: Bare-metal LED blink

This task implements a simple LED blink without relying on the HAL GPIO abstraction.

The goal is to understand how GPIO peripherals are controlled closer to the hardware level. The LED pin is configured and toggled through low-level register-oriented logic, making this task useful for learning what the HAL later hides from the programmer.

## Task 2: HAL LED blink

This task reimplements the LED blink using STM32 HAL.

The GPIO pin is initialized through CubeMX-generated setup code, and the LED is toggled inside the main loop with HAL GPIO functions. Compared to Task 1, this version is cleaner and more portable, but also abstracts away most of the direct register manipulation.

## Task 3: UART serial output

This task extends the project with UART-based serial communication.

The board is configured to use USART pins for transmitting serial output from the microcontroller. The goal is to move beyond visual LED debugging and enable text-based runtime feedback, which is much more useful for debugging embedded applications.
