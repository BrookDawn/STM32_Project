/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_ui.c
  * @brief   OLED UI绘制实现文件，支持U8G2风格的绘制和流畅动画
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
#include "oled_ui.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 缓动函数指针 */
OLED_EasingFunc_t OLED_Easing_Linear = OLED_Easing_Linear_Func;
OLED_EasingFunc_t OLED_Easing_EaseInQuad = OLED_Easing_EaseInQuad_Func;
OLED_EasingFunc_t OLED_Easing_EaseOutQuad = OLED_Easing_EaseOutQuad_Func;
OLED_EasingFunc_t OLED_Easing_EaseInOutQuad = OLED_Easing_EaseInOutQuad_Func;

/* Private function prototypes -----------------------------------------------*/
static bool OLED_UI_IsInClip(OLED_UIContext_t *ctx, uint8_t x, uint8_t y);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  检查点是否在裁剪区域内
  * @param  ctx: UI上下文指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @retval true=在区域内，false=不在区域内
  */
static bool OLED_UI_IsInClip(OLED_UIContext_t *ctx, uint8_t x, uint8_t y)
{
    if (!ctx->clip_enabled)
    {
        return true;
    }
    
    return (x >= ctx->clip_x && x < (ctx->clip_x + ctx->clip_w) &&
            y >= ctx->clip_y && y < (ctx->clip_y + ctx->clip_h));
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  开始绘制（设置裁剪区域）
  * @param  ctx: UI上下文指针
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  */
void OLED_UI_Begin(OLED_UIContext_t *ctx, OLED_HandleTypeDef *holed, 
                   uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (ctx == NULL || holed == NULL)
    {
        return;
    }
    
    ctx->holed = holed;
    ctx->clip_x = x;
    ctx->clip_y = y;
    ctx->clip_w = w;
    ctx->clip_h = h;
    ctx->clip_enabled = true;
}

/**
  * @brief  结束绘制（清除裁剪区域）
  * @param  ctx: UI上下文指针
  */
void OLED_UI_End(OLED_UIContext_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    
    ctx->clip_enabled = false;
}

/**
  * @brief  绘制像素点（带裁剪）
  * @param  ctx: UI上下文指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  color: 颜色
  */
void OLED_UI_DrawPixel(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, uint8_t color)
{
    if (ctx == NULL || ctx->holed == NULL)
    {
        return;
    }
    
    if (OLED_UI_IsInClip(ctx, x, y))
    {
        OLED_SetPixel(ctx->holed, x, y, color);
    }
}

/**
  * @brief  绘制线条（Bresenham算法）
  * @param  ctx: UI上下文指针
  * @param  x1: 起点X坐标
  * @param  y1: 起点Y坐标
  * @param  x2: 终点X坐标
  * @param  y2: 终点Y坐标
  * @param  color: 颜色
  */
void OLED_UI_DrawLine(OLED_UIContext_t *ctx, uint8_t x1, uint8_t y1, 
                      uint8_t x2, uint8_t y2, uint8_t color)
{
    int16_t dx, dy, sx, sy, err, e2;
    
    if (ctx == NULL || ctx->holed == NULL)
    {
        return;
    }
    
    dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;
    
    while (1)
    {
        OLED_UI_DrawPixel(ctx, x1, y1, color);
        
        if (x1 == x2 && y1 == y2)
        {
            break;
        }
        
        e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

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
                     uint8_t w, uint8_t h, uint8_t color)
{
    if (ctx == NULL || ctx->holed == NULL)
    {
        return;
    }
    
    /* 上边 */
    OLED_UI_DrawLine(ctx, x, y, x + w - 1, y, color);
    /* 下边 */
    OLED_UI_DrawLine(ctx, x, y + h - 1, x + w - 1, y + h - 1, color);
    /* 左边 */
    OLED_UI_DrawLine(ctx, x, y, x, y + h - 1, color);
    /* 右边 */
    OLED_UI_DrawLine(ctx, x + w - 1, y, x + w - 1, y + h - 1, color);
}

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
                           uint8_t w, uint8_t h, uint8_t color)
{
    uint8_t i, j;
    
    if (ctx == NULL || ctx->holed == NULL)
    {
        return;
    }
    
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            OLED_UI_DrawPixel(ctx, x + j, y + i, color);
        }
    }
}

