一、总体系统架构（先在脑子里形成“地图”）
1️⃣ 系统启动全流程（极其重要）
上电 / 复位
   ↓
内部 Flash
┌────────────────────┐
│ Bootloader         │
│  - 初始化最小系统  │
│  - 初始化 QSPI     │
│  - 读取 OTA 信息   │
│  - 选择 A / B      │
│  - 校验 CRC        │
│  - 配置 QSPI XIP   │
│  - 跳转 App        │
└────────────────────┘
   ↓
外部 QSPI Flash（Memory-Mapped）
┌──────────┬──────────┐
│ App A    │ App B    │
└──────────┴──────────┘


📌 核心思想一句话：

Bootloader 永远不升级，App 永远在外部 Flash，A/B 轮换 + 可回滚

二、QSPI Flash 分区设计（AB + OTA 信息）
1️⃣ W25Q64 基本参数

总容量：8 MB

扇区：4 KB

Block：64 KB

支持 QSPI Memory-Mapped（XIP）

2️⃣ 推荐 Flash 分区表（非常成熟、好用）
QSPI Flash (0x9000_0000 映射后)

0x000000 ───────────────
          OTA Info 区（64KB）
0x010000 ───────────────
          App A（2MB）
0x210000 ───────────────
          App B（2MB）
0x410000 ───────────────
          Download 区（2MB）
0x610000 ───────────────
          预留 / 日志
0x800000 ───────────────

分区表（偏移地址）
分区	偏移	大小
OTA Info	0x000000	64 KB
App A	0x010000	2 MB
App B	0x210000	2 MB
Download	0x410000	2 MB
3️⃣ OTA Info 结构体（决定一切）
#define OTA_MAGIC 0x4F544131  // "OTA1"

typedef struct
{
    uint32_t magic;

    uint32_t active_slot;    // 0 = A, 1 = B
    uint32_t update_slot;    // 当前升级目标

    uint32_t app_size[2];
    uint32_t app_crc[2];

    uint32_t upgrade_flag;   // 1 = 正在升级
    uint32_t rollback_flag;  // 1 = 需要回滚

    uint32_t boot_count;     // 启动失败计数
} ota_info_t;


📌 Bootloader 只信这个结构体

三、Bootloader 设计（QSPI + AB + OTA 核心）
1️⃣ Bootloader 放在哪里？

内部 Flash

0x08000000 ~ 0x0801FFFF (128KB)


建议预留 128KB，QSPI + OTA 逻辑会比 SPI 大

2️⃣ CubeMX（Bootloader）配置重点
QSPI（必须）
项	设置
Mode	Quad
FIFO	Enabled
ClockPrescaler	2~4
FlashSize	23（8MB = 2^23）
SampleShifting	Half Cycle
⚠️ 重要

不要一开始就开 Memory-Mapped

Bootloader 前半段用 Indirect 模式

3️⃣ Bootloader 主流程（伪代码）
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    QSPI_Init_Indirect();

    ota_info_t ota;
    ota_read(&ota);

    int slot = select_slot(&ota);

    if (!verify_app(slot, &ota))
    {
        rollback(&ota);
        slot = ota.active_slot;
    }

    QSPI_Enable_MemoryMapped();

    jump_to_app(slot);
}

4️⃣ Slot 地址定义
#define QSPI_BASE_ADDR   0x90000000

#define APP_A_OFFSET    0x010000
#define APP_B_OFFSET    0x210000

#define APP_A_ADDR  (QSPI_BASE_ADDR + APP_A_OFFSET)
#define APP_B_ADDR  (QSPI_BASE_ADDR + APP_B_OFFSET)

5️⃣ 跳转到 App（QSPI XIP）
void jump_to_app(int slot)
{
    uint32_t app_addr = (slot == 0) ? APP_A_ADDR : APP_B_ADDR;
    uint32_t *vector = (uint32_t *)app_addr;

    __disable_irq();

    SCB->VTOR = app_addr;

    __set_MSP(vector[0]);

    void (*app_reset)(void) = (void (*)(void))vector[1];
    app_reset();
}


