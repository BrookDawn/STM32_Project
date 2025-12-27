# STM32H750 QSPI XIP 启动与 OTA 项目总结

本项目实现了基于 STM32H750 的工业级 Bootloader，支持从外部 QSPI Flash 直接执行程序 (XIP, Execute-In-Place) 及其 A/B 分区 OTA 更新。

## 1. 核心架构与内存布局

- **内部 Flash (0x08000000)**: 存储 Bootloader (128KB)。
- **外部 QSPI Flash (0x90000000)**: 映射到 CPU 地址空间。
  - `0x90000000`: OTA 元数据 (64KB)
  - `0x90010000`: App A 分区 (2MB) - **当前运行地址**
  - `0x90210000`: App B 分区 (2MB)
  - `0x90410000`: 固件下载暂存区 (2MB)

## 2. 关键技术细节与坑点修复 (Important!)

在开发过程中，解决了以下导致跳转失败的核心问题：

### A. MPU 权限管理
- **现象**: 跳转后立即 HardFault 或程序挂死。
- **原因**: H7 默认将 `0x90000000` 区域视为设备空间，禁止指令执行 (Execute Never)。
- **解决方法**: Bootloader 必须配置 MPU 开启 QSPI 区域的 **Instruction Access**。且跳转前**不能禁用 MPU**，必须将权限延续给 App。

### B. 时钟继承与波特率修正
- **现象**: 跳转后串口无输出或乱码。
- **原因**: Bootloader 将时钟升至 480MHz，而 App 默认认为自己处于 64MHz (HSI)。
- **解决方法**:
  - App 禁止再次调用 `SystemClock_Config()`（防止 QSPI 运行中时钟抖动导致挂死）。
  - App 必须调用 `HAL_Init()` 和 `SystemCoreClockUpdate()`，使硬件抽象层自动识别出 480MHz 的真实频率，从而修正串口波特率和 `HAL_Delay`。

### C. 向量表重定位 (VTOR)
- **现象**: 中断触发时程序跑飞。
- **原因**: 中断向量表默认在 0x08000000。
- **解决方法**: App 的 `system_stm32h7xx.c` 中必须开启 `USER_VECT_TAB_ADDRESS`，并将 `VECT_TAB_BASE_ADDRESS` 设置为 `0x90010000`。

### D. 跳转前的环境清理
- **解决方法**:
  - 必须关闭所有外设中断。
  - 必须关闭全局中断 `__disable_irq()`。
  - 必须清除并关闭 SysTick。
  - **不要在跳转前关闭 Cache**，H7 XIP 运行非常依赖 Cache 性能。

## 3. 常用操作指令

### 编译 (CMake)
```bash
# 编译 Bootloader
cd bootloader_1 && cmake -B build -G "Unix Makefiles" -D CMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake && cmake --build build

# 编译 App
cd APP_1 && cmake -B build -G "Unix Makefiles" -D CMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake && cmake --build build
```

### 烧录 (STM32CubeProgrammer)
1. **Bootloader**: 烧录至 `0x08000000`。
2. **App**: 烧录至 `0x90010000` (需要选择正确的外部 Flash Loader, 如 `W25Q64_FANKE`)。

## 4. 硬件配置参考
- **USART1**: PA9 (TX), PA10 (RX) @ 115200 8N1
- **LED**: PC13 (Active Low)
- **QSPI**: Quad Mode, Memory Mapped Enabled
