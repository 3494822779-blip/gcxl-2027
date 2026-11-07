#!/bin/bash
# 快速启动优化性能的感知系统
# 自动启用MAXN模式并启动perception

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== 物块抓取感知系统 - 高性能模式 ==="
echo ""

# 检查是否已经是MAXN模式
CURRENT_MODE=$(nvpmodel -q 2>/dev/null | grep "NV Power Mode" | grep -oP "MODE_\d+" || echo "")
if [ "$CURRENT_MODE" != "MODE_0" ]; then
    echo "当前功率模式: $CURRENT_MODE"
    echo "需要启用MAXN模式（MODE_0）以获得最佳性能"
    echo ""
    echo "运行以下命令启用MAXN模式（需要sudo权限）："
    echo "  sudo $SCRIPT_DIR/enable_max_performance.sh"
    echo ""
    read -p "是否现在启用MAXN模式？(y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo "$SCRIPT_DIR/enable_max_performance.sh"
        echo ""
    else
        echo "跳过MAXN模式设置，继续启动（性能可能不是最佳）"
        echo ""
    fi
else
    echo "✓ 已启用MAXN模式"
    echo ""
fi

# 检查TensorRT引擎文件
ENGINE_FILE="$PROJECT_ROOT/models/yolov8n_gcxl_aug/best_b1_gpu0_fp16_1280x720.engine"
if [ ! -f "$ENGINE_FILE" ]; then
    echo "⚠ 警告: 未找到720p优化的TensorRT引擎"
    echo "   路径: $ENGINE_FILE"
    echo ""
    echo "需要先生成TensorRT引擎（约5-10分钟）"
    echo "运行: $SCRIPT_DIR/rebuild_engine_720p.sh"
    echo ""
    read -p "是否现在生成引擎？(y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        "$SCRIPT_DIR/rebuild_engine_720p.sh"
        echo ""
    else
        echo "跳过引擎生成，将使用已有引擎（如果存在）"
        echo ""
    fi
else
    echo "✓ 已找到720p TensorRT引擎"
    echo ""
fi

# 解析命令行参数
TASK="normal"
CAMERA="true"
SERIAL="true"

while [[ $# -gt 0 ]]; do
    case $1 in
        --first-grasp)
            TASK="first_grasp"
            shift
            ;;
        --no-camera)
            CAMERA="false"
            shift
            ;;
        --no-serial)
            SERIAL="false"
            shift
            ;;
        *)
            echo "未知参数: $1"
            echo "用法: $0 [--first-grasp] [--no-camera] [--no-serial]"
            exit 1
            ;;
    esac
done

echo "启动配置："
echo "  任务模式: $TASK"
echo "  相机: $CAMERA"
echo "  串口: $SERIAL"
echo ""

# Source ROS环境
cd "$PROJECT_ROOT/ros2_ws"
source /opt/ros/humble/setup.bash
source install/setup.bash

# 启动对应的launch文件
if [ "$TASK" = "first_grasp" ]; then
    echo "启动首次抓取任务..."
    echo ""
    ros2 launch block_perception first_grasp.launch.py camera:=$CAMERA serial:=$SERIAL
else
    echo "启动普通抓取任务..."
    echo ""
    ros2 launch block_perception perception.launch.py camera:=$CAMERA serial:=$SERIAL
fi
