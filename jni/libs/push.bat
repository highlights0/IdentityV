@echo off
chcp 65001 > nul
echo 正在推送SH文件到设备...

:: 检查ADB
adb devices | find "device" > nul
if errorlevel 1 (
    echo 错误：设备未连接！
    pause
    exit /b 1
)

:: 推送当前目录下的所有.sh文件
for %%f in (*.sh) do (
    echo 推送：%%f
    adb push "%%f" /data/local/tmp/
    adb shell chmod 777 "/data/local/tmp/%%f"
)

:: 如果存在arm64-v8a子文件夹，则推送其中的.sh文件到设备的对应子目录
if exist "arm64-v8a" (
    echo 发现arm64-v8a文件夹，推送其中的sh文件...
    rem 在设备上创建目标子目录（如果不存在）
    adb shell mkdir -p /data/local/tmp/arm64-v8a
    for %%f in (arm64-v8a\*.sh) do (
        echo 推送：%%f
        adb push "%%f" /data/local/tmp/arm64-v8a/
        rem 使用%%~nxf获取纯文件名（不含路径）
        adb shell chmod 777 "/data/local/tmp/arm64-v8a/%%~nxf"
    )
)

echo 完成！
pause