# STM32F401xC J-Link下载工具使用说明

## 文件说明

### 1. download.jlink
J-Link脚本文件，包含完整的下载功能：
- 自动连接STM32F401xC设备
- 使用SWD接口
- 擦除Flash
- 下载HEX文件
- 验证程序
- 复位并运行

### 2. download.bat
Windows批处理文件，用于执行下载操作：
- 自动查找J-Link Commander路径
- 检查编译输出文件是否存在
- 执行J-Link脚本
- 显示下载结果

## 使用方法

### 方法一：使用批处理文件（推荐）
1. 确保项目已编译完成（`make all`）
2. 连接J-Link调试器到目标板
3. 双击运行 `download.bat`
4. 按照提示操作

### 方法二：直接使用J-Link Commander
1. 打开命令提示符
2. 切换到项目目录
3. 运行命令：
   ```
   "C:\Program Files\SEGGER\JLink_V840\JLink.exe" -CommandFile download.jlink
   ```

## 硬件连接

### SWD接口连接
| J-Link引脚 | STM32F401xC引脚 | 说明 |
|-----------|----------------|------|
| VCC       | 3.3V           | 电源（可选） |
| GND       | GND            | 地线 |
| SWDIO     | PA13           | 数据线 |
| SWCLK     | PA14           | 时钟线 |
| RESET     | NRST           | 复位线（可选） |

## 故障排除

### 常见问题
1. **连接失败**
   - 检查J-Link驱动是否正确安装
   - 确认目标板已上电
   - 检查SWD接口连接

2. **下载失败**
   - 确认编译输出文件存在
   - 检查Flash是否被保护
   - 尝试降低SWD速度

3. **程序不运行**
   - 检查复位电路
   - 确认时钟配置正确
   - 检查程序入口点

### 调试技巧
- 使用 `download_advanced.jlink` 获取更多调试信息
- 可以手动修改脚本中的速度设置
- 添加 `si 1` 命令可以进入单步调试模式

## 注意事项
- 确保J-Link软件版本与目标芯片兼容
- 下载前建议备份重要数据
- 如果使用外部晶振，确保晶振电路正常工作
- 某些情况下可能需要先解锁Flash保护
