/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_ui.h
  * @brief   OLED UI绘制头文件，支持U8G2风格的绘制和流畅动画
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __OLED_UI_H__
#define __OLED_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "oled_driver.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  动画状态枚举
  */
typedef enum {
    OLED_ANIM_STOPPED = 0,    /* 停止 */
    OLED_ANIM_PLAYING,        /* 播放中 */
    OLED_ANIM_PAUSED          /* 暂停 */
} OLED_AnimState_t;

/**
  * @brief  动画缓动函数类型
  */
typedef float (*OLED_EasingFunc_t)(float t);

/**
  * @brief  动画结构体
  */
typedef struct {
    float start_value;        /* 起始值 */
    float end_value;          /* 结束值 */
    float current_value;      /* 当前值 */
    uint32_t duration;        /* 持续时间（毫秒） */
    uint32_t elapsed;         /* 已过时间（毫秒） */
    OLED_AnimState_t state;   /* 状态 */
    OLED_EasingFunc_t easing; /* 缓动函数 */
    bool loop;                /* 是否循环 */
    bool reverse;             /* 是否反向 */
} OLED_Animation_t;

/**
  * @brief  进度条结构体
  */
typedef struct {
    uint8_t x;                 /* X坐标 */
    uint8_t y;                 /* Y坐标 */
    uint8_t width;             /* 宽度 */
    uint8_t height;            /* 高度 */
    uint8_t value;             /* 当前值（0-100） */
    uint8_t border_color;      /* 边框颜色 */
    uint8_t fill_color;        /* 填充颜色 */
} OLED_ProgressBar_t;

/**
  * @brief  滚动文本结构体
  */
typedef struct {
    uint8_t x;                 /* X坐标 */
    uint8_t y;                 /* Y坐标 */
    const char *text;          /* 文本内容 */
    uint8_t text_len;          /* 文本长度 */
    int16_t offset;            /* 滚动偏移 */
    uint8_t speed;             /* 滚动速度 */
    bool enabled;              /* 是否启用 */
} OLED_ScrollText_t;

/**
  * @brief  UI绘制上下文结构体
  */
typedef struct {
    OLED_HandleTypeDef *holed; /* OLED句柄 */
    uint8_t clip_x;            /* 裁剪区域X */
    uint8_t clip_y;            /* 裁剪区域Y */
    uint8_t clip_w;            /* 裁剪区域宽度 */
    uint8_t clip_h;            /* 裁剪区域高度 */
    bool clip_enabled;         /* 是否启用裁剪 */
} OLED_UIContext_t;

/* Exported constants --------------------------------------------------------*/

/* 常用缓动函数 */
extern OLED_EasingFunc_t OLED_Easing_Linear;
extern OLED_EasingFunc_t OLED_Easing_EaseInQuad;
extern OLED_EasingFunc_t OLED_Easing_EaseOutQuad;
extern OLED_EasingFunc_t OLED_Easing_EaseInOutQuad;

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* ========== 基础绘制函数（U8G2风格） ========== */

/**
  * @brief  开始绘制（设置裁剪区域）
  * @param  ctx: UI上下文指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  */
