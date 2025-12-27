@echo off
cd /d %~dp0
set JLINK_PATH="C:\Program Files\SEGGER\JLink_V840\JLink.exe"
set JFLASH_PATH="C:\Program Files\SEGGER\JLink_V840\JFlash.exe"
set DEVICE=STM32H750VB
set FLM_PATH=FANKE_FK750M1_V1.FLM

echo ========================================
echo STM32H750VB Flash Download Tool
echo ========================================
echo.

if "%1"=="boot" (
    echo [1/1] Downloading Bootloader to internal Flash...
    %JLINK_PATH% -device %DEVICE% -if SWD -speed 4000 -autoconnect 1 -CommandFile download_bootloader.jlink
    echo.
    echo ========================================
    echo Bootloader download complete!
    echo ========================================
) else if "%1"=="app" (
    echo [0/1] Registering Custom Flash Device...
    if not exist "%AppData%\SEGGER\JLinkDevices" mkdir "%AppData%\SEGGER\JLinkDevices"
    copy /y "JLinkDevices.xml" "%AppData%\SEGGER\JLinkDevices\" >nul
    echo [1/1] Downloading APP to QSPI Flash via Custom Device...
    %JLINK_PATH% -device STM32H750VB_FANKE -if SWD -speed 4000 -autoconnect 1 -CommandFile download_app.jlink
    if %errorlevel% neq 0 (
        echo.
        echo ERROR: J-Link download failed with error code %errorlevel%
        exit /b %errorlevel%
    )
    echo.
    echo ========================================
    echo APP download complete!
    echo ========================================
) else if "%1"=="test" (
    echo [1/1] Testing connection and reading Flash ID...
    %JLINK_PATH% -device %DEVICE% -if SWD -speed 4000 -autoconnect 1 -CommandFile test_qspi_read_id.jlink
    echo.
    echo ========================================
    echo Test complete!
    echo ========================================
) else (
    echo Usage: download.bat [boot^|app^|test]
    echo.
    echo Commands:
    echo   boot  - Download bootloader to internal Flash
    echo   app   - Show instructions for APP download
    echo   test  - Test QSPI connection and read Flash ID
    echo.
)
exit /b %errorlevel%
