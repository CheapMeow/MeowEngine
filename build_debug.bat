@echo off

cls

REM Configure a debug build
cmake -S . -B build/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ --parallel 8 --config Debug

REM 锁定 UTF-8
chcp 65001 >nul

REM Windows 气泡提示
powershell -NoProfile -WindowStyle Hidden -Command "[void][System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); $obj=New-Object System.Windows.Forms.NotifyIcon; $obj.Icon=[System.Drawing.Icon]::ExtractAssociatedIcon('C:\Windows\explorer.exe'); $obj.BalloonTipIcon='Info'; $obj.BalloonTipTitle='MeowEngine'; $obj.BalloonTipText='构建完成。'; $obj.Visible=$true; $obj.ShowBalloonTip(10000);"

pause