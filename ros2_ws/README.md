# 普通抓取任务

完整逻辑、代码映射和 Action 用法见 **[项目功能与动作接口](../docs/project_actions.md)**。默认启动自动提交一次抓取 Action；`autonomous:=false` 等待外部 Goal。完成、取消或超时后停车保持，继续执行需提交新 Goal。

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

参数回调立即生效并检查非有限数、负方差、无效死区、越界的 `bottom_target`、颠倒的 pitch 区间。修改区间时中间状态也必须合法，需要同时更改时通过 `set_parameters_atomically` 服务提交。改动抓取参数会重新计算持续时间；已发出的抓取请求不会因为调参重复发出。参数不会自动持久化，确认标定后修改 `config/perception.yaml`。

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

## 独立任务：首次抓取 class_0

首次抓取是独立的 `first_grasp` ROS 节点，任务状态、目标筛选与清障循环均位于
`block_perception/first_grasp_task.py`。普通 `selector` 不包含此任务逻辑。

独立启动任务及其依赖节点（替代普通 perception.launch.py；两套入口不要同时运行）：

```bash
ros2 launch block_perception first_grasp.launch.py serial:=true
```

相机、串口等参数沿用普通 launch；按实际输入设置。启动后任务处于等待状态，不自动抓取。
单独触发：

```bash
ros2 action send_goal /robot/execute block_interfaces/action/RobotTask "{operation: first_grasp}" --feedback
```

执行中重复触发被忽略。选择 class_0 框左右边缘之间、从上边缘延伸至画面顶部的区域，
要求该区域与 class_1 框的交集面积占该 class_1 框面积的比例不超过
`first_grasp.first_grasp_overlap`（默认 0，即不相交；可调为小正数允许少量重叠）。
上边缘已出画的 class_0 不作为候选。优先选择无遮挡的合格 class_0。

前方遮挡暂按横向相交且框下边缘更低判断；有遮挡时抓取最近的遮挡物并复用丢弃任务，
丢弃结束后只使用新图像重新选目标，允许多次清障。清障抓取不计入首次正式抓取。
无合格目标时停车等待；最终只以 class_0 作为正式目标，合拢后保持夹持并结束任务，
不受普通抓取 `discard_enabled` 自动丢弃设置影响。清障则始终丢弃。
`/task/grasp_finished` 发布各次动作完成信息（AlignmentCommand，含任务 ID 和 clearance）。
完成是上位机动作计时结束，不代表传感器确认物块已夹住。

该规则使用图像框近似前后关系，无法凭检测框保证真实夹爪范围内没有其他物块；
需实车验证遮挡判断、检测漏检和抓取范围。重新编译 block_interfaces、block_perception 并
重新 source 安装环境后使用新增话题及消息字段。普通 target/reset 不控制独立首次抓取任务。

## 丢弃任务的底盘参数与标定

独立丢弃任务执行：逆时针转 90° → 夹爪开到 90° → 后退 x → 夹爪再次开到 90° → 前进 x → 顺时针转 90° → 停车。
参数位于 [`config/perception.yaml`](src/block_perception/config/perception.yaml) 的 `actuator.ros__parameters`：

| 参数 | 单位 | 用途 |
|---|---|---|
| `track_width` | m | 左右驱动轮间距，用于计算原地转 90° 所需时间，须测量实车 |
| `wheel_speed_scale` | 下位机指令值 / (m/s) | 下发轮速值 = 轮速（m/s）× 此系数，须匹配下位机接口 |
| `max_wheel_speed` | m/s | 每侧轮速上限 |
| `min_turn_speed` | m/s | 最小转向轮速，须标定到能可靠转动 |
| `discard_distance_m` | m | x，后退和返回前进使用同一距离 |
| `discard_speed_mps` | m/s | 丢弃任务的后退、前进速度，受最大轮速限制 |
| `discard_turn_speed_mps` | m/s | 原地旋转的每侧轮速，受最小转向轮速及最大轮速限制 |
| `claw_settle_sec` | s | 合拢及每次开爪后的等待时间 |
| `discard_enabled` | 布尔值 | 普通抓取完成后是否自动调用丢弃任务；不影响独立触发 |

方向约定：`lw` 对应左轮，`rw` 对应右轮；两轮正值均表示前进。从车顶向下看，左轮负、右轮正为逆时针，左轮正、右轮负为顺时针。实车运行前需确认轮子映射和正负方向。

若下位机接收实际轮速，轮径、减速比和编码器参数由下位机处理；若接收的是 PWM，则必须实测“指令值与实际速度”的关系来标定 `wheel_speed_scale`。PWM 与速度通常并非固定比例，负载、电池电压及地面变化会影响结果。

当前底盘没有向任务提供里程计或航向反馈，使用开环计时：直行时间为 `x / 实际下发速度`；原地转 90° 的时间为 `π × track_width / (4 × 每侧实际下发轮速)`。这里的速度是限幅后的指令速度，并非反馈测得的真实速度。先校准直行距离，再验证两个方向的 90° 转角；轮胎打滑及控制周期也会造成误差。若需要准确返回原位置和朝向，应增加编码器距离与 IMU 航向反馈，并将任务改为闭环到位判定。

执行节点及串口桥运行时，可独立触发：

```bash
ros2 action send_goal /robot/execute block_interfaces/action/RobotTask "{operation: discard}" --feedback
```

详细参数与任务复用接口见 [`docs/parameters.md`](../docs/parameters.md)。
