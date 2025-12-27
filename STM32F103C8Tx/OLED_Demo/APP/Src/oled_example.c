/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_example.c
  * @brief   OLED驱动使用示例文件
  * @note    此文件展示了如何使用OLED驱动和UI模块
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "oled_driver.h"
#include "oled_ui.h"
#include "oled_config.h"  /* 包含配置文件 */
#include "i2c.h"

/* Private variables ---------------------------------------------------------*/
/* 帧缓冲区 - 根据屏幕类型自动调整大小 */
#if (OLED_SIZE_TYPE == OLED_SIZE_128X64)
    #define OLED_FB_SIZE  (128 * 64 / 8)  /* 1024字节 */
#elif (OLED_SIZE_TYPE == OLED_SIZE_128X32)
    #define OLED_FB_SIZE  (128 * 32 / 8)  /* 512字节 */
#else
    #define OLED_FB_SIZE  (128 * 64 / 8)  /* 默认1024字节 */
#endif
static uint8_t oled_framebuffer[OLED_FB_SIZE];

/* OLED句柄 */
static OLED_HandleTypeDef holed;

/* UI上下文 */
static OLED_UIContext_t ui_ctx;

/* 动画和UI组件 */
static OLED_Animation_t progress_anim;
static OLED_ProgressBar_t progress_bar;

/* Private function prototypes -----------------------------------------------*/
static void OLED_Example_Init(void);
static void OLED_Example_DrawUI(void);
static void OLED_Example_Animation(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  OLED示例初始化
  */
static void OLED_Example_Init(void)
{
    /* 方法1: 使用预编译宏自动配置（推荐） */
    OLED_InitAuto(&holed, &hi2c1, OLED_I2C_ADDR_0x78, oled_framebuffer);
    
    /* 方法2: 手动指定配置 */
    /*
    OLED_Config_t config = OLED_CONFIG_096_SSD1306_128X64;  // 或 OLED_CONFIG_091_SSD1306_128X32 或 OLED_CONFIG_13_SH1106_128X64
    OLED_Init(&holed, &hi2c1, OLED_I2C_ADDR_0x78, &config, oled_framebuffer);
    */
    
    /* 初始化UI上下文（根据屏幕高度自动调整） */
    OLED_UI_Begin(&ui_ctx, &holed, 0, 0, holed.width, holed.height);
    
    /* 初始化进度条 */
    progress_bar.x = 10;
    progress_bar.y = 40;
    progress_bar.width = 108;
    progress_bar.height = 10;
    progress_bar.value = 0;
    progress_bar.border_color = 1;
    progress_bar.fill_color = 1;
    
    /* 初始化进度条动画 */
    OLED_UI_AnimInit(&progress_anim, 0.0f, 100.0f, 3000, OLED_Easing_EaseInOutQuad);
    progress_anim.loop = true;
    progress_anim.reverse = true;
    OLED_UI_AnimStart(&progress_anim);
}

/**
  * @brief  绘制UI界面
  */
static void OLED_Example_DrawUI(void)
{
    uint8_t screen_height = holed.height;
    
    /* 清空屏幕 */
    OLED_Clear(&holed);
    
    /* 绘制标题 */
    OLED_UI_DrawStr(&ui_ctx, 20, 5, "OLED Demo", 1);
    
    /* 绘制分隔线 */
    OLED_UI_DrawLine(&ui_ctx, 0, 15, 128, 15, 1);
    
    /* 根据屏幕高度调整布局 */
    if (screen_height == 64)
    {
        /* 128x64布局 */
        /* 绘制一些图形 */
        OLED_UI_DrawCircle(&ui_ctx, 20, 30, 8, 1);
        OLED_UI_DrawFilledCircle(&ui_ctx, 50, 30, 8, 1);
        OLED_UI_DrawBox(&ui_ctx, 70, 22, 16, 16, 1);
        OLED_UI_DrawFilledBox(&ui_ctx, 90, 22, 16, 16, 1);
        
        /* 绘制进度条 */
        progress_bar.y = 40;
        OLED_UI_DrawAnimatedProgressBar(&ui_ctx, &progress_bar, &progress_anim);
        
        /* 绘制文本 */
        OLED_UI_DrawStr(&ui_ctx, 10, 55, "Progress:", 1);
    }
    else if (screen_height == 32)
    {
        /* 128x32布局 */
        /* 绘制一些图形 */
        OLED_UI_DrawCircle(&ui_ctx, 20, 20, 6, 1);
        OLED_UI_DrawFilledCircle(&ui_ctx, 50, 20, 6, 1);
        OLED_UI_DrawBox(&ui_ctx, 70, 14, 12, 12, 1);
        OLED_UI_DrawFilledBox(&ui_ctx, 90, 14, 12, 12, 1);
        
        /* 绘制进度条 */
        progress_bar.y = 25;
        progress_bar.height = 6;
        OLED_UI_DrawAnimatedProgressBar(&ui_ctx, &progress_bar, &progress_anim);
    }
    
    /* 刷新显示（使用增量刷新） */
    OLED_RefreshDirty(&holed);
}

/**
  * @brief  动画示例
  */
static void OLED_Example_Animation(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    uint32_t delta_ms = current_tick - last_tick;
    
    if (delta_ms > 50) /* 限制更新频率 */
    {
        /* 更新动画 */
        OLED_UI_AnimUpdate(&progress_anim, delta_ms);
        
        /* 重新绘制UI */
        OLED_Example_DrawUI();
        
        last_tick = current_tick;
    }
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  OLED示例主函数（在main.c的while循环中调用）
  */
void OLED_Example_Main(void)
{
    static bool initialized = false;
    
    if (!initialized)
    {
        OLED_Example_Init();
        initialized = true;
    }
    
    /* 执行动画 */
    OLED_Example_Animation();
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

