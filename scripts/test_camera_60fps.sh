#!/bin/bash

# 测试相机是否支持720p@60fps MJPEG
# 用法: ./test_camera_60fps.sh [/dev/videoN]

set -e

DEVICE=${1:-/dev/video0}

echo "========================================"
echo "相机60fps支持测试"
echo "========================================"
echo "设备: $DEVICE"
echo ""

# 1. 检查设备是否存在
if [ ! -e "$DEVICE" ]; then
    echo "❌ 错误: 设备 $DEVICE 不存在"
    echo "可用设备:"
    ls -l /dev/video* 2>/dev/null || echo "  未找到video设备"
    exit 1
fi

# 2. 检查USB连接速度
echo "1. USB连接检查"
echo "----------------------------------------"
USB_SPEED=$(lsusb -t | grep -i "video\|camera" -A 3 | grep -oP '\d+M' | head -1 || echo "unknown")
if [ "$USB_SPEED" == "5000M" ] || [ "$USB_SPEED" == "10000M" ]; then
    echo "✅ USB 3.0连接 ($USB_SPEED) - 支持60fps"
elif [ "$USB_SPEED" == "480M" ]; then
    echo "⚠️  USB 2.0连接 ($USB_SPEED) - 可能不支持720p@60fps"
    echo "   建议: 连接到蓝色USB 3.0端口"
elif [ "$USB_SPEED" == "unknown" ]; then
    echo "⚠️  无法检测USB速度，请手动确认使用USB 3.0端口"
fi
echo ""

# 3. 列出支持的格式
echo "2. 支持的MJPEG格式"
echo "----------------------------------------"
v4l2-ctl --device=$DEVICE --list-formats-ext | grep -A 20 "MJPEG" | grep -E "Size:|Interval:" | head -20
echo ""

# 4. 检查是否明确支持720p@60fps
echo "3. 720p@60fps支持检查"
echo "----------------------------------------"
if v4l2-ctl --device=$DEVICE --list-formats-ext | grep -A 10 "1280x720" | grep -q "16.7\|16.666"; then
    echo "✅ 明确支持 1280x720@60fps (16.7ms interval)"
elif v4l2-ctl --device=$DEVICE --list-formats-ext | grep -q "1280x720"; then
    echo "⚠️  支持1280x720但未确认60fps"
    echo "   可用帧率:"
    v4l2-ctl --device=$DEVICE --list-formats-ext | grep -A 10 "1280x720" | grep "Interval:"
else
    echo "❌ 不支持1280x720分辨率"
fi
echo ""

# 5. 测试实际捕获（5秒）
echo "4. 实际捕获测试（5秒）"
echo "----------------------------------------"
echo "正在以720p@60fps MJPEG捕获..."
gst-launch-1.0 -e \
    v4l2src device=$DEVICE num-buffers=300 ! \
    image/jpeg,width=1280,height=720,framerate=60/1 ! \
    nvjpegdec ! \
    'video/x-raw(memory:NVMM),format=I420' ! \
    nvvideoconvert ! \
    'video/x-raw(memory:NVMM),format=RGBA' ! \
    fpsdisplaysink video-sink=fakesink text-overlay=false sync=false \
    2>&1 | tee /tmp/gst_test.log

# 6. 分析结果
echo ""
echo "5. 性能分析"
echo "----------------------------------------"
if grep -q "rendered:" /tmp/gst_test.log; then
    FPS=$(grep "rendered:" /tmp/gst_test.log | tail -1 | grep -oP '\d+\.\d+' | head -1)
    echo "实际帧率: $FPS fps"

    if (( $(echo "$FPS >= 58" | bc -l) )); then
        echo "✅ 性能优秀: 达到60fps目标"
    elif (( $(echo "$FPS >= 55" | bc -l) )); then
        echo "✅ 性能良好: 接近60fps"
    elif (( $(echo "$FPS >= 50" | bc -l) )); then
        echo "⚠️  性能一般: 未达到60fps，可能存在瓶颈"
    else
        echo "❌ 性能差: 严重低于60fps"
    fi
else
    echo "⚠️  无法解析帧率信息"
fi

echo ""
echo "========================================"
echo "测试完成"
echo "========================================"
echo ""
echo "建议:"
echo "- 确保使用USB 3.0蓝色端口"
echo "- 如果帧率低，检查系统负载: htop"
echo "- 确保MAXN模式: sudo nvpmodel -q"
echo "- 锁定时钟: sudo jetson_clocks"
echo ""
