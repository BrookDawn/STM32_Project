/**
 ******************************************************************************
 * @file    lcd_spi_dma.c
 * @brief   Optimized LCD SPI driver with DMA support
 ******************************************************************************
 */

#include "lcd_spi_dma.h"
#include "lcd_spi_154.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "usart.h"

/* 静态DMA缓冲区 - 放在D2 SRAM (SRAM1/2)，该区域通常配置为Non-Cacheable，适合DMA */
__attribute__((section(".ram_d2"))) __attribute__((aligned(32))) static uint16_t lcd_dma_buffer0[LCD_DMA_BUFFER_SIZE];
__attribute__((section(".ram_d2"))) __attribute__((aligned(32))) static uint16_t lcd_dma_buffer1[LCD_DMA_BUFFER_SIZE];

/* 可选的帧缓冲区 - 115KB，放在D2 SRAM */
__attribute__((section(".ram_d2"))) __attribute__((aligned(32))) static uint16_t lcd_frame_buffer[LCD_FRAME_BUFFER_SIZE];

/**
 * @brief 初始化LCD SPI DMA操作句柄
 */
void LCD_SPI_DMA_Init(LCD_SPI_DMA_Handle_t *hlcd, SPI_HandleTypeDef *hspi)
{
    hlcd->hspi = hspi;
    hlcd->hdma_tx = hspi->hdmatx;
    hlcd->dma_busy = false;
    hlcd->dma_buffer[0] = lcd_dma_buffer0;
    hlcd->dma_buffer[1] = lcd_dma_buffer1;
    hlcd->current_buffer = 0;
    hlcd->dma_buffer_size = LCD_DMA_BUFFER_SIZE;
    hlcd->frame_buffer = NULL;
    hlcd->frame_buffer_enabled = false;
    hlcd->tc_callback = NULL;
    hlcd->task_to_notify = NULL;  // 添加：初始化任务通知句柄
}

/**
 * @brief 反初始化LCD SPI DMA句柄
 */
void LCD_SPI_DMA_DeInit(LCD_SPI_DMA_Handle_t *hlcd)
{
    LCD_SPI_DMA_WaitComplete(hlcd);
    LCD_SPI_DMA_DisableFrameBuffer(hlcd);
    hlcd->hspi = NULL;
    hlcd->hdma_tx = NULL;
}

/**
 * @brief 等待DMA传输完成 - 简化版（轮询模式）
 */
void LCD_SPI_DMA_WaitComplete(LCD_SPI_DMA_Handle_t *hlcd)
{
    if (!hlcd->dma_busy) return;

    // 使用轮询方式等待DMA完成
    uint32_t timeout = HAL_GetTick() + 2000;  // 2秒超时

    while (hlcd->dma_busy && (HAL_GetTick() < timeout)) {
        // 让出CPU给其他任务
        osDelay(1);
    }

    if (hlcd->dma_busy) {
        // 超时 - 强制停止
        extern UART_HandleTypeDef huart1;
        HAL_UART_Transmit(&huart1, (uint8_t*)"[DMA] Timeout!\r\n", 16, 100);
        HAL_SPI_DMAStop(hlcd->hspi);
        hlcd->dma_busy = false;
    }
}

/**
 * @brief DMA传输完成回调 (内部) - 简化版
 */
void LCD_SPI_DMA_TxCpltCallback(LCD_SPI_DMA_Handle_t *hlcd)
{
    extern UART_HandleTypeDef huart1;
    HAL_UART_Transmit(&huart1, (uint8_t*)".", 1, 10);  // 简短的完成标记

    hlcd->dma_busy = false;

    if (hlcd->tc_callback != NULL) {
        hlcd->tc_callback();
    }
}

/**
 * @brief 映射到 HAL 的回调函数 - 简化版
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    extern LCD_SPI_DMA_Handle_t hlcd_dma;
    if (hspi->Instance == SPI4) {
        LCD_SPI_DMA_TxCpltCallback(&hlcd_dma);
    }
}

/**
 * @brief 使用DMA发送命令（8位）- 非阻塞优化版
 */
