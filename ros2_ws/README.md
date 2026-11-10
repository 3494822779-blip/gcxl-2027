# 普通抓取任务

> 目标抓取接口已更新：启动默认等待人工任务；`target_grasp`（以及 `approach_grasp`、`first_grasp`）必须显式提供 `has_target_class: true, target_class_id: <类别>`。自动运行使用 `autonomous:=true target_class:=<类别>`。清障采用有次数预算的单爪逐个扫走，丢弃已停用。完整规则和当前命令以 [动作说明](../docs/project_actions.md) 为准。接口变更后须重建全部依赖包并重启节点。


完整逻辑、代码映射和 Action 用法见 **[项目功能与动作接口](../docs/project_actions.md)**。默认启动等待外部 Goal；`autonomous:=true target_class:=<类别>` 自动执行一次指定类别抓取。完成、取消或超时后停车保持，继续执行需提交新 Goal。

ROS 2 物块对准抓取视觉感知

代码直接位于当前 DeepStream-Yolo 工程内，支持 ROS 2 Humble + DeepStream 7.1（Jetson）。通过 actuator 和可选串口桥控制底盘、夹爪和相机俯仰。已接入 `models/yolov8n_gcxl_aug/best.pt` 导出的四类 YOLOv8n 模型，类别顺序为 `class_0`、`class_1`、`class_2`、`class_3`；实际物块名称仍需对应训练数据确认。

## 节点与接口

```text
perception（单个原生 C++ 进程）
  V4L2 MJPEG → appsrc → nvjpegdec
  → NVMM/Y42B → nvvidconv → NVMM/NV12
  → nvstreammux → nvinfer → nvtracker
      ├─ detections_meta / tracks_meta [FrameMetadata：时间戳、尺寸、框，无像素]
      │   ├─ selector / first_grasp → target/raw → filter → target/filtered
      │   │   └─ alignment → alignment/command → actuator → serial_tx
      │   └─ gripper → gripper/angles
      └─ 有界显示队列 → NVMM/RGBA → nvdsosd（GPU）→ EGL 本机显示
camera/joint_states → alignment（仅 camera_servo=true）
target/reset → selector
serial_tx → serial_node（可选）→ serial_rx
```

仅有这一条图像链路；原 `camera`、`detector`、`tracker`、Python `display` 可执行程序及 `Frame` 整图消息已移除。`/camera/image_raw`、`/detections`、`/tracks` 不再发布。控制节点统一读取 `FrameMetadata` 的 `header`、`width`、`height`、`boxes`，`Target` 和动作接口不变。相机、解析器到解码器之间仍有压缩 JPEG 的 CPU 内存传递；从硬件解码输出开始，像素留在 NVMM，CPU 仅访问框和时间戳等元数据。这不代表 GPU 内部没有格式转换或显存复制。

- 检测：`class_id=0..3`；模板要求 TensorRT FP16。原始候选阈值固定为 0.01，在线 `confidence_threshold` 支持 `[0.01, 1]`。不要在 nvinfer 配置里增加更高的分类阈值，否则降低 ROS 阈值不能恢复已被丢弃的框。候选仍受 NMS/top-k 限制。
- 选择：按 `y + height` 最大选取真实观测框，同分按置信度和 ID 排序；锁定后不因 pitch、排序变化或丢失切换。长时间丢失仍保持 ID，但禁止继续运动。显式 reset 后重新选取；跟踪器重启或目标 ID 被回收后也需要 reset。
- 滤波：状态 `[x, y, vx, vy]`，像素坐标和秒，匀速卡尔曼模型。`process_noise` 是加速度噪声方差，`measurement_noise` 是中心点观测噪声方差，`initial_variance` 是初始协方差对角项。目标漏检但图像仍到达时预测，超过 `prediction_timeout` 输出 invalid；长时间后原 ID 恢复则重置速度/协方差。整个视频停止时，指令节点独立超时归零。
- 对准：`error_x=(框中心x-W/2)/(W/2)`，`error_y=(框下边缘y-bottom_target·H)/(H/2)`。正值分别表示目标在右侧、下边缘偏下；死区也是该归一化单位。`yaw_rate`、`pitch_rate` 单位 rad/s，经增益、死区、限幅计算。默认负增益仅是初始示例，必须按实际底盘/云台正方向标定。
- 相机俯仰舵机开关 `camera_servo`（alignment 与 actuator 两节必须一致）：
  - `false`（相机固定，当前默认）：`pitch_rate=0`，不需要 pitch 反馈。下边缘在目标带（默认画面下 20%）上方时底盘按 `forward_speed` 前进（水平偏差不超过 `forward_max_error_x`），在带下方只停车，不倒车。
  - `true`：订阅真实反馈 `JointState`，取 `camera_pitch` 的 position（rad）。相机没低到 `grasp_pitch` 时底盘前进，舵机把下边缘带进目标带。`grasp_pitch` 只代表标定后的抓取姿态，不是通用测距公式；相机高度、物块高度、安装角或地面条件变化后应重新标定。
