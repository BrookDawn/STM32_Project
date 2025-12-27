/**
 * @file app_lcd_benchmark.c
 * @brief LCD性能基准测试 - 复杂页面渲染
 */

#include "lcd_spi_dma.h"
#include "lcd_spi_154.h"
#include "usart.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* 颜色定义 (RGB565) */
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_WHITE     0xFFFF
#define COLOR_BLACK     0x0000
#define COLOR_GRAY      0x7BEF
#define COLOR_ORANGE    0xFD20
#define COLOR_PURPLE    0x780F

/**
 * @brief 绘制填充矩形（带边框）
 */
void LCD_DrawFilledRectWithBorder(LCD_SPI_DMA_Handle_t *hlcd,
                                   uint16_t x, uint16_t y,
                                   uint16_t width, uint16_t height,
                                   uint16_t fill_color, uint16_t border_color)
{
    // 绘制填充矩形
    LCD_DMA_FillRect(hlcd, x, y, width, height, fill_color);

    // 绘制边框（使用原始LCD函数）
    LCD_SetColor(border_color);
    LCD_DrawRect(x, y, width, height);
}

/**
 * @brief 绘制渐变矩形（垂直渐变）
 */
void LCD_DrawGradientRect(LCD_SPI_DMA_Handle_t *hlcd,
                          uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height,
                          uint16_t color1, uint16_t color2)
{
    // 提取RGB分量
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;

    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;

    // 逐行绘制渐变
    for (uint16_t row = 0; row < height; row++) {
        // 计算当前行的颜色
        uint8_t r = r1 + ((int32_t)(r2 - r1) * row / height);
        uint8_t g = g1 + ((int32_t)(g2 - g1) * row / height);
        uint8_t b = b1 + ((int32_t)(b2 - b1) * row / height);

        uint16_t line_color = (r << 11) | (g << 5) | b;

        // 绘制一行
        LCD_DMA_FillRect(hlcd, x, y + row, width, 1, line_color);
    }
}

/**
 * @brief 绘制仪表盘样式的UI
 */
void LCD_DrawDashboard(LCD_SPI_DMA_Handle_t *hlcd, uint32_t fps, uint32_t frame_count)
{
    char text_buf[32];

    // 1. 清屏 - 深蓝色背景
    LCD_DMA_Clear(hlcd, 0x0010);

    // 2. 顶部标题栏 - 渐变
    LCD_DrawGradientRect(hlcd, 0, 0, 240, 30, COLOR_BLUE, COLOR_CYAN);
    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(COLOR_WHITE);
    LCD_SetBackColor(COLOR_BLUE);
    LCD_DisplayText(30, 3, "性能测试");

    // 3. FPS显示区域 - 绿色卡片
    LCD_DrawFilledRectWithBorder(hlcd, 10, 40, 220, 50, 0x0660, COLOR_GREEN);
    LCD_SetTextFont(&ASCII_Font24);
    LCD_SetColor(COLOR_WHITE);
    LCD_SetBackColor(0x0660);
    snprintf(text_buf, sizeof(text_buf), "FPS: %lu", fps);
    LCD_DisplayString(20, 50, text_buf);

    // 4. 帧计数显示 - 橙色卡片
    LCD_DrawFilledRectWithBorder(hlcd, 10, 100, 220, 40, 0x8200, COLOR_ORANGE);
    LCD_SetColor(COLOR_WHITE);
    LCD_SetBackColor(0x8200);
    snprintf(text_buf, sizeof(text_buf), "Frame: %lu", frame_count);
    LCD_DisplayString(20, 110, text_buf);

    // 5. 彩色进度条效果
    uint16_t colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA};
    for (int i = 0; i < 7; i++) {
        LCD_DMA_FillRect(hlcd, 10 + i * 32, 150, 30, 20, colors[i]);
    }

    // 6. 底部动态条纹
    uint16_t stripe_offset = (frame_count * 5) % 240;
    for (int i = 0; i < 6; i++) {
        uint16_t x = (stripe_offset + i * 40) % 240;
        LCD_DMA_FillRect(hlcd, x, 220, 30, 20, colors[i % 7]);
    }

    // 7. 状态指示灯
    LCD_SetColor(COLOR_GREEN);
    LCD_FillCircle(220, 15, 8);
}

/**
 * @brief 绘制复杂的图形测试页面
 */
void LCD_DrawComplexGraphicsTest(LCD_SPI_DMA_Handle_t *hlcd, uint32_t phase)
{
    // 清屏
    LCD_DMA_Clear(hlcd, COLOR_BLACK);

    // 标题
    LCD_SetTextFont(&CH_Font20);
    LCD_SetColor(COLOR_YELLOW);
    LCD_SetBackColor(COLOR_BLACK);
    LCD_DisplayText(60, 5, "图形测试");

    // 绘制多个旋转的彩色方块
    uint16_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA};
    for (int i = 0; i < 6; i++) {
        uint16_t x = 30 + (i % 3) * 70;
        uint16_t y = 40 + (i / 3) * 70;
        uint16_t offset = (phase * (i + 1)) % 20;

        LCD_DMA_FillRect(hlcd, x + offset, y, 50, 50, colors[i]);
    }

    // 绘制圆形
    LCD_SetColor(COLOR_WHITE);
    LCD_DrawCircle(120, 160, 30);
    LCD_SetColor(COLOR_RED);
    LCD_DrawCircle(120, 160, 25);
    LCD_SetColor(COLOR_GREEN);
    LCD_DrawCircle(120, 160, 20);

    // 绘制对角线
    LCD_SetColor(COLOR_CYAN);
    LCD_DrawLine(0, 200, 240, 240);
    LCD_DrawLine(240, 200, 0, 240);
}

/**
 * @brief 绘制数据可视化页面
 */
void LCD_DrawDataVisualization(LCD_SPI_DMA_Handle_t *hlcd, uint32_t tick)
{
    LCD_DMA_Clear(hlcd, 0x0010);

    // 标题
    LCD_SetTextFont(&CH_Font20);
    LCD_SetColor(COLOR_WHITE);
    LCD_SetBackColor(0x0010);
    LCD_DisplayText(50, 5, "数据可视化");

    // 模拟波形显示
    uint16_t prev_y = 120;
    for (uint16_t x = 0; x < 240; x++) {
        // 生成正弦波
        float angle = (x + tick) * 0.05f;
        uint16_t y = 120 + (uint16_t)(30.0f * sinf(angle));

        LCD_DMA_FillRect(hlcd, x, (y < prev_y) ? y : prev_y, 2, abs(y - prev_y) + 1, COLOR_GREEN);
        prev_y = y;
    }

    // 绘制坐标轴
    LCD_SetColor(COLOR_GRAY);
    LCD_DrawLine(0, 120, 240, 120);

    // 绘制柱状图
    for (int i = 0; i < 8; i++) {
        uint16_t height = 20 + ((tick + i * 20) % 60);
        uint16_t x = 10 + i * 28;
        LCD_DMA_FillRect(hlcd, x, 180 - height, 20, height, COLOR_CYAN);
    }
}

/**
 * @brief 性能基准测试主函数
 */
void LCD_Benchmark_Run(LCD_SPI_DMA_Handle_t *hlcd)
{
    char msg[128];
    uint32_t frame_count = 0;
    uint32_t last_fps_time = HAL_GetTick();
    uint32_t fps = 0;
    uint32_t test_mode = 0;

    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n=== LCD Benchmark Started ===\r\n", 32, 100);

    while (1) {
        uint32_t frame_start = HAL_GetTick();

        // 根据测试模式绘制不同页面
        switch (test_mode) {
            case 0:
                // 仪表盘模式
                LCD_DrawDashboard(hlcd, fps, frame_count);
                break;

            case 1:
                // 图形测试模式
                LCD_DrawComplexGraphicsTest(hlcd, frame_count);
                break;

            case 2:
                // 数据可视化模式
                LCD_DrawDataVisualization(hlcd, frame_count);
                break;

            case 3:
                // 全屏刷新测试
                LCD_DMA_Clear(hlcd, (frame_count % 2) ? COLOR_WHITE : COLOR_BLACK);
                LCD_SetTextFont(&ASCII_Font24);
                LCD_SetColor((frame_count % 2) ? COLOR_BLACK : COLOR_WHITE);
                LCD_SetBackColor((frame_count % 2) ? COLOR_WHITE : COLOR_BLACK);
                snprintf(msg, sizeof(msg), "FPS:%lu", fps);
                LCD_DisplayString(80, 110, msg);
                break;
        }

        frame_count++;

        // 计算FPS
        uint32_t current_time = HAL_GetTick();
        if (current_time - last_fps_time >= 1000) {
            fps = frame_count * 1000 / (current_time - last_fps_time);

            snprintf(msg, sizeof(msg), "[Benchmark] Mode:%lu FPS:%lu AvgTime:%lums\r\n",
                     test_mode, fps, (current_time - last_fps_time) / frame_count);
            HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

            frame_count = 0;
            last_fps_time = current_time;
        }

        // 每10秒切换测试模式
        if ((HAL_GetTick() / 10000) % 4 != test_mode) {
            test_mode = (HAL_GetTick() / 10000) % 4;
            snprintf(msg, sizeof(msg), "[Benchmark] Switch to Mode %lu\r\n", test_mode);
            HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
        }

        // 控制帧率，避免过快
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < 50) {
            osDelay(50 - frame_time);  // 限制最高20 FPS
        }
    }
}
