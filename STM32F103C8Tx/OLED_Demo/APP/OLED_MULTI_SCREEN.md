# OLED多屏幕适配使用说明

## 概述

驱动现在支持多种OLED屏幕，包括：
- **0.96寸 SSD1306** (128x64)
- **0.91寸 SSD1306** (128x32)
- **1.3寸 SH1106** (128x64)

所有屏幕类型在编译前通过预编译宏进行选择，实现完全解耦。

## 快速选择屏幕类型

### 方法1：使用快速选择宏（推荐）

在 `oled_config.h` 文件中，取消注释对应的宏：

```c
// 选择 0.96寸 SSD1306 128x64
#define OLED_USE_096_SSD1306_128X64

// 或选择 0.91寸 SSD1306 128x32
// #define OLED_USE_091_SSD1306_128X32

// 或选择 1.3寸 SH1106 128x64
// #define OLED_USE_13_SH1106_128X64
```

### 方法2：在项目设置中定义预编译宏

在IAR/Keil等IDE的项目设置中，添加预编译宏：

**对于0.96寸 SSD1306:**
```
OLED_CHIP_TYPE=OLED_TYPE_SSD1306
OLED_SIZE_TYPE=OLED_SIZE_128X64
```

**对于0.91寸 SSD1306:**
```
OLED_CHIP_TYPE=OLED_TYPE_SSD1306
OLED_SIZE_TYPE=OLED_SIZE_128X32
```

**对于1.3寸 SH1106:**
```
OLED_CHIP_TYPE=OLED_TYPE_SH1106
OLED_SIZE_TYPE=OLED_SIZE_128X64
```

### 方法3：在代码中直接定义

在包含 `oled_driver.h` 之前定义：

```c
#define OLED_CHIP_TYPE    OLED_TYPE_SSD1306
#define OLED_SIZE_TYPE    OLED_SIZE_128X64
#include "oled_driver.h"
```

## 初始化方式

### 方式1：自动初始化（使用预编译宏）

```c
#include "oled_driver.h"
#include "oled_config.h"

// 帧缓冲区（根据屏幕类型自动调整）
#if (OLED_SIZE_TYPE == OLED_SIZE_128X64)
    static uint8_t oled_framebuffer[128 * 64 / 8];  // 1024字节
#elif (OLED_SIZE_TYPE == OLED_SIZE_128X32)
    static uint8_t oled_framebuffer[128 * 32 / 8];  // 512字节
#endif

OLED_HandleTypeDef holed;

// 自动初始化
OLED_InitAuto(&holed, &hi2c1, OLED_I2C_ADDR_0x78, oled_framebuffer);
```

### 方式2：手动指定配置

```c
#include "oled_driver.h"

// 帧缓冲区
static uint8_t oled_framebuffer[128 * 64 / 8];

OLED_HandleTypeDef holed;
OLED_Config_t config;

// 使用预定义配置宏
config = OLED_CONFIG_096_SSD1306_128X64;  // 0.96寸 SSD1306
// config = OLED_CONFIG_091_SSD1306_128X32;  // 0.91寸 SSD1306
// config = OLED_CONFIG_13_SH1106_128X64;   // 1.3寸 SH1106

OLED_Init(&holed, &hi2c1, OLED_I2C_ADDR_0x78, &config, oled_framebuffer);
```

### 方式3：完全自定义配置

```c
OLED_Config_t config;

config.chip_type = OLED_TYPE_SH1106;
config.size_type = OLED_SIZE_128X64;
config.width = 128;
config.height = 64;
config.col_offset = 2;      // SH1106需要2像素偏移
config.contrast = 0x80;
config.com_pins = 0x12;

OLED_Init(&holed, &hi2c1, OLED_I2C_ADDR_0x78, &config, oled_framebuffer);
```

## 屏幕配置参数说明

### SSD1306配置

| 参数 | 128x64 | 128x32 |
|------|--------|--------|
| chip_type | OLED_TYPE_SSD1306 | OLED_TYPE_SSD1306 |
| width | 128 | 128 |
| height | 64 | 32 |
| col_offset | 0 | 0 |
| contrast | 0xCF | 0x8F |
| com_pins | 0x12 | 0x02 |

