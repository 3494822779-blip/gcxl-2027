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

# 解析命令行参数
TASK="normal"
DISPLAY_VIDEO="true"
HEADLESS="false"
SERIAL="true"

while [[ $# -gt 0 ]]; do
    case $1 in
        --first-grasp)
            TASK="first_grasp"
            shift
            ;;
        --no-display)
            DISPLAY_VIDEO="false"
            shift
            ;;
        --headless)
            HEADLESS="true"
            shift
            ;;
        --no-serial)
            SERIAL="false"
            shift
            ;;
        *)
            echo "未知参数: $1"
            echo "用法: $0 [--first-grasp] [--no-display] [--headless] [--no-serial]"
            exit 1
            ;;
    esac
done

echo "启动配置："
echo "  任务模式: $TASK"
echo "  NVMM 本机显示: $DISPLAY_VIDEO (headless=$HEADLESS)"
echo "  串口: $SERIAL"
echo ""

# Source ROS环境
cd "$PROJECT_ROOT/ros2_ws"
source /opt/ros/humble/setup.bash
source install_gcxl/setup.bash

# 启动对应的launch文件
if [ "$TASK" = "first_grasp" ]; then
    echo "启动首次抓取任务..."
    echo ""
    ros2 launch block_perception first_grasp.launch.py display:=$DISPLAY_VIDEO headless:=$HEADLESS serial:=$SERIAL
else
    echo "启动普通抓取任务..."
    echo ""
    ros2 launch block_perception perception.launch.py display:=$DISPLAY_VIDEO headless:=$HEADLESS serial:=$SERIAL
fi
