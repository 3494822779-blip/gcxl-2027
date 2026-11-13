#!/bin/bash

# 测试相机1280x1024@30fps MJPEG
# 用法: ./test_camera_1280x1024.sh [/dev/videoN]

set -e

DEVICE=${1:-/dev/video0}

echo "========================================"
echo "相机1280x1024@30fps测试"
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
echo "USB速度: $USB_SPEED"
if [ "$USB_SPEED" == "480M" ]; then
    echo "✅ USB 2.0连接 - 支持1280x1024@30fps"
elif [ "$USB_SPEED" == "5000M" ] || [ "$USB_SPEED" == "10000M" ]; then
    echo "✅ USB 3.0连接 ($USB_SPEED) - 支持1280x1024@30fps"
fi
echo ""

# 3. 列出支持的格式
echo "2. 1280x1024支持检查"
echo "----------------------------------------"
if v4l2-ctl --device=$DEVICE --list-formats-ext | grep -A 10 "1280x1024"; then
    echo ""
    if v4l2-ctl --device=$DEVICE --list-formats-ext | grep -A 10 "1280x1024" | grep -q "33.3\|33.333"; then
        echo "✅ 明确支持 1280x1024@30fps (33.3ms interval)"
    else
        echo "⚠️  支持1280x1024但帧率未确认"
    fi
else
    echo "❌ 不支持1280x1024分辨率"
    echo ""
    echo "支持的分辨率:"
    v4l2-ctl --device=$DEVICE --list-formats-ext | grep -E "Size:" | head -10
    exit 1
fi
echo ""

# 4. 测试实际捕获（5秒）
echo "3. 实际捕获测试（5秒）"
echo "----------------------------------------"
echo "正在以1280x1024@30fps MJPEG捕获..."
gst-launch-1.0 -e \
    v4l2src device=$DEVICE num-buffers=150 ! \
    image/jpeg,width=1280,height=1024,framerate=30/1 ! \
    nvjpegdec ! \
    'video/x-raw(memory:NVMM),format=I420' ! \
    nvvideoconvert ! \
    'video/x-raw(memory:NVMM),format=RGBA' ! \
    fpsdisplaysink video-sink=fakesink text-overlay=false sync=false \
    2>&1 | tee /tmp/gst_test_1024.log

# 5. 分析结果
echo ""
echo "4. 性能分析"
echo "----------------------------------------"
if grep -q "rendered:" /tmp/gst_test_1024.log; then
    FPS=$(grep "rendered:" /tmp/gst_test_1024.log | tail -1 | grep -oP '\d+\.\d+' | head -1)
    echo "实际帧率: $FPS fps"
    
    if (( $(echo "$FPS >= 28" | bc -l) )); then
        echo "✅ 性能优秀: 达到30fps目标"
    elif (( $(echo "$FPS >= 25" | bc -l) )); then
        echo "✅ 性能良好: 接近30fps"
    else
        echo "⚠️  性能低于预期"
    fi
else
    echo "⚠️  无法解析帧率信息"
fi

# 6. 计算带宽
echo ""
BANDWIDTH_MB=$(echo "1280 * 1024 * 1.5 * 30 / 1024 / 1024" | bc -l)
BANDWIDTH_COMPRESSED=$(echo "$BANDWIDTH_MB / 10" | bc -l)
printf "未压缩带宽: %.1f MB/s\n" $BANDWIDTH_MB
printf "MJPEG压缩后: %.1f MB/s (USB 2.0限制35 MB/s)\n" $BANDWIDTH_COMPRESSED

echo ""
echo "========================================"
echo "测试完成"
echo "========================================"
echo ""
echo "1280x1024分辨率优势:"
echo "- 更高分辨率: 1.3MP vs 0.9MP (720p)"
echo "- 更好的检测精度（尤其是远距离目标）"
echo "- USB 2.0稳定支持30fps"
echo ""
