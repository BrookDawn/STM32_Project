/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_font.h
  * @brief   OLED字体数据头文件
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __OLED_FONT_H__
#define __OLED_FONT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
#define FONT_6X8_WIDTH    6
#define FONT_6X8_HEIGHT   8

/* Exported variables --------------------------------------------------------*/
extern const uint8_t font_6x8[][6];

#ifdef __cplusplus
}
#endif

#endif /* __OLED_FONT_H__ */

