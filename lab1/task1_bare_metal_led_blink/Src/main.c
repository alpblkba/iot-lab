/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    GPIO/GPIO_IOToggle/Src/main.c
  * @author  MCD Application Team
  * @brief   This example describes how to configure and use GPIOs through
  *          the STM32U5xx HAL API.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// offsets are in unsigned long
#define GREEN_LED_PIN                 7U

// non-secure peripheral region starts at 0x40000000
#define LAB_PERIPH_BASE_NS        0x40000000UL

// AHB2 peripheral region for non-secure starts from 0x02
#define LAB_AHB2PERIPH_OFFSET_NS  0x02020000UL

// GPIO offset in unsigned-long
#define LAB_GPIOH_OFFSET          0x01C00UL

// 0x42021C00UL
#define LAB_GPIOH_BASE            (LAB_PERIPH_BASE_NS + LAB_AHB2PERIPH_OFFSET_NS + LAB_GPIOH_OFFSET)
#define GPIOH_MODER                (*(volatile uint32_t *)(LAB_GPIOH_BASE + 0x00UL))
#define GPIOH_BSRR                 (*(volatile uint32_t *)(LAB_GPIOH_BASE + 0x18UL))

/*
1 KB allocation for GPIOH, therefore:
GPIOH register range = 0x42021C00 - 0x42021FFF

00 : input
01 : output
10 : alternate function
11 : analog
*/

#define GREEN_LED_MODE_SHIFT       (GREEN_LED_PIN * 2U)  // bit 14-15 should be modified
#define GREEN_LED_MODE_MASK        (3U << GREEN_LED_MODE_SHIFT) // write 11 on bit 14-15
#define GREEN_LED_OUTPUT_MODE      (1U << GREEN_LED_MODE_SHIFT) // write 01 on bit 14-15

// lower 16 bits for set, upper 16 bits for reset
#define GREEN_LED_SET_MASK         (1U << GREEN_LED_PIN)  // write to 7
#define GREEN_LED_RESET_MASK       (1U << (GREEN_LED_PIN + 16U)) // write to 7 + 16

/*
RCC_BASE_NS
// RCC lives under AHB3
// RCC, also controls clocks on the peripherals of AHB2.

= AHB3PERIPH_BASE_NS + 0x0C00
= PERIPH_BASE_NS + 0x06020000 + 0x0C00
= 0x40000000 + 0x06020000 + 0x0C00
= 0x46020C00
*/
#define LAB_AHB3PERIPH_BASE_NS    (LAB_PERIPH_BASE_NS + 0x06020000UL)
#define LAB_RCC_BASE_NS           (LAB_AHB3PERIPH_BASE_NS + 0x0C00UL)

// GPIOHEN is bit 7 in RCC_AHB2ENR1.
// 1U = 0000 0000 0000 0000 0000 0000 0000 0001
// 1U << 7U = 0000 0000 0000 0000 0000 0000 1000 0000
// 0x00000080, 128
// 1U << 7U = 0x00000080
#define LAB_RCC_AHB2ENR1_GPIOHEN  (1U << 7U)

/*!< AHB2 Peripherals Clock Enable Register 1                                
addr offset: 0x8C */
#define RCC_AHB2ENR1               (*(volatile uint32_t *)(RCC_BASE_NS + 0x8CUL))

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
static void MX_ICACHE_Init(void);

/* USER CODE BEGIN PFP */
static void Enable_GPIOH_Clock(void);
static void Configure_Green_Led_Output(void);
static void Delay(volatile uint32_t count);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* STM32U5xx HAL library initialization:
       - Configure the Flash prefetch
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 3
       - Low Level Initialization
     */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the System Power */
  SystemPower_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_ICACHE_Init();
  /* USER CODE BEGIN 2 */

   /* -1- Enable GPIO Clock (to be able to program the configuration registers) */
  Enable_GPIOH_Clock();
  Configure_Green_Led_Output();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    GPIOH_BSRR = GREEN_LED_SET_MASK;
    Delay(200000);

    GPIOH_BSRR = GREEN_LED_RESET_MASK;
    Delay(200000);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
   */
  HAL_PWREx_DisableUCPDDeadBattery();

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/* USER CODE BEGIN 4 */
static void Enable_GPIOH_Clock(void) {
  // Enable GPIOH peripheral clock through RCC.
  RCC_AHB2ENR1 |= LAB_RCC_AHB2ENR1_GPIOHEN;
}

static void Configure_Green_Led_Output(void) {
  // PH7 does output mode
  GPIOH_MODER &= ~GREEN_LED_MODE_MASK;
  GPIOH_MODER |= GREEN_LED_OUTPUT_MODE;
}

static void Delay(volatile uint32_t count) {
  while (count--) {
    __NOP();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
