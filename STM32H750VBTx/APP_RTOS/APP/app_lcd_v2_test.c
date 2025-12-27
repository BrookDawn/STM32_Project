/**
 * @file app_lcd_v2_test.c
 * @brief High-performance LCD SPI DMA Driver v2 performance tests
 */

#include "lcd_spi_dma.h"
#include "lcd_spi_154.h"
#include <stdio.h>
#include "cmsis_os2.h"
#include "usart.h"
#include <string.h>

/**
 * @brief 运行LCD v2性能测试
 * @param hlcd LCD DMA句柄
 */
void LCD_V2_Performance_Test(LCD_SPI_DMA_Handle_t *hlcd)
{
    uint32_t start, end;
    char log_buf[128];

    // 1. 全屏填充测试 (240x240 pixels)
    HAL_UART_Transmit(&huart1, (uint8_t*)"[Test] Starting Full Screen Fill (v2)...\r\n", 42, 100);

    start = HAL_GetTick();
    for(int i = 0; i < 100; i++) {
        LCD_DMA_Clear(hlcd, (i % 2) ? 0xF800 : 0x07E0); // 红绿交替
    }
    end = HAL_GetTick();

    float time_per_frame = (float)(end - start) / 100.0f;
    float fps = 1000.0f / time_per_frame;

    snprintf(log_buf, sizeof(log_buf), "[Test] Full Fill: %.2f ms/frame, %.1f FPS\r\n", time_per_frame, fps);
    HAL_UART_Transmit(&huart1, (uint8_t*)log_buf, strlen(log_buf), 100);

    // 2. 局部矩形流水线测试
    HAL_UART_Transmit(&huart1, (uint8_t*)"[Test] Starting 100x100 Rect Pipelining...\r\n", 44, 100);

    start = HAL_GetTick();
    for(int i = 0; i < 500; i++) {
        LCD_DMA_FillRect(hlcd, 70, 70, 100, 100, (i % 2) ? 0x001F : 0xFFE0); // 蓝黄交替
    }
    end = HAL_GetTick();

    time_per_frame = (float)(end - start) / 500.0f;
    snprintf(log_buf, sizeof(log_buf), "[Test] 100x100 Rect: %.2f ms/operation\r\n", time_per_frame);
    HAL_UART_Transmit(&huart1, (uint8_t*)log_buf, strlen(log_buf), 100);
}

/**
 * @brief 使用示例：绘制一个带渐变效果的矩形
 */
void LCD_V2_Example_Gradient(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    LCD_SetAddress(x, y, x + w - 1, y + h - 1);

    uint32_t total_pixels = (uint32_t)w * h;
    uint32_t remaining = total_pixels;

    uint16_t r = 0, g = 0, b = 0;

    while (remaining > 0) {
        uint32_t transfer_size = (remaining > hlcd->dma_buffer_size) ? hlcd->dma_buffer_size : remaining;

        // 在CPU填充下一个缓冲区的同时，DMA可能正在传输前一个
        for (uint32_t i = 0; i < transfer_size; i++) {
            // 简单的渐变逻辑
            r = (remaining - i) * 31 / total_pixels;
            hlcd->dma_buffer[hlcd->current_buffer][i] = (r << 11) | (g << 5) | b;
        }

        LCD_SPI_DMA_WriteBuffer_Async(hlcd, hlcd->dma_buffer[hlcd->current_buffer], transfer_size);
        hlcd->current_buffer = (hlcd->current_buffer + 1) % 2;

        remaining -= transfer_size;
    }

    LCD_SPI_DMA_WaitComplete(hlcd);
    LCD_CS_Deselect;
}
