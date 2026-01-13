@echo off
chcp 65001 >nul
echo ========================================
echo 🚀 快速部署脚本
echo ========================================
echo.

REM 步骤1：关闭可能占用串口的程序
echo [1/4] 正在释放串口...
taskkill /F /IM arduino.exe 2>nul
taskkill /F /IM putty.exe 2>nul
timeout /t 2 /nobreak >nul

REM 步骤2：重新烧录固件
echo [2/4] 正在烧录固件到 ESP8266...
cd /d D:\pythonproject\clock
pio run --target upload

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ 烧录失败！
    echo.
    echo 解决方法：
    echo 1. 拔出ESP8266的USB线
    echo 2. 等待5秒
    echo 3. 重新插入USB线
    echo 4. 再次运行此脚本
    echo.
    pause
    exit /b 1
)

echo [3/4] 烧录成功！等待设备重启...
timeout /t 3 /nobreak >nul

REM 步骤3：启动串口监视器
echo [4/4] 正在启动串口监视器...
echo.
echo ========================================
echo 📺 串口输出（查找设备IP地址）
echo ========================================
echo.
echo 按 Ctrl+C 退出监视器
echo.

pio device monitor --baud 115200