void OLED_UI_Begin(OLED_UIContext_t *ctx, OLED_HandleTypeDef *holed, 
                   uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
  * @brief  结束绘制（清除裁剪区域）
  * @param  ctx: UI上下文指针
  */
void OLED_UI_End(OLED_UIContext_t *ctx);

/**
  * @brief  绘制像素点（带裁剪）
  * @param  ctx: UI上下文指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  color: 颜色
  */
void OLED_UI_DrawPixel(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, uint8_t color);

/**
  * @brief  绘制线条
  * @param  ctx: UI上下文指针
  * @param  x1: 起点X坐标
  * @param  y1: 起点Y坐标
  * @param  x2: 终点X坐标
  * @param  y2: 终点Y坐标
  * @param  color: 颜色
  */
void OLED_UI_DrawLine(OLED_UIContext_t *ctx, uint8_t x1, uint8_t y1, 
                      uint8_t x2, uint8_t y2, uint8_t color);

/**
  * @brief  绘制矩形框
  * @param  ctx: UI上下文指针
  * @param  x: 左上角X坐标
  * @param  y: 左上角Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_UI_DrawBox(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                     uint8_t w, uint8_t h, uint8_t color);

/**
  * @brief  填充矩形
  * @param  ctx: UI上下文指针
  * @param  x: 左上角X坐标
  * @param  y: 左上角Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_UI_DrawFilledBox(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                           uint8_t w, uint8_t h, uint8_t color);

/**
  * @brief  绘制圆
  * @param  ctx: UI上下文指针
  * @param  x: 圆心X坐标
  * @param  y: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_UI_DrawCircle(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                        uint8_t r, uint8_t color);

/**
  * @brief  填充圆
  * @param  ctx: UI上下文指针
  * @param  x: 圆心X坐标
  * @param  y: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_UI_DrawFilledCircle(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                               uint8_t r, uint8_t color);

/**
  * @brief  绘制字符串（带裁剪）
  * @param  ctx: UI上下文指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  str: 字符串
  * @param  color: 颜色
  */
void OLED_UI_DrawStr(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                     const char *str, uint8_t color);

/* ========== 动画函数 ========== */

/**
  * @brief  初始化动画
  * @param  anim: 动画结构体指针
  * @param  start: 起始值
  * @param  end: 结束值
  * @param  duration: 持续时间（毫秒）
  * @param  easing: 缓动函数
  */
void OLED_UI_AnimInit(OLED_Animation_t *anim, float start, float end, 
                      uint32_t duration, OLED_EasingFunc_t easing);

/**
  * @brief  更新动画
  * @param  anim: 动画结构体指针
  * @param  delta_ms: 时间增量（毫秒）
  * @retval true=动画完成，false=动画进行中
  */
bool OLED_UI_AnimUpdate(OLED_Animation_t *anim, uint32_t delta_ms);

/**
  * @brief  启动动画
  * @param  anim: 动画结构体指针
  */
void OLED_UI_AnimStart(OLED_Animation_t *anim);

/**
  * @brief  停止动画
  * @param  anim: 动画结构体指针
  */
void OLED_UI_AnimStop(OLED_Animation_t *anim);

/**
  * @brief  重置动画
  * @param  anim: 动画结构体指针
  */
void OLED_UI_AnimReset(OLED_Animation_t *anim);

/* ========== UI组件函数 ========== */

/**
  * @brief  绘制进度条
  * @param  ctx: UI上下文指针
  * @param  bar: 进度条结构体指针
  */
void OLED_UI_DrawProgressBar(OLED_UIContext_t *ctx, OLED_ProgressBar_t *bar);

/**
  * @brief  更新进度条值
  * @param  bar: 进度条结构体指针
  * @param  value: 新值（0-100）
  */
void OLED_UI_SetProgressBar(OLED_ProgressBar_t *bar, uint8_t value);

/**
  * @brief  初始化滚动文本
  * @param  scroll: 滚动文本结构体指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  text: 文本内容
  * @param  speed: 滚动速度
  */
void OLED_UI_InitScrollText(OLED_ScrollText_t *scroll, uint8_t x, uint8_t y, 
                            const char *text, uint8_t speed);

/**
  * @brief  更新滚动文本
  * @param  ctx: UI上下文指针
  * @param  scroll: 滚动文本结构体指针
  */
void OLED_UI_UpdateScrollText(OLED_UIContext_t *ctx, OLED_ScrollText_t *scroll);

/**
  * @brief  绘制带动画的进度条
  * @param  ctx: UI上下文指针
  * @param  bar: 进度条结构体指针
  * @param  anim: 动画结构体指针
  */
void OLED_UI_DrawAnimatedProgressBar(OLED_UIContext_t *ctx, 
                                     OLED_ProgressBar_t *bar, 
                                     OLED_Animation_t *anim);

/* ========== 缓动函数实现 ========== */

/**
  * @brief  线性缓动
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_Linear_Func(float t);

/**
  * @brief  二次缓入
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_EaseInQuad_Func(float t);

/**
  * @brief  二次缓出
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_EaseOutQuad_Func(float t);

/**
  * @brief  二次缓入缓出
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_EaseInOutQuad_Func(float t);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_UI_H__ */

