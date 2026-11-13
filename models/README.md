# 模型部署

默认使用 `best.onnx` 和 `best_b1_gpu0_fp16.engine`，运行时通过 `model_dir` 指定它们所在的目录。权重和 engine 为本机部署文件，不应作为普通 Git 源文件提交；已有文件保留，不删除。

运行配置只有 `block_bringup/config/infer_blocks.txt`，标签使用同目录的 `blocks_labels.txt`。`reference/*.example` 是旧 DeepStream 示例，仅供参考，不参与安装和启动。

parser 源码在 `third_party/yolo_parser/nvdsinfer_custom_impl_Yolo`：

```bash
make -C third_party/yolo_parser/nvdsinfer_custom_impl_Yolo CUDA_VER=12.6
```

然后重新构建 `block_bringup`，它将 parser 动态库安装到包 share 的 `lib/`。可通过 CMake 变量 `YOLO_PARSER_LIBRARY` 指定外部构建的动态库。

导出脚本：`scripts/model/export_yoloV8.py`。engine 构建工具：`scripts/model/rebuild_engine_720p.sh`。