/**
  * @brief  绘制圆
  * @param  ctx: UI上下文指针
  * @param  x: 圆心X坐标
  * @param  y: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_UI_DrawCircle(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                        uint8_t r, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t px = 0;
    int16_t py = r;
    
    if (ctx == NULL || ctx->holed == NULL)
    {
        return;
    }
    
    OLED_UI_DrawPixel(ctx, x, y + r, color);
    OLED_UI_DrawPixel(ctx, x, y - r, color);
    OLED_UI_DrawPixel(ctx, x + r, y, color);
    OLED_UI_DrawPixel(ctx, x - r, y, color);
    
    while (px < py)
    {
        if (f >= 0)
        {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x;
        
        OLED_UI_DrawPixel(ctx, x + px, y + py, color);
        OLED_UI_DrawPixel(ctx, x - px, y + py, color);
        OLED_UI_DrawPixel(ctx, x + px, y - py, color);
        OLED_UI_DrawPixel(ctx, x - px, y - py, color);
        OLED_UI_DrawPixel(ctx, x + py, y + px, color);
        OLED_UI_DrawPixel(ctx, x - py, y + px, color);
        OLED_UI_DrawPixel(ctx, x + py, y - px, color);
        OLED_UI_DrawPixel(ctx, x - py, y - px, color);
    }
}

/**
  * @brief  填充圆
  * @param  ctx: UI上下文指针
  * @param  x: 圆心X坐标
  * @param  y: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_UI_DrawFilledCircle(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                                uint8_t r, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t px = 0;
    int16_t py = r;
    
    if (ctx == NULL || ctx->holed == NULL)
    {
        return;
    }
    
    /* 绘制中心线 */
    OLED_UI_DrawLine(ctx, x, y - r, x, y + r, color);
    
    while (px < py)
    {
        if (f >= 0)
        {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x;
        
        OLED_UI_DrawLine(ctx, x + px, y - py, x + px, y + py, color);
        OLED_UI_DrawLine(ctx, x - px, y - py, x - px, y + py, color);
        OLED_UI_DrawLine(ctx, x + py, y - px, x + py, y + px, color);
        OLED_UI_DrawLine(ctx, x - py, y - px, x - py, y + px, color);
    }
}

/**
  * @brief  绘制字符串（带裁剪）
  * @param  ctx: UI上下文指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  str: 字符串
  * @param  color: 颜色
  */
void OLED_UI_DrawStr(OLED_UIContext_t *ctx, uint8_t x, uint8_t y, 
                     const char *str, uint8_t color)
{
    uint8_t pos_x = x;
    const char *p = str;
    
    if (ctx == NULL || ctx->holed == NULL || str == NULL)
    {
        return;
    }
    
    while (*p)
    {
        if (OLED_UI_IsInClip(ctx, pos_x, y))
        {
            OLED_DrawChar(ctx->holed, pos_x, y, *p, color);
        }
        pos_x += 6;
        if (pos_x >= ctx->holed->width)
        {
            break;
        }
        p++;
    }
}

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
                      uint32_t duration, OLED_EasingFunc_t easing)
{
    if (anim == NULL)
    {
        return;
    }
    
    anim->start_value = start;
    anim->end_value = end;
    anim->current_value = start;
    anim->duration = duration;
    anim->elapsed = 0;
    anim->state = OLED_ANIM_STOPPED;
    anim->easing = (easing != NULL) ? easing : OLED_Easing_Linear;
    anim->loop = false;
    anim->reverse = false;
}

/**
  * @brief  更新动画
  * @param  anim: 动画结构体指针
  * @param  delta_ms: 时间增量（毫秒）
  * @retval true=动画完成，false=动画进行中
  */
bool OLED_UI_AnimUpdate(OLED_Animation_t *anim, uint32_t delta_ms)
{
    float t;
    float range;
    
    if (anim == NULL || anim->state != OLED_ANIM_PLAYING)
    {
        return true;
    }
    
    anim->elapsed += delta_ms;
    
    if (anim->elapsed >= anim->duration)
    {
        if (anim->loop)
        {
            if (anim->reverse)
            {
                /* 反向循环 */
                float temp = anim->start_value;
                anim->start_value = anim->end_value;
                anim->end_value = temp;
            }
            anim->elapsed = 0;
            anim->current_value = anim->start_value;
            return false;
        }
        else
        {
            anim->elapsed = anim->duration;
            anim->current_value = anim->end_value;
            anim->state = OLED_ANIM_STOPPED;
            return true;
        }
    }
    
    t = (float)anim->elapsed / (float)anim->duration;
    if (t > 1.0f) t = 1.0f;
    
    if (anim->easing != NULL)
    {
        t = anim->easing(t);
    }
    
    range = anim->end_value - anim->start_value;
    anim->current_value = anim->start_value + range * t;
    
    return false;
}

/**
  * @brief  启动动画
  * @param  anim: 动画结构体指针
  */
void OLED_UI_AnimStart(OLED_Animation_t *anim)
{
    if (anim == NULL)
    {
        return;
    }
    
    anim->state = OLED_ANIM_PLAYING;
    anim->elapsed = 0;
    anim->current_value = anim->start_value;
}

/**
  * @brief  停止动画
  * @param  anim: 动画结构体指针
  */
void OLED_UI_AnimStop(OLED_Animation_t *anim)
{
    if (anim == NULL)
    {
        return;
    }
    
    anim->state = OLED_ANIM_STOPPED;
}

/**
  * @brief  重置动画
  * @param  anim: 动画结构体指针
  */
