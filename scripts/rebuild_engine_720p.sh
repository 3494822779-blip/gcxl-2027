#!/bin/bash
# 重新生成优化的TensorRT引擎以支持720p@30fps
# 这会针对1280x720分辨率优化inference性能

set -e

MODEL_DIR="/home/nvidia/project/gcxl-2027/models/yolov8n_gcxl_aug"
ONNX_FILE="$MODEL_DIR/best.onnx"
ENGINE_FILE="$MODEL_DIR/best_b1_gpu0_fp16_1280x720.engine"

if [ ! -f "$ONNX_FILE" ]; then
    echo "错误: ONNX模型文件不存在: $ONNX_FILE"
    exit 1
fi

echo "=== 生成优化的TensorRT引擎 ==="
echo "ONNX模型: $ONNX_FILE"
echo "输出引擎: $ENGINE_FILE"
echo "优化分辨率: 1280x720"
echo ""

# 使用trtexec生成引擎
# 参数说明:
#   --onnx: ONNX模型路径
#   --saveEngine: 输出引擎路径
#   --fp16: 启用FP16精度（在Jetson上性能提升显著）
#   --workspace: 最大工作空间（MB）
#   --minShapes/optShapes/maxShapes: 动态shape范围，固定为720p
#   --tacticSources: 使用cuDNN和cuBLAS优化策略
#   --verbose: 显示详细日志

echo "开始构建引擎（这需要5-10分钟）..."
/usr/src/tensorrt/bin/trtexec \
    --onnx="$ONNX_FILE" \
    --saveEngine="$ENGINE_FILE" \
    --fp16 \
    --workspace=2048 \
    --minShapes=images:1x3x720x1280 \
    --optShapes=images:1x3x720x1280 \
    --maxShapes=images:1x3x720x1280 \
    --tacticSources=+CUDNN,+CUBLAS \
    --verbose

if [ -f "$ENGINE_FILE" ]; then
    echo ""
    echo "=== 引擎生成成功 ==="
    ls -lh "$ENGINE_FILE"
    echo ""
    echo "现在可以使用720p分辨率运行感知系统"
else
    echo ""
    echo "=== 引擎生成失败 ==="
    exit 1
fi
