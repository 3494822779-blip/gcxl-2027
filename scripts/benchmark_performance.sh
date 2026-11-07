#!/bin/bash
# 性能对比测试脚本
# 比较优化前后的帧率、延迟和资源使用

set -e

echo "=== 感知系统性能对比测试 ==="
echo ""
echo "此脚本将测试并记录系统性能指标"
echo "测试项目："
echo "  1. 相机话题帧率 (/camera/image_raw)"
echo "  2. 检测话题帧率 (/detections)"
echo "  3. 跟踪话题帧率 (/tracks)"
echo "  4. 平均延迟"
echo "  5. GPU/CPU利用率"
echo ""

# 检查ROS2环境
if ! command -v ros2 &> /dev/null; then
    echo "错误: ROS2环境未加载"
    echo "请先运行: source /opt/ros/humble/setup.bash"
    exit 1
fi

# 检查节点是否运行
RUNNING_NODES=$(ros2 node list 2>/dev/null)
if ! echo "$RUNNING_NODES" | grep -q "camera"; then
    echo "警告: 未检测到camera节点"
    echo "请先启动感知系统"
    exit 1
fi

# 创建结果目录
RESULT_DIR="/tmp/perf_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"

echo "测试结果将保存到: $RESULT_DIR"
echo ""
echo "开始测试（每项测试30秒）..."
echo ""

# 测试1: 相机帧率
echo "【测试1/5】相机话题帧率..."
timeout 30s ros2 topic hz /camera/image_raw > "$RESULT_DIR/camera_hz.txt" 2>&1 || true
CAMERA_HZ=$(grep "average rate" "$RESULT_DIR/camera_hz.txt" | awk '{print $3}')
echo "  结果: ${CAMERA_HZ:-N/A} Hz"

# 测试2: 检测帧率
echo "【测试2/5】检测话题帧率..."
timeout 30s ros2 topic hz /detections > "$RESULT_DIR/detections_hz.txt" 2>&1 || true
DETECT_HZ=$(grep "average rate" "$RESULT_DIR/detections_hz.txt" | awk '{print $3}')
echo "  结果: ${DETECT_HZ:-N/A} Hz"

# 测试3: 跟踪帧率
echo "【测试3/5】跟踪话题帧率..."
timeout 30s ros2 topic hz /tracks > "$RESULT_DIR/tracks_hz.txt" 2>&1 || true
TRACK_HZ=$(grep "average rate" "$RESULT_DIR/tracks_hz.txt" | awk '{print $3}')
echo "  结果: ${TRACK_HZ:-N/A} Hz"

# 测试4: 延迟测试（相机到跟踪）
echo "【测试4/5】端到端延迟..."
# 采样30个消息，计算平均延迟
{
    echo "timestamp,latency_ms" > "$RESULT_DIR/latency.csv"
    timeout 30s ros2 topic echo /tracks --field image.header.stamp --once 2>/dev/null | head -30 | while read line; do
        if [[ $line =~ sec:\ ([0-9]+) ]]; then
            SEC="${BASH_REMATCH[1]}"
        fi
        if [[ $line =~ nanosec:\ ([0-9]+) ]]; then
            NANOSEC="${BASH_REMATCH[1]}"
            TIMESTAMP_MS=$((SEC * 1000 + NANOSEC / 1000000))
            NOW_MS=$(date +%s%3N)
            LATENCY=$((NOW_MS - TIMESTAMP_MS))
            echo "$NOW_MS,$LATENCY" >> "$RESULT_DIR/latency.csv"
        fi
    done
} 2>/dev/null || true

if [ -f "$RESULT_DIR/latency.csv" ] && [ $(wc -l < "$RESULT_DIR/latency.csv") -gt 1 ]; then
    AVG_LATENCY=$(awk -F',' 'NR>1 {sum+=$2; count++} END {if(count>0) print sum/count; else print "N/A"}' "$RESULT_DIR/latency.csv")
    echo "  结果: ${AVG_LATENCY} ms"
else
    echo "  结果: N/A"
fi

# 测试5: 资源使用
echo "【测试5/5】GPU/CPU使用率（采样10次）..."
{
    echo "sample,gpu_util,cpu_util,gpu_temp,cpu_temp" > "$RESULT_DIR/resource.csv"
    for i in {1..10}; do
        if command -v tegrastats &> /dev/null; then
            TEGRA_SAMPLE=$(timeout 1s tegrastats 2>/dev/null | head -1)
            GPU_UTIL=$(echo "$TEGRA_SAMPLE" | grep -oP 'GR3D_FREQ \K[0-9]+' || echo "0")
            CPU_UTIL=$(echo "$TEGRA_SAMPLE" | grep -oP 'CPU \[\K[^\]]+' | awk -F',' '{sum=0; for(i=1;i<=NF;i++) {gsub(/%/,"",$i); sum+=$i}} END {print sum/NF}')
            GPU_TEMP=$(echo "$TEGRA_SAMPLE" | grep -oP 'GPU@\K[0-9.]+' || echo "0")
            CPU_TEMP=$(echo "$TEGRA_SAMPLE" | grep -oP 'CPU@\K[0-9.]+' || echo "0")
            echo "$i,$GPU_UTIL,$CPU_UTIL,$GPU_TEMP,$CPU_TEMP" >> "$RESULT_DIR/resource.csv"
        fi
        sleep 1
    done
} 2>/dev/null || true

if [ -f "$RESULT_DIR/resource.csv" ] && [ $(wc -l < "$RESULT_DIR/resource.csv") -gt 1 ]; then
    AVG_GPU=$(awk -F',' 'NR>1 {sum+=$2; count++} END {if(count>0) print sum/count; else print "N/A"}' "$RESULT_DIR/resource.csv")
    AVG_CPU=$(awk -F',' 'NR>1 {sum+=$3; count++} END {if(count>0) print sum/count; else print "N/A"}' "$RESULT_DIR/resource.csv")
    echo "  GPU利用率: ${AVG_GPU}%"
    echo "  CPU利用率: ${AVG_CPU}%"
else
    echo "  结果: N/A"
fi

# 生成总结报告
echo ""
echo "=== 测试完成 ==="
echo ""
cat > "$RESULT_DIR/summary.txt" << EOF
性能测试总结
测试时间: $(date)
系统: Jetson Orin NX

帧率指标:
  相机帧率: ${CAMERA_HZ:-N/A} Hz
  检测帧率: ${DETECT_HZ:-N/A} Hz
  跟踪帧率: ${TRACK_HZ:-N/A} Hz

延迟指标:
  平均延迟: ${AVG_LATENCY:-N/A} ms

资源使用:
  GPU平均利用率: ${AVG_GPU:-N/A}%
  CPU平均利用率: ${AVG_CPU:-N/A}%

详细数据文件:
  - camera_hz.txt
  - detections_hz.txt
  - tracks_hz.txt
  - latency.csv
  - resource.csv
EOF

cat "$RESULT_DIR/summary.txt"
echo ""
echo "完整结果已保存到: $RESULT_DIR"
