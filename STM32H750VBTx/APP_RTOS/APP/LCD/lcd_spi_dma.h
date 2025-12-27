/**
 ******************************************************************************
 * @file    lcd_spi_dma.h
 * @brief   Optimized LCD SPI driver with DMA support
 * @author  Modified for high performance
 ******************************************************************************
 */

#ifndef __LCD_SPI_DMA_H
#define __LCD_SPI_DMA_H

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"

/* LCD屏幕尺寸 */
#define LCD_WIDTH     240
#define LCD_HEIGHT    240

/* DMA传输缓冲区配置 */
#define LCD_DMA_BUFFER_SIZE    (LCD_WIDTH * 32)  // 32行缓冲（15KB，可根据RAM调整）
#define LCD_FRAME_BUFFER_SIZE  (LCD_WIDTH * LCD_HEIGHT)  // 完整帧缓冲（115KB）

/* LCD SPI操作结构体 */
typedef struct {
    SPI_HandleTypeDef *hspi;          // SPI句柄
    DMA_HandleTypeDef *hdma_tx;       // DMA TX句柄
    volatile bool dma_busy;           // DMA传输忙标志
    uint16_t *dma_buffer[2];          // 双DMA缓冲区指针 (v2)
    uint8_t current_buffer;           // 当前正在使用的缓冲区索引 (v2)
    uint32_t dma_buffer_size;         // DMA缓冲区大小
    uint16_t *frame_buffer;           // 帧缓冲区指针（可选）
    bool frame_buffer_enabled;        // 帧缓冲模式启用标志
    void (*tc_callback)(void);        // 传输完成回调 (v2)
    osThreadId_t task_to_notify;      // 用于非阻塞同步的任务句柄 (v2)
} LCD_SPI_DMA_Handle_t;

/* LCD SPI DMA操作函数 */
void LCD_SPI_DMA_Init(LCD_SPI_DMA_Handle_t *hlcd, SPI_HandleTypeDef *hspi);
void LCD_SPI_DMA_DeInit(LCD_SPI_DMA_Handle_t *hlcd);

/* DMA传输函数 */
HAL_StatusTypeDef LCD_SPI_DMA_WriteCommand(LCD_SPI_DMA_Handle_t *hlcd, uint8_t cmd);
HAL_StatusTypeDef LCD_SPI_DMA_WriteData8(LCD_SPI_DMA_Handle_t *hlcd, uint8_t data);
HAL_StatusTypeDef LCD_SPI_DMA_WriteData16(LCD_SPI_DMA_Handle_t *hlcd, uint16_t data);
HAL_StatusTypeDef LCD_SPI_DMA_WriteBuffer(LCD_SPI_DMA_Handle_t *hlcd, uint16_t *pData, uint32_t length);
HAL_StatusTypeDef LCD_SPI_DMA_WriteBuffer_Async(LCD_SPI_DMA_Handle_t *hlcd, uint16_t *pData, uint32_t length); // (v2)

/* 等待DMA传输完成 */
void LCD_SPI_DMA_WaitComplete(LCD_SPI_DMA_Handle_t *hlcd);

/* 帧缓冲相关函数 */
HAL_StatusTypeDef LCD_SPI_DMA_EnableFrameBuffer(LCD_SPI_DMA_Handle_t *hlcd);
void LCD_SPI_DMA_DisableFrameBuffer(LCD_SPI_DMA_Handle_t *hlcd);
HAL_StatusTypeDef LCD_SPI_DMA_FlushFrameBuffer(LCD_SPI_DMA_Handle_t *hlcd);

/* DMA传输完成回调（由用户在stm32h7xx_it.c中调用） */
void LCD_SPI_DMA_TxCpltCallback(LCD_SPI_DMA_Handle_t *hlcd);

/* 高性能绘图函数 */
void LCD_DMA_FillRect(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y,
                      uint16_t width, uint16_t height, uint16_t color);
void LCD_DMA_Clear(LCD_SPI_DMA_Handle_t *hlcd, uint16_t color);
void LCD_DMA_DrawImage(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, const uint16_t *image);

/* 帧缓冲绘图函数 */
void LCD_FB_SetPixel(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t color);
void LCD_FB_FillRect(LCD_SPI_DMA_Handle_t *hlcd, uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height, uint16_t color);
void LCD_FB_Clear(LCD_SPI_DMA_Handle_t *hlcd, uint16_t color);

#endif /* __LCD_SPI_DMA_H */
