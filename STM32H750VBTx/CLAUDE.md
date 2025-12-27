# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This repository contains two main STM32 projects: `bootloader_1` and `APP_1`. Both use CMake.

### Build Bootloader
```bash
cd bootloader_1
# Configure
cmake -B build -D CMAKE_BUILD_TYPE=Debug -D CMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -G "Unix Makefiles"
# Build
cmake --build build
```

### Build Application
```bash
cd APP_1
# Configure
cmake -B build -D CMAKE_BUILD_TYPE=Debug -D CMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -G "Unix Makefiles"
# Build
cmake --build build
```

## Flashing & Debugging

Flashing is handled via J-Link. A `download.bat` script is provided in the root directory.

- **Flash Bootloader**: `download.bat boot` (Flashes to internal Flash at 0x08000000)
- **Flash Application**: `download.bat app` (Flashes to external QSPI Flash via custom device)
- **Test Connection**: `download.bat test` (Tests QSPI connection and reads Flash ID)

## High-Level Architecture

The system implements an **Industrial-grade OTA (Over-The-Air)** update mechanism with **A/B Slot** redundancy and **QSPI XIP (Execute-In-Place)**.

### Memory Map
- **Internal Flash (128KB)**: Bootloader (0x08000000)
- **External QSPI Flash (8MB)**: Mapped to 0x90000000
    - `0x000000` (0x90000000): OTA Info (64KB) - Stores metadata for boot selection.
    - `0x010000` (0x90010000): App A (2MB)
    - `0x210000` (0x90210000): App B (2MB)
    - `0x410000` (0x90410000): Download Area (2MB)

### Bootloader Workflow
1. Initializes minimum system and QSPI in Indirect mode.
2. Reads `ota_info_t` from the OTA Info area.
3. Selects the active slot (A or B), verifies CRC/integrity.
4. Switches QSPI to Memory-Mapped mode.
5. Sets Vector Table Offset (`SCB->VTOR`) and jumps to the App.

### OTA Update Workflow
1. Running App downloads new firmware into the **Download Area**.
2. App updates `ota_info_t` (sets `update_slot` and `upgrade_flag`) and resets.
3. Bootloader detects `upgrade_flag`, copies firmware from Download Area to the target slot, verifies it, updates `active_slot`, and clears the flag.
4. If the new App fails to boot (tracked via `boot_count`), the Bootloader can roll back to the previous known good slot.

### Code Structure
- `bootloader_1/Core/Src/ota.c`: Contains the core boot selection and firmware management logic.
- `APP_1/Core/Src/ota.c`: Contains the logic for the App to trigger updates and confirm successful boot.
- `STM32H750XX_FLASH.ld`: Linker scripts for both projects. App linker scripts must match their respective slot ORIGIN (0x90010000 or 0x90210000).
