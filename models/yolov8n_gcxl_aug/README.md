# GCXL YOLOv8n augmented model

- Source: `rog@192.168.31.152:/media/rog/D0EEA8CAEEA8A9E4/gcxl_2027/runs/detect/yolov8n_gcxl_aug/weights/best.pt`
- Weight SHA256: `a03ed45fdd4ff523828ffa7e66f92b9628069a42d8cad6e7fbc15dc16b00a16e`
- Classes, in order: `class_0`, `class_1`, `class_2`, `class_3`. These are the actual names stored in the checkpoint, not inferred physical labels.
- Export: repository `utils/export_yoloV8.py`, opset 17, fixed batch 1, input `input` `[1,3,640,640]`, output `output` `[1,8400,6]` (xyxy, confidence, class).
- Engine: TensorRT 10.3, built locally on this Jetson with FP16 enabled. Rebuild for another target device/runtime.
- `args.yaml` and `results.csv` are copied training records. Training validation metrics do not establish real-world grasp performance.

See `ros2_ws/README.md` at the project root for export, build and deployment commands.

## Device verification (2026-10-18)

- ONNX checker passed; TensorRT FP16 engine built and deserialized successfully.
- All three ROS 2 packages built in `ros2_ws/build_gcxl`.
- Full on-device suite: **8 passed**, including detector, NvDCF, target selection, filtering, alignment and annotations.
- Positive sample: remote `images/val/val_0000.png`, resized to 640x480; detector returned class 0 (0.967) and class 1 (0.959). This single sample is a smoke test, not an accuracy benchmark.
- Fixed Jetson RGB conversion by explicitly selecting GPU in `nvvideoconvert`.
- No `/dev/video*` device was present during verification; live camera and physical pitch/grasp calibration remain pending.
