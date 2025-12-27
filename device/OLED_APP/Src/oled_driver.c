/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_driver.c
  * @brief   OLED驱动实现文件，使用结构体和函数指针实现解耦，支持IIC DMA传输和分区刷新
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
#include "oled_font.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define OLED_CMD_MODE     0x00
#define OLED_DATA_MODE    0x40

/* Private variables ---------------------------------------------------------*/
static uint8_t oled_cmd_buffer[2];    /* 命令缓冲区：控制字节 + 命令字节 */
static uint8_t oled_data_buffer[129]; /* 数据缓冲区：控制字节 + 128字节数据 */

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef OLED_SendCmd_Blocking(OLED_HandleTypeDef *holed, uint8_t cmd);
static HAL_StatusTypeDef OLED_SendData_DMA(OLED_HandleTypeDef *holed, uint8_t *data, uint16_t len);
static void OLED_DelayMs(uint32_t delay);
static void OLED_ResetDirtyRegion(OLED_HandleTypeDef *holed);
static void OLED_ExpandDirtyRegion(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y);
static HAL_StatusTypeDef OLED_Init_SSD1306(OLED_HandleTypeDef *holed);
static HAL_StatusTypeDef OLED_Init_SH1106(OLED_HandleTypeDef *holed);
static void OLED_WaitDMAComplete(OLED_HandleTypeDef *holed, uint32_t timeout);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  通过DMA发送命令
  * @param  holed: OLED句柄指针
  * @param  cmd: 命令字节
  * @retval HAL状态
  */
/**
  * @brief  通过阻塞方式发送命令（优化：命令数据量小，使用阻塞更高效且稳定）
  * @param  holed: OLED句柄指针
  * @param  cmd: 命令字节
  * @retval HAL状态
  */
static HAL_StatusTypeDef OLED_SendCmd_Blocking(OLED_HandleTypeDef *holed, uint8_t cmd)
{
    HAL_StatusTypeDef status;
    
    if (holed == NULL || holed->hi2c == NULL)
    {
        return HAL_ERROR;
    }
    
    /* 等待之前的DMA传输完成 */
    OLED_WaitDMAComplete(holed, 100);
    
    oled_cmd_buffer[0] = OLED_CMD_MODE;
    oled_cmd_buffer[1] = cmd;
    
    /* 使用阻塞方式发送 */
    status = HAL_I2C_Master_Transmit(holed->hi2c, holed->i2c_addr, oled_cmd_buffer, 2, 100);
    
    return status;
}

/**
  * @brief  通过DMA发送数据（改进版）
  * @param  holed: OLED句柄指针
  * @param  data: 数据缓冲区
  * @param  len: 数据长度（最大128字节）
  * @retval HAL状态
  */
static HAL_StatusTypeDef OLED_SendData_DMA(OLED_HandleTypeDef *holed, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    uint32_t timeout;
    
    if (holed == NULL || holed->hi2c == NULL || data == NULL)
    {
        return HAL_ERROR;
    }
    
    /* 等待I2C空闲，最多20ms */
    timeout = HAL_GetTick();
    while (holed->hi2c->State != HAL_I2C_STATE_READY)
    {
        if (HAL_GetTick() - timeout > 20)
        {
            return HAL_TIMEOUT;
        }
    }
    
    if (len > 128) len = 128;
    
    oled_data_buffer[0] = OLED_DATA_MODE;
    memcpy(&oled_data_buffer[1], data, len);
    
    /* 启动DMA传输 */
    status = HAL_I2C_Master_Transmit_DMA(holed->hi2c, holed->i2c_addr, oled_data_buffer, len + 1);
    
    if (status == HAL_OK)
    {
        /* 等待传输完成，最多50ms */
        timeout = HAL_GetTick();
        while (holed->hi2c->State != HAL_I2C_STATE_READY)
        {
            if (HAL_GetTick() - timeout > 50)
            {
                /* 超时，中止传输 */
                HAL_I2C_Master_Abort_IT(holed->hi2c, holed->i2c_addr);
                return HAL_TIMEOUT;
            }
        }
    }
    
    return status;
}

/**
  * @brief  延时函数（使用HAL_Delay）
  * @param  delay: 延时时间（毫秒）
  */