- 抓取：水平和下边缘都在死区、目标是真实观测（`camera_servo=true` 时还要 pitch 反馈未过期且已低到 `grasp_pitch`），当前实现满足条件立即触发（`hold_seconds` 暂不参与延时），`grasp_request` **只发出一次**。预测框不触发抓取。之后该锁定周期运动输出保持零，actuator 合拢夹爪并保持，等待下一个 Action Goal。该话题作为 Action 内部决策输入；对外任务使用 `/robot/execute` 提供完成、取消和超时反馈。
- 指令：20 Hz 发布，`valid=false` 时两轴为零；消费者必须检查 `header.stamp` 和 `valid_for_sec`，订阅中断后自行停止。目标、pitch 与指令时间必须来自同一 ROS 时钟。相机标定角度和执行方向并未在此工程中虚构。

## 编译与运行

```bash
cd /home/nvidia/project/gcxl-2027
source /opt/ros/humble/setup.bash
# DeepStream SDK/CUDA 和 YOLO parser 按原工程要求安装/编译。
# 系统依赖：
sudo apt-get install python3-numpy python3-pytest libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
colcon --log-base ros2_ws/log_gcxl build --base-paths ros2_ws/src \
  --build-base ros2_ws/build_gcxl --install-base ros2_ws/install_gcxl --cmake-args -DBUILD_TESTING=ON
source ros2_ws/install_gcxl/setup.bash
```

USB 相机由原生节点直接 V4L2 mmap 采集，默认请求 1280×720@60。每次启动恢复设备报告的可写标准图像控制默认值，先恢复自动模式；跳过非活动手动项、只读项和运动动作，不使用曝光 toggle。实际帧率仍受光照影响。不可恢复的相机或管线故障导致进程非零退出，并由 launch 停止整栈；不会自动恢复运动。

修改后重新构建并 source `ros2_ws/install_gcxl/setup.bash`。配置集中于 `perception.yaml` 的 `perception` 节，不再使用旧 camera/detector/tracker 配置节。`exposure` 和旧图像输入／视频文件回放入口已移除。

`ros2_ws/src/block_perception/config/infer_blocks.txt` 已配置当前项目的 ONNX、独立 FP16 engine、训练模型标签和 parser 路径。迁移目录后需要同步修改这些绝对路径并重新编译。构建采用 `build_gcxl/install_gcxl`，避免复用迁移前含旧绝对路径的 CMake 缓存。

重新导出和构建模型：

```bash
cd /home/nvidia/project/gcxl-2027/models/yolov8n_gcxl_aug
/usr/bin/python3 ../../utils/export_yoloV8.py -w best.pt
/usr/src/tensorrt/bin/trtexec --onnx=best.onnx \
  --saveEngine=best_b1_gpu0_fp16.engine --fp16 --skipInference
cd ../..
```

导出器生成 `[1,8400,6]` 的 xyxy/score/class 输出供 DeepStream parser 使用；不能直接替换为 Ultralytics 默认输出格式。TensorRT engine 应在部署设备上生成。

```bash
# 内置 USB 相机；全部节点参数统一集中在 config/perception.yaml（相机源、图像尺寸、
# perception 的 DeepStream 配置路径、算法参数、串口参数），source 为 V4L2 设备。
ros2 launch block_perception perception.launch.py

# 无桌面时关闭显示；仅 perception 处理图像，控制逻辑正常运行。
ros2 launch block_perception perception.launch.py display:=false autonomous:=false

# 串口桥默认不启动；需要时加 serial:=true，设备、波特率、轮询周期在 perception.yaml 的 serial_node 段。
ros2 launch block_perception perception.launch.py serial:=true
```

时间戳在 V4L2 出队时由单调采集时间映射到 ROS 时钟，并通过 GStreamer PTS 关联到检测、跟踪和 OSD。输入限定为物理 MJPEG 相机，统一管线不支持 `use_sim_time`。

`camera_servo=true` 时由外部模块发布实际 pitch（`camera_servo=false` 不需要）：

```bash
# 仅用于台架调试的示例反馈，不可代替实际云台反馈。
ros2 topic pub -r 20 /camera/joint_states sensor_msgs/msg/JointState \
  '{header: auto, name: [camera_pitch], position: [-0.6]}'
ros2 topic echo /alignment/command
# 完成抓取/取消当前目标后重新选择
ros2 topic pub --once /target/reset std_msgs/msg/Bool '{data: true}'
```

控制节点仍可用 `ros2 run block_perception selector` 等单独运行。只测试感知、避免启动任何执行节点：

```bash
ros2 run block_deepstream perception --ros-args \
  --params-file ros2_ws/src/block_perception/config/perception.yaml -p headless:=true
```