void OLED_UI_AnimReset(OLED_Animation_t *anim)
{
    if (anim == NULL)
    {
        return;
    }
    
    anim->elapsed = 0;
    anim->current_value = anim->start_value;
    anim->state = OLED_ANIM_STOPPED;
}

/* ========== UI组件函数 ========== */

/**
  * @brief  绘制进度条
  * @param  ctx: UI上下文指针
  * @param  bar: 进度条结构体指针
  */
void OLED_UI_DrawProgressBar(OLED_UIContext_t *ctx, OLED_ProgressBar_t *bar)
{
    uint8_t fill_width;
    
    if (ctx == NULL || ctx->holed == NULL || bar == NULL)
    {
        return;
    }
    
    /* 绘制边框 */
    OLED_UI_DrawBox(ctx, bar->x, bar->y, bar->width, bar->height, bar->border_color);
    
    /* 计算填充宽度 */
    fill_width = (bar->width - 2) * bar->value / 100;
    if (fill_width > (bar->width - 2))
    {
        fill_width = bar->width - 2;
    }
    
    /* 绘制填充 */
    if (fill_width > 0)
    {
        OLED_UI_DrawFilledBox(ctx, bar->x + 1, bar->y + 1, 
                              fill_width, bar->height - 2, bar->fill_color);
    }
}

/**
  * @brief  更新进度条值
  * @param  bar: 进度条结构体指针
  * @param  value: 新值（0-100）
  */
void OLED_UI_SetProgressBar(OLED_ProgressBar_t *bar, uint8_t value)
{
    if (bar == NULL)
    {
        return;
    }
    
    if (value > 100)
    {
        value = 100;
    }
    
    bar->value = value;
}

/**
  * @brief  初始化滚动文本
  * @param  scroll: 滚动文本结构体指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  text: 文本内容
  * @param  speed: 滚动速度
  */
void OLED_UI_InitScrollText(OLED_ScrollText_t *scroll, uint8_t x, uint8_t y, 
                            const char *text, uint8_t speed)
{
    if (scroll == NULL || text == NULL)
    {
        return;
    }
    
    scroll->x = x;
    scroll->y = y;
    scroll->text = text;
    scroll->text_len = strlen(text);
    scroll->offset = 0;
    scroll->speed = speed;
    scroll->enabled = true;
}

/**
  * @brief  更新滚动文本
  * @param  ctx: UI上下文指针
  * @param  scroll: 滚动文本结构体指针
  */
void OLED_UI_UpdateScrollText(OLED_UIContext_t *ctx, OLED_ScrollText_t *scroll)
{
    uint8_t i;
    int16_t char_x;
    uint8_t visible_chars;
    uint8_t char_width = 6;
    
    if (ctx == NULL || ctx->holed == NULL || scroll == NULL || !scroll->enabled)
    {
        return;
    }
    
    /* 计算可见字符数 */
    visible_chars = ctx->holed->width / char_width;
    
    /* 更新偏移 */
    scroll->offset -= scroll->speed;
    if (scroll->offset < -(scroll->text_len * char_width))
    {
        scroll->offset = ctx->holed->width;
    }
    
    /* 绘制可见字符 */
    for (i = 0; i < scroll->text_len && i < visible_chars + 1; i++)
    {
        char_x = scroll->x + scroll->offset + i * char_width;
        if (char_x >= -char_width && char_x < ctx->holed->width)
        {
            OLED_UI_DrawStr(ctx, char_x, scroll->y, &scroll->text[i], 1);
            break; /* 只绘制第一个可见字符，简化实现 */
        }
    }
}

/**
  * @brief  绘制带动画的进度条
  * @param  ctx: UI上下文指针
  * @param  bar: 进度条结构体指针
  * @param  anim: 动画结构体指针
  */
void OLED_UI_DrawAnimatedProgressBar(OLED_UIContext_t *ctx, 
                                      OLED_ProgressBar_t *bar, 
                                      OLED_Animation_t *anim)
{
    if (ctx == NULL || bar == NULL || anim == NULL)
    {
        return;
    }
    
    /* 更新动画值到进度条 */
    bar->value = (uint8_t)anim->current_value;
    
    /* 绘制进度条 */
    OLED_UI_DrawProgressBar(ctx, bar);
}

/* ========== 缓动函数实现 ========== */

/**
  * @brief  线性缓动
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_Linear_Func(float t)
{
    return t;
}

/**
  * @brief  二次缓入
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_EaseInQuad_Func(float t)
{
    return t * t;
}

/**
  * @brief  二次缓出
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_EaseOutQuad_Func(float t)
{
    return t * (2.0f - t);
}

/**
  * @brief  二次缓入缓出
  * @param  t: 时间参数（0.0-1.0）
  * @retval 缓动值
  */
float OLED_Easing_EaseInOutQuad_Func(float t)
{
    if (t < 0.5f)
    {
        return 2.0f * t * t;
    }
    else
    {
        return -1.0f + (4.0f - 2.0f * t) * t;
    }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