static void OLED_DelayMs(uint32_t delay)
{
    HAL_Delay(delay);
}

/**
  * @brief  等待DMA传输完成
  * @param  holed: OLED句柄指针
  * @param  timeout: 超时时间（毫秒）
  */
static void OLED_WaitDMAComplete(OLED_HandleTypeDef *holed, uint32_t timeout)
{
    uint32_t start_tick = HAL_GetTick();
    
    if (holed == NULL || holed->hi2c == NULL)
    {
        return;
    }
    
    /* 如果I2C状态已经是READY，直接清除busy标志 */
    if (holed->hi2c->State == HAL_I2C_STATE_READY)
    {
        holed->dma_busy = false;
        return;
    }
    
    /* 等待DMA传输完成或超时 */
    while (holed->dma_busy)
    {
        /* 检查I2C状态 */
        if (holed->hi2c->State == HAL_I2C_STATE_READY)
        {
            holed->dma_busy = false;
            break;
        }
        
        if (HAL_GetTick() - start_tick >= timeout)
        {
            /* 超时，强制清除busy标志 */
            holed->dma_busy = false;
            break;
        }
    }
}

/**
  * @brief  重置脏矩形区域
  * @param  holed: OLED句柄指针
  */
static void OLED_ResetDirtyRegion(OLED_HandleTypeDef *holed)
{
    holed->dirty_region.x_min = holed->width;
    holed->dirty_region.x_max = 0;
    holed->dirty_region.y_min = holed->height;
    holed->dirty_region.y_max = 0;
    holed->dirty_region.is_dirty = false;
}

/**
  * @brief  扩展脏矩形区域
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  */
static void OLED_ExpandDirtyRegion(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y)
{
    if (!holed->dirty_region.is_dirty)
    {
        holed->dirty_region.x_min = x;
        holed->dirty_region.x_max = x;
        holed->dirty_region.y_min = y;
        holed->dirty_region.y_max = y;
        holed->dirty_region.is_dirty = true;
    }
    else
    {
        if (x < holed->dirty_region.x_min) holed->dirty_region.x_min = x;
        if (x > holed->dirty_region.x_max) holed->dirty_region.x_max = x;
        if (y < holed->dirty_region.y_min) holed->dirty_region.y_min = y;
        if (y > holed->dirty_region.y_max) holed->dirty_region.y_max = y;
    }
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  SSD1306初始化函数
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
static HAL_StatusTypeDef OLED_Init_SSD1306(OLED_HandleTypeDef *holed)
{
    HAL_StatusTypeDef status;
    uint8_t height = holed->height;
    uint8_t contrast = holed->config.contrast;
    uint8_t com_pins = holed->config.com_pins;
    
    /* 发送初始化命令序列 */
    status = holed->hw.send_cmd(holed, OLED_CMD_DISPLAY_OFF);
    holed->hw.delay_ms(10);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_DISPLAY_CLOCK);
    status = holed->hw.send_cmd(holed, 0x80);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_MULTIPLEX);
    status = holed->hw.send_cmd(holed, height - 1);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_DISPLAY_OFFSET);
    status = holed->hw.send_cmd(holed, 0x00);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_START_LINE | 0x00);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_CHARGE_PUMP);
    status = holed->hw.send_cmd(holed, 0x14);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_MEMORY_MODE);
    status = holed->hw.send_cmd(holed, 0x00);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SEG_REMAP | 0x01);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_COM_SCAN_DEC);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_COM_PINS);
    status = holed->hw.send_cmd(holed, com_pins);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_CONTRAST);
    status = holed->hw.send_cmd(holed, contrast);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_PRECHARGE);
    status = holed->hw.send_cmd(holed, 0xF1);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_VCOM_DETECT);
    status = holed->hw.send_cmd(holed, 0x40);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_DISPLAY_ALL_ON_RESUME);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_NORMAL_DISPLAY);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_DEACTIVATE_SCROLL);
    holed->hw.delay_ms(1);
    
    return status;
}