`headless:=true` 保留 GPU OSD 绘制、使用 fakesink，适合验证；`display:=false` 完全关闭显示分支。

## 在线调参

```bash
ros2 param set /perception confidence_threshold 0.4
ros2 param set /filter process_noise 80.0
ros2 param set /filter measurement_noise 25.0
ros2 param set /filter prediction_timeout 0.35
ros2 param set /alignment deadzone_x 0.05
ros2 param set /alignment deadzone_y 0.2
ros2 param set /alignment bottom_target 0.9
ros2 param set /alignment grasp_pitch -0.3   # 仅 camera_servo=true
```

参数回调立即生效并检查非有限数、负方差、无效死区、越界的 `bottom_target`、颠倒的 pitch 区间。修改区间时中间状态也必须合法，需要同时更改时通过 `set_parameters_atomically` 服务提交。抓取就绪条件每周期重新检查，由执行器保证扫走、张爪和抓取阶段只执行一次。参数不会自动持久化，确认标定后修改 `config/perception.yaml`。

## 屏幕显示与记录

默认由 `nvdsosd process-mode=1` 在 NVMM 上画框、ID 和目标标记，EGL 本机显示需要图形桌面。目标按精确采集时间戳和 frame_id 匹配；显示分支最多等待 50 ms，超时只保留该帧检测框，不使用旧目标。显示队列最多两帧、溢出丢旧帧；采集 appsrc 队列也限制为两帧。控制元数据在显示等待之前发布。

本版未接入 NVENC 或网络预览。元数据可以直接录制：

```bash
ros2 bag record -o block_session /detections_meta /tracks_meta /target/raw \
  /target/filtered /target/reset /alignment/command /gripper/angles /parameter_events
```

元数据记录不包含视频，不能用于重新运行检测或跟踪。若要复现控制算法，可单独启动 selector/filter/alignment 并用 ROS 仿真时钟回放 `/tracks_meta`；不要同时运行实时感知或硬件执行节点。旧 `Frame` 图像录包不兼容新接口。

## 验证

当前 10 分钟真机结果、延迟、资源占用和故障测试见 [NVMM 验证记录](../docs/nvmm_validation.md)。

```bash
source /opt/ros/humble/setup.bash
source ros2_ws/install_gcxl/setup.bash
# 禁用环境中与旧版 pytest 冲突的第三方自动插件；不影响这些测试。
PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 /usr/bin/python3 -m pytest -q ros2_ws/src/block_perception/test
# Jetson 真机：验证硬解、NVMM caps、检测/跟踪元数据及 GPU OSD。
BLOCK_TEST_DEEPSTREAM=1 PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 /usr/bin/python3 -m pytest -q \
  ros2_ws/src/block_perception/test/test_deepstream.py
ctest --test-dir ros2_ws/build_gcxl/block_deepstream --output-on-failure
```

控制测试覆盖锁定、预测、过期停止、首次抓取和夹爪同帧匹配；原生测试覆盖 OSD 精确匹配、缓存上限及退出唤醒。实际检测精度和物理抓取仍取决于场景、模型与标定。

## 左右夹爪舵机角度接口

`gripper` 订阅 `/target/raw` 和 `/tracks_meta`，按采集时间戳、frame_id、track_id 匹配真实观测框，提取左右边界。预测框、其他 ID 和过期帧不能用于角度计算。根据 `mm_per_px`、`link_ref_deg`、`link_offset`、`link_length` 等标定参数计算建议角度，算法与本次图像链路重构前相同。

输出 `/gripper/angles`（`GripperAngles`），包含时间戳、目标 ID、边界、建议角度、有效期和状态。该节点只计算建议；是否执行由 actuator 和动作状态决定。输入中断时输出失效状态，默认 `input_timeout=0.25` 秒。

## 目标型抓取与单爪扫走

普通启动默认等待任务；`first_grasp.launch.py` 是同一选择逻辑的兼容入口，不再只选 class_0。

```bash
ros2 launch block_perception perception.launch.py serial:=true
ros2 action send_goal /robot/execute block_interfaces/action/RobotTask "{operation: target_grasp, has_target_class: true, target_class_id: 2}" --feedback
```

必须显式指定类别，没有默认类别。`approach_grasp` 和 `first_grasp` 是相同流程的兼容名称，同样要求类别。
选中指定类别中下边缘最低的真实目标后锁定；优先处理前方下边缘最低、与目标横向相交的障碍，单爪内收 −30°，等待后继续接近正式目标并叠加指数衰减的反向转向偏置。
扫完恢复爪位并依据新观测复查，默认同一障碍最多两次、每任务总共八次；仍有障碍则停车，清障后抓取原目标。目标丢失时停车，可按同类别框几何重叠接管跟踪器新编号。
抓取结束保持夹持、停车并发出相机回正指令。`discard` 和 `discard_after: true` 已停用并拒绝执行。
详细参数、取消和记忆接口见 [动作说明](../docs/project_actions.md)。
