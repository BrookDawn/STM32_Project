# OLED驱动使用说明

## 概述

这是一个基于STM32 HAL库的OLED驱动，支持：
- I2C DMA传输方式
- 结构体和函数指针解耦设计
- U8G2风格的绘制API
- 分区刷新机制（只刷新修改的像素点）
- 流畅的动画绘制支持

## 文件结构

```
APP/
├── oled_driver.h      # OLED驱动头文件
├── oled_driver.c      # OLED驱动实现文件
├── oled_font.h        # 字体数据头文件
├── oled_font.c        # 字体数据实现文件（6x8 ASCII字体）
├── oled_ui.h          # UI绘制头文件
├── oled_ui.c          # UI绘制实现文件
├── oled_example.c     # 使用示例文件
└── README.md          # 本文件
```

## 快速开始

### 0. 选择屏幕类型（编译前配置）

在项目设置中添加预编译宏，或在`oled_config.h`中取消注释对应的宏：

**方法1：使用快速选择宏（推荐）**
```c
// 在 oled_config.h 中取消注释其中一个：
#define OLED_USE_096_SSD1306_128X64  // 0.96寸 SSD1306 128x64
// #define OLED_USE_091_SSD1306_128X32  // 0.91寸 SSD1306 128x32
// #define OLED_USE_13_SH1106_128X64    // 1.3寸 SH1106 128x64
```

**方法2：直接定义宏**
```c
// 在项目预编译宏中添加：
#define OLED_CHIP_TYPE    OLED_TYPE_SSD1306  // 或 OLED_TYPE_SH1106
#define OLED_SIZE_TYPE    OLED_SIZE_128X64   // 或 OLED_SIZE_128X32
```

### 1. 初始化OLED

**方法1：使用预编译宏自动配置（推荐）**
```c
#include "oled_driver.h"
#include "oled_config.h"
#include "i2c.h"

// 定义帧缓冲区（根据屏幕类型自动调整）
#if (OLED_SIZE_TYPE == OLED_SIZE_128X64)
    static uint8_t oled_framebuffer[128 * 64 / 8];  // 1024字节
#elif (OLED_SIZE_TYPE == OLED_SIZE_128X32)
    static uint8_t oled_framebuffer[128 * 32 / 8];  // 512字节
#endif

// 定义OLED句柄
static OLED_HandleTypeDef holed;

// 自动初始化（使用预编译宏配置）
OLED_InitAuto(&holed, &hi2c1, OLED_I2C_ADDR_0x78, oled_framebuffer);
```

**方法2：手动指定配置**
```c
#include "oled_driver.h"
#include "i2c.h"

// 定义帧缓冲区
static uint8_t oled_framebuffer[128 * 64 / 8];

// 定义OLED句柄
static OLED_HandleTypeDef holed;

// 使用预定义配置宏
OLED_Config_t config = OLED_CONFIG_096_SSD1306_128X64;  // 或 OLED_CONFIG_091_SSD1306_128X32 或 OLED_CONFIG_13_SH1106_128X64
OLED_Init(&holed, &hi2c1, OLED_I2C_ADDR_0x78, &config, oled_framebuffer);
```

### 2. 基本绘制

```c
// 清空屏幕
OLED_Clear(&holed);

// 设置像素点
OLED_SetPixel(&holed, 10, 10, 1);  // 1=白色，0=黑色

// 绘制线条
OLED_DrawHLine(&holed, 0, 20, 128, 1);
OLED_DrawVLine(&holed, 64, 0, 64, 1);

// 绘制矩形
OLED_DrawRect(&holed, 10, 30, 50, 20, 1);
OLED_FillRect(&holed, 70, 30, 50, 20, 1);

// 绘制圆形
OLED_DrawCircle(&holed, 64, 32, 20, 1);
OLED_FillCircle(&holed, 100, 50, 10, 1);

// 绘制文字
OLED_DrawString(&holed, 0, 0, "Hello OLED!", 1);

// 刷新显示（全屏刷新）
OLED_Refresh(&holed);
```

### 3. 使用UI模块（U8G2风格）

```c
#include "oled_ui.h"

// 创建UI上下文
OLED_UIContext_t ui_ctx;

// 开始绘制（设置裁剪区域）
OLED_UI_Begin(&ui_ctx, &holed, 0, 0, 128, 64);

// 绘制图形（自动裁剪）
OLED_UI_DrawLine(&ui_ctx, 0, 0, 128, 64, 1);
OLED_UI_DrawBox(&ui_ctx, 10, 10, 50, 30, 1);
OLED_UI_DrawFilledBox(&ui_ctx, 70, 10, 50, 30, 1);
OLED_UI_DrawCircle(&ui_ctx, 64, 32, 20, 1);
OLED_UI_DrawStr(&ui_ctx, 10, 50, "UI Test", 1);

// 结束绘制
OLED_UI_End(&ui_ctx);

// 刷新显示（增量刷新，只刷新修改的区域）
OLED_RefreshDirty(&holed);
```

### 4. 动画示例

