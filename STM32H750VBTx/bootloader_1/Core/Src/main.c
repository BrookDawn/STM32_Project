/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "dma.h"
#include "quadspi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ota.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
void jump_to_app(int slot);
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

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_QUADSPI_Init();
  /* USER CODE BEGIN 2 */
  printf("\r\n--- Bootloader Starting (LED Blinking Test) ---\r\n");

  // 闪烁 5 次以确认 Bootloader 已启动
  for(int i = 0; i < 5; i++) {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
      HAL_Delay(100);
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
      HAL_Delay(100);
  }

  QSPI_Reset();

  // 读取并显示 Flash ID
  uint8_t flash_id[3];
  printf("\r\n--- Reading QSPI Flash ID ---\r\n");
  if (QSPI_ReadID(flash_id) == HAL_OK) {
      printf("Flash ID: 0x%02X 0x%02X 0x%02X\r\n", flash_id[0], flash_id[1], flash_id[2]);

      // 识别Flash芯片型号
      if (flash_id[0] == 0xEF && flash_id[1] == 0x40) {
          switch(flash_id[2]) {
              case 0x15:
                  printf("Flash Type: W25Q16 (2MB)\r\n");
                  break;
              case 0x16:
                  printf("Flash Type: W25Q32 (4MB)\r\n");
                  break;
              case 0x17:
                  printf("Flash Type: W25Q64 (8MB)\r\n");
                  break;
              case 0x18:
                  printf("Flash Type: W25Q128 (16MB)\r\n");
                  break;
              default:
                  printf("Flash Type: Unknown Winbond Flash\r\n");
          }
      } else if (flash_id[0] == 0xFF && flash_id[1] == 0xFF && flash_id[2] == 0xFF) {
          printf("ERROR: Flash not responding (all 0xFF) - Check hardware connections!\r\n");
      } else if (flash_id[0] == 0x00 && flash_id[1] == 0x00 && flash_id[2] == 0x00) {
          printf("ERROR: Flash not responding (all 0x00) - Check hardware connections!\r\n");
      } else {
          printf("WARNING: Unknown Flash manufacturer (Expected 0xEF for Winbond)\r\n");
      }
  } else {
      printf("ERROR: Failed to read Flash ID - QSPI communication error!\r\n");
  }
  printf("----------------------------\r\n\r\n");

  ota_info_t ota;
  ota_read(&ota);

  if (ota.upgrade_flag)
  {
    printf("Upgrading to Slot %u...\r\n", (unsigned int)ota.update_slot);
    uint32_t slot_offset = (ota.update_slot == 0) ? APP_A_OFFSET : APP_B_OFFSET;
    copy_download_to_slot(slot_offset, ota.app_size[ota.update_slot]);

    if (verify_app(ota.update_slot, &ota))
    {
        printf("Upgrade successful! Setting Slot %u as active.\r\n", (unsigned int)ota.update_slot);
        ota.active_slot = ota.update_slot;
        ota.upgrade_flag = 0;
        ota.boot_count = 0;
    }
    else
    {
        printf("Upgrade failed! Rolling back.\r\n");
        ota.rollback_flag = 1;
    }
    ota_write(&ota);
  }

  int slot = select_slot(&ota);

  if (!verify_app(slot, &ota))
  {
      printf("App verification failed in Slot %d! Attempting rollback.\r\n", slot);
      rollback(&ota);
      slot = ota.active_slot;
      if (!verify_app(slot, &ota))
      {
          printf("Rollback failed! No valid app found.\r\n");
          Error_Handler();
      }
  }

    printf("Jumping to App in Slot %d...\r\n", slot);
    HAL_UART_Transmit(&huart1, (uint8_t *)"QSPI Mapped...\r\n", 16, 100);

    QSPI_EnableMemoryMappedMode();

    /* Diagnostic: Try to read first 16 bytes of App via Memory Mapped pointer */
    uint32_t jump_addr = (slot == 0) ? APP_A_ADDR : APP_B_ADDR;
    volatile uint32_t *app_ptr = (uint32_t *)jump_addr;
    printf("XIP Test - First 4 words: 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
           (unsigned int)app_ptr[0], (unsigned int)app_ptr[1],
           (unsigned int)app_ptr[2], (unsigned int)app_ptr[3]);
    HAL_UART_Transmit(&huart1, (uint8_t *)"Starting Jump...\r\n", 18, 100);

    jump_to_app(slot);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 5;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void jump_to_app(int slot)
{
    uint32_t app_addr = (slot == 0) ? APP_A_ADDR : APP_B_ADDR;
    uint32_t *vector = (uint32_t *)app_addr;

    /* Check if the address is valid */
    if (vector[0] < 0x20000000 || vector[0] > 0x24100000)
    {
        printf("Invalid Stack Pointer: 0x%08X\r\n", (unsigned int)vector[0]);
        Error_Handler();
    }

    printf("App Address: 0x%08X, Stack Pointer: 0x%08X, Reset Handler: 0x%08X\r\n",
           (unsigned int)app_addr, (unsigned int)vector[0], (unsigned int)vector[1]);

    /* Disable interrupts */
    __disable_irq();

    /* Disable Systick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    /* Disable all peripheral interrupts */
    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* Set Vector Table Offset Register */
    SCB->VTOR = app_addr;

    /* Set Main Stack Pointer */
    __set_MSP(vector[0]);

    /* Jump to application Reset Handler */
    void (*app_reset)(void) = (void (*)(void))vector[1];
    app_reset();
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Configure the MPU attributes as Normal Non-cacheable for the whole 4GB
   *  (Otherwise, accessing undefined addresses will cause a fault)
   */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Configure the MPU attributes for QSPI Flash (0x90000000)
   */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
