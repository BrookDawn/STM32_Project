/**
 ******************************************************************************
 * @file    lcd_spi_dma_v2.h
 * @brief   高性能LCD SPI DMA驱动 - 支持双缓冲、流水线传输、零等待刷屏
 * @version V2.0
 * @date    2025-12-27
 ******************************************************************************
 * @features
 *   - 双缓冲DMA传输，CPU和DMA可并行工作
 *   - 流水线传输，无需等待上一次DMA完成
 *   - 整屏一次刷新，最大化DMA效率
 *   - CS保持模式，减少GPIO切换开销
 *   - FIFO模式DMA，提升突发传输性能
 *   - 智能缓冲区管理，自动切换前后缓冲
 ******************************************************************************
 */

#ifndef __LCD_SPI_DMA_V2_H
#define __LCD_SPI_DMA_V2_H

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* LCD屏幕尺寸 */
#define LCD_WIDTH_V2     240
#define LCD_HEIGHT_V2    240

/* 双缓冲配置 */
#define LCD_USE_DOUBLE_BUFFER    1      // 启用双缓冲模式（推荐）
#define LCD_BUFFER_LINES         80     // 每个缓冲区行数（80行 = 38.4KB，双缓冲76.8KB）

/* 缓冲区大小计算 */
#define LCD_SINGLE_BUFFER_SIZE   (LCD_WIDTH_V2 * LCD_BUFFER_LINES)
#define LCD_FULL_FRAME_SIZE      (LCD_WIDTH_V2 * LCD_HEIGHT_V2)

/* 传输模式 */
typedef enum {
    LCD_TRANSFER_POLLING = 0,    // 轮询模式（调试用）
    LCD_TRANSFER_DMA_BLOCK,      // DMA阻塞模式（兼容旧代码）
    LCD_TRANSFER_DMA_ASYNC       // DMA异步模式（最高性能）
} LCD_TransferMode_t;

/* 缓冲区状态 */
typedef enum {
    BUFFER_IDLE = 0,             // 缓冲区空闲
    BUFFER_FILLING,              // CPU正在填充
    BUFFER_READY,                // 已填充，待传输
    BUFFER_TRANSFERRING          // DMA正在传输
} BufferState_t;

/* 双缓冲控制结构 */
typedef struct {
    uint16_t *buffer[2];         // 双缓冲区指针
    BufferState_t state[2];      // 双缓冲区状态
    uint8_t active_buffer;       // 当前活动缓冲区索引（0或1）
    uint8_t transfer_buffer;     // 当前传输缓冲区索引
    uint32_t buffer_size;        // 单个缓冲区大小
} DoubleBuffer_t;

/* LCD SPI DMA V2 操作句柄 */
typedef struct {
    /* 硬件句柄 */
    SPI_HandleTypeDef *hspi;         // SPI句柄
    DMA_HandleTypeDef *hdma_tx;      // DMA TX句柄

    /* 传输控制 */
    LCD_TransferMode_t transfer_mode;// 传输模式
    volatile bool dma_busy;          // DMA忙标志
    volatile bool transfer_complete; // 传输完成标志

    /* 双缓冲管理 */
    DoubleBuffer_t double_buffer;    // 双缓冲控制

    /* 帧缓冲（可选，用于整屏刷新）*/
    uint16_t *frame_buffer;          // 帧缓冲指针（115KB）
    bool frame_buffer_enabled;       // 帧缓冲模式

    /* 性能统计 */
    uint32_t transfer_count;         // 传输次数
    uint32_t total_pixels;           // 已传输像素数
    uint32_t last_transfer_time;     // 上次传输时间（ms）

    /* CS保持模式 */
    bool cs_hold_mode;               // CS保持模式（减少切换开销）
} LCD_SPI_DMA_V2_Handle_t;

/* ==================== 初始化和配置函数 ==================== */

/**
 * @brief 初始化LCD SPI DMA V2驱动
 * @param hlcd LCD句柄指针
 * @param hspi SPI句柄指针
 * @param mode 传输模式
 * @retval HAL状态
 */
HAL_StatusTypeDef LCD_V2_Init(LCD_SPI_DMA_V2_Handle_t *hlcd,
                               SPI_HandleTypeDef *hspi,
                               LCD_TransferMode_t mode);

/**
 * @brief 反初始化LCD驱动
 */
void LCD_V2_DeInit(LCD_SPI_DMA_V2_Handle_t *hlcd);

/**
 * @brief 启用帧缓冲模式（整屏刷新）
 * @retval HAL状态
 */
HAL_StatusTypeDef LCD_V2_EnableFrameBuffer(LCD_SPI_DMA_V2_Handle_t *hlcd);

/**
 * @brief 禁用帧缓冲模式
 */
void LCD_V2_DisableFrameBuffer(LCD_SPI_DMA_V2_Handle_t *hlcd);

/**
 * @brief 启用CS保持模式（减少GPIO切换）
 * @note 在连续传输时保持CS为低，提升性能
 */
void LCD_V2_EnableCSHold(LCD_SPI_DMA_V2_Handle_t *hlcd);

/**
 * @brief 禁用CS保持模式
 */
void LCD_V2_DisableCSHold(LCD_SPI_DMA_V2_Handle_t *hlcd);

/* ==================== 数据传输函数 ==================== */

/**
 * @brief 发送命令
 * @param cmd 8位命令
 */
HAL_StatusTypeDef LCD_V2_WriteCommand(LCD_SPI_DMA_V2_Handle_t *hlcd, uint8_t cmd);

/**
 * @brief 发送8位数据
 */
HAL_StatusTypeDef LCD_V2_WriteData8(LCD_SPI_DMA_V2_Handle_t *hlcd, uint8_t data);

/**
 * @brief 批量发送16位数据（核心优化函数）
 * @param pData 数据指针
 * @param length 数据长度（16位字数量）
 * @param async 是否异步传输（true=不等待完成，false=等待完成）
 * @retval HAL状态
 */
HAL_StatusTypeDef LCD_V2_WriteBuffer(LCD_SPI_DMA_V2_Handle_t *hlcd,
                                      uint16_t *pData,
                                      uint32_t length,
                                      bool async);

/**
 * @brief 等待DMA传输完成
 * @param timeout_ms 超时时间（毫秒）
 * @retval HAL状态（HAL_OK或HAL_TIMEOUT）
 */
HAL_StatusTypeDef LCD_V2_WaitComplete(LCD_SPI_DMA_V2_Handle_t *hlcd, uint32_t timeout_ms);

/* ==================== 双缓冲管理函数 ==================== */

/**
 * @brief 获取当前可填充的缓冲区指针
 * @retval 缓冲区指针，如果没有可用缓冲区则返回NULL
 */
uint16_t* LCD_V2_GetWriteBuffer(LCD_SPI_DMA_V2_Handle_t *hlcd);

/**
 * @brief 标记缓冲区已填充完成，准备传输
 * @param pixel_count 实际填充的像素数
 * @retval HAL状态
 */
HAL_StatusTypeDef LCD_V2_SubmitBuffer(LCD_SPI_DMA_V2_Handle_t *hlcd, uint32_t pixel_count);

/**
 * @brief 启动双缓冲流水线传输
 * @note 自动管理缓冲区切换和DMA传输
 */
HAL_StatusTypeDef LCD_V2_FlushBuffers(LCD_SPI_DMA_V2_Handle_t *hlcd);

/* ==================== 高性能绘图函数 ==================== */

/**
 * @brief 高性能清屏（整屏DMA）
 * @param color 16位RGB565颜色
 */
void LCD_V2_Clear(LCD_SPI_DMA_V2_Handle_t *hlcd, uint16_t color);

/**
 * @brief 高性能填充矩形（分块DMA）
 */
void LCD_V2_FillRect(LCD_SPI_DMA_V2_Handle_t *hlcd,
                     uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height,
                     uint16_t color);

/**
 * @brief 高性能绘制图像（流水线传输）
 */
void LCD_V2_DrawImage(LCD_SPI_DMA_V2_Handle_t *hlcd,
                      uint16_t x, uint16_t y,
                      uint16_t width, uint16_t height,
                      const uint16_t *image);

/**
 * @brief 刷新整个帧缓冲到屏幕（最高性能）
 * @note 使用双缓冲流水线，CPU和DMA并行工作
 */
HAL_StatusTypeDef LCD_V2_RefreshFrame(LCD_SPI_DMA_V2_Handle_t *hlcd);

/* ==================== 帧缓冲操作函数 ==================== */

/**
 * @brief 在帧缓冲中设置像素
 */
void LCD_V2_FB_SetPixel(LCD_SPI_DMA_V2_Handle_t *hlcd,
                        uint16_t x, uint16_t y,
                        uint16_t color);

/**
 * @brief 在帧缓冲中填充矩形
 */
void LCD_V2_FB_FillRect(LCD_SPI_DMA_V2_Handle_t *hlcd,
                        uint16_t x, uint16_t y,
                        uint16_t width, uint16_t height,
                        uint16_t color);

/**
 * @brief 清空帧缓冲
 */
void LCD_V2_FB_Clear(LCD_SPI_DMA_V2_Handle_t *hlcd, uint16_t color);

/* ==================== 回调函数（需在中断中调用）==================== */

/**
 * @brief DMA传输完成回调
 * @note 必须在 HAL_SPI_TxCpltCallback 或 DMA中断中调用
 */
void LCD_V2_DMA_TxCpltCallback(LCD_SPI_DMA_V2_Handle_t *hlcd);

/**
 * @brief DMA传输错误回调
 */
void LCD_V2_DMA_TxErrorCallback(LCD_SPI_DMA_V2_Handle_t *hlcd);

/* ==================== 性能监控函数 ==================== */

/**
 * @brief 获取传输统计信息
 */
void LCD_V2_GetStats(LCD_SPI_DMA_V2_Handle_t *hlcd,
                     uint32_t *transfer_count,
                     uint32_t *total_pixels,
                     uint32_t *last_time_ms);

/**
 * @brief 重置统计信息
 */
void LCD_V2_ResetStats(LCD_SPI_DMA_V2_Handle_t *hlcd);

#endif /* __LCD_SPI_DMA_V2_H */
