/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_test_demo.h
  * @brief   0.96寸OLED完整测试Demo头文件
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __OLED_TEST_DEMO_H__
#define __OLED_TEST_DEMO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "oled_driver.h"

/* Exported variables --------------------------------------------------------*/
extern OLED_HandleTypeDef holed;  /* OLED句柄，供中断回调使用 */

/* Exported functions prototypes ---------------------------------------------*/



/**
  * @brief  初始化测试Demo
  */
void OLED_TestDemo_Init(void);

/**
  * @brief  OLED测试Demo主函数（在main.c的while循环中调用）
  */
void OLED_TestDemo_Main(void);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_TEST_DEMO_H__ */

