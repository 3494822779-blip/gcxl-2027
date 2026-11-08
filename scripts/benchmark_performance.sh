#!/bin/bash
# NVMM 性能采样：不订阅像素，不启动任何执行节点。
set -e
RESULT_DIR="/tmp/nvmm_perf_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"
echo "结果目录: $RESULT_DIR"
echo "采集 FPS、驱动丢帧、跟踪延迟和 OSD 匹配见 perception 每五秒日志。"
timeout 30s ros2 topic hz /detections_meta > "$RESULT_DIR/detections_hz.txt" 2>&1 &
DETECT_JOB=$!
timeout 30s ros2 topic hz /tracks_meta > "$RESULT_DIR/tracks_hz.txt" 2>&1 &
TRACK_JOB=$!
ps -C perception -o pid,%cpu,%mem,rss,etime,comm > "$RESULT_DIR/process.txt"
wait "$DETECT_JOB" || true
wait "$TRACK_JOB" || true
tail -n 3 "$RESULT_DIR/detections_hz.txt"
tail -n 3 "$RESULT_DIR/tracks_hz.txt"
cat "$RESULT_DIR/process.txt"
