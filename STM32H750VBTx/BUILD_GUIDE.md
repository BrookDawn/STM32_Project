# STM32H750 CMake Build Guide

This document describes how to build the Bootloader and Application projects using CMake.

## Prerequisites
- ARM GCC Toolchain (`arm-none-eabi-gcc`)
- CMake (version 3.20 or higher)
- Build system (e.g., `make` or `ninja`)

## Build Bootloader
```bash
cd bootloader_1
# Clean build directory (optional)
rm -rf build
# Configure for Debug
cmake -B build -D CMAKE_BUILD_TYPE=Debug -D CMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -G "Unix Makefiles"
# Build
cmake --build build
```
Output: `bootloader_1/build/bootloader_1.bin`, `bootloader_1.elf`, `bootloader_1.hex`

## Build Application (APP_1)
```bash
cd APP_1
# Clean build directory (optional)
rm -rf build
# Configure for Debug
cmake -B build -D CMAKE_BUILD_TYPE=Debug -D CMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -G "Unix Makefiles"
# Build
cmake --build build
```
Output: `APP_1/build/APP_1.bin`, `APP_1.elf`, `APP_1.hex`

## Flashing
Use the `download.bat` script in the root directory:
- `download.bat boot` - Flash bootloader
- `download.bat app` - Flash application
