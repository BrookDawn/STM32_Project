@echo off
REM STM32F401xC程序下载脚本
REM 自动查找J-Link并下载程序

echo ==========================================
echo STM32F401xC 程序下载工具
echo ==========================================
echo.

REM 设置J-Link Commander路径
set JLINK_PATH=

REM 检查常见的J-Link安装路径
if exist "C:\Program Files\SEGGER\JLink_V840\JLink.exe" (
    set JLINK_PATH="C:\Program Files\SEGGER\JLink_V840\JLink.exe"
) else if exist "C:\Program Files (x86)\SEGGER\JLink_V840\JLink.exe" (
    set JLINK_PATH="C:\Program Files (x86)\SEGGER\JLink_V840\JLink.exe"
) else if exist "C:\Program Files (x86)\SEGGER\JLink_V794\JLink.exe" (
    set JLINK_PATH="C:\Program Files (x86)\SEGGER\JLink_V794\JLink.exe"
) else if exist "C:\Program Files\SEGGER\JLink_V794\JLink.exe" (
    set JLINK_PATH="C:\Program Files\SEGGER\JLink_V794\JLink.exe"
) else if exist "C:\Program Files (x86)\SEGGER\JLink\JLink.exe" (
    set JLINK_PATH="C:\Program Files (x86)\SEGGER\JLink\JLink.exe"
) else if exist "C:\Program Files\SEGGER\JLink\JLink.exe" (
    set JLINK_PATH="C:\Program Files\SEGGER\JLink\JLink.exe"
) else if exist "C:\SEGGER\JLink_V840\JLink.exe" (
    set JLINK_PATH="C:\SEGGER\JLink_V840\JLink.exe"
) else if exist "C:\SEGGER\JLink_V794\JLink.exe" (
    set JLINK_PATH="C:\SEGGER\JLink_V794\JLink.exe"
) else if exist "C:\SEGGER\JLink\JLink.exe" (
    set JLINK_PATH="C:\SEGGER\JLink\JLink.exe"
) else (
    REM 尝试从系统PATH中查找
    where JLink.exe >nul 2>nul
    if %errorlevel% equ 0 (
        set JLINK_PATH=JLink.exe
    ) else (
        echo 错误: 找不到J-Link Commander
        echo 请确认J-Link已安装
        echo.
        pause
        exit /b 1
    )
)

REM 检查编译输出文件
if not exist "build\F401Cx_test1001.hex" (
    echo 错误: 找不到编译输出文件
    echo 请先编译项目: make all
    echo.
    pause
    exit /b 1
)

echo 正在下载程序...
echo 目标: STM32F401xC
echo 接口: SWD
echo.

REM 执行J-Link脚本
%JLINK_PATH% -CommandFile download.jlink

if %errorlevel% equ 0 (
    echo.
    echo 程序下载成功！
) else (
    echo.
    echo 程序下载失败！
    echo 请检查硬件连接
)

echo.
pause
