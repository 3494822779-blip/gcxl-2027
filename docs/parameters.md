# 参数说明

配置文件：`ros2_ws/src/block_perception/config/perception.yaml`。每个节点只读与自己同名的小节。
改完 yaml 要重新构建（安装目录里是副本）：

```bash
colcon build --base-paths ros2_ws/src --packages-select block_perception \
  --build-base ros2_ws/build_gcxl --install-base ros2_ws/install_gcxl
```

在线可调的参数可用 `ros2 param set /<节点名> <参数> <值>` 临时修改，非法值会被拒绝。重启后恢复为 yaml 里的值。标"启动时固定"的参数在线修改无效。

## 控制流程与对应参数

相机目前不能调俯仰，默认 `camera_servo: false`（alignment 和 actuator 两节必须一致）。舵机相关代码和参数都保留，把它改成 `true` 即恢复舵机模式。

1. 选目标：下边缘离画面底最近的框，锁定 track_id（无参数）。
2. 底盘转向：框中心 x 对准画面中心 → `deadzone_x`、`yaw_gain`、`max_yaw_rate`、`track_width`。
3. 垂直方向：
   - `camera_servo=false`：下边缘还在目标带上方（物块远），底盘前进，直到下边缘进入画面下 20% → `bottom_target`、`deadzone_y`、`forward_speed`、`forward_max_error_x`。
   - `camera_servo=true`：舵机俯仰把下边缘带进目标带 → `pitch_gain`、`max_pitch_rate`、`cam_min_deg`、`cam_max_deg`；相机没低到 `grasp_pitch` 时底盘前进。
4. 夹爪张开：框左右边缘 → 张开角 → `mm_per_px`、`claw_margin_mm`、`link_*`、`claw_init_deg`、`claw_max_deg`。
5. 触发抓取：水平和下边缘都在死区内、实测观测并持续 → `hold_seconds`（舵机开时还要相机低头到 `grasp_pitch`）。
6. 抓取执行：停轮，夹爪合拢 → `claw_close_deg`、`claw_settle_sec`（舵机开时相机先低头到 `grasp_pitch_deg`，夹完后回正 → `grasp_pitch_deg`、`grasp_pitch_tol`、`pitch_home`、`pitch_home_tol`）。

## alignment（20 Hz 发布 /alignment/command，在线可调）

| 参数 | 默认 | 单位 | 说明 |
|---|---|---|---|
| `camera_servo` | false | 开关 | 相机俯仰舵机开关。**须与 actuator 一致**。false=相机固定，只靠底盘前进把下边缘带进目标带；true=舵机俯仰对准 |
| `deadzone_x` | 0.04 | 归一化 [0,1) | 水平死区。`error_x=(框中心x−W/2)/(W/2)`，正=目标在右 |
| `deadzone_y` | 0.2 | 归一化 [0,1) | 垂直死区。`error_y=(下边缘y−bottom_target·H)/(H/2)`，正=下边缘偏下。0.2 即目标线上下各 10% 画面高度 |
| `bottom_target` | 0.9 | 画面高度比例 (0,1] | 下边缘目标位置。0.9±0.1 即画面下部 20% |
| `forward_speed` | 0.5 | m/s | 舵机关：下边缘在目标带上方时前进，带下方只停车，不倒车。舵机开：相机没低到 `grasp_pitch` 时前进 |
| `forward_max_error_x` | 0.3 | 归一化 | 水平偏差超过此值时不前进，先转向 |
| `yaw_gain` | −2.0 | rad/s 每单位误差 | 负值：目标在右时左轮比右轮快，车向右转。已实测 `lw`=左轮、正=前进；右轮待确认 |
| `pitch_gain` | −0.5 | rad/s 每单位误差 | 仅舵机开。**符号要标定**：下边缘偏下时相机应低头（角度减小） |
| `max_yaw_rate` | 2.0 | rad/s | 转向限幅。原地转向时每侧轮速 = yaw × `track_width`/2 × `wheel_speed_scale`，默认满幅 ±200 |
| `max_pitch_rate` | 0.2 | rad/s | 仅舵机开。俯仰限幅，约 11°/s |
| `grasp_pitch` | −0.26 | rad | 仅舵机开。夹爪触发角（约 −15°）：前进中相机持续低头，到达此角度并稳定后触发夹爪。**须与 actuator 的 `grasp_pitch_deg` 同一角度**，且须在相机可达范围内；按实际抓取姿态标定 |
| `pitch_timeout` | 0.25 | 秒 | 仅舵机开。俯仰反馈过期判定 |
| `input_timeout` | 0.25 | 秒 | 目标过期判定 |
| `hold_seconds` | 0.3 | 秒 | 水平和下边缘都在死区内、实测观测（舵机开时还要相机低头到 `grasp_pitch`）持续此时长后发一次抓取请求 |
| `pitch_joint` | camera_pitch | | 仅舵机开。`camera/joint_states` 中的关节名 |

## gripper（框边缘 → 夹爪角，在线可调）

夹爪舵机角 = `claw_init_deg + (−Δθ)`，限幅在 `[claw_init_deg, claw_max_deg]`。
`d = mm_per_px × (右边缘 − 左边缘) + claw_margin_mm`，`Δθ = link_ref_deg − arccos((link_offset − d/2)/link_length)`。

| 参数 | 默认 | 单位 | 说明 |
|---|---|---|---|
| `input_timeout` | 0.25 | 秒 | 目标、图像过期判定 |
| `claw_init_deg` | 0.0 | 度 | 夹爪初始角（合拢位置），只能向外张开。**须与 actuator 一致，也须与下位机固件一致** |
| `claw_max_deg` | 90.0 | 度 | 最大张开角。同上 |
| `mm_per_px` | 0.5 | mm/像素 | 框宽像素换算成物体宽度。**要按实物标定** |
| `claw_margin_mm` | 5.0 | mm | 在物体宽度上加的余量 |
| `link_ref_deg` | 60.0 | 度 | 公式中的 60 |
| `link_offset` | 44.0 | mm | 公式中的 44，与 d 同单位 |
| `link_length` | 91.0 | mm | 公式中的 91 |

