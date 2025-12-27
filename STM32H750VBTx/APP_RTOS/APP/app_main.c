/**
 * @file app_main.c
 * @brief Application main file with FreeRTOS tasks
 */

#include "app_main.h"
#include "gpio.h"
#include "usart.h"
#include "spi.h"
#include "lcd_spi_154.h"
#include "lcd_spi_dma.h"
#include <stdio.h>
#include <string.h>

/* LCD DMA操作句柄 - 需要在中断中访问，声明为全局 */
LCD_SPI_DMA_Handle_t hlcd_dma;

/* Task handles */
osThreadId_t task_log_handle;
osThreadId_t task_lcd_handle;

/* Task attributes */
const osThreadAttr_t task_log_attributes = {
    .name = "LogTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t task_lcd_attributes = {
    .name = "LcdTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

/**
 * @brief Initialize application tasks
 */
void app_main_init(void)
{
    /* Create LOG thread */
    task_log_handle = osThreadNew(task_log_entry, NULL, &task_log_attributes);

    /* Create LCD thread */
    task_lcd_handle = osThreadNew(task_lcd_entry, NULL, &task_lcd_attributes);
}

/**
 * @brief LOG Thread - Controls LED breathing effect and UART debug information
 * @param argument: Not used
 */
void task_log_entry(void *argument)
{
    uint32_t tick_count = 0;
    char msg_buffer[100];
    uint8_t brightness = 0;
    int8_t direction = 1; // 1 for increasing, -1 for decreasing

    /* Initialize PC13 GPIO if not already done */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    for(;;)
    {
        /* Software PWM for breathing effect */
        // Set LED state based on PWM duty cycle
        for(uint8_t pwm_cycle = 0; pwm_cycle < 100; pwm_cycle++)
        {
            if(pwm_cycle < brightness)
            {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED ON (active high)
            }
            else
            {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LED OFF
            }
            osDelay(1); // 1ms per PWM cycle
        }

        /* Update brightness for breathing effect */
        brightness += direction * 5; // Change brightness by 5 each cycle

        if(brightness >= 100)
        {
            brightness = 100;
            direction = -1; // Start decreasing
        }
        else if(brightness <= 0)
        {
            brightness = 0;
            direction = 1; // Start increasing

            /* Send debug message via UART once per breath cycle */
            tick_count++;
            snprintf(msg_buffer, sizeof(msg_buffer),
                     "[LOG] Breath Cycle: %lu, FreeRTOS running from QSPI XIP\r\n",
                     tick_count);
            HAL_UART_Transmit(&huart1, (uint8_t*)msg_buffer, strlen(msg_buffer), HAL_MAX_DELAY);
        }
    }
}

/**
 * @brief LCD Thread - Tests ST7789 LCD display demo with DMA acceleration
 * @param argument: Not used
 */
void task_lcd_entry(void *argument)
{
    uint16_t color_index = 0;
    uint32_t frame_count = 0;
    uint32_t last_tick = 0;
    float fps = 0.0f;
    char fps_str[32];
    char debug_msg[100];

    uint32_t colors[] = {
        LCD_RED,
        LCD_GREEN,
        LCD_BLUE,
        LCD_YELLOW,
        LCD_CYAN,
        LCD_MAGENTA,
        LCD_WHITE
    };
    const char *color_names[] = {
        "RED",
        "GREEN",
        "BLUE",
        "YELLOW",
        "CYAN",
        "MAGENTA",
        "WHITE"
    };

    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Initializing LCD...\r\n", 27, 100);

    /* Initialize LCD with original driver */
    SPI_LCD_Init();
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] LCD Init OK\r\n", 19, 100);

    LCD_SetDirection(Direction_H);
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Direction Set\r\n", 21, 100);

    LCD_Backlight_ON;
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Backlight ON\r\n", 20, 100);

    /* Initialize DMA driver */
    LCD_SPI_DMA_Init(&hlcd_dma, &hspi4);
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] DMA Init OK\r\n", 19, 100);

    /* 打印DMA缓冲区地址，验证是否在D2 SRAM */
    char addr_msg[128];
    snprintf(addr_msg, sizeof(addr_msg),
             "[LCD] DMA Buf@0x%08lX (D2:0x30000000)\r\n"
             "[LCD] D-Cache %s for this region\r\n",
             (uint32_t)hlcd_dma.dma_buffer[0],
             ((uint32_t)hlcd_dma.dma_buffer[0] >= 0x30000000 &&
              (uint32_t)hlcd_dma.dma_buffer[0] < 0x40000000) ? "DISABLED" : "ENABLED");
    HAL_UART_Transmit(&huart1, (uint8_t*)addr_msg, strlen(addr_msg), 100);

    /* 跳过性能测试，直接显示内容 */
    // extern void LCD_V2_Performance_Test(LCD_SPI_DMA_Handle_t *hlcd);
    // HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Starting Performance Test...\r\n", 37, 100);
    // LCD_V2_Performance_Test(&hlcd_dma);

    /* 直接使用DMA模式填充红色 - 快速验证LCD和DMA */
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Filling RED with DMA...\r\n", 31, 100);
    LCD_DMA_Clear(&hlcd_dma, 0xF800);  // 红色 (RGB565: 11111 000000 00000)
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] RED Fill OK\r\n", 18, 100);
    osDelay(2000);  // 等待2秒

    /* 测试蓝色 */
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Filling BLUE with DMA...\r\n", 32, 100);
    LCD_DMA_Clear(&hlcd_dma, 0x001F);  // 蓝色
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] BLUE Fill OK\r\n", 19, 100);
    osDelay(2000);

    /* 方案1: 使用直接DMA模式（推荐，内存占用小） */
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Clearing to BLACK with DMA...\r\n", 37, 100);
    LCD_DMA_Clear(&hlcd_dma, 0x0000);  // 黑色背景
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Clear OK\r\n", 16, 100);

    /* Display title using original driver (for text) */
    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_WHITE);
    LCD_SetBackColor(LCD_BLACK);
    LCD_DisplayText(30, 10, "DMA加速测试");

    LCD_SetTextFont(&CH_Font20);
    LCD_DisplayText(20, 45, "FreeRTOS+DMA");

    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Text Displayed\r\n", 22, 100);

    /* 方案2: 使用帧缓冲模式（高级，占用115KB RAM）*/
    // LCD_SPI_DMA_EnableFrameBuffer(&hlcd_dma);
    // LCD_FB_Clear(&hlcd_dma, 0x0000);

    last_tick = HAL_GetTick();

    HAL_UART_Transmit(&huart1, (uint8_t*)"[LCD] Entering main loop...\r\n", 29, 100);

    for(;;)
    {
        /* Cycle through colors */
        uint16_t rgb565_color;

        // 转换24位RGB888到16位RGB565
        uint32_t color_rgb888 = colors[color_index];
        uint16_t r = ((color_rgb888 & 0xF80000) >> 8);
        uint16_t g = ((color_rgb888 & 0x00FC00) >> 5);
        uint16_t b = ((color_rgb888 & 0x0000F8) >> 3);
        rgb565_color = r | g | b;

        snprintf(debug_msg, sizeof(debug_msg), "[LCD] Drawing %s color (0x%04X)...\r\n",
                 color_names[color_index], rgb565_color);
        HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 100);

        /* 使用DMA填充矩形 - 速度大幅提升！ */
        LCD_DMA_FillRect(&hlcd_dma, 50, 85, 140, 80, rgb565_color);

        /* Display color name - 使用原驱动 */
        LCD_SetColor(LCD_WHITE);
        LCD_SetBackColor(LCD_BLACK);
        LCD_DMA_FillRect(&hlcd_dma, 50, 180, 140, 30, 0x0000);  // 黑色背景
        LCD_DisplayText(70, 185, (char*)color_names[color_index]);

        /* 绘制圆形（使用DMA填充） */
        // 简化版圆形 - 用DMA填充方块模拟
        LCD_DMA_FillRect(&hlcd_dma, 100, 95, 40, 40, rgb565_color);

        /* 计算并显示FPS */
        frame_count++;
        uint32_t current_tick = HAL_GetTick();
        if (current_tick - last_tick >= 1000) {
            fps = frame_count * 1000.0f / (current_tick - last_tick);
            frame_count = 0;
            last_tick = current_tick;

            // 显示FPS
            snprintf(fps_str, sizeof(fps_str), "FPS: %.1f", fps);
            LCD_SetColor(LCD_YELLOW);
            LCD_DMA_FillRect(&hlcd_dma, 10, 215, 100, 20, 0x0000);
            LCD_DisplayString(10, 215, fps_str);

            snprintf(debug_msg, sizeof(debug_msg), "[LCD] FPS: %.1f\r\n", fps);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 100);
        }

        /* 如果使用帧缓冲模式，需要刷新到屏幕 */
        // LCD_SPI_DMA_FlushFrameBuffer(&hlcd_dma);

        /* Move to next color */
        color_index = (color_index + 1) % 7;

        /* 更快的刷新 - DMA加速后可以缩短延时 */
        osDelay(500);  // 从2000ms减少到500ms，FPS提升4倍
    }
}
