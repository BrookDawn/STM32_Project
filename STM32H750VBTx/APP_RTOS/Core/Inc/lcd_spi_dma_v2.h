/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_spi_dma_v2.h
  * @brief   高性能LCD SPI DMA驱动v2 - 双缓冲流水线架构
  * @author  Claude Code Assistant
  * @date    2025-12-27
  ******************************************************************************
  * @attention
  *
  * 特性:
  * - 双缓冲机制：CPU准备数据和DMA传输同时进行
  * - FIFO模式优化：提升突发传输效率
  * - 流水线架构：最小化等待时间
  * - 零拷贝设计：直接操作缓冲区指针
  * - 中断驱动：异步传输，CPU利用率最大化
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LCD_SPI_DMA_V2_H__
#define __LCD_SPI_DMA_V2_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief LCD SPI DMA驱动状态枚举
 */
typedef enum {
    LCD_DMA_STATE_IDLE = 0,        /**< 空闲状态 */
    LCD_DMA_STATE_BUSY,            /**< 忙碌状态 */
    LCD_DMA_STATE_ERROR,           /**< 错误状态 */
    LCD_DMA_STATE_TIMEOUT          /**< 超时状态 */
} LCD_DMA_State_t;

/**
 * @brief 缓冲区状态枚举
 */
typedef enum {
    BUFFER_STATE_EMPTY = 0,        /**< 缓冲区空闲，可写入 */
    BUFFER_STATE_FILLED,           /**< 缓冲区已填充，待传输 */
    BUFFER_STATE_TRANSMITTING      /**< 缓冲区正在传输 */
} BufferState_t;

/**
 * @brief 双缓冲管理结构体
 */
typedef struct {
    uint8_t *buffer[2];            /**< 双缓冲区指针数组 */
    uint32_t buffer_size;          /**< 每个缓冲区大小(字节) */
    volatile BufferState_t state[2]; /**< 缓冲区状态 */
    volatile uint8_t write_index;  /**< 当前写入缓冲区索引(0或1) */
    volatile uint8_t tx_index;     /**< 当前传输缓冲区索引(0或1) */
    volatile uint32_t tx_count;    /**< 传输计数器 */
    volatile uint32_t error_count; /**< 错误计数器 */
} DoubleBuffer_t;

/**
 * @brief LCD SPI DMA驱动配置结构体
 */
typedef struct {
    SPI_HandleTypeDef *hspi;       /**< SPI句柄 */
    GPIO_TypeDef *dc_port;         /**< DC引脚端口 */
    uint16_t dc_pin;               /**< DC引脚编号 */
    GPIO_TypeDef *cs_port;         /**< CS引脚端口 */
    uint16_t cs_pin;               /**< CS引脚编号 */
    GPIO_TypeDef *rst_port;        /**< RST引脚端口(可选) */
    uint16_t rst_pin;              /**< RST引脚编号 */
    uint32_t timeout_ms;           /**< 超时时间(毫秒) */
} LCD_SPI_Config_t;

/**
 * @brief LCD SPI DMA驱动句柄结构体
 */
typedef struct {
    LCD_SPI_Config_t config;       /**< 驱动配置 */
    DoubleBuffer_t double_buffer;  /**< 双缓冲管理 */
    volatile LCD_DMA_State_t state; /**< 驱动状态 */
    SemaphoreHandle_t tx_sem;      /**< 传输完成信号量 */
    SemaphoreHandle_t buffer_sem;  /**< 缓冲区可用信号量 */
    volatile bool initialized;     /**< 初始化标志 */
} LCD_SPI_Handle_t;

/**
 * @brief 性能统计结构体
 */
typedef struct {
    uint32_t total_bytes;          /**< 总传输字节数 */
    uint32_t total_frames;         /**< 总帧数 */
    uint32_t dma_transfers;        /**< DMA传输次数 */
    uint32_t buffer_swaps;         /**< 缓冲区切换次数 */
    uint32_t wait_times;           /**< 等待次数 */
    uint32_t errors;               /**< 错误次数 */
    uint32_t max_wait_time_us;     /**< 最大等待时间(微秒) */
    float avg_throughput_mbps;     /**< 平均吞吐量(Mbps) */
} LCD_DMA_Stats_t;

/* Exported constants --------------------------------------------------------*/
#define LCD_SPI_DMA_OK              0     /**< 操作成功 */
#define LCD_SPI_DMA_ERROR          -1     /**< 通用错误 */
#define LCD_SPI_DMA_TIMEOUT        -2     /**< 超时错误 */
#define LCD_SPI_DMA_BUSY           -3     /**< 设备忙 */
#define LCD_SPI_DMA_INVALID_PARAM  -4     /**< 无效参数 */

/* 默认缓冲区大小 - 适合240x240 RGB565屏幕的一行数据 */
#define LCD_DMA_DEFAULT_BUFFER_SIZE  (240 * 2)  /* 480字节 */

