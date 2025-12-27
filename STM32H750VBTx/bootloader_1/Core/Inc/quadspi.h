/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    quadspi.h
  * @brief   This file contains all the function prototypes for
  *          the quadspi.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __QUADSPI_H__
#define __QUADSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern QSPI_HandleTypeDef hqspi;

/* USER CODE BEGIN Private defines */
#define W25Q64_READ_STATUS_REG1         0x05
#define W25Q64_WRITE_ENABLE             0x06
#define W25Q64_SECTOR_ERASE             0x20
#define W25Q64_BLOCK_ERASE              0xD8
#define W25Q64_CHIP_ERASE               0x60
#define W25Q64_PAGE_PROG                0x32
#define W25Q64_QUAD_READ                0x6B
#define W25Q64_ENABLE_RESET             0x66
#define W25Q64_RESET_DEVICE             0x99
#define W25Q64_READ_JEDEC_ID            0x9F
/* USER CODE END Private defines */

void MX_QUADSPI_Init(void);

/* USER CODE BEGIN Prototypes */
HAL_StatusTypeDef QSPI_WriteEnable(void);
HAL_StatusTypeDef QSPI_WaitBusy(void);
HAL_StatusTypeDef QSPI_EraseSector(uint32_t address);
HAL_StatusTypeDef QSPI_WritePage(uint32_t address, uint32_t size, uint8_t* buffer);
HAL_StatusTypeDef QSPI_Read(uint32_t address, uint32_t size, uint8_t* buffer);
HAL_StatusTypeDef QSPI_EnableMemoryMappedMode(void);
HAL_StatusTypeDef QSPI_Reset(void);
HAL_StatusTypeDef QSPI_ReadID(uint8_t* id);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __QUADSPI_H__ */

