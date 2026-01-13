@echo off
chcp 65001 >nul
echo ========================================
echo 🔍 ESP8266 串口监视器
echo ========================================
echo.
echo 📌 操作提示：
echo 1. 如果没有输出，请按设备上的 RST 按钮
echo 2. 或者拔出USB线重新插入
echo 3. 按 Ctrl+C 退出监视器
echo.
echo ========================================
echo.

cd /d D:\pythonproject\clock
pio device monitor --baud 115200

