/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_driver.h
  * @brief   OLED驱动头文件，使用结构体和函数指针实现解耦
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __OLED_DRIVER_H__
#define __OLED_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/* 前向声明 */
typedef struct OLED_HandleTypeDef OLED_HandleTypeDef;

/**
  * @brief  OLED芯片类型枚举
  */
typedef enum {
    OLED_TYPE_SSD1306 = 0,    /* SSD1306芯片 */
    OLED_TYPE_SH1106          /* SH1106芯片 */
} OLED_ChipType_t;

/**
  * @brief  OLED屏幕尺寸枚举
  */
typedef enum {
    OLED_SIZE_128X64 = 0,      /* 128x64像素 */
    OLED_SIZE_128X32          /* 128x32像素 */
} OLED_SizeType_t;

/**
  * @brief  OLED显示方向枚举
  */
typedef enum {
    OLED_DIRECTION_NORMAL = 0,    /* 正常方向 */
    OLED_DIRECTION_ROTATE_180     /* 旋转180度 */
} OLED_DirectionTypeDef;

/**
  * @brief  OLED屏幕配置结构体
  */
typedef struct {
    OLED_ChipType_t chip_type;     /* 芯片类型 */
    OLED_SizeType_t size_type;     /* 屏幕尺寸 */
    uint8_t width;                 /* 屏幕宽度 */
    uint8_t height;                /* 屏幕高度 */
    uint8_t col_offset;            /* 列偏移（SH1106需要，通常为2） */
    uint8_t contrast;              /* 对比度值 */
    uint8_t com_pins;              /* COM引脚配置 */
} OLED_Config_t;

/**
  * @brief  脏矩形区域结构体（用于分区刷新）
  */
typedef struct {
    uint8_t x_min;      /* 最小X坐标 */
    uint8_t x_max;      /* 最大X坐标 */
    uint8_t y_min;      /* 最小Y坐标 */
    uint8_t y_max;      /* 最大Y坐标 */
    bool is_dirty;      /* 是否有脏数据 */
} OLED_DirtyRegion_t;

/**
  * @brief  绘制上下文结构体
  */
typedef struct {
    uint8_t x;          /* 当前X坐标 */
    uint8_t y;          /* 当前Y坐标 */
    uint8_t font_width; /* 字体宽度 */
    uint8_t font_height;/* 字体高度 */
    uint8_t color;      /* 绘制颜色（0=黑色，1=白色） */
} OLED_DrawContext_t;

/**
  * @brief  底层硬件接口函数指针结构体
  */
typedef struct {
    /**
      * @brief  发送命令函数指针
      * @param  holed: OLED句柄指针
      * @param  cmd: 命令字节
      * @retval HAL状态
      */
    HAL_StatusTypeDef (*send_cmd)(OLED_HandleTypeDef *holed, uint8_t cmd);
    
    /**
      * @brief  发送数据函数指针
      * @param  holed: OLED句柄指针
      * @param  data: 数据缓冲区
      * @param  len: 数据长度
      * @retval HAL状态
      */
    HAL_StatusTypeDef (*send_data)(OLED_HandleTypeDef *holed, uint8_t *data, uint16_t len);
    
    /**
      * @brief  延时函数指针
      * @param  delay: 延时时间（毫秒）
      */
    void (*delay_ms)(uint32_t delay);
} OLED_HWInterface_t;

/**
  * @brief  OLED驱动结构体（主结构体）
  */
typedef struct OLED_HandleTypeDef {
    /* 硬件接口 */
    OLED_HWInterface_t hw;           /* 硬件接口函数指针 */
    I2C_HandleTypeDef *hi2c;         /* I2C句柄 */
    uint8_t i2c_addr;                /* I2C地址 */
    
    /* 屏幕配置 */
    OLED_Config_t config;             /* 屏幕配置 */
    
    /* 显示参数 */
    uint8_t width;                   /* 屏幕宽度（像素） */
    uint8_t height;                  /* 屏幕高度（像素） */
    uint8_t pages;                   /* 页数（高度/8） */
    OLED_DirectionTypeDef direction; /* 显示方向 */
    
    /* 帧缓冲区 */
    uint8_t *framebuffer;            /* 帧缓冲区指针 */
    uint16_t framebuffer_size;       /* 帧缓冲区大小 */
    
    /* 脏矩形跟踪 */
    OLED_DirtyRegion_t dirty_region; /* 脏矩形区域 */
    bool full_refresh;                /* 是否需要全屏刷新 */
    
    /* 绘制上下文 */
    OLED_DrawContext_t draw_ctx;     /* 当前绘制上下文 */
    
    /* 状态标志 */
    bool is_initialized;              /* 是否已初始化 */
    bool dma_busy;                    /* DMA是否忙碌 */
} OLED_HandleTypeDef;