```c
// 创建动画
OLED_Animation_t anim;
OLED_UI_AnimInit(&anim, 0.0f, 100.0f, 2000, OLED_Easing_EaseInOutQuad);
anim.loop = true;
anim.reverse = true;
OLED_UI_AnimStart(&anim);

// 在主循环中更新动画
uint32_t last_tick = HAL_GetTick();
while (1)
{
    uint32_t current_tick = HAL_GetTick();
    uint32_t delta_ms = current_tick - last_tick;
    
    // 更新动画
    OLED_UI_AnimUpdate(&anim, delta_ms);
    
    // 使用动画值绘制
    uint8_t value = (uint8_t)anim.current_value;
    // ... 绘制代码 ...
    
    // 刷新显示
    OLED_RefreshDirty(&holed);
    
    last_tick = current_tick;
    HAL_Delay(50);
}
```

### 5. 进度条组件

```c
// 创建进度条
OLED_ProgressBar_t progress_bar;
progress_bar.x = 10;
progress_bar.y = 40;
progress_bar.width = 108;
progress_bar.height = 10;
progress_bar.value = 50;  // 0-100
progress_bar.border_color = 1;
progress_bar.fill_color = 1;

// 绘制进度条
OLED_UI_DrawProgressBar(&ui_ctx, &progress_bar);

// 更新进度值
OLED_UI_SetProgressBar(&progress_bar, 75);
```

### 6. 带动画的进度条

```c
// 创建动画和进度条
OLED_Animation_t progress_anim;
OLED_ProgressBar_t progress_bar;

// 初始化
OLED_UI_AnimInit(&progress_anim, 0.0f, 100.0f, 3000, OLED_Easing_EaseInOutQuad);
progress_anim.loop = true;
progress_anim.reverse = true;
OLED_UI_AnimStart(&progress_anim);

progress_bar.x = 10;
progress_bar.y = 40;
progress_bar.width = 108;
progress_bar.height = 10;
progress_bar.border_color = 1;
progress_bar.fill_color = 1;

// 在主循环中
OLED_UI_DrawAnimatedProgressBar(&ui_ctx, &progress_bar, &progress_anim);
OLED_UI_AnimUpdate(&progress_anim, delta_ms);
OLED_RefreshDirty(&holed);
```

## 分区刷新机制

驱动会自动跟踪修改的像素点区域（脏矩形），使用`OLED_RefreshDirty()`可以只刷新修改的区域，大大提高刷新效率：

```c
// 绘制一些内容
OLED_SetPixel(&holed, 10, 10, 1);
OLED_DrawRect(&holed, 20, 20, 50, 30, 1);

// 只刷新修改的区域（高效）
OLED_RefreshDirty(&holed);

// 或者全屏刷新（较慢）
OLED_Refresh(&holed);
```

## 缓动函数

支持多种缓动函数，用于创建流畅的动画效果：

- `OLED_Easing_Linear` - 线性
- `OLED_Easing_EaseInQuad` - 二次缓入
- `OLED_Easing_EaseOutQuad` - 二次缓出
- `OLED_Easing_EaseInOutQuad` - 二次缓入缓出

## DMA传输完成回调

驱动使用I2C DMA传输，需要在`HAL_I2C_MasterTxCpltCallback`中调用`OLED_DMATxCpltCallback`：

```c
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        OLED_DMATxCpltCallback(&holed);
    }
}
```

已在`stm32f1xx_it.c`中实现，如果使用多个OLED或I2C，需要相应修改。

## 支持的屏幕类型

| 屏幕类型 | 芯片 | 尺寸 | 配置宏 | 帧缓冲区大小 |
|---------|------|------|--------|-------------|
| 0.96寸 | SSD1306 | 128x64 | `OLED_CONFIG_096_SSD1306_128X64` | 1024字节 |
| 0.91寸 | SSD1306 | 128x32 | `OLED_CONFIG_091_SSD1306_128X32` | 512字节 |
| 1.3寸 | SH1106 | 128x64 | `OLED_CONFIG_13_SH1106_128X64` | 1024字节 |

## 注意事项

1. **帧缓冲区大小**：帧缓冲区大小 = width * (height / 8) 字节
   - 128x64: 128 * 8 = 1024 字节
   - 128x32: 128 * 4 = 512 字节

2. **屏幕类型选择**：必须在编译前通过预编译宏选择屏幕类型，否则使用默认配置（SSD1306 128x64）

3. **SH1106特殊处理**：SH1106芯片内部RAM为132x64，但显示区域为128x64，驱动会自动处理2像素的列偏移

2. **DMA传输**：确保I2C已配置DMA传输，驱动会自动使用DMA

3. **刷新频率**：建议刷新频率不超过50Hz，避免DMA传输冲突

4. **裁剪区域**：使用UI模块时，绘制会自动裁剪到指定区域

5. **动画更新**：动画需要在主循环中定期调用`OLED_UI_AnimUpdate()`

## 完整示例

参考`oled_example.c`文件，其中包含完整的使用示例。