HAL_StatusTypeDef LCD_SPI_DMA_WriteCommand(LCD_SPI_DMA_Handle_t *hlcd, uint8_t cmd)
{
    LCD_SPI_DMA_WaitComplete(hlcd);

    LCD_CS_Select;
    LCD_DC_Command;

    // 切换到8位数据模式（仅在必要时）
    static bool is_8bit_mode = true;  // 假设初始为8位
    if (!is_8bit_mode) {
        hlcd->hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        HAL_SPI_Init(hlcd->hspi);
        is_8bit_mode = true;
    }

    // 使用轮询方式发送单字节命令（速度快）
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hlcd->hspi, &cmd, 1, 100);

    LCD_CS_Deselect;
    return status;
}

/**
 * @brief 使用DMA发送8位数据 - 非阻塞优化版
 */
HAL_StatusTypeDef LCD_SPI_DMA_WriteData8(LCD_SPI_DMA_Handle_t *hlcd, uint8_t data)
{
    LCD_SPI_DMA_WaitComplete(hlcd);

    LCD_CS_Select;
    LCD_DC_Data;

    // 切换到8位数据模式（仅在必要时）
    static bool is_8bit_mode = true;
    if (!is_8bit_mode) {
        hlcd->hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        HAL_SPI_Init(hlcd->hspi);
        is_8bit_mode = true;
    }

    // 使用轮询方式发送单字节数据
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hlcd->hspi, &data, 1, 100);

    LCD_CS_Deselect;
    return status;
}

/**
 * @brief 使用DMA发送16位数据
 */
HAL_StatusTypeDef LCD_SPI_DMA_WriteData16(LCD_SPI_DMA_Handle_t *hlcd, uint16_t data)
{
    LCD_SPI_DMA_WaitComplete(hlcd);

    LCD_CS_Select;
    LCD_DC_Data;

    if (hlcd->hspi->Init.DataSize != SPI_DATASIZE_16BIT) {
        hlcd->hspi->Init.DataSize = SPI_DATASIZE_16BIT;
        HAL_SPI_Init(hlcd->hspi);
    }

    HAL_StatusTypeDef status = HAL_SPI_Transmit(hlcd->hspi, (uint8_t*)&data, 1, 100);

    LCD_CS_Deselect;
    return status;
}

/**
 * @brief 使用DMA批量发送16位数据缓冲区（核心优化函数）
 * @param pData 数据指针
 * @param length 数据长度（16位字数量）
 */
HAL_StatusTypeDef LCD_SPI_DMA_WriteBuffer(LCD_SPI_DMA_Handle_t *hlcd, uint16_t *pData, uint32_t length)
{
    HAL_StatusTypeDef status = LCD_SPI_DMA_WriteBuffer_Async(hlcd, pData, length);
    if (status == HAL_OK) {
        LCD_SPI_DMA_WaitComplete(hlcd);
        LCD_CS_Deselect;
    }
    return status;
}

/**
 * @brief 异步发送16位数据缓冲区 - 简化优化版
 */
HAL_StatusTypeDef LCD_SPI_DMA_WriteBuffer_Async(LCD_SPI_DMA_Handle_t *hlcd, uint16_t *pData, uint32_t length)
{
    // 等待上一次DMA传输完成
    LCD_SPI_DMA_WaitComplete(hlcd);

    LCD_CS_Select;
    LCD_DC_Data;

    // 切换到16位数据模式
    static bool is_16bit_mode = false;
    if (!is_16bit_mode) {
        hlcd->hspi->Init.DataSize = SPI_DATASIZE_16BIT;
        HAL_SPI_Init(hlcd->hspi);
        is_16bit_mode = true;
    }

    // 标记DMA忙
    hlcd->dma_busy = true;

    // 由于使用D2 SRAM (Non-Cacheable)，不需要Cache维护
    // 如果使用AXI SRAM，取消注释下面这行：
    // SCB_CleanDCache_by_Addr((uint32_t*)pData, length * sizeof(uint16_t));

    // 启动DMA传输
    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(hlcd->hspi, (uint8_t*)pData, length);

    if (status != HAL_OK) {
        hlcd->dma_busy = false;
        LCD_CS_Deselect;

        extern UART_HandleTypeDef huart1;
        char msg[32];
        snprintf(msg, sizeof(msg), "[DMA] Err:%d\r\n", status);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }

    return status;
}