/**
  * @brief  SH1106初始化函数
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
static HAL_StatusTypeDef OLED_Init_SH1106(OLED_HandleTypeDef *holed)
{
    HAL_StatusTypeDef status;
    uint8_t height = holed->height;
    uint8_t contrast = holed->config.contrast;
    uint8_t com_pins = holed->config.com_pins;
    
    /* SH1106初始化命令序列 */
    status = holed->hw.send_cmd(holed, OLED_CMD_DISPLAY_OFF);
    holed->hw.delay_ms(10);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_DISPLAY_CLOCK);
    status = holed->hw.send_cmd(holed, 0x80);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_MULTIPLEX);
    status = holed->hw.send_cmd(holed, height - 1);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_DISPLAY_OFFSET);
    status = holed->hw.send_cmd(holed, 0x00);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_START_LINE | 0x00);
    holed->hw.delay_ms(1);
    
    /* SH1106不需要电荷泵命令 */
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SEG_REMAP | 0x01);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_COM_SCAN_DEC);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_COM_PINS);
    status = holed->hw.send_cmd(holed, com_pins);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_CONTRAST);
    status = holed->hw.send_cmd(holed, contrast);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_PRECHARGE);
    status = holed->hw.send_cmd(holed, 0xF1);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_SET_VCOM_DETECT);
    status = holed->hw.send_cmd(holed, 0x40);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_DISPLAY_ALL_ON_RESUME);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_NORMAL_DISPLAY);
    holed->hw.delay_ms(1);
    
    status = holed->hw.send_cmd(holed, OLED_CMD_DEACTIVATE_SCROLL);
    holed->hw.delay_ms(1);
    
    return status;
}

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
                            uint8_t *framebuffer)
{
    HAL_StatusTypeDef status;
    
    if (holed == NULL || hi2c == NULL || config == NULL || framebuffer == NULL)
    {
        return HAL_ERROR;
    }
    
    /* 初始化硬件接口函数指针 */
    holed->hw.send_cmd = OLED_SendCmd_Blocking;
    holed->hw.send_data = OLED_SendData_DMA;  /* 改用DMA提高性能 */
    holed->hw.delay_ms = OLED_DelayMs;
    
    /* 保存配置 */
    holed->config = *config;
    
    /* 初始化基本参数 */
    holed->hi2c = hi2c;
    holed->i2c_addr = i2c_addr;
    holed->width = config->width;
    holed->height = config->height;
    holed->pages = config->height / 8;
    holed->direction = OLED_DIRECTION_NORMAL;
    holed->framebuffer = framebuffer;
    holed->framebuffer_size = config->width * holed->pages;
    holed->is_initialized = false;
    holed->dma_busy = false;
    holed->full_refresh = true;
    
    /* 重置脏矩形区域 */
    OLED_ResetDirtyRegion(holed);
    
    /* 初始化绘制上下文 */
    holed->draw_ctx.x = 0;
    holed->draw_ctx.y = 0;
    holed->draw_ctx.font_width = 6;
    holed->draw_ctx.font_height = 8;
    holed->draw_ctx.color = 1;
    
    /* 等待I2C就绪 */
    holed->hw.delay_ms(100);
    
    /* 根据芯片类型调用不同的初始化函数 */
    if (config->chip_type == OLED_TYPE_SSD1306)
    {
        status = OLED_Init_SSD1306(holed);
    }
    else if (config->chip_type == OLED_TYPE_SH1106)
    {
        status = OLED_Init_SH1106(holed);
    }
    else
    {
        return HAL_ERROR;
    }
    
    if (status == HAL_OK)
    {
        /* 清空帧缓冲区 */
        OLED_Clear(holed);
        holed->is_initialized = true;
        
        /* 刷新屏幕（清空显存，避免花屏） */
        OLED_Refresh(holed);
    }
    else
    {
        /* 即使初始化失败，也尝试标记为初始化，以便后续重试 */
        holed->is_initialized = true;
    }
    
    holed->hw.delay_ms(100);
    
    /* 强制打开显示，确保屏幕点亮 */
    OLED_SetDisplayOn(holed, true);
    
    return status;
}

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
                                 uint8_t *framebuffer)
{
    OLED_Config_t config;
    
    /* 根据预编译宏设置配置 */
    config.chip_type = OLED_CHIP_TYPE;
    config.size_type = OLED_SIZE_TYPE;
    
    if (config.size_type == OLED_SIZE_128X64)
    {
        config.width = 128;
        config.height = 64;
        config.com_pins = 0x12;
    }
    else if (config.size_type == OLED_SIZE_128X32)
    {
        config.width = 128;
        config.height = 32;
        config.com_pins = 0x02;
    }
    else
    {
        return HAL_ERROR;
    }
    
    if (config.chip_type == OLED_TYPE_SSD1306)
    {
        config.col_offset = 0;
        if (config.size_type == OLED_SIZE_128X64)
        {
            config.contrast = 0xCF;
        }
        else
        {
            config.contrast = 0x8F;
        }
    }
    else if (config.chip_type == OLED_TYPE_SH1106)
    {
        config.col_offset = 2;  /* SH1106需要2像素偏移 */
        config.contrast = 0x80;
    }
    else
    {
        return HAL_ERROR;
    }
    
    return OLED_Init(holed, hi2c, i2c_addr, &config, framebuffer);
}

/**
  * @brief  反初始化OLED驱动
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_DeInit(OLED_HandleTypeDef *holed)
{
    if (holed == NULL || !holed->is_initialized)
    {
        return HAL_ERROR;
    }
    
    /* 关闭显示 */
    OLED_SetDisplayOn(holed, false);
    
    holed->is_initialized = false;
    
    return HAL_OK;
}

/**
  * @brief  清空帧缓冲区
  * @param  holed: OLED句柄指针
  */
void OLED_Clear(OLED_HandleTypeDef *holed)
{
    if (holed == NULL || holed->framebuffer == NULL)
    {
        return;
    }
    
    memset(holed->framebuffer, 0, holed->framebuffer_size);
    holed->full_refresh = true;
    OLED_ResetDirtyRegion(holed);
}

/**
  * @brief  刷新显示（全屏刷新）
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_Refresh(OLED_HandleTypeDef *holed)
{
    HAL_StatusTypeDef status;
    uint8_t page;
    uint8_t col_start, col_end;
    uint8_t col_offset;
    
    if (holed == NULL || !holed->is_initialized)
    {
        return HAL_ERROR;
    }
    
    col_offset = holed->config.col_offset;
    col_start = col_offset;
    col_end = col_offset + holed->width - 1;
    
    /* 逐页刷新 */
    for (page = 0; page < holed->pages; page++)
    {
        if (holed->config.chip_type == OLED_TYPE_SSD1306)
        {
            /* SSD1306使用列地址命令 */
            status = holed->hw.send_cmd(holed, OLED_CMD_COLUMN_ADDR);
            status = holed->hw.send_cmd(holed, col_start);
            status = holed->hw.send_cmd(holed, col_end);
            holed->hw.delay_ms(1);
            
            /* 设置页地址 */
            status = holed->hw.send_cmd(holed, OLED_CMD_PAGE_ADDR);
            status = holed->hw.send_cmd(holed, page);
            status = holed->hw.send_cmd(holed, page);
            holed->hw.delay_ms(1);
        }
        else if (holed->config.chip_type == OLED_TYPE_SH1106)
        {
            /* SH1106使用页地址和列地址低4位+高4位 */
            status = holed->hw.send_cmd(holed, 0xB0 + page);  /* 设置页地址 */
            holed->hw.delay_ms(1);
            
            status = holed->hw.send_cmd(holed, 0x00 + (col_start & 0x0F));  /* 列地址低4位 */
            status = holed->hw.send_cmd(holed, 0x10 + ((col_start >> 4) & 0x0F));  /* 列地址高4位 */
            holed->hw.delay_ms(1);
        }
        
        /* 发送该页的数据 */
        status = holed->hw.send_data(holed, &holed->framebuffer[page * holed->width], holed->width);
    }
    
    holed->full_refresh = false;
    OLED_ResetDirtyRegion(holed);
    
    return status;
}

/**
  * @brief  刷新脏区域（增量刷新）
  * @param  holed: OLED句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef OLED_RefreshDirty(OLED_HandleTypeDef *holed)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t page_start, page_end;
    uint8_t page;
    uint8_t col_start, col_end;
    uint8_t col_offset;
    uint8_t *page_data;
    uint16_t data_len;
    
    if (holed == NULL || !holed->is_initialized)
    {
        return HAL_ERROR;
    }
    
    /* 如果需要全屏刷新 */
    if (holed->full_refresh)
    {
        return OLED_Refresh(holed);
    }
    
    /* 如果没有脏区域，直接返回 */
    if (!holed->dirty_region.is_dirty)
    {
        return HAL_OK;
    }
    
    /* 等待DMA完成 */
    while (holed->dma_busy)
    {
        if (!OLED_IsDMAReady(holed))
        {
            HAL_Delay(1);
        }
        else
        {
            break;
        }
    }
    
    col_offset = holed->config.col_offset;
    
    /* 计算需要刷新的页范围 */
    page_start = holed->dirty_region.y_min / 8;
    page_end = holed->dirty_region.y_max / 8;
    
    /* 计算列范围（加上偏移） */
    col_start = col_offset + holed->dirty_region.x_min;
    col_end = col_offset + holed->dirty_region.x_max;
    
    /* 逐页刷新脏区域 */
    for (page = page_start; page <= page_end; page++)
    {
        if (holed->config.chip_type == OLED_TYPE_SSD1306)
        {
            /* SSD1306使用列地址命令 */
            status = holed->hw.send_cmd(holed, OLED_CMD_COLUMN_ADDR);
            status = holed->hw.send_cmd(holed, col_start);
            status = holed->hw.send_cmd(holed, col_end);
            holed->hw.delay_ms(1);
            
            /* 设置页地址 */
            status = holed->hw.send_cmd(holed, OLED_CMD_PAGE_ADDR);
            status = holed->hw.send_cmd(holed, page);
            status = holed->hw.send_cmd(holed, page);
            holed->hw.delay_ms(1);
        }
        else if (holed->config.chip_type == OLED_TYPE_SH1106)
        {
            /* SH1106使用页地址和列地址低4位+高4位 */
            status = holed->hw.send_cmd(holed, 0xB0 + page);  /* 设置页地址 */
            holed->hw.delay_ms(1);
            
            status = holed->hw.send_cmd(holed, 0x00 + (col_start & 0x0F));  /* 列地址低4位 */
            status = holed->hw.send_cmd(holed, 0x10 + ((col_start >> 4) & 0x0F));  /* 列地址高4位 */
            holed->hw.delay_ms(1);
        }
        
        /* 计算该页的数据长度和起始位置 */
        data_len = holed->dirty_region.x_max - holed->dirty_region.x_min + 1;
        page_data = &holed->framebuffer[page * holed->width + holed->dirty_region.x_min];
        
        /* 发送该页的数据 */
        status = holed->hw.send_data(holed, page_data, data_len);
        
        /* 等待DMA完成 */
        while (holed->dma_busy)
        {
            if (!OLED_IsDMAReady(holed))
            {
                HAL_Delay(1);
            }
            else
            {
                break;
            }
        }
    }
    
    /* 重置脏区域 */
    OLED_ResetDirtyRegion(holed);
    
    return status;
}

/**
  * @brief  设置像素点
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  color: 颜色（0=黑色，1=白色）
  */
void OLED_SetPixel(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t color)
{
    uint16_t index;
    uint8_t page, bit_pos;
    
    if (holed == NULL || holed->framebuffer == NULL || 
        x >= holed->width || y >= holed->height)
    {
        return;
    }
    
    page = y / 8;
    bit_pos = y % 8;
    index = page * holed->width + x;
    
    /* 边界检查，确保索引不越界 */
    if (index >= holed->framebuffer_size)
    {
        return;
    }
    
    if (color)
    {
        holed->framebuffer[index] |= (1 << bit_pos);
    }
    else
    {
        holed->framebuffer[index] &= ~(1 << bit_pos);
    }
    
    /* 更新脏矩形区域 */
    OLED_ExpandDirtyRegion(holed, x, y);
}

/**
  * @brief  获取像素点
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @retval 像素值（0或1）
  */
uint8_t OLED_GetPixel(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y)
{
    uint16_t index;
    uint8_t page, bit_pos;
    
    if (holed == NULL || holed->framebuffer == NULL || 
        x >= holed->width || y >= holed->height)
    {
        return 0;
    }
    
    page = y / 8;
    bit_pos = y % 8;
    index = page * holed->width + x;
    
    /* 边界检查，确保索引不越界 */
    if (index >= holed->framebuffer_size)
    {
        return 0;
    }
    
    return (holed->framebuffer[index] >> bit_pos) & 0x01;
}

/**
  * @brief  绘制水平线
  * @param  holed: OLED句柄指针
  * @param  x: 起始X坐标
  * @param  y: Y坐标
  * @param  w: 宽度
  * @param  color: 颜色
  */
void OLED_DrawHLine(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t color)
{
    uint8_t i;
    
    if (holed == NULL || y >= holed->height)
    {
        return;
    }
    
    for (i = 0; i < w && (x + i) < holed->width; i++)
    {
        OLED_SetPixel(holed, x + i, y, color);
    }
}

/**
  * @brief  绘制垂直线
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: 起始Y坐标
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_DrawVLine(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t h, uint8_t color)
{
    uint8_t i;
    
    if (holed == NULL || x >= holed->width)
    {
        return;
    }
    
    for (i = 0; i < h && (y + i) < holed->height; i++)
    {
        OLED_SetPixel(holed, x, y + i, color);
    }
}

/**
  * @brief  绘制矩形框
  * @param  holed: OLED句柄指针
  * @param  x: 左上角X坐标
  * @param  y: 左上角Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_DrawRect(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    OLED_DrawHLine(holed, x, y, w, color);
    OLED_DrawHLine(holed, x, y + h - 1, w, color);
    OLED_DrawVLine(holed, x, y, h, color);
    OLED_DrawVLine(holed, x + w - 1, y, h, color);
}

/**
  * @brief  填充矩形
  * @param  holed: OLED句柄指针
  * @param  x: 左上角X坐标
  * @param  y: 左上角Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  color: 颜色
  */
void OLED_FillRect(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    uint8_t i, j;
    
    if (holed == NULL)
    {
        return;
    }
    
    for (i = 0; i < h && (y + i) < holed->height; i++)
    {
        for (j = 0; j < w && (x + j) < holed->width; j++)
        {
            OLED_SetPixel(holed, x + j, y + i, color);
        }
    }
}

/**
  * @brief  绘制圆
  * @param  holed: OLED句柄指针
  * @param  x0: 圆心X坐标
  * @param  y0: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_DrawCircle(OLED_HandleTypeDef *holed, uint8_t x0, uint8_t y0, uint8_t r, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    OLED_SetPixel(holed, x0, y0 + r, color);
    OLED_SetPixel(holed, x0, y0 - r, color);
    OLED_SetPixel(holed, x0 + r, y0, color);
    OLED_SetPixel(holed, x0 - r, y0, color);
    
    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        OLED_SetPixel(holed, x0 + x, y0 + y, color);
        OLED_SetPixel(holed, x0 - x, y0 + y, color);
        OLED_SetPixel(holed, x0 + x, y0 - y, color);
        OLED_SetPixel(holed, x0 - x, y0 - y, color);
        OLED_SetPixel(holed, x0 + y, y0 + x, color);
        OLED_SetPixel(holed, x0 - y, y0 + x, color);
        OLED_SetPixel(holed, x0 + y, y0 - x, color);
        OLED_SetPixel(holed, x0 - y, y0 - x, color);
    }
}

/**
  * @brief  填充圆
  * @param  holed: OLED句柄指针
  * @param  x0: 圆心X坐标
  * @param  y0: 圆心Y坐标
  * @param  r: 半径
  * @param  color: 颜色
  */
void OLED_FillCircle(OLED_HandleTypeDef *holed, uint8_t x0, uint8_t y0, uint8_t r, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    OLED_DrawVLine(holed, x0, y0 - r, 2 * r + 1, color);
    
    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        OLED_DrawVLine(holed, x0 + x, y0 - y, 2 * y + 1, color);
        OLED_DrawVLine(holed, x0 - x, y0 - y, 2 * y + 1, color);
        OLED_DrawVLine(holed, x0 + y, y0 - x, 2 * x + 1, color);
        OLED_DrawVLine(holed, x0 - y, y0 - x, 2 * x + 1, color);
    }
}

/**
  * @brief  绘制字符（ASCII，6x8字体）
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  ch: 字符
  * @param  color: 颜色
  */
