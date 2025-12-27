# LCD SPI DMA驱动在FreeRTOS环境下的修复总结

## 问题分析

使用新的SPI DMA驱动后屏幕没有内容显示，主要原因如下:

### 1. **句柄作用域问题** ✅ 已修复
- **问题**: `hlcd_dma`在`app_main.c`中声明为`static`，但在`lcd_spi_dma.c`的中断回调函数中使用`extern`声明访问
- **后果**: 可能导致链接错误或访问到错误的句柄地址
- **修复**: 将`hlcd_dma`改为全局非static声明

### 2. **中断优先级配置** ✅ 已修复
- **问题**: SPI4和DMA中断优先级设为5，与FreeRTOS的`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5`冲突
- **后果**: 在中断中调用FreeRTOS API（如`osThreadFlagsSet`）会触发断言或硬件故障
- **修复**:
  - SPI4中断优先级: 5 → 6
  - DMA1_Stream0中断优先级: 5 → 6
  - DMA1_Stream1中断优先级: 5 → 6
  - **重要**: 数值越大优先级越低，6比5优先级低，但可以安全调用FreeRTOS API

### 3. **DMA完成标志管理** ✅ 已修复
- **问题**: `LCD_SPI_DMA_WaitComplete`中DMA完成后未正确清除`dma_busy`标志
- **后果**: 可能导致后续DMA传输被错误判断为仍在进行中，产生死锁
- **修复**: 在等待超时和正常完成两个分支都正确设置`dma_busy = false`

### 4. **SPI数据宽度频繁切换** ✅ 已优化
- **问题**: 每次发送数据都会检查并重新初始化SPI数据宽度（8位/16位）
- **后果**: 性能开销大，可能在RTOS多任务环境下引起竞态条件
- **修复**: 使用静态变量缓存当前数据宽度，仅在必要时切换，减少`HAL_SPI_Init()`调用

### 5. **移除不必要的SPI模式恢复** ✅ 已优化
- **问题**: 在每次DMA传输后将SPI从16位恢复到8位
- **后果**: 不必要的开销，下次使用时又要切换回16位
- **修复**: 移除恢复操作，保持当前状态

## 关键代码修改

### app_main.c:16
```c
// 修改前
static LCD_SPI_DMA_Handle_t hlcd_dma;

// 修改后
LCD_SPI_DMA_Handle_t hlcd_dma;  // 需要在中断中访问，声明为全局
```

### spi.c:131 - 中断优先级
```c
// 修改前
HAL_NVIC_SetPriority(SPI4_IRQn, 5, 0);

// 修改后
HAL_NVIC_SetPriority(SPI4_IRQn, 6, 0);  // 必须高于configMAX_SYSCALL(5)
```

### dma.c:47-50 - DMA中断优先级
```c
// 修改前
HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);

// 修改后
HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6, 0);
HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6, 0);
```

### lcd_spi_dma.c:59-68 - WaitComplete修复
```c
uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 1000);

if (flags == (uint32_t)osErrorTimeout) {
    HAL_SPI_DMAStop(hlcd->hspi);
    hlcd->dma_busy = false;  // 超时也要清除标志
} else if (flags == 0x01) {
    hlcd->dma_busy = false;  // 正常完成清除标志
}
```

### lcd_spi_dma.c:116-121 - 静态变量优化
```c
// 使用静态变量缓存SPI数据宽度状态
static bool is_8bit_mode = true;
if (!is_8bit_mode) {
    hlcd->hspi->Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(hlcd->hspi);
    is_8bit_mode = true;
}
```

## FreeRTOS中断优先级说明

STM32H7 有 4 位优先级位 (`configPRIO_BITS = 4`)，优先级范围 0-15:
- **0**: 最高优先级（不可被抢占）
- **5**: `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`（可调用FreeRTOS API的最高优先级）
- **6-14**: 可安全调用FreeRTOS API的中断优先级范围
- **15**: 最低优先级

**关键规则**:
- 优先级 ≤ 5 的中断**不能**调用FreeRTOS API（如`osThreadFlagsSet`）
- 优先级 > 5 的中断**可以**调用FreeRTOS API
- DMA/SPI中断需要调用`osThreadFlagsSet`通知任务，因此必须设为 6 或更高

## 调试增强

在`app_main.c`的`task_lcd_entry`中添加了详细的UART调试输出：
- LCD初始化各阶段状态
- DMA传输状态
- 每帧绘制的颜色信息
- 实时FPS显示

## 测试建议

1. 编译并烧录到MCU
2. 通过UART（115200,8N1）查看调试输出
3. 观察LCD屏幕是否正常显示彩色矩形
4. 检查UART输出是否有超时或错误信息

## 预期行为

- LCD应显示循环变化的彩色矩形（红→绿→蓝→黄→青→洋红→白）
- 屏幕上方显示中文标题 "DMA加速测试" 和 "FreeRTOS+DMA"
- 屏幕下方显示当前颜色名称
- 左下角显示FPS（应该>10 FPS）
- UART输出详细的初始化和运行日志

## 可能的额外问题

如果修复后仍无显示，检查：
1. LCD硬件连接（CS, DC, SCK, MOSI, BACKLIGHT引脚）
2. SPI4时钟频率是否过高（当前为PCLK/2，可能需要降低）
3. LCD_CS_Select/Deselect宏定义是否正确
4. 是否启用了D-Cache但未正确维护（已在代码中添加SCB_CleanDCache）