## actuator（控制量 → 串口，20 Hz）

| 参数 | 默认 | 单位 | 说明 |
|---|---|---|---|
| `camera_servo` | false | 开关 | 相机俯仰舵机开关。**须与 alignment 一致**。false 时 `CMD` 的 `cp` 固定发 `camera_fixed_deg`，不处理 `$FB`，抓取时夹爪立刻合拢 |
| `camera_fixed_deg` | 0.0 | 度 | 舵机关时发给下位机的固定相机角度 |
| `track_width` | 0.2 | m | 两轮间距，用于差速换算。**要量实物** |
| `wheel_speed_scale` | 1000 | | 下位机收到的轮速原始值 = m/s × 系数（0.5 m/s → 500） |
| `max_wheel_speed` | 0.5 | m/s | 轮速限幅，×`wheel_speed_scale` 即下位机收到的上限 |
| `min_turn_speed` | 0.1 | m/s | 有转向指令时每侧轮子的转向分量至少这么大，避免小偏差时左右轮差太小、电机不动；0=关闭 |
| `cam_min_deg` | −15.0 | 度 | 仅舵机开。相机角度指令下限（负=低头）。**按机械极限改**（本机舵机最低 −15°） |
| `cam_max_deg` | 30.0 | 度 | 仅舵机开。相机角度指令上限（正=抬头） |
| `grasp_pitch_deg` | −15.0 | 度 | 仅舵机开。抓取时相机先低头到的目标角（0=回正，负=低头），**到位后夹爪才合拢**。须在 `[cam_min_deg, cam_max_deg]` 内，须与 alignment 的 `grasp_pitch` 同一角度（deg = rad×180/π） |
| `grasp_pitch_tol` | 0.03 | rad | 仅舵机开。相机到达 `grasp_pitch_deg` 的到位容差，约 1.7° |
| `claw_init_deg` / `claw_max_deg` | 0.0 / 90.0 | 度 | 同 gripper |
| `claw_close_deg` | 0.0 | 度 | 抓取时合拢到的角度（回到初始角），须在 `[claw_init_deg, claw_max_deg]` |
| `pitch_home` | 0.0 | rad | 仅舵机开。抓取后相机回正的初始角 |
| `pitch_home_tol` | 0.03 | rad | 仅舵机开。回正到位容差，约 1.7° |
| `claw_settle_sec` | 0.5 | 秒 | 夹爪合拢后等待的时间，之后进入 done |
| `pitch_timeout` | 0.25 | 秒 | 仅舵机开。俯仰反馈过期判定 |
| `pitch_joint` | camera_pitch | | 仅舵机开。发布到 `camera/joint_states` 的关节名 |

## filter（卡尔曼滤波，在线可调）

| 参数 | 默认 | 说明 |
|---|---|---|
| `process_noise` | 100.0 | 加速度噪声方差 (px²/s⁴)，越大越跟手 |
| `measurement_noise` | 16.0 | 中心点观测噪声方差 (px²)，越大越平滑 |
| `initial_variance` | 100.0 | 初始协方差对角项 |
| `prediction_timeout` | 0.4 秒 | 目标漏检时预测的最长时间，超过则输出无效 |

## 图像与检测（启动时固定，除 `confidence_threshold`）

| 节点.参数 | 默认 | 说明 |
|---|---|---|
| `camera.source` | /dev/video0 | USB 设备或视频文件路径 |
| `camera.width` / `height` / `fps` | 1280 / 720 / 60 | camera、detector、tracker 三段的宽高必须一致，宽度须能被 4 整除 |
| `camera.frame_id` | camera_optical_frame | |
| `detector.confidence_threshold` | 0.9 | **在线可调** [0.01, 1] |
| `detector.config_file` | …/infer_blocks.txt | 模型、标签、NMS 参数。迁移目录后要改绝对路径 |
| `tracker.config_file` | …/tracker_blocks.yml | NvDCF 参数 |
| `tracker.tracker_library` | /opt/nvidia/…/libnvds_nvmultiobjecttracker.so | |
| `tracker.tracker_width` / `tracker_height` | 640 / 384 | nvtracker 内部处理尺寸 |
| `display.window` | block_perception | 显示窗口名；无桌面时启动加 `display:=false` |

## 串口（serial_node，启动时固定，launch 加 `serial:=true`）

| 参数 | 默认 | 说明 |
|---|---|---|
| `port` | /dev/ttyUSB0 | USB 转 TTL 在本机可能是 `/dev/ttyACM0`，以 `ls /dev/tty*` 为准 |
| `baud` | 115200 | 仅支持 9600/19200/38400/57600/115200 |
| `poll_period` | 0.01 秒 | 读轮询周期 |

协议见 [serial_protocol.md](serial_protocol.md)。

## 首次上车必须标定的项

1. `yaw_gain` 的符号：目标在右时底盘应向右转（舵机开时还有 `pitch_gain`）。
2. `bottom_target`、`deadzone_y`：实物抓取时，物块下边缘在画面里的位置。
3. `mm_per_px`：量一个已知宽度的物块，看框宽像素。
4. `track_width`：量两轮间距。
5. `claw_init_deg`、`claw_max_deg`：按机械极限，并与下位机固件一致（舵机开时还有 `cam_min_deg`、`cam_max_deg`、`grasp_pitch` / `grasp_pitch_deg`）。
6. `serial_node.port`：按实际设备名。