/**
 * @brief 启用帧缓冲模式
 */
HAL_StatusTypeDef LCD_SPI_DMA_EnableFrameBuffer(LCD_SPI_DMA_Handle_t *hlcd)
{
    if (hlcd->frame_buffer_enabled) {
        return HAL_OK;  // 已经启用
    }

    hlcd->frame_buffer = lcd_frame_buffer;
    hlcd->frame_buffer_enabled = true;

    // 清空帧缓冲
    memset(hlcd->frame_buffer, 0, LCD_FRAME_BUFFER_SIZE * sizeof(uint16_t));

    return HAL_OK;
}

/**
 * @brief 禁用帧缓冲模式
 */
void LCD_SPI_DMA_DisableFrameBuffer(LCD_SPI_DMA_Handle_t *hlcd)
{
    if (hlcd->frame_buffer_enabled) {
        hlcd->frame_buffer = NULL;
        hlcd->frame_buffer_enabled = false;
    }
}

/**
 * @brief 刷新帧缓冲到LCD（整屏DMA传输）
 */
HAL_StatusTypeDef LCD_SPI_DMA_FlushFrameBuffer(LCD_SPI_DMA_Handle_t *hlcd)
{
    if (!hlcd->frame_buffer_enabled || hlcd->frame_buffer == NULL) {
        return HAL_ERROR;
    }

    // 设置全屏地址
    LCD_SetAddress(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    // 分块传输帧缓冲（避免单次DMA传输过大）
    const uint32_t chunk_size = LCD_DMA_BUFFER_SIZE;  // 每次传输32行
    uint32_t remaining = LCD_FRAME_BUFFER_SIZE;
    uint16_t *src = hlcd->frame_buffer;

    while (remaining > 0) {
        uint32_t transfer_size = (remaining > chunk_size) ? chunk_size : remaining;

        HAL_StatusTypeDef status = LCD_SPI_DMA_WriteBuffer(hlcd, src, transfer_size);
        if (status != HAL_OK) {
            return status;
        }

        src += transfer_size;
        remaining -= transfer_size;
    }

    return HAL_OK;
}

/**
 * @brief 使用DMA填充矩形区域（高性能v2：双缓冲流水线）
 */
void LCD_DMA_FillRect(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y,
                      uint16_t width, uint16_t height, uint16_t color)
{
    if (hlcd->frame_buffer_enabled) {
        // 帧缓冲模式 - 直接写入帧缓冲
        LCD_FB_FillRect(hlcd, x, y, width, height, color);
        return;
    }

    // 直接模式 - DMA传输到LCD
    LCD_SetAddress(x, y, x + width - 1, y + height - 1);

    uint32_t total_pixels = (uint32_t)width * height;

    // 预先填充两个缓冲区（对于单色填充，其实只需要填一次，这里为了演示流水线通用性）
    uint32_t buffer_pixels = (total_pixels > hlcd->dma_buffer_size) ? hlcd->dma_buffer_size : total_pixels;
    for (uint32_t i = 0; i < buffer_pixels; i++) {
        hlcd->dma_buffer[0][i] = color;
        hlcd->dma_buffer[1][i] = color;
    }

    // 分块DMA传输
    uint32_t remaining = total_pixels;
    while (remaining > 0) {
        uint32_t transfer_size = (remaining > hlcd->dma_buffer_size) ? hlcd->dma_buffer_size : remaining;

        // 使用异步传输和双缓冲切换
        LCD_SPI_DMA_WriteBuffer_Async(hlcd, hlcd->dma_buffer[hlcd->current_buffer], transfer_size);

        // 切换缓冲区索引
        hlcd->current_buffer = (hlcd->current_buffer + 1) % 2;

        remaining -= transfer_size;
    }

    // 等待最后一次传输完成
    LCD_SPI_DMA_WaitComplete(hlcd);
    LCD_CS_Deselect;
}

/**
 * @brief 使用DMA清屏（高性能）
 */
void LCD_DMA_Clear(LCD_SPI_DMA_Handle_t *hlcd, uint16_t color)
{
    LCD_DMA_FillRect(hlcd, 0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

/**
 * @brief 使用DMA绘制图像（高性能v2：双缓冲流水线）
 */
void LCD_DMA_DrawImage(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, const uint16_t *image)
{
    if (hlcd->frame_buffer_enabled) {
        // 帧缓冲模式 - 复制到帧缓冲
        for (uint16_t row = 0; row < height; row++) {
            uint32_t fb_offset = (y + row) * LCD_WIDTH + x;
            uint32_t img_offset = row * width;
            memcpy(&hlcd->frame_buffer[fb_offset], &image[img_offset], width * sizeof(uint16_t));
        }
        return;
    }

    // 直接模式 - DMA传输到LCD
    LCD_SetAddress(x, y, x + width - 1, y + height - 1);

    uint32_t total_pixels = (uint32_t)width * height;
    uint32_t remaining = total_pixels;
    const uint16_t *src = image;

    while (remaining > 0) {
        uint32_t transfer_size = (remaining > hlcd->dma_buffer_size) ? hlcd->dma_buffer_size : remaining;

        // 1. 等待上一个缓冲区传输完成 (如果是第一次调用，WriteBuffer_Async 内部会等待)
        // 其实 WriteBuffer_Async 内部已经调用了 WaitComplete，所以这里可以直接复制

        // 2. 将数据复制到“下一个”缓冲区（当前非忙的那个）
        // 实际上 WriteBuffer_Async 等待的是 dma_busy，所以只要它返回，
        // 说明我们可以开始准备下一个缓冲区了
        memcpy(hlcd->dma_buffer[hlcd->current_buffer], src, transfer_size * sizeof(uint16_t));

        // 3. 异步启动传输
        LCD_SPI_DMA_WriteBuffer_Async(hlcd, hlcd->dma_buffer[hlcd->current_buffer], transfer_size);

        // 4. 切换索引
        hlcd->current_buffer = (hlcd->current_buffer + 1) % 2;

        src += transfer_size;
        remaining -= transfer_size;
    }

    // 等待最后一次完成
    LCD_SPI_DMA_WaitComplete(hlcd);
    LCD_CS_Deselect;
}

/* ==================== 帧缓冲绘图函数 ==================== */

/**
 * @brief 在帧缓冲中设置像素
 */
void LCD_FB_SetPixel(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t color)
{
    if (!hlcd->frame_buffer_enabled || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }

    hlcd->frame_buffer[y * LCD_WIDTH + x] = color;
}

/**
 * @brief 在帧缓冲中填充矩形
 */
void LCD_FB_FillRect(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height, uint16_t color)
{
    if (!hlcd->frame_buffer_enabled) {
        return;
    }

    // 边界检查
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + width > LCD_WIDTH) width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT) height = LCD_HEIGHT - y;

    // 填充矩形
    for (uint16_t row = 0; row < height; row++) {
        uint32_t offset = (y + row) * LCD_WIDTH + x;
        for (uint16_t col = 0; col < width; col++) {
            hlcd->frame_buffer[offset + col] = color;
        }
    }
}

/**
 * @brief 清空帧缓冲
 */
void LCD_FB_Clear(LCD_SPI_DMA_Handle_t *hlcd, uint16_t color)
{
    if (!hlcd->frame_buffer_enabled) {
        return;
    }

    for (uint32_t i = 0; i < LCD_FRAME_BUFFER_SIZE; i++) {
        hlcd->frame_buffer[i] = color;
    }
}