### SH1106配置

| 参数 | 128x64 |
|------|--------|
| chip_type | OLED_TYPE_SH1106 |
| width | 128 |
| height | 64 |
| col_offset | 2 (重要！) |
| contrast | 0x80 |
| com_pins | 0x12 |

**注意：** SH1106芯片内部RAM为132x64，但显示区域为128x64，需要2像素的列偏移。驱动会自动处理这个偏移。

## 帧缓冲区大小

根据屏幕尺寸计算帧缓冲区大小：

- **128x64**: 128 × (64/8) = 1024 字节
- **128x32**: 128 × (32/8) = 512 字节

## 代码示例

### 完整示例

```c
#include "oled_driver.h"
#include "oled_ui.h"
#include "oled_config.h"
#include "i2c.h"

// 帧缓冲区（根据屏幕类型自动调整）
#if (OLED_SIZE_TYPE == OLED_SIZE_128X64)
    #define OLED_FB_SIZE  (128 * 64 / 8)
#elif (OLED_SIZE_TYPE == OLED_SIZE_128X32)
    #define OLED_FB_SIZE  (128 * 32 / 8)
#else
    #define OLED_FB_SIZE  (128 * 64 / 8)
#endif

static uint8_t oled_framebuffer[OLED_FB_SIZE];
static OLED_HandleTypeDef holed;
static OLED_UIContext_t ui_ctx;

void OLED_Init_Example(void)
{
    // 自动初始化（使用预编译宏）
    OLED_InitAuto(&holed, &hi2c1, OLED_I2C_ADDR_0x78, oled_framebuffer);
    
    // 初始化UI上下文（自动适配屏幕尺寸）
    OLED_UI_Begin(&ui_ctx, &holed, 0, 0, holed.width, holed.height);
    
    // 清空并刷新
    OLED_Clear(&holed);
    OLED_Refresh(&holed);
}

void OLED_Draw_Example(void)
{
    // 绘制内容（自动适配不同屏幕）
    OLED_UI_DrawStr(&ui_ctx, 10, 5, "Hello", 1);
    OLED_UI_DrawCircle(&ui_ctx, 64, holed.height / 2, 20, 1);
    
    // 增量刷新
    OLED_RefreshDirty(&holed);
}
```

## 不同屏幕的差异

### SSD1306 vs SH1106

1. **初始化序列**：SH1106不需要电荷泵命令
2. **列地址设置**：SH1106使用不同的命令格式（页地址+列地址低4位+高4位）
3. **列偏移**：SH1106需要2像素的列偏移

驱动已自动处理这些差异，用户无需关心。

### 128x64 vs 128x32

1. **帧缓冲区大小**：128x32只需要512字节，128x64需要1024字节
2. **对比度值**：不同尺寸使用不同的对比度值
3. **COM引脚配置**：不同尺寸使用不同的COM引脚配置

## 编译时检查

驱动会在编译时检查配置：

- 如果未定义 `OLED_CHIP_TYPE`，默认使用 `OLED_TYPE_SSD1306`
- 如果未定义 `OLED_SIZE_TYPE`，默认使用 `OLED_SIZE_128X64`
- 帧缓冲区大小会根据 `OLED_SIZE_TYPE` 自动计算

## 常见问题

### Q: 如何切换屏幕类型？

A: 只需修改预编译宏定义，重新编译即可。无需修改代码。

### Q: SH1106显示偏移怎么办？

A: 驱动已自动处理SH1106的2像素列偏移，无需手动调整。

### Q: 如何同时支持多种屏幕？

A: 可以在运行时通过不同的配置结构体初始化不同的OLED句柄。

### Q: 帧缓冲区大小如何确定？

A: 使用预编译宏 `OLED_SIZE_TYPE`，驱动会自动计算。或手动计算：width × (height / 8)。

## 总结

多屏幕适配功能通过预编译宏实现完全解耦，用户只需：
1. 在编译前选择屏幕类型（通过宏定义）
2. 使用 `OLED_InitAuto()` 自动初始化
3. 正常使用所有绘制和刷新功能

驱动会自动处理不同屏幕的差异，用户代码无需修改。