/* Exported constants --------------------------------------------------------*/

/* 预编译宏定义 - 在编译前选择屏幕类型 */
#ifndef OLED_CHIP_TYPE
#define OLED_CHIP_TYPE    OLED_TYPE_SSD1306  /* 默认使用SSD1306 */
#endif

#ifndef OLED_SIZE_TYPE
#define OLED_SIZE_TYPE    OLED_SIZE_128X64   /* 默认使用128x64 */
#endif

/* OLED常用尺寸定义 */
#define OLED_WIDTH_128       128
#define OLED_HEIGHT_64       64
#define OLED_HEIGHT_32       32
#define OLED_WIDTH_128_64    OLED_WIDTH_128
#define OLED_HEIGHT_128_64   OLED_HEIGHT_64
#define OLED_WIDTH_128_32    OLED_WIDTH_128
#define OLED_HEIGHT_128_32   OLED_HEIGHT_32

/* OLED I2C地址 */
#define OLED_I2C_ADDR_0x78   0x78
#define OLED_I2C_ADDR_0x7A   0x7A

/* 屏幕配置宏定义 */
/* 0.96寸 SSD1306 128x64 */
#define OLED_CONFIG_096_SSD1306_128X64 \
    {OLED_TYPE_SSD1306, OLED_SIZE_128X64, 128, 64, 0, 0xCF, 0x12}

/* 0.91寸 SSD1306 128x32 */
#define OLED_CONFIG_091_SSD1306_128X32 \
    {OLED_TYPE_SSD1306, OLED_SIZE_128X32, 128, 32, 0, 0x8F, 0x02}

/* 1.3寸 SH1106 128x64 */
#define OLED_CONFIG_13_SH1106_128X64 \
    {OLED_TYPE_SH1106, OLED_SIZE_128X64, 128, 64, 2, 0x80, 0x12}

/* OLED命令定义 */
#define OLED_CMD_DISPLAY_OFF         0xAE
#define OLED_CMD_DISPLAY_ON          0xAF
#define OLED_CMD_SET_DISPLAY_CLOCK   0xD5
#define OLED_CMD_SET_MULTIPLEX       0xA8
#define OLED_CMD_SET_DISPLAY_OFFSET  0xD3
#define OLED_CMD_SET_START_LINE      0x40
#define OLED_CMD_CHARGE_PUMP         0x8D
#define OLED_CMD_MEMORY_MODE         0x20
#define OLED_CMD_SEG_REMAP           0xA1
#define OLED_CMD_COM_SCAN_DEC        0xC8
#define OLED_CMD_COM_SCAN_INC        0xC0
#define OLED_CMD_SET_COM_PINS        0xDA
#define OLED_CMD_SET_CONTRAST        0x81
#define OLED_CMD_SET_PRECHARGE       0xD9
#define OLED_CMD_SET_VCOM_DETECT     0xDB
#define OLED_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define OLED_CMD_NORMAL_DISPLAY       0xA6
#define OLED_CMD_INVERSE_DISPLAY      0xA7
#define OLED_CMD_ACTIVATE_SCROLL      0x2F
#define OLED_CMD_DEACTIVATE_SCROLL    0x2E
#define OLED_CMD_SET_VERTICAL_SCROLL_AREA 0xA3
#define OLED_CMD_COLUMN_ADDR          0x21
#define OLED_CMD_PAGE_ADDR            0x22

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化OLED驱动（使用配置结构体）
  * @param  holed: OLED句柄指针
  * @param  hi2c: I2C句柄指针
  * @param  i2c_addr: I2C地址
  * @param  config: 屏幕配置结构体指针
  * @param  framebuffer: 帧缓冲区指针（外部提供，大小应为width*height/8）
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_Init(OLED_HandleTypeDef *holed, 
                            I2C_HandleTypeDef *hi2c, 
                            uint8_t i2c_addr,
                            const OLED_Config_t *config,
                            uint8_t *framebuffer);

/**
  * @brief  初始化OLED驱动（兼容旧接口，使用预编译宏）
  * @param  holed: OLED句柄指针
  * @param  hi2c: I2C句柄指针
  * @param  i2c_addr: I2C地址
  * @param  framebuffer: 帧缓冲区指针（外部提供，大小应为width*height/8）
  * @retval HAL状态
  * @note   此函数使用预编译宏OLED_CHIP_TYPE和OLED_SIZE_TYPE
  */
HAL_StatusTypeDef OLED_InitAuto(OLED_HandleTypeDef *holed, 
                                 I2C_HandleTypeDef *hi2c, 
                                 uint8_t i2c_addr,
                                 uint8_t *framebuffer);

/**
  * @brief  反初始化OLED驱动
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_DeInit(OLED_HandleTypeDef *holed);

/**
  * @brief  清空帧缓冲区
  * @param  holed: OLED句柄指针
  */
void OLED_Clear(OLED_HandleTypeDef *holed);

/**
  * @brief  刷新显示（全屏刷新）
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_Refresh(OLED_HandleTypeDef *holed);

/**
  * @brief  刷新脏区域（增量刷新）
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_RefreshDirty(OLED_HandleTypeDef *holed);

/**
  * @brief  设置像素点
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  color: 颜色（0=黑色，1=白色）
  */
void OLED_SetPixel(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t color);

/**
  * @brief  获取像素点
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @retval 像素值（0或1）
  */
uint8_t OLED_GetPixel(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y);

/**
  * @brief  绘制水平线
  * @param  holed: OLED句柄指针
  * @param  x: 起始X坐标
  * @param  y: Y坐标
  * @param  w: 宽度
  * @param  color: 颜色
  */
void OLED_DrawHLine(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t color);

/**
  * @brief  绘制垂直线
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: 起始Y坐标
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_DrawVLine(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t h, uint8_t color);

/**
  * @brief  绘制矩形框
  * @param  holed: OLED句柄指针
  * @param  x: 左上角X坐标
  * @param  y: 左上角Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_DrawRect(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/**
  * @brief  填充矩形
  * @param  holed: OLED句柄指针
  * @param  x: 左上角X坐标
  * @param  y: 左上角Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_FillRect(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/**
  * @brief  绘制圆
  * @param  holed: OLED句柄指针
  * @param  x0: 圆心X坐标
  * @param  y0: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_DrawCircle(OLED_HandleTypeDef *holed, uint8_t x0, uint8_t y0, uint8_t r, uint8_t color);

/**
  * @brief  填充圆
  * @param  holed: OLED句柄指针
  * @param  x0: 圆心X坐标
  * @param  y0: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_FillCircle(OLED_HandleTypeDef *holed, uint8_t x0, uint8_t y0, uint8_t r, uint8_t color);

/**
  * @brief  绘制字符（ASCII）
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  ch: 字符
  * @param  color: 颜色
  */
void OLED_DrawChar(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, char ch, uint8_t color);

/**
  * @brief  绘制字符串
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  str: 字符串
  * @param  color: 颜色
  */
void OLED_DrawString(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, const char *str, uint8_t color);

/**
  * @brief  设置显示方向
  * @param  holed: OLED句柄指针
  * @param  direction: 方向
  */
void OLED_SetDirection(OLED_HandleTypeDef *holed, OLED_DirectionTypeDef direction);

/**
  * @brief  设置对比度
  * @param  holed: OLED句柄指针
  * @param  contrast: 对比度值（0-255）
  */
void OLED_SetContrast(OLED_HandleTypeDef *holed, uint8_t contrast);

/**
  * @brief  设置显示开关
  * @param  holed: OLED句柄指针
  * @param  on: 是否开启（true=开启，false=关闭）
  */
void OLED_SetDisplayOn(OLED_HandleTypeDef *holed, bool on);

/**
  * @brief  更新脏矩形区域
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  */
void OLED_UpdateDirtyRegion(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
  * @brief  检查DMA是否完成
  * @param  holed: OLED句柄指针
  * @retval true=完成，false=未完成
  */
bool OLED_IsDMAReady(OLED_HandleTypeDef *holed);

/**
  * @brief  I2C DMA传输完成回调（需要在HAL_I2C_MasterTxCpltCallback中调用）
  * @param  holed: OLED句柄指针
  */
void OLED_DMATxCpltCallback(OLED_HandleTypeDef *holed);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_DRIVER_H__ */

