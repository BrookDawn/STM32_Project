#ifndef __OTA_H
#define __OTA_H

#include "main.h"

#define OTA_MAGIC 0x4F544131  // "OTA1"

#define QSPI_BASE_ADDR   0x90000000

#define OTA_INFO_OFFSET  0x000000
#define APP_A_OFFSET     0x010000
#define APP_B_OFFSET     0x210000
#define DOWNLOAD_OFFSET  0x410000

#define OTA_INFO_ADDR    (QSPI_BASE_ADDR + OTA_INFO_OFFSET)
#define APP_A_ADDR       (QSPI_BASE_ADDR + APP_A_OFFSET)
#define APP_B_ADDR       (QSPI_BASE_ADDR + APP_B_OFFSET)
#define DOWNLOAD_ADDR    (QSPI_BASE_ADDR + DOWNLOAD_OFFSET)

#define APP_SIZE_MAX     (2 * 1024 * 1024) // 2MB

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

void ota_read(ota_info_t *ota);
void ota_write(ota_info_t *ota);
int select_slot(ota_info_t *ota);
int verify_app(int slot, ota_info_t *ota);
void rollback(ota_info_t *ota);
void copy_download_to_slot(uint32_t slot_offset, uint32_t size);

#endif /* __OTA_H */
