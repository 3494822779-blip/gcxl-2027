# ROS 2 物块对准抓取视觉感知

代码直接位于当前 DeepStream-Yolo 工程内，支持 ROS 2 Humble + DeepStream 7.1（Jetson）。只输出视觉对准信息和抓取请求，不控制电机。已有 COCO 模型不是四类物块模型；部署前必须提供训练好的四类 YOLO ONNX 和对应标签。

## 节点与接口

```text
camera（可选 USB/视频文件节点，或外部 ROS 相机驱动）
  └─ camera/image_raw [sensor_msgs/Image, rgb8]
      └─ detector [GStreamer → nvstreammux → nvinfer/TensorRT]
          └─ detections [block_interfaces/Frame]
              └─ tracker [独立 GStreamer → nvstreammux → nvtracker/NvDCF]
                  └─ tracks [block_interfaces/Frame]
                      ├─ selector → target/raw [Target]
                      │   └─ filter → target/filtered [Target]
                      │       ├─ alignment → alignment/command [AlignmentCommand]
                      │       └─ annotations ← tracks
                      │            └─ debug/annotations [foxglove_msgs/ImageAnnotations]
                      └─ 任意新增节点可直接订阅
camera/joint_states [sensor_msgs/JointState] → alignment
target/reset [std_msgs/Bool] → selector
```

每个功能是独立可执行程序和 ROS 进程，仅通过话题和参数连接；检测和跟踪不是把同一管线伪装成两个节点。tracker 从 `Frame` 的图像和检测框重建 `NvDsObjectMeta`，交给真正的 NVIDIA nvtracker，不使用自写 ID 匹配替代。`Frame` 原子携带图像和框，避免跨话题丢帧错配，同时可以独立回放跟踪输入。代价是进程间复制图像，640×480 RGB 每帧约 0.92 MB；输入/中间图像采用 SensorDataQoS（best effort），算法输出和指令采用 reliable。内置单相机、batch=1，输入分辨率固定；需多相机时按命名空间分别启动。

- 检测：`class_id=0..3`；模板要求 TensorRT FP16。原始候选阈值固定为 0.01，在线 `confidence_threshold` 支持 `[0.01, 1]`。不要在 nvinfer 配置里增加更高的分类阈值，否则降低 ROS 阈值不能恢复已被丢弃的框。候选仍受 NMS/top-k 限制。
- 选择：按 `y + height` 最大选取真实观测框，同分按置信度和 ID 排序；锁定后不因 pitch、排序变化或丢失切换。长时间丢失仍保持 ID，但禁止继续运动。显式 reset 后重新选取；跟踪器重启或目标 ID 被回收后也需要 reset。
- 滤波：状态 `[x, y, vx, vy]`，像素坐标和秒，匀速卡尔曼模型。`process_noise` 是加速度噪声方差，`measurement_noise` 是中心点观测噪声方差，`initial_variance` 是初始协方差对角项。目标漏检但图像仍到达时预测，超过 `prediction_timeout` 输出 invalid；长时间后原 ID 恢复则重置速度/协方差。整个视频停止时，指令节点独立超时归零。
- 对准：`error_x=(x-W/2)/(W/2)`，`error_y=(y-H/2)/(H/2)`。正值分别表示目标在右侧、下侧；死区也是该归一化单位。`yaw_rate`、`pitch_rate` 单位 rad/s，经增益、死区、限幅计算。默认负增益仅是初始示例，必须按实际底盘/云台正方向标定。默认不输出前进/后退指令。
- pitch：订阅真实反馈 `JointState`，取 `camera_pitch` 的 position（rad）；必须有采集时间戳。`pitch_min..pitch_max` 只代表标定后的可抓取距离带，不是通用测距公式。相机高度、物块高度、安装角或地面条件变化后应重新标定。
- 抓取：双轴均在死区、目标是真实观测、pitch 反馈未过期且在范围内，持续 `hold_seconds` 后，`grasp_request` **只发出一次**。预测框不触发抓取。之后该锁定周期运动输出保持零，等待执行方完成/取消后发布 reset。执行方需要订阅 reliable 指令、按 ID 去重；一次请求不是具备确认重试的动作协议。
- 指令：20 Hz 发布，`valid=false` 时两轴为零；消费者必须检查 `header.stamp` 和 `valid_for_sec`，订阅中断后自行停止。目标、pitch 与指令时间必须来自同一 ROS 时钟。相机标定角度和执行方向并未在此工程中虚构。

## 编译与运行

```bash
cd /home/nvidia/project/DeepStream-Yolo-master
source /opt/ros/humble/setup.bash
# DeepStream SDK/CUDA 和 YOLO parser 按原工程要求安装/编译。
# 系统依赖（foxglove_bridge 已安装也可重复执行）：
sudo apt-get install ros-humble-foxglove-msgs ros-humble-foxglove-bridge \
  python3-numpy python3-opencv python3-pytest libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
colcon --log-base ros2_ws/log build --base-paths ros2_ws/src \
  --build-base ros2_ws/build --install-base ros2_ws/install
source ros2_ws/install/setup.bash
```

编辑 `src/block_perception/config/infer_blocks.txt` 中的绝对路径：四类 ONNX、独立 engine 输出文件、四行标签文件、当前工程 parser `.so`。不要覆盖现有 COCO engine，也不能只把 COCO 模型的 `num-detected-classes` 改成 4。类别名和顺序必须与训练模型一致。模板标签 `block_class_0..3` 是占位名。

```bash
# 内置 USB 相机；也可 source:=/absolute/path/video.mp4
ros2 launch block_perception perception.launch.py camera:=true \
  infer_config:="$PWD/ros2_ws/src/block_perception/config/infer_blocks.txt"

# 若已有 ROS 相机驱动，camera 默认 false；驱动应发布 /camera/image_raw，rgb8、640x480。
# width/height 可作为 launch 参数修改；width 必须能被 4 整除。
# 外部驱动不满足编码时，应加独立转换节点，不修改检测节点。
```

内置相机节点对图像缩放并转为 RGB；它使用软件读取时间戳，精确同步场景请使用硬件相机驱动和采集时间戳。视频文件按配置帧率播放，到结尾停止，不自动循环。

外部模块发布实际 pitch：

```bash
# 仅用于台架调试的示例反馈，不可代替实际云台反馈。
ros2 topic pub -r 20 /camera/joint_states sensor_msgs/msg/JointState \
  '{header: auto, name: [camera_pitch], position: [-0.6]}'
ros2 topic echo /alignment/command
# 完成抓取/取消当前目标后重新选择
ros2 topic pub --once /target/reset std_msgs/msg/Bool '{data: true}'
```

每个节点也可以 `ros2 run block_perception selector` 等单独启动。DeepStream 节点为 `ros2 run block_deepstream detector|tracker --ros-args -p config_file:=...`。`width`、`height`、DeepStream 配置路径为只读启动参数；算法参数在线修改。

## 在线调参

```bash
ros2 param set /detector confidence_threshold 0.4
ros2 param set /filter process_noise 80.0
ros2 param set /filter measurement_noise 25.0
ros2 param set /filter prediction_timeout 0.35
ros2 param set /alignment deadzone_x 0.05
ros2 param set /alignment deadzone_y 0.05
ros2 param set /alignment pitch_min -0.9
ros2 param set /alignment pitch_max -0.6
```

参数回调立即生效并检查非有限数、负方差、无效死区和颠倒的 pitch 区间。修改区间时中间状态也必须合法，需要同时更改时通过 `set_parameters_atomically` 服务提交。改动抓取参数会重新计算持续时间；已发出的抓取请求不会因为调参重复发出。参数不会自动持久化，确认标定后修改 `config/perception.yaml`。

## Foxglove

launch 默认启动 foxglove_bridge，浏览器/客户端连接 `ws://设备IP:8765`。Image 面板选择 `/camera/image_raw`，在 annotations 设置中启用 `/debug/annotations`，查看绿色检测框/ID、黄色滤波前中心、青色滤波后中心和中心偏差连线。标注按精确图像时间戳匹配，最多缓存 30 帧；不会把旧帧坐标叠加到新帧上。Plot 面板可查看 `/alignment/command.error_x`、`error_y`、`yaw_rate`、`pitch_rate`，Raw Messages 面板查看锁定 ID 和抓取请求。可用 `foxglove:=false annotations:=false` 关闭调试节点。

## rosbag 录制与回放

```bash
# 完整调试记录（图像被多个 Frame 消息携带，磁盘流量较高）
ros2 bag record -o block_session /camera/image_raw /camera/joint_states \
  /detections /tracks /target/raw /target/filtered /target/reset \
  /alignment/command /debug/annotations /parameter_events

# 算法复现：从 tracks 重算选择、滤波和指令，关闭实时相机/检测/跟踪。
ros2 launch block_perception perception.launch.py detector:=false tracker:=false use_sim_time:=true
# 另一个终端，先 source 相同 overlay：仅回放上游输入，避免同一输出话题多个发布者。
ros2 bag play block_session --clock --topics /tracks /camera/image_raw /camera/joint_states /target/reset

# 若要复现 tracker，启动 tracker:=true detector:=false，再只播放：
# /detections /camera/image_raw /camera/joint_states /target/reset
# 若要复现 detector，启动全部算法节点、camera:=false、use_sim_time:=true，
# 只播放 /camera/image_raw /camera/joint_states /target/reset。
```

只查看既有结果时，不启动算法节点，只启动 bridge 并回放原始记录。`/parameter_events` 记录调参事件，但不会自动恢复参数；复现时加载相同 YAML。回放采用单调时间，重新开始、向后 seek 或循环播放前应重启算法节点和 tracker（旧 ID/时间序列不能延用）。纯回放对准节点不依赖 GPU；重跑 detector/tracker 需要对应 DeepStream 环境。不要把回放生成的指令接到实际执行模块。

## 验证

```bash
source /opt/ros/humble/setup.bash
source ros2_ws/install/setup.bash
# 禁用环境中与旧版 pytest 冲突的第三方自动插件；不影响这些测试。
PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 /usr/bin/python3 -m pytest -q ros2_ws/src/block_perception/test
# Jetson 上实测 nvtracker：发布带检测框的 ROS 图像，验证稳定 ID 和采集时间戳。
BLOCK_TEST_DEEPSTREAM=1 PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 /usr/bin/python3 -m pytest -q \
  ros2_ws/src/block_perception/test/test_deepstream.py
```

当前验证覆盖：三个包编译、锁定不切换、卡尔曼抑噪/预测、超时归零、禁止预测触发抓取、真实 pitch 条件、单次触发、动态参数校验、Foxglove 消息构造/时间戳匹配、实际 nvtracker 管线。四类 YOLO 检测效果和实体抓取距离尚需实际模型、相机和云台反馈进行联调。

环境说明：本次验证时系统未安装 `foxglove_msgs`，标注测试使用官方 ROS apt 包解包后的消息类型完成；正式运行前执行上述 apt 安装命令。`foxglove_bridge` 已在本机安装。