/* 默认超时时间 */
#define LCD_DMA_DEFAULT_TIMEOUT_MS   1000

/* Exported macro ------------------------------------------------------------*/

/**
 * @brief 设置DC引脚为命令模式
 */
#define LCD_DC_CMD(handle)  \
    HAL_GPIO_WritePin((handle)->config.dc_port, (handle)->config.dc_pin, GPIO_PIN_RESET)

/**
 * @brief 设置DC引脚为数据模式
 */
#define LCD_DC_DATA(handle) \
    HAL_GPIO_WritePin((handle)->config.dc_port, (handle)->config.dc_pin, GPIO_PIN_SET)

/**
 * @brief 片选使能
 */
#define LCD_CS_SELECT(handle) \
    HAL_GPIO_WritePin((handle)->config.cs_port, (handle)->config.cs_pin, GPIO_PIN_RESET)

/**
 * @brief 片选禁用
 */
#define LCD_CS_DESELECT(handle) \
    HAL_GPIO_WritePin((handle)->config.cs_port, (handle)->config.cs_pin, GPIO_PIN_SET)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化LCD SPI DMA驱动
 * @param handle 驱动句柄指针
 * @param config 驱动配置指针
 * @param buffer_size 每个缓冲区大小(字节)
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_DMA_Init(LCD_SPI_Handle_t *handle,
                         const LCD_SPI_Config_t *config,
                         uint32_t buffer_size);

/**
 * @brief 反初始化LCD SPI DMA驱动
 * @param handle 驱动句柄指针
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_DMA_DeInit(LCD_SPI_Handle_t *handle);

/**
 * @brief 发送命令到LCD (阻塞模式)
 * @param handle 驱动句柄指针
 * @param cmd 命令字节
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_SendCmd(LCD_SPI_Handle_t *handle, uint8_t cmd);

/**
 * @brief 发送数据到LCD (阻塞模式, 小数据量)
 * @param handle 驱动句柄指针
 * @param data 数据指针
 * @param size 数据大小(字节)
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_SendData(LCD_SPI_Handle_t *handle, const uint8_t *data, uint32_t size);

/**
 * @brief 发送大数据块到LCD (DMA模式, 双缓冲流水线)
 * @param handle 驱动句柄指针
 * @param data 数据指针
 * @param size 数据大小(字节)
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 * @note 此函数使用双缓冲机制，自动分块传输大数据
 */
int32_t LCD_SPI_SendDataDMA(LCD_SPI_Handle_t *handle, const uint8_t *data, uint32_t size);

/**
 * @brief 获取可写缓冲区指针(零拷贝接口)
 * @param handle 驱动句柄指针
 * @param buffer 输出缓冲区指针的指针
 * @param max_size 输出缓冲区最大可用大小
 * @param timeout_ms 超时时间(毫秒)
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 * @note 用户填充缓冲区后需调用LCD_SPI_CommitBuffer提交
 */
int32_t LCD_SPI_GetWriteBuffer(LCD_SPI_Handle_t *handle,
                                uint8_t **buffer,
                                uint32_t *max_size,
                                uint32_t timeout_ms);

/**
 * @brief 提交已填充的缓冲区进行传输
 * @param handle 驱动句柄指针
 * @param size 实际填充的数据大小(字节)
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_CommitBuffer(LCD_SPI_Handle_t *handle, uint32_t size);

/**
 * @brief 等待所有DMA传输完成
 * @param handle 驱动句柄指针
 * @param timeout_ms 超时时间(毫秒)
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_WaitComplete(LCD_SPI_Handle_t *handle, uint32_t timeout_ms);

/**
 * @brief 硬件复位LCD
 * @param handle 驱动句柄指针
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_HardwareReset(LCD_SPI_Handle_t *handle);

/**
 * @brief DMA传输完成回调函数 (需在stm32h7xx_it.c中调用)
 * @param hspi SPI句柄指针
 * @retval None
 */
void LCD_SPI_DMA_TxCpltCallback(SPI_HandleTypeDef *hspi);

/**
 * @brief DMA传输错误回调函数 (需在stm32h7xx_it.c中调用)
 * @param hspi SPI句柄指针
 * @retval None
 */
void LCD_SPI_DMA_ErrorCallback(SPI_HandleTypeDef *hspi);

/**
 * @brief 获取性能统计信息
 * @param handle 驱动句柄指针
 * @param stats 统计信息结构体指针
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_GetStats(LCD_SPI_Handle_t *handle, LCD_DMA_Stats_t *stats);

/**
 * @brief 重置性能统计信息
 * @param handle 驱动句柄指针
 * @retval LCD_SPI_DMA_OK: 成功, 其他: 错误码
 */
int32_t LCD_SPI_ResetStats(LCD_SPI_Handle_t *handle);

/* Private defines -----------------------------------------------------------*/
/* 内部使用 - 请勿直接调用 */
void _LCD_SPI_DMA_TransferNext(LCD_SPI_Handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_SPI_DMA_V2_H__ */
