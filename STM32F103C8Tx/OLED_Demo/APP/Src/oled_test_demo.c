/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_test_demo.c
  * @brief   0.96寸OLED完整测试Demo，包含所有UI动画元素顺序演示
  * @note    适用于SSD1306 128x64，使用I2C1 (PB6/PB7)
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
#include "oled_test_demo.h"
#include "oled_driver.h"
#include "oled_ui.h"
#include "i2c.h"
#include "tim.h"
#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define DEMO_SCENE_DURATION    3000  /* 每个场景持续时间（毫秒） */
#define ANIM_UPDATE_INTERVAL   50    /* 动画更新间隔（毫秒） */

/* Private variables ---------------------------------------------------------*/
/* 帧缓冲区（128x64需要1024字节） */
static uint8_t oled_framebuffer[128 * 64 / 8];

/* OLED句柄和UI上下文 */
OLED_HandleTypeDef holed;  /* 改为非静态，供中断回调使用 */
static OLED_UIContext_t ui_ctx;

/* 演示场景枚举 */
typedef enum {
    DEMO_SCENE_START = 0,
    DEMO_SCENE_PIXEL_ANIM,
    DEMO_SCENE_LINE_ANIM,
    DEMO_SCENE_RECT_ANIM,
    DEMO_SCENE_CIRCLE_ANIM,
    DEMO_SCENE_PROGRESS_BAR,
    DEMO_SCENE_TEXT_SCROLL,
    DEMO_SCENE_EASING_DEMO,
    DEMO_SCENE_COMPLEX_UI,
    DEMO_SCENE_END,
    DEMO_SCENE_MAX
} DemoScene_t;

/* 动画对象 */
static OLED_Animation_t anim_x, anim_y, anim_radius, anim_progress;
static OLED_Animation_t anim_rect_x, anim_rect_y, anim_rect_w, anim_rect_h;
static OLED_ProgressBar_t progress_bar;
static OLED_ScrollText_t scroll_text;

/* 演示状态 */
static DemoScene_t current_scene = DEMO_SCENE_START;
static uint32_t scene_start_time = 0;
static uint32_t last_update_time = 0;
static bool demo_initialized = false;

/* 时间测量变量 */
static volatile uint32_t microsecond_counter = 0;  /* TIM4中断计数器，每1ms加1 */
static uint32_t loop_start_us = 0;                 /* 循环开始时的微秒值 */
static uint32_t loop_time_ms = 0;                  /* 一次循环的时间（毫秒） */
static uint32_t loop_time_us = 0;                  /* 一次循环的时间（微秒部分） */

/* Private function prototypes -----------------------------------------------*/

static void OLED_TestDemo_Update(void);
static void OLED_TestDemo_Scene_Start(void);
static void OLED_TestDemo_Scene_PixelAnim(void);
static void OLED_TestDemo_Scene_LineAnim(void);
static void OLED_TestDemo_Scene_RectAnim(void);
static void OLED_TestDemo_Scene_CircleAnim(void);
static void OLED_TestDemo_Scene_ProgressBar(void);
static void OLED_TestDemo_Scene_TextScroll(void);
static void OLED_TestDemo_Scene_EasingDemo(void);
static void OLED_TestDemo_Scene_ComplexUI(void);
static void OLED_TestDemo_Scene_End(void);
static void OLED_TestDemo_SwitchScene(void);
static uint32_t GetMicrosecond(void);
static void DrawLoopTime(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  初始化测试Demo
  */
void OLED_TestDemo_Init(void)
{
    OLED_Config_t config;
    
    /* 配置0.96寸SSD1306 128x64 */
    config = (OLED_Config_t)OLED_CONFIG_096_SSD1306_128X64;
    
    /* 初始化OLED驱动 */
    OLED_Init(&holed, &hi2c1, OLED_I2C_ADDR_0x78, &config, oled_framebuffer);
    
    /* 再次确保显示打开 */
    OLED_SetDisplayOn(&holed, true);
    
    /* 初始化UI上下文 */
    OLED_UI_Begin(&ui_ctx, &holed, 0, 0, 128, 64);
    
    /* 初始化进度条 */
    progress_bar.x = 10;
    progress_bar.y = 50;
    progress_bar.width = 108;
    progress_bar.height = 8;
    progress_bar.value = 0;
    progress_bar.border_color = 1;
    progress_bar.fill_color = 1;
    
    /* 初始化滚动文本 */
    OLED_UI_InitScrollText(&scroll_text, 0, 56, "OLED Test Demo - All UI Elements Animation", 2);
    
    demo_initialized = true;
    current_scene = DEMO_SCENE_START;
    scene_start_time = HAL_GetTick();
    
    /* 启动TIM4用于时间测量 */
    HAL_TIM_Base_Start_IT(&htim4);
}

/**
  * @brief  更新测试Demo
  */
static void OLED_TestDemo_Update(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t delta_ms;
    
    if (!demo_initialized)
    {
        OLED_TestDemo_Init();
        return;
    }
    
    /* 检查是否需要切换场景 */
    if (current_time - scene_start_time >= DEMO_SCENE_DURATION)
    {
        OLED_TestDemo_SwitchScene();
        scene_start_time = current_time;
    }
    
    /* 更新动画（限制更新频率） */
    delta_ms = current_time - last_update_time;
    if (delta_ms >= ANIM_UPDATE_INTERVAL)
    {
        /* 记录循环开始时间 */
        loop_start_us = GetMicrosecond();
        
        /* 更新所有动画 */
        OLED_UI_AnimUpdate(&anim_x, delta_ms);
        OLED_UI_AnimUpdate(&anim_y, delta_ms);
        OLED_UI_AnimUpdate(&anim_radius, delta_ms);
        OLED_UI_AnimUpdate(&anim_progress, delta_ms);
        OLED_UI_AnimUpdate(&anim_rect_x, delta_ms);
        OLED_UI_AnimUpdate(&anim_rect_y, delta_ms);
        OLED_UI_AnimUpdate(&anim_rect_w, delta_ms);
        OLED_UI_AnimUpdate(&anim_rect_h, delta_ms);
        
        /* 根据当前场景绘制 */
        switch (current_scene)
        {
            case DEMO_SCENE_START:
                OLED_TestDemo_Scene_Start();
                break;
            case DEMO_SCENE_PIXEL_ANIM:
                OLED_TestDemo_Scene_PixelAnim();
                break;
            case DEMO_SCENE_LINE_ANIM:
                OLED_TestDemo_Scene_LineAnim();
                break;
            case DEMO_SCENE_RECT_ANIM:
                OLED_TestDemo_Scene_RectAnim();
                break;
            case DEMO_SCENE_CIRCLE_ANIM:
                OLED_TestDemo_Scene_CircleAnim();
                break;
            case DEMO_SCENE_PROGRESS_BAR:
                OLED_TestDemo_Scene_ProgressBar();
                break;
            case DEMO_SCENE_TEXT_SCROLL:
                OLED_TestDemo_Scene_TextScroll();
                break;
            case DEMO_SCENE_EASING_DEMO:
                OLED_TestDemo_Scene_EasingDemo();
                break;
            case DEMO_SCENE_COMPLEX_UI:
                OLED_TestDemo_Scene_ComplexUI();
                break;
            case DEMO_SCENE_END:
                OLED_TestDemo_Scene_End();
                break;
            default:
                break;
        }
        
        /* 绘制循环时间（显示在屏幕右上角） */
        DrawLoopTime();
        
        /* 刷新显示 */
        OLED_RefreshDirty(&holed);
        
        /* 计算循环时间 */
        uint32_t loop_end_us = GetMicrosecond();
        uint32_t elapsed_us = (loop_end_us >= loop_start_us) ? 
                              (loop_end_us - loop_start_us) : 
                              (0xFFFFFFFF - loop_start_us + loop_end_us + 1);
        loop_time_ms = elapsed_us / 1000;
        loop_time_us = elapsed_us % 1000;
        
        last_update_time = current_time;
    }
}

/**
  * @brief  切换场景
  */
static void OLED_TestDemo_SwitchScene(void)
{
    /* 停止当前场景的动画 */
    OLED_UI_AnimStop(&anim_x);
    OLED_UI_AnimStop(&anim_y);
    OLED_UI_AnimStop(&anim_radius);
    OLED_UI_AnimStop(&anim_progress);
    OLED_UI_AnimStop(&anim_rect_x);
    OLED_UI_AnimStop(&anim_rect_y);
    OLED_UI_AnimStop(&anim_rect_w);
    OLED_UI_AnimStop(&anim_rect_h);
    
    /* 切换到下一个场景 */
    current_scene = (DemoScene_t)((current_scene + 1) % DEMO_SCENE_MAX);
    
    /* 清空屏幕 */
    OLED_Clear(&holed);
    
    /* 根据新场景初始化动画 */
    switch (current_scene)
    {
        case DEMO_SCENE_PIXEL_ANIM:
            OLED_UI_AnimInit(&anim_x, 0.0f, 127.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            OLED_UI_AnimInit(&anim_y, 0.0f, 63.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            anim_x.loop = true;
            anim_x.reverse = true;
            anim_y.loop = true;
            anim_y.reverse = true;
            OLED_UI_AnimStart(&anim_x);
            OLED_UI_AnimStart(&anim_y);
            break;
            
        case DEMO_SCENE_LINE_ANIM:
            OLED_UI_AnimInit(&anim_x, 0.0f, 127.0f, DEMO_SCENE_DURATION, OLED_Easing_Linear);
            OLED_UI_AnimInit(&anim_y, 0.0f, 63.0f, DEMO_SCENE_DURATION, OLED_Easing_Linear);
            anim_x.loop = true;
            anim_x.reverse = true;
            anim_y.loop = true;
            anim_y.reverse = true;
            OLED_UI_AnimStart(&anim_x);
            OLED_UI_AnimStart(&anim_y);
            break;
            
        case DEMO_SCENE_RECT_ANIM:
            OLED_UI_AnimInit(&anim_rect_x, 10.0f, 80.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            OLED_UI_AnimInit(&anim_rect_y, 10.0f, 40.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            OLED_UI_AnimInit(&anim_rect_w, 20.0f, 100.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            OLED_UI_AnimInit(&anim_rect_h, 10.0f, 40.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            anim_rect_x.loop = true;
            anim_rect_x.reverse = true;
            anim_rect_y.loop = true;
            anim_rect_y.reverse = true;
            anim_rect_w.loop = true;
            anim_rect_w.reverse = true;
            anim_rect_h.loop = true;
            anim_rect_h.reverse = true;
            OLED_UI_AnimStart(&anim_rect_x);
            OLED_UI_AnimStart(&anim_rect_y);
            OLED_UI_AnimStart(&anim_rect_w);
            OLED_UI_AnimStart(&anim_rect_h);
            break;
            
        case DEMO_SCENE_CIRCLE_ANIM:
            OLED_UI_AnimInit(&anim_x, 64.0f, 64.0f, DEMO_SCENE_DURATION, OLED_Easing_Linear);
            OLED_UI_AnimInit(&anim_y, 32.0f, 32.0f, DEMO_SCENE_DURATION, OLED_Easing_Linear);
            OLED_UI_AnimInit(&anim_radius, 5.0f, 30.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            anim_radius.loop = true;
            anim_radius.reverse = true;
            OLED_UI_AnimStart(&anim_x);
            OLED_UI_AnimStart(&anim_y);
            OLED_UI_AnimStart(&anim_radius);
            break;
            
        case DEMO_SCENE_PROGRESS_BAR:
            OLED_UI_AnimInit(&anim_progress, 0.0f, 100.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInOutQuad);
            anim_progress.loop = true;
            anim_progress.reverse = true;
            OLED_UI_AnimStart(&anim_progress);
            break;
            
        case DEMO_SCENE_TEXT_SCROLL:
            scroll_text.offset = 128;
            scroll_text.enabled = true;
            break;
            
        case DEMO_SCENE_EASING_DEMO:
            OLED_UI_AnimInit(&anim_x, 10.0f, 118.0f, DEMO_SCENE_DURATION, OLED_Easing_Linear);
            OLED_UI_AnimInit(&anim_y, 10.0f, 118.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseInQuad);
            OLED_UI_AnimInit(&anim_radius, 10.0f, 118.0f, DEMO_SCENE_DURATION, OLED_Easing_EaseOutQuad);
            anim_x.loop = true;
            anim_x.reverse = true;
            anim_y.loop = true;
            anim_y.reverse = true;
            anim_radius.loop = true;
            anim_radius.reverse = true;
            OLED_UI_AnimStart(&anim_x);
            OLED_UI_AnimStart(&anim_y);
            OLED_UI_AnimStart(&anim_radius);
            break;
            
        default:
            break;
    }
}

/**
  * @brief  场景1：启动画面
  */
static void OLED_TestDemo_Scene_Start(void)
{
    OLED_UI_DrawStr(&ui_ctx, 20, 20, "OLED Test", 1);
    OLED_UI_DrawStr(&ui_ctx, 15, 35, "Demo Start", 1);
    OLED_UI_DrawStr(&ui_ctx, 10, 50, "0.96\" SSD1306", 1);
}

/**
  * @brief  场景2：像素点动画
  */
static void OLED_TestDemo_Scene_PixelAnim(void)
{
    uint8_t x, y;
    
    OLED_UI_DrawStr(&ui_ctx, 30, 5, "Pixel Anim", 1);
    
    /* 绘制动画像素点 */
    x = (uint8_t)anim_x.current_value;
    y = (uint8_t)anim_y.current_value;
    
    /* 绘制一个移动的像素点轨迹 */
    for (int i = 0; i < 10; i++)
    {
        int px = (x + i * 3) % 128;
        int py = (y + i * 2) % 64;
        if (py > 15)  /* 避免覆盖标题 */
        {
            OLED_UI_DrawPixel(&ui_ctx, px, py, 1);
        }
    }
    
    /* 绘制当前像素点（更大） */
    if (y > 15)
    {
        OLED_UI_DrawFilledBox(&ui_ctx, x - 1, y - 1, 3, 3, 1);
    }
}

/**
  * @brief  场景3：线条动画
  */
static void OLED_TestDemo_Scene_LineAnim(void)
{
    uint8_t x1, y1, x2, y2;
    
    OLED_UI_DrawStr(&ui_ctx, 30, 5, "Line Anim", 1);
    
    x1 = (uint8_t)anim_x.current_value;
    y1 = (uint8_t)anim_y.current_value;
    x2 = 127 - x1;
    y2 = 63 - y1;
    
    /* 绘制多条动态线条 */
    OLED_UI_DrawLine(&ui_ctx, 64, 32, x1, y1 > 15 ? y1 : 16, 1);
    OLED_UI_DrawLine(&ui_ctx, 64, 32, x2, y2 > 15 ? y2 : 16, 1);
    OLED_UI_DrawLine(&ui_ctx, x1, y1 > 15 ? y1 : 16, x2, y2 > 15 ? y2 : 16, 1);
}

/**
  * @brief  场景4：矩形动画
  */
static void OLED_TestDemo_Scene_RectAnim(void)
{
    uint8_t x, y, w, h;
    
    OLED_UI_DrawStr(&ui_ctx, 30, 5, "Rect Anim", 1);
    
    x = (uint8_t)anim_rect_x.current_value;
    y = (uint8_t)anim_rect_y.current_value;
    w = (uint8_t)anim_rect_w.current_value;
    h = (uint8_t)anim_rect_h.current_value;
    
    if (y < 15) y = 16;
    if (y + h > 64) h = 64 - y;
    
    /* 绘制矩形框 */
    OLED_UI_DrawBox(&ui_ctx, x, y, w, h, 1);
    
    /* 绘制填充矩形 */
    OLED_UI_DrawFilledBox(&ui_ctx, x + 2, y + 2, w - 4, h - 4, 1);
}

/**
  * @brief  场景5：圆形动画
  */
static void OLED_TestDemo_Scene_CircleAnim(void)
{
    uint8_t x, y, r;
    
    OLED_UI_DrawStr(&ui_ctx, 30, 5, "Circle Anim", 1);
    
    x = (uint8_t)anim_x.current_value;
    y = (uint8_t)anim_y.current_value;
    r = (uint8_t)anim_radius.current_value;
    
    if (y < 15) y = 16;
    if (r > y - 16) r = y - 16;
    if (r > 63 - y) r = 63 - y;
    
    /* 绘制多个同心圆 */
    OLED_UI_DrawCircle(&ui_ctx, x, y, r, 1);
    if (r > 5)
    {
        OLED_UI_DrawCircle(&ui_ctx, x, y, r - 5, 1);
    }
    if (r > 10)
    {
        OLED_UI_DrawFilledCircle(&ui_ctx, x, y, r - 10, 1);
    }
}

/**
  * @brief  场景6：进度条动画
  */
static void OLED_TestDemo_Scene_ProgressBar(void)
{
    OLED_UI_DrawStr(&ui_ctx, 30, 5, "Progress Bar", 1);
    
    /* 更新进度条值 */
    progress_bar.value = (uint8_t)anim_progress.current_value;
    
    /* 绘制进度条 */
    OLED_UI_DrawProgressBar(&ui_ctx, &progress_bar);
    
    /* 显示百分比 */
    char str[16];
    str[0] = (progress_bar.value / 10) + '0';
    str[1] = (progress_bar.value % 10) + '0';
    str[2] = '%';
    str[3] = '\0';
    OLED_UI_DrawStr(&ui_ctx, 55, 35, str, 1);
}

/**
  * @brief  场景7：滚动文本
  */
static void OLED_TestDemo_Scene_TextScroll(void)
{
    OLED_UI_DrawStr(&ui_ctx, 30, 5, "Text Scroll", 1);
    
    /* 更新滚动文本 */
    OLED_UI_UpdateScrollText(&ui_ctx, &scroll_text);
    
    /* 绘制静态文本 */
    OLED_UI_DrawStr(&ui_ctx, 20, 35, "Scrolling...", 1);
}

/**
  * @brief  场景8：缓动函数演示
  */
static void OLED_TestDemo_Scene_EasingDemo(void)
{
    uint8_t x1, x2;
    
    OLED_UI_DrawStr(&ui_ctx, 25, 5, "Easing Demo", 1);
    
    x1 = (uint8_t)anim_x.current_value;
    x2 = (uint8_t)anim_radius.current_value;
    
    /* 绘制三个不同缓动函数的动画点 */
    OLED_UI_DrawFilledCircle(&ui_ctx, x1, 25, 3, 1);  /* Linear */
    OLED_UI_DrawFilledCircle(&ui_ctx, x2, 40, 3, 1);  /* EaseOut */
    OLED_UI_DrawFilledCircle(&ui_ctx, x1, 55, 3, 1);  /* EaseIn */
    
    /* 绘制标签 */
    OLED_UI_DrawStr(&ui_ctx, 5, 23, "L", 1);
    OLED_UI_DrawStr(&ui_ctx, 5, 38, "O", 1);
    OLED_UI_DrawStr(&ui_ctx, 5, 53, "I", 1);
}

/**
  * @brief  场景9：复杂UI组合
  */
static void OLED_TestDemo_Scene_ComplexUI(void)
{
    static uint8_t frame_count = 0;
    uint8_t x, y;
    int16_t dx, dy;
    
    OLED_UI_DrawStr(&ui_ctx, 25, 5, "Complex UI", 1);
    
    frame_count++;
    
    /* 简单的正弦/余弦近似计算（使用查表法） */
    /* 绘制旋转的线条（8条从中心辐射的线） */
    for (int i = 0; i < 8; i++)
    {
        /* 使用简单的角度计算，避免使用sin/cos */
        int16_t angle = (frame_count * 2 + i * 45) % 360;
        
        /* 简单的三角函数近似 */
        if (angle < 90)
        {
            dx = angle;
            dy = 90 - angle;
        }
        else if (angle < 180)
        {
            dx = 180 - angle;
            dy = angle - 90;
        }
        else if (angle < 270)
        {
            dx = -(angle - 180);
            dy = 270 - angle;
        }
        else
        {
            dx = -(360 - angle);
            dy = -(angle - 270);
        }
        
        /* 归一化并缩放 */
        uint8_t x2 = 64 + (dx * 25) / 90;
        uint8_t y2 = 32 + (dy * 25) / 90;
        
        /* 限制在屏幕范围内 */
        if (x2 > 127) x2 = 127;
        if (y2 > 63) y2 = 63;
        if (y2 < 16) y2 = 16;
        
        OLED_UI_DrawLine(&ui_ctx, 64, 32, x2, y2, 1);
    }
    
    /* 绘制中心圆 */
    OLED_UI_DrawFilledCircle(&ui_ctx, 64, 32, 5, 1);
    
    /* 绘制多个小圆围绕中心 */
    for (int i = 0; i < 6; i++)
    {
        int16_t angle = (frame_count * 3 + i * 60) % 360;
        
        int16_t dx2, dy2;
        if (angle < 90)
        {
            dx2 = angle;
            dy2 = 90 - angle;
        }
        else if (angle < 180)
        {
            dx2 = 180 - angle;
            dy2 = angle - 90;
        }
        else if (angle < 270)
        {
            dx2 = -(angle - 180);
            dy2 = 270 - angle;
        }
        else
        {
            dx2 = -(360 - angle);
            dy2 = -(angle - 270);
        }
        
        x = 64 + (dx2 * 20) / 90;
        y = 32 + (dy2 * 20) / 90;
        
        if (x > 127) x = 127;
        if (y > 63) y = 63;
        if (y < 16) y = 16;
        
        OLED_UI_DrawCircle(&ui_ctx, x, y, 3, 1);
    }
    
    /* 绘制进度条 */
    progress_bar.value = (frame_count * 2) % 100;
    progress_bar.y = 50;
    OLED_UI_DrawProgressBar(&ui_ctx, &progress_bar);
}

/**
  * @brief  场景10：结束画面
  */
static void OLED_TestDemo_Scene_End(void)
{
    OLED_UI_DrawStr(&ui_ctx, 25, 20, "Demo End", 1);
    OLED_UI_DrawStr(&ui_ctx, 15, 35, "Restart...", 1);
    OLED_UI_DrawStr(&ui_ctx, 20, 50, "Loop Mode", 1);
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  OLED测试Demo主函数（在main.c的while循环中调用）
  */
void OLED_TestDemo_Main(void)
{
    OLED_TestDemo_Update();
}

void OLED_Test_Init(void)
{
    OLED_TestDemo_Init();
}




/* USER CODE BEGIN 1 */

/**
  * @brief  获取微秒级时间戳
  * @retval 微秒时间戳（基于TIM4中断计数）
  */
static uint32_t GetMicrosecond(void)
{
    uint32_t ms = microsecond_counter;
    uint16_t cnt = __HAL_TIM_GET_COUNTER(&htim4);
    /* TIM4计数到1000为1ms，所以当前微秒 = ms*1000 + cnt */
    return (ms * 1000) + cnt;
}

/**
  * @brief  在屏幕右上角绘制循环时间
  */
static void DrawLoopTime(void)
{
    char time_str[16];
    
    /* 格式化时间字符串：xx.xxxms */
    if (loop_time_ms < 100)
    {
        snprintf(time_str, sizeof(time_str), "%lums", loop_time_ms);
    }
    else
    {
        snprintf(time_str, sizeof(time_str), "%lums", loop_time_ms);
    }
    
    /* 在屏幕右上角显示时间，使用6x8字体 */
    /* 计算文本宽度（大约6像素/字符） */
    uint8_t str_len = strlen(time_str);
    uint8_t text_width = str_len * 6;
    uint8_t x_pos = (text_width < 128) ? (128 - text_width - 2) : 0;
    
    /* 绘制黑色背景（确保文字清晰） */
    OLED_UI_DrawFilledBox(&ui_ctx, x_pos - 1, 0, text_width + 2, 8, 0);
    
    /* 绘制白色文字 */
    OLED_UI_DrawStr(&ui_ctx, x_pos, 0, time_str, 1);
}

/**
  * @brief  TIM4中断回调函数
  * @param  htim: TIM句柄
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        microsecond_counter++;
    }
}

/* USER CODE END 1 */