void OLED_DrawChar(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, char ch, uint8_t color)
{
    uint8_t i, j;
    uint8_t font_index;
    uint8_t font_data;
    
    if (holed == NULL || ch < 32 || ch > 126)
    {
        return;
    }
    
    font_index = ch - 32;
    
    for (i = 0; i < 6; i++)
    {
        font_data = font_6x8[font_index][i];
        for (j = 0; j < 8; j++)
        {
            if (font_data & (1 << j))
            {
                OLED_SetPixel(holed, x + i, y + j, color);
            }
            else
            {
                OLED_SetPixel(holed, x + i, y + j, !color);
            }
        }
    }
}

/**
  * @brief  绘制字符串
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  str: 字符串
  * @param  color: 颜色
  */
void OLED_DrawString(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, const char *str, uint8_t color)
{
    uint8_t pos_x = x;
    
    if (holed == NULL || str == NULL)
    {
        return;
    }
    
    while (*str)
    {
        OLED_DrawChar(holed, pos_x, y, *str, color);
        pos_x += 6;
        if (pos_x >= holed->width)
        {
            break;
        }
        str++;
    }
}

/**
  * @brief  设置显示方向
  * @param  holed: OLED句柄指针
  * @param  direction: 方向
  */
void OLED_SetDirection(OLED_HandleTypeDef *holed, OLED_DirectionTypeDef direction)
{
    if (holed == NULL || !holed->is_initialized)
    {
        return;
    }
    
    holed->direction = direction;
    
    if (direction == OLED_DIRECTION_NORMAL)
    {
        holed->hw.send_cmd(holed, OLED_CMD_SEG_REMAP | 0x01);
        holed->hw.send_cmd(holed, OLED_CMD_COM_SCAN_DEC);
    }
    else
    {
        holed->hw.send_cmd(holed, OLED_CMD_SEG_REMAP | 0x00);
        holed->hw.send_cmd(holed, OLED_CMD_COM_SCAN_INC);
    }
}

/**
  * @brief  设置对比度
  * @param  holed: OLED句柄指针
  * @param  contrast: 对比度值（0-255）
  */
void OLED_SetContrast(OLED_HandleTypeDef *holed, uint8_t contrast)
{
    if (holed == NULL || !holed->is_initialized)
    {
        return;
    }
    
    holed->hw.send_cmd(holed, OLED_CMD_SET_CONTRAST);
    holed->hw.send_cmd(holed, contrast);
}

/**
  * @brief  设置显示开关
  * @param  holed: OLED句柄指针
  * @param  on: 是否开启（true=开启，false=关闭）
  */
void OLED_SetDisplayOn(OLED_HandleTypeDef *holed, bool on)
{
    if (holed == NULL || !holed->is_initialized)
    {
        return;
    }
    
    if (on)
    {
        holed->hw.send_cmd(holed, OLED_CMD_DISPLAY_ON);
    }
    else
    {
        holed->hw.send_cmd(holed, OLED_CMD_DISPLAY_OFF);
    }
}

/**
  * @brief  更新脏矩形区域
  * @param  holed: OLED句柄指针
  * @param  x: X坐标
  * @param  y: Y坐标
  * @param  w: 宽度
  * @param  h: 高度
  */
void OLED_UpdateDirtyRegion(OLED_HandleTypeDef *holed, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i, j;
    
    if (holed == NULL)
    {
        return;
    }
    
    for (i = 0; i < h && (y + i) < holed->height; i++)
    {
        for (j = 0; j < w && (x + j) < holed->width; j++)
        {
            OLED_ExpandDirtyRegion(holed, x + j, y + i);
        }
    }
}

/**
  * @brief  检查DMA是否完成
  * @param  holed: OLED句柄指针
  * @retval true=完成，false=未完成
  */
bool OLED_IsDMAReady(OLED_HandleTypeDef *holed)
{
    if (holed == NULL || holed->hi2c == NULL)
    {
        return true;
    }
    
    if (holed->hi2c->State == HAL_I2C_STATE_READY)
    {
        holed->dma_busy = false;
        return true;
    }
    
    return false;
}

/* USER CODE BEGIN 1 */

/**
  * @brief  I2C DMA传输完成回调（需要在HAL_I2C_MasterTxCpltCallback中调用）
  * @param  holed: OLED句柄指针
  */
void OLED_DMATxCpltCallback(OLED_HandleTypeDef *holed)
{
    if (holed != NULL)
    {
        holed->dma_busy = false;
    }
}

/* USER CODE END 1 */

