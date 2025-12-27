#include "ota.h"
#include "quadspi.h"
#include <string.h>

void ota_read(ota_info_t *ota)
{
    QSPI_Read(OTA_INFO_OFFSET, sizeof(ota_info_t), (uint8_t *)ota);
    if (ota->magic != OTA_MAGIC)
    {
        // Initialize if magic is not found
        memset(ota, 0, sizeof(ota_info_t));
        ota->magic = OTA_MAGIC;
        ota->active_slot = 0; // Default to Slot A
    }
}

void ota_write(ota_info_t *ota)
{
    QSPI_EraseSector(OTA_INFO_OFFSET);
    // Write ota_info_t. It's smaller than a page (256 bytes)
    QSPI_WritePage(OTA_INFO_OFFSET, sizeof(ota_info_t), (uint8_t *)ota);
}

int select_slot(ota_info_t *ota)
{
    return ota->active_slot;
}

int verify_app(int slot, ota_info_t *ota)
{
    // Simplified verification: just check if the stack pointer is in RAM
    // In a real application, you would check CRC

    // For simplicity, let's assume we can use QSPI_Read to check the first 4 bytes
    uint32_t initial_msp;
    QSPI_Read((slot == 0) ? APP_A_OFFSET : APP_B_OFFSET, 4, (uint8_t *)&initial_msp);

    // STM32H750 RAM ranges from 0x20000000 to 0x24000000 (roughly)
    if (initial_msp >= 0x20000000 && initial_msp <= 0x24100000)
    {
        return 1;
    }
    return 0;
}

void rollback(ota_info_t *ota)
{
    ota->active_slot = 1 - ota->active_slot;
    ota->rollback_flag = 0;
    ota->boot_count = 0;
    ota_write(ota);
}

void copy_download_to_slot(uint32_t slot_offset, uint32_t size)
{
    uint8_t buffer[4096];
    uint32_t bytes_to_copy = size;
    uint32_t current_offset = 0;

    while (bytes_to_copy > 0)
    {
        uint32_t chunk_size = (bytes_to_copy > 4096) ? 4096 : bytes_to_copy;

        // Read from Download area
        QSPI_Read(DOWNLOAD_OFFSET + current_offset, chunk_size, buffer);

        // Erase sector in destination slot
        if (current_offset % 4096 == 0)
        {
            QSPI_EraseSector(slot_offset + current_offset);
        }

        // Write to destination slot
        for (uint32_t i = 0; i < chunk_size; i += 256)
        {
            uint32_t page_size = (chunk_size - i > 256) ? 256 : chunk_size - i;
            QSPI_WritePage(slot_offset + current_offset + i, page_size, &buffer[i]);
        }

        bytes_to_copy -= chunk_size;
        current_offset += chunk_size;
    }
}
