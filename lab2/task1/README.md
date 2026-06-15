# Lab 1 Task 2: HAL LED Blink

This task implements a simple LED blink on the B-U585I-IOT02A board using the STM32 HAL API.

In Task 1, the LED was controlled through direct register access. In this task, the same GPIO behavior is implemented through the HAL abstraction layer. The board-specific LED mapping is taken from the BSP definitions:

- LED6 is mapped to GPIOH / GPIO_PIN_6
- LED7 is mapped to GPIOH / GPIO_PIN_7
- LED7 is also defined as the green user LED

The GPIO clock is enabled through the BSP macros, and the LED pins are configured as output push-pull using HAL_GPIO_Init. The main loop then toggles the LED with HAL_GPIO_TogglePin and uses HAL_Delay for timing.

This demonstrates the difference between bare-metal register-level GPIO control and HAL-based GPIO control.
