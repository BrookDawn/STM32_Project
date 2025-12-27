/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_config.h
  * @brief   OLED驱动配置文件 - 在编译前选择屏幕类型
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __OLED_CONFIG_H__
#define __OLED_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "oled_driver.h"

/* Exported constants --------------------------------------------------------*/

/**
  * @brief  屏幕类型选择
  * 
  * 在编译前，通过定义以下宏来选择屏幕类型：
  * 
  * 1. 0.96寸 SSD1306 128x64:
  *    #define OLED_CHIP_TYPE    OLED_TYPE_SSD1306
  *    #define OLED_SIZE_TYPE    OLED_SIZE_128X64
  * 
  * 2. 0.91寸 SSD1306 128x32:
  *    #define OLED_CHIP_TYPE    OLED_TYPE_SSD1306
  *    #define OLED_SIZE_TYPE    OLED_SIZE_128X32
  * 
  * 3. 1.3寸 SH1106 128x64:
  *    #define OLED_CHIP_TYPE    OLED_TYPE_SH1106
  *    #define OLED_SIZE_TYPE    OLED_SIZE_128X64
  * 
  * 使用方法：
  * 1. 在项目设置中添加预编译宏定义
  * 2. 或者在包含此头文件之前定义这些宏
  * 3. 或者在main.h中定义这些宏
  */

/* 默认配置（如果未定义，使用默认值） */
#ifndef OLED_CHIP_TYPE
#define OLED_CHIP_TYPE    OLED_TYPE_SSD1306  /* 默认使用SSD1306 */
#endif

#ifndef OLED_SIZE_TYPE
#define OLED_SIZE_TYPE    OLED_SIZE_128X64   /* 默认使用128x64 */
#endif

/* 屏幕配置快速选择宏 */
/* 取消注释其中一个来选择屏幕类型 */

/* 0.96寸 SSD1306 128x64 */
#define OLED_USE_096_SSD1306_128X64
#ifdef OLED_USE_096_SSD1306_128X64
    #undef OLED_CHIP_TYPE
    #undef OLED_SIZE_TYPE
    #define OLED_CHIP_TYPE    OLED_TYPE_SSD1306
    #define OLED_SIZE_TYPE    OLED_SIZE_128X64
#endif

/* 0.91寸 SSD1306 128x32 */
//#define OLED_USE_091_SSD1306_128X32
#ifdef OLED_USE_091_SSD1306_128X32
    #undef OLED_CHIP_TYPE
    #undef OLED_SIZE_TYPE
    #define OLED_CHIP_TYPE    OLED_TYPE_SSD1306
    #define OLED_SIZE_TYPE    OLED_SIZE_128X32
#endif

/* 1.3寸 SH1106 128x64 */
//#define OLED_USE_13_SH1106_128X64
#ifdef OLED_USE_13_SH1106_128X64
    #undef OLED_CHIP_TYPE
    #undef OLED_SIZE_TYPE
    #define OLED_CHIP_TYPE    OLED_TYPE_SH1106
    #define OLED_SIZE_TYPE    OLED_SIZE_128X64
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OLED_CONFIG_H__ */

