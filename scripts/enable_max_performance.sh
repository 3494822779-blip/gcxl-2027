#!/bin/bash
# 启用Jetson Orin NX最大性能模式
# 需要root权限运行

set -e

if [ "$EUID" -ne 0 ]; then
    echo "请使用sudo运行此脚本"
    echo "sudo $0"
    exit 1
fi

echo "=== 启用Jetson最大性能模式 ==="

# 设置最大性能模式（MAXN模式）
echo "1. 设置MAXN功率模式..."
if command -v nvpmodel &> /dev/null; then
    nvpmodel -m 0
    echo "   ✓ 已设置为MAXN模式（模式0）"
else
    echo "   ⚠ nvpmodel命令未找到"
fi

# 启用jetson_clocks（锁定最大时钟频率）
echo "2. 锁定CPU/GPU到最大时钟频率..."
if command -v jetson_clocks &> /dev/null; then
    jetson_clocks
    echo "   ✓ 已锁定所有时钟到最大值"
else
    echo "   ⚠ jetson_clocks命令未找到"
fi

# 显示当前状态
echo ""
echo "=== 当前性能状态 ==="
if command -v nvpmodel &> /dev/null; then
    echo "功率模式:"
    nvpmodel -q
fi

echo ""
echo "时钟频率状态:"
if command -v jetson_clocks &> /dev/null; then
    jetson_clocks --show 2>&1 | head -15
fi

# 设置GPU最大性能调度器
echo ""
echo "3. 优化GPU调度..."
if [ -f /sys/devices/gpu.0/devfreq/17000000.ga10b/governor ]; then
    echo performance > /sys/devices/gpu.0/devfreq/17000000.ga10b/governor
    echo "   ✓ GPU调度器设置为performance"
fi

# 禁用不必要的后台服务以释放资源
echo ""
echo "4. 优化系统服务..."

# 检查并临时停止桌面环境（如果在无头模式运行）
# 注意：这会关闭图形界面，仅在远程SSH运行时使用
# systemctl stop gdm3 2>/dev/null || true

# 设置CPU调度策略
echo "   设置CPU性能调度器..."
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    if [ -f "$cpu" ]; then
        echo performance > "$cpu" 2>/dev/null || true
    fi
done
echo "   ✓ CPU调度器已优化"

echo ""
echo "=== 性能优化完成 ==="
echo ""
echo "提示："
echo "  - 此配置在重启后会重置"
echo "  - 建议在运行感知任务前每次执行此脚本"
echo "  - 高性能模式会增加功耗和发热"
echo ""
