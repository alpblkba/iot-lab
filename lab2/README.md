# IoT Lab 2 — interrupts and sensors

This lab explores interrupt-driven firmware design on the B-U585I-IOT02A board. The three tasks build from a non-blocking UART command interface, to BSP-based environmental sensor readout, and finally to waking the MCU from sleep using an IMU interrupt.

The main idea across the lab is to avoid designs where the CPU waits passively inside blocking calls. Instead, the firmware uses interrupt callbacks, shared event flags, BSP sensor drivers, and low-power sleep with `WFI`.

## Repository layout

```text
lab2/
├── IoT_Lab2.pdf
├── README.md
├── task1/
├── task2/
└── task3/
```

Each task is kept as a standalone STM32Cube/CMake project.

## Common build and test commands

Build a task:

```bash
cd lab2/task1
rm -rf build
cmake --preset Debug
cmake --build --preset Debug
```

Flash the board:

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/GPIO_IOToggle.elf -v -rst
```

Open UART:

```bash
PORT=$(ls /dev/cu.usbmodem* | head -n 1)
screen "$PORT" 115200
```

Exit `screen`:

```text
Ctrl-A
K
Y
```

If the serial port is busy:

```bash
lsof /dev/cu.usbmodem21403
kill -9 <pid>
```

## Task 1 — interrupt-driven UART CLI

Task 1 implements a small UART command interface using interrupt-based reception on `USART1`.

The firmware starts a one-byte receive operation with:

```c
HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
```

This does not block the CPU. When a byte arrives, the HAL UART interrupt path calls the receive-complete callback. The callback stores the received byte, marks a command-ready flag, and immediately restarts the receive operation:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    cmd = rx_byte;
    cmd_ready = 1;
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}
```

The re-arm step is important because `HAL_UART_Receive_IT()` is a one-shot receive operation. Without calling it again, only the first byte would be received.

The main loop keeps incrementing a counter while waiting for commands. This shows that the firmware is not blocked on UART input.

Supported commands:

```text
h -> print help
c -> print counter
other -> unknown command
```

Tested behavior:

```text
Lab2 Task1: interrupt UART CLI
h: print help
c: print counter

counter = 271137989
counter = 275868338
Unknown command
```

### What this task demonstrates

Task 1 demonstrates the basic embedded interrupt pattern:

```text
UART byte arrives
  -> USART interrupt
  -> HAL IRQ handler
  -> HAL receive-complete callback
  -> command flag set
  -> main loop handles command
```

The interrupt callback only records the event. The actual command handling is done in the main loop.

## Task 2 — environmental sensor readout through BSP

Task 2 reads temperature and humidity using the ST board support package.

Instead of manually writing I2C register transactions, the application uses the BSP environmental sensor API:

```c
BSP_ENV_SENSOR_Init(0, ENV_TEMPERATURE | ENV_HUMIDITY);
BSP_ENV_SENSOR_Enable(0, ENV_TEMPERATURE);
BSP_ENV_SENSOR_Enable(0, ENV_HUMIDITY);
```

The main loop reads the values once per second:

```c
BSP_ENV_SENSOR_GetValue(0, ENV_TEMPERATURE, &temperature);
BSP_ENV_SENSOR_GetValue(0, ENV_HUMIDITY, &humidity);
```

The firmware then formats the values and prints them over UART.

Tested behavior:

```text
Lab2 Task2: HTS221 temperature and humidity readout
temperature = 30.11 C, humidity = 30.87 %
temperature = 30.14 C, humidity = 30.87 %
temperature = 30.12 C, humidity = 30.80 %
```

### BSP files and integration

The implementation adds the board and component driver layers required by the BSP:

```text
Drivers/BSP/B-U585I-IOT02A/
Drivers/BSP/Components/Common/
Drivers/BSP/Components/hts221/
Drivers/BSP/Components/lps22hh/
Inc/b_u585i_iot02a_conf.h
```

The HAL I2C module is enabled in `Inc/stm32u5xx_hal_conf.h`:

```c
#define HAL_I2C_MODULE_ENABLED
```

The task-level `CMakeLists.txt` adds the required BSP and component sources. A key debugging point was avoiding duplicate compilation of HAL I2C sources. The generated CubeMX CMake layer already includes the HAL driver sources, so the task-level CMake file should only add the missing BSP and sensor component sources.

### What this task demonstrates

Task 2 demonstrates the BSP abstraction stack:

```text
application main.c
  -> BSP environmental sensor API
  -> BSP board bus API
  -> sensor component driver
  -> HAL I2C
  -> physical sensor
```

The successful UART output verifies that the sensor, I2C bus, BSP bus layer, BSP environmental sensor layer, and UART print path work together.

## Task 3 — wake on motion using IMU interrupt

Task 3 configures the IMU so that motion events wake the MCU from sleep.

The IMU interrupt line is connected to `PE11` on the board. The firmware configures `PE11` as a rising-edge EXTI input:

```c
static void MX_IMU_INT1_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOE_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI11_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI11_IRQn);
}
```

The EXTI interrupt handler passes the event into the HAL GPIO EXTI path:

```c
void EXTI11_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
}
```

The callback only sets a flag:

```c
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == GPIO_PIN_11) {
    motion_event = 1;
  }
}
```

This is intentional. The interrupt path stays short, and the main loop performs the sensor read and UART print.

### Motion sensor setup

The accelerometer is initialized and enabled through the BSP:

```c
BSP_MOTION_SENSOR_Init(0, MOTION_ACCELERO);
BSP_MOTION_SENSOR_Enable(0, MOTION_ACCELERO);
BSP_MOTION_SENSOR_SetOutputDataRate(0, MOTION_ACCELERO, 26.0f);
BSP_MOTION_SENSOR_SetFullScale(0, MOTION_ACCELERO, 2);
```

The low-level ISM330DHCX driver is then used to route a motion-related event to the IMU INT1 pin:

```c
ISM330DHCX_Object_t *obj =
    (ISM330DHCX_Object_t *)Motion_Sensor_CompObj[0];

ism330dhcx_pin_int1_route_t int1_route = {0};

ism330dhcx_act_pin_notification_set(&obj->Ctx,
                                    ISM330DHCX_DRIVE_SLEEP_CHG_EVENT);

ism330dhcx_act_mode_set(&obj->Ctx,
                        ISM330DHCX_XL_AND_GY_NOT_AFFECTED);

ism330dhcx_wkup_threshold_set(&obj->Ctx, 2);

int1_route.md1_cfg.int1_sleep_change = 1;
ism330dhcx_pin_int1_route_set(&obj->Ctx, &int1_route);
```

### Sleep and wake flow

When there is no pending motion event, the main loop enters sleep:

```c
if (!motion_event) {
  HAL_SuspendTick();
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
}
```

`WFI` means “wait for interrupt”. When the IMU asserts INT1, the MCU wakes through the `PE11` EXTI interrupt. The callback sets `motion_event`, and the main loop reads the accelerometer axes:

```c
BSP_MOTION_SENSOR_Axes_t acc;

BSP_MOTION_SENSOR_GetAxes(0, MOTION_ACCELERO, &acc);
```

The BSP axis fields used by this driver are:

```c
acc.xval
acc.yval
acc.zval
```

Tested behavior:

```text
Lab2 Task3: wake on motion using IMU interrupt
move the board to trigger motion interrupt
motion detected: x=21 mg, y=48 mg, z=989 mg
motion detected: x=17 mg, y=19 mg, z=1008 mg
```

The `z` value being close to `1000 mg` is expected, because the accelerometer measures gravity when the board is stationary.

### BSP linking note

The board motion sensor BSP source also references the IIS2MDC magnetometer driver. Even though this task uses the accelerometer, the linker required IIS2MDC symbols from the same BSP source file.

The fix was to include the IIS2MDC component driver files:

```text
Drivers/BSP/Components/iis2mdc/iis2mdc.c
Drivers/BSP/Components/iis2mdc/iis2mdc_reg.c
Drivers/BSP/Components/iis2mdc/iis2mdc.h
Drivers/BSP/Components/iis2mdc/iis2mdc_reg.h
```

and add the corresponding source files and include directory to CMake.

### What this task demonstrates

Task 3 demonstrates the full interrupt wake-up path:

```text
IMU motion event
  -> IMU INT1 signal
  -> PE11 EXTI interrupt
  -> HAL GPIO EXTI callback
  -> motion_event flag
  -> wake from WFI
  -> accelerometer read in main loop
  -> UART print
```

This is the most important design pattern in the lab: interrupt handlers should stay short, and the main loop should do the heavier work after the event has been recorded.

## Presentation notes

For Task 1, explain that `HAL_UART_Receive_IT()` is non-blocking and one-shot. The receive callback must re-arm the receive operation.

For Task 2, explain that the application does not manually speak I2C to the sensor. It uses the BSP environmental sensor API, which internally connects to the board bus layer and the component driver.

For Task 3, explain that the IMU generates an interrupt on motion, the MCU wakes from `WFI`, and the callback only sets a flag. The accelerometer read and UART print happen in the main loop.

## Final tested status

All three tasks were built and tested on the B-U585I-IOT02A board.

```text
Task 1: interrupt UART CLI works
Task 2: temperature and humidity readout works
Task 3: motion-triggered wake-up and accelerometer printout works
```