📌 此时 App 直接在 QSPI 上执行（XIP）

四、App A / App B 工程（完全一样）
1️⃣ CubeMX（App）

QSPI：不用初始化（Bootloader 已完成）

不要重新配置时钟

不要重置 Cache

2️⃣ App linker script（关键）
App A
FLASH (rx) :
{
  ORIGIN = 0x90010000,
  LENGTH = 2M
}

App B
FLASH (rx) :
{
  ORIGIN = 0x90210000,
  LENGTH = 2M
}


📌 A / B 唯一区别：FLASH 起始地址

3️⃣ App 启动注意事项（新手必看）
void SystemInit(void)
{
    // 空实现
}

int main(void)
{
    // ❌ 不要 HAL_Init()
    // ❌ 不要 SystemClock_Config()

    while (1)
    {
        // 正常业务
    }
}

五、OTA 升级完整流程（重点）
1️⃣ OTA 步骤总览
App 运行中
   ↓
下载新固件 → Download 区
   ↓
CRC 校验
   ↓
更新 OTA Info（update_slot）
   ↓
重启
   ↓
Bootloader 切换 Slot
   ↓
新 App 启动
   ↓
运行成功 → 确认

2️⃣ App 中的 OTA 关键代码
写入 Download 区
qspi_erase(DOWNLOAD_OFFSET);
qspi_write(DOWNLOAD_OFFSET, firmware, size);

更新 OTA Info
ota.update_slot = 1 - ota.active_slot;
ota.upgrade_flag = 1;
ota_write(&ota);
NVIC_SystemReset();

3️⃣ Bootloader 处理升级
if (ota.upgrade_flag)
{
    copy_download_to_slot(ota.update_slot);

    if (verify_app(ota.update_slot))
    {
        ota.active_slot = ota.update_slot;
        ota.upgrade_flag = 0;
        ota.boot_count = 0;
    }
    else
    {
        ota.rollback_flag = 1;
    }
    ota_write(&ota);
}

4️⃣ App 启动成功确认（防止死机）
void ota_confirm_ok(void)
{
    ota_info_t ota;
    ota_read(&ota);

    ota.boot_count = 0;
    ota_write(&ota);
}


📌 若 N 次启动没确认 → 自动回滚

六、Cache & MPU（H750 必须注意）
1️⃣ Bootloader 中
SCB_EnableICache();
SCB_EnableDCache();


QSPI 区域建议：

Cacheable

Bufferable

2️⃣ QSPI Memory-Mapped 必须
HAL_QSPI_MemoryMapped(&hqspi, &cmd, &cfg);

七、J-Link 下载方式（推荐）
1️⃣ Bootloader

J-Link 直接烧内部 Flash

2️⃣ App（调试）

J-Link 支持 QSPI Flash Loader

或 Bootloader 提供 UART/USB 升级

八、你现在已经具备的能力

做到这里，你已经：

✅ 掌握 QSPI XIP
✅ 掌握 工业级 OTA 架构
✅ 理解 A/B 回滚机制
✅ 可用于 量产设备



用 JLinkExe（命令行）下载到外部flash

适合 自动化 / CI / 批量烧录

1️⃣ 启动 JLinkExe
JLinkExe

2️⃣ 输入以下命令（完整示例）
connect
device STM32H750VB
if swd
speed 4000

loadfile QSPI_W25Q64_H750.FLM
loadbin appA.bin,0x90010000

r
g
exit


📌 关键点说明：

loadfile xxx.FLM：加载外部 Flash 算法

loadbin xxx.bin,addr：写入外置 Flash

3️⃣ 验证读取
mem32 0x90010000,4


能看到非 0xFFFFFFFF 即正常