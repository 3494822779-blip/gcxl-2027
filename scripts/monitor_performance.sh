#!/bin/bash
# 实时监控感知系统性能指标
# 包括FPS、GPU利用率、CPU使用率、内存、温度等

echo "=== 感知系统性能监控 ==="
echo "按Ctrl+C退出"
echo ""

# 检查tegrastats是否可用
if ! command -v tegrastats &> /dev/null; then
    echo "警告: tegrastats未找到，无法显示完整的硬件统计"
    echo ""
fi

# 启动tegrastats在后台
TEGRA_LOG="/tmp/tegrastats_$$.log"
if command -v tegrastats &> /dev/null; then
    tegrastats --interval 1000 --logfile "$TEGRA_LOG" &
    TEGRA_PID=$!
    trap "kill $TEGRA_PID 2>/dev/null; rm -f $TEGRA_LOG; exit" INT TERM EXIT
fi

# 主监控循环
while true; do
    clear
    echo "=== Jetson Orin NX 性能监控 === $(date +'%H:%M:%S')"
    echo ""

    # ROS2话题帧率统计
    echo "【ROS2话题帧率】"
    timeout 2s ros2 topic hz /camera/image_raw 2>/dev/null | grep "average rate" || echo "  /camera/image_raw: 无数据"
    timeout 2s ros2 topic hz /detections 2>/dev/null | grep "average rate" || echo "  /detections: 无数据"
    timeout 2s ros2 topic hz /tracks 2>/dev/null | grep "average rate" || echo "  /tracks: 无数据"
    echo ""

    # GPU统计
    if [ -f "$TEGRA_LOG" ]; then
        echo "【GPU状态】"
        tail -1 "$TEGRA_LOG" | grep -oP 'GR3D_FREQ \K[0-9]+%' | head -1 | xargs -I {} echo "  利用率: {}%"
        tail -1 "$TEGRA_LOG" | grep -oP 'GPU@[0-9.]+C' | head -1 | xargs -I {} echo "  温度: {}"
        tail -1 "$TEGRA_LOG" | grep -oP 'VDD_GPU_SOC [0-9]+mW' | head -1 | xargs -I {} echo "  功耗: {}"
        echo ""
    fi

    # CPU统计
    echo "【CPU状态】"
    if [ -f "$TEGRA_LOG" ]; then
        tail -1 "$TEGRA_LOG" | grep -oP 'CPU \[\K[^\]]+' | tr ',' '\n' | nl -v 0 | sed 's/^/  核心/'
        tail -1 "$TEGRA_LOG" | grep -oP 'CPU@[0-9.]+C' | head -1 | xargs -I {} echo "  温度: {}"
    else
        top -bn1 | grep "Cpu(s)" | sed 's/.*, *\([0-9.]*\)%* id.*/\1/' | awk '{print "  平均使用率: " 100 - $1 "%"}'
    fi
    echo ""

    # 内存统计
    echo "【内存状态】"
    if [ -f "$TEGRA_LOG" ]; then
        tail -1 "$TEGRA_LOG" | grep -oP 'RAM [0-9]+/[0-9]+MB' | xargs -I {} echo "  {}"
    else
        free -h | grep Mem | awk '{print "  已用/总计: " $3 "/" $2}'
    fi
    echo ""

    # DeepStream进程状态
    echo "【DeepStream进程】"
    DETECTOR_PID=$(pgrep -f "detector_node")
    TRACKER_PID=$(pgrep -f "tracker_node")

    if [ -n "$DETECTOR_PID" ]; then
        DETECTOR_CPU=$(ps -p $DETECTOR_PID -o %cpu --no-headers)
        DETECTOR_MEM=$(ps -p $DETECTOR_PID -o %mem --no-headers)
        echo "  detector_node (PID $DETECTOR_PID): CPU ${DETECTOR_CPU}%, MEM ${DETECTOR_MEM}%"
    else
        echo "  detector_node: 未运行"
    fi

    if [ -n "$TRACKER_PID" ]; then
        TRACKER_CPU=$(ps -p $TRACKER_PID -o %cpu --no-headers)
        TRACKER_MEM=$(ps -p $TRACKER_PID -o %mem --no-headers)
        echo "  tracker_node (PID $TRACKER_PID): CPU ${TRACKER_CPU}%, MEM ${TRACKER_MEM}%"
    else
        echo "  tracker_node: 未运行"
    fi
    echo ""

    # 功率模式
    if command -v nvpmodel &> /dev/null; then
        echo "【功率模式】"
        nvpmodel -q 2>/dev/null | grep "NV Power Mode" | sed 's/^/  /'
        echo ""
    fi

    echo "刷新间隔: 3秒"
    sleep 3
done
