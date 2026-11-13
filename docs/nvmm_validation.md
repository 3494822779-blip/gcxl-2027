# NVMM 感知链路验证（2026-11-08）

## 当前实现

单进程 `block_deepstream/perception`：V4L2 MJPEG → appsrc → nvjpegdec → NVMM/Y42B → nvvidconv → NVMM/NV12 → nvstreammux → nvinfer → nvtracker。显示分支经有界队列、NVMM/RGBA、GPU nvdsosd 到 EGL；无桌面测试使用 fakesink。

V4L2 已按完整 JPEG 帧交付，因此未加入会重新生成时间戳的 jpegparse。采集时间映射到 ROS Header，并以保留的 PTS 精确关联检测、跟踪和 OSD。运行时 pad caps 检查确认 decode、convert、mux、infer、tracker、RGBA 和 OSD 均为 `memory:NVMM`；应用代码不映射原始像素到 CPU。

ROS 只发布 `FrameMetadata`，不再发布 `/camera/image_raw`、`/detections`、`/tracks`。压缩 JPEG 输入仍在 CPU 内存中，GPU 内部也仍有格式转换，不能称为绝对零拷贝。

## 实测结果

1280×720，请求 60fps，默认自动曝光、自动白平衡、50Hz 防闪烁，动态帧率关闭。没有调整系统功耗模式或锁定时钟。

- 连续运行 600 秒；检测和跟踪各收到 35,612 帧。
- 去除首个预热统计窗口后，跟踪接收速率平均 **59.52 fps**。
- 从 V4L2 采集时间戳到测试 ROS 订阅回调：平均 **46.15 ms**，P95 **55.05 ms**。包含传感器时间戳语义、管线和 DDS 接收延迟，不等同于曝光到屏幕的光学延迟。
- 原生 perception 进程 CPU 平均 **75.7%**（100% 为一个逻辑核；不包含 Python 控制节点）。期间有编译及测试负载，不与旧链路作严格功耗/CPU A/B 比较。
- 原生进程 RSS 从 **766.1 MB** 到 **766.9 MB**，没有持续增长。
- 驱动序号缺口累计 1（启动阶段），错误帧计数 0；所有测试进程正常退出，退出码 0。
- 另外抽查 299 帧，每帧均有 class_0 的真实观测框，同一跟踪 ID 连续出现 299 次。此结果验证数据通路和 ID 连续性，不代表完整检测精度评估。

## 显示隔离与控制回归

在长测第 60 秒暂停测试滤波节点 10 秒。OSD 等待超时后显示降至约 18–19fps，检测和跟踪仍约 59.5fps；恢复滤波后显示恢复约 59.5fps。旧坐标不套用到新帧。

83 项 Python 控制测试通过，覆盖锁定、滤波、过期停止、首次抓取、动作状态及夹爪同帧匹配。C++ TargetCache 测试覆盖时间戳/frame_id 精确匹配、延迟到达、超时、64 项缓存上限和退出唤醒。真机集成测试检查 RGB 像素不进入 ROS 消息、采集时间单调、检测与跟踪同帧关联及 NVMM OSD。

本机 EGL 窗口运行 15 秒，收到 791 帧，预热后显示约 59.4fps，正常关闭退出码 0。相机启动失败测试使用不存在的设备路径：perception 退出码 1，launch 在约 2.37 秒内向其余节点发出 SIGINT，selector/filter/alignment/gripper/actuator 均正常退出。该测试未模拟物理热拔插。

测试只运行视觉和控制计算，不运行硬件串口或实际运动。

## 复现

```bash
source /opt/ros/humble/setup.bash
source ros2_ws/install_gcxl/setup.bash
ROS_DOMAIN_ID=74 ROS_LOG_DIR=/tmp/nvmm-tests PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 \
  /usr/bin/python3 -m pytest -q ros2_ws/src/block_perception/test
ctest --test-dir ros2_ws/build_gcxl/block_deepstream --output-on-failure
ROS_DOMAIN_ID=72 ROS_LOG_DIR=/tmp/nvmm-live PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 \
  BLOCK_TEST_DEEPSTREAM=1 /usr/bin/python3 -m pytest -q \
  ros2_ws/src/block_perception/test/test_deepstream.py
```

只运行视觉节点：

```bash
ros2 run block_deepstream perception --ros-args \
  --params-file ros2_ws/src/block_bringup/config/perception.yaml -p headless:=true
```

原始长测汇总保存在本机 `/tmp/gcxl-nvmm-soak.json`；该临时文件可能随清理消失，关键结果已记录在本文。
