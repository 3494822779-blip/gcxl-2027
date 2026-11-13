# 参数说明

动作 Goal、取消、超时和代码映射见 [项目功能与动作接口](project_actions.md)。`autonomous`（默认 false）和 `task_mode` 是 actuator 启动参数，由 launch 传入；运行中只读。目标型抓取必须显式指定类别；`discard_after: true` 和丢弃动作均被拒绝。

配置文件：`ros2_ws/src/block_bringup/config/perception.yaml`。每个节点只读与自己同名的小节。
改完 yaml 要重新构建（安装目录里是副本）：

```bash
colcon build --base-paths ros2_ws/src --packages-select block_perception \
  --build-base ros2_ws/build_gcxl --install-base ros2_ws/install_gcxl
```

在线可调的参数可用 `ros2 param set /<节点名> <参数> <值>` 临时修改，非法值会被拒绝。重启后恢复为 yaml 里的值。标"启动时固定"的参数在线修改无效。

## 控制流程与对应参数

节点内置 `camera_servo` 默认为 false，当前项目 YAML 已在 alignment 和 actuator 同时设为 true。下方旧参数表的内置默认值与项目配置可能不同；运行值以 YAML 和节点参数为准，动作规则以 [动作说明](project_actions.md) 为准。

1. 选目标：下边缘离画面底最近的框，锁定 track_id；追踪器给同一物块换成此前未出现过的新 ID 时锁定才会跟过去 → `rebind_iou`、`rebind_timeout`、`search_rebind_iou`。
2. 底盘转向：框中心 x 对准画面中心 → `deadzone_x`、`yaw_gain`、`max_yaw_rate`、`track_width`。
3. 垂直方向：
   - `camera_servo=false`：下边缘还在目标带上方（物块远），底盘前进，直到下边缘进入画面下 20% → `bottom_target`、`deadzone_y`、`forward_speed`、`forward_max_error_x`。
   - `camera_servo=true`：舵机俯仰把下边缘带进目标带 → `pitch_gain`、`max_pitch_rate`、`cam_min_deg`、`cam_max_deg`；相机没低到 `grasp_pitch` 时底盘前进。
4. 夹爪张开：框左右边缘 → 张开角 → `mm_per_px`、`claw_margin_mm`、`link_*`、`claw_init_deg`、`claw_max_deg`。
5. 触发抓取：水平和下边缘都在死区内、实测观测即触发（当前 `hold_seconds` 不参与延时）（舵机开时还要相机低头到 `grasp_pitch`）。
6. 抓取执行：停轮，夹爪合拢 → `left_claw_close_deg`、`right_claw_close_deg`、`claw_settle_sec`（舵机开时相机先低头到 `grasp_pitch_deg`，夹完后回正 → `grasp_pitch_deg`、`grasp_pitch_tol`、`pitch_home`、`pitch_home_tol`）。

## selector（目标选择与锁定，在线可调）

下列重新绑定参数用于 align 和目标型抓取，目标型任务启动时固定本次规则。重新绑定仅凭同类别框的几何重叠推断身份，不能保证物理身份相同。

| 参数 | 默认 | 单位 | 说明 |
|---|---|---|---|
| `rebind_iou` | 0.3 | [0,1] | 锁定目标丢失前从未出现过的新 ID，且与最后一次看到锁定目标的框同类、IoU 不低于此值，才允许接管。已出现过的障碍 ID 会被排除；0=关闭 |
| `rebind_timeout` | 3.0 | 秒 | 最后一次看到锁定目标之后，超过此时长不再接管新 ID |
| `search_rebind_iou` | 0.1 | (0, `rebind_iou`] | 搜索期间相机停住后恢复目标时的较低 IoU 阈值；搜索期间暂停 `rebind_timeout` 累计 |

## alignment（20 Hz 发布 /alignment/command，在线可调）

| 参数 | 默认 | 单位 | 说明 |
|---|---|---|---|
| `camera_servo` | false | 开关 | 相机俯仰舵机开关。**须与 actuator 一致**。false=相机固定，只靠底盘前进把下边缘带进目标带；true=舵机俯仰对准 |
| `deadzone_x` | 0.04 | 归一化 [0,1) | 水平死区。`error_x=(框中心x−W/2)/(W/2)`，正=目标在右 |
| `deadzone_y` | 0.1 | 归一化 [0,1) | 垂直死区。`error_y=(下边缘y−bottom_target·H)/(H/2)`，正=下边缘偏下。0.1 即目标线上下各 5% 画面高度 |
| `bottom_target` | 0.8 | 画面高度比例 (0,1] | 下边缘目标位置。当前目标带约为画面高度 75%～85% |
| `forward_speed` | 0.5 | m/s | 舵机关：下边缘在目标带上方时前进，带下方且居中时以 reverse_speed 倒退。舵机开：相机没低到 `grasp_pitch` 时前进 |
| `forward_max_error_x` | 0.3 | 归一化 | 水平偏差超过此值时不前进，先转向 |
| `grasp_error_x` | 0.15 | 归一化 | 物块下边缘到达目标线时的水平死区。水平死区按距离从 `deadzone_x`（远）线性放宽到此值（近），转向和抓取判定共用 |
| `yaw_near_weight` | 0.2 | [0,1] | 转向增益的距离权重：远处为 1（全增益，前期转到位），随下边缘接近目标线线性降到此值。1=不衰减，0=到线时不再转向 |
| `yaw_center_weight` | 0.3 | [0,1] | 转向增益的偏离权重下限。偏离权重 = `max(yaw_center_weight, min(1, \|error_x\|/yaw_full_error_x))`，目标靠近画面中心时增益只剩此比例。1=不按偏离衰减 |
| `yaw_full_error_x` | 0.5 | 归一化 (0,1] | `\|error_x\|` 达到此值及以上时偏离权重为 1（全增益），以内线性过渡 |
| `yaw_gain` | −2.0 | rad/s 每单位误差 | 负值：目标在右时左轮比右轮快，车向右转。已实测 `lw`=左轮、正=前进；右轮待确认 |
| `pitch_gain` | 1.5 | rad/s 每单位误差 | 仅舵机开。**符号要标定**：下边缘偏下时相机应低头（角度增大） |
| `max_yaw_rate` | 2.0 | rad/s | 转向限幅。原地转向时每侧轮速 = yaw × `track_width`/2 × `wheel_speed_scale`，默认满幅 ±200 |
| `max_pitch_rate` | 0.2 | rad/s | 仅舵机开。俯仰限幅，约 11°/s |
| `grasp_pitch` | 0.6981317 | rad | 仅舵机开。夹爪触发角（40°）：位置及角度到位、真实观测有效即可请求合爪。**须与 actuator 的 `grasp_pitch_deg` 同一角度**，且须在相机可达范围内；按实际抓取姿态标定 |
| `pitch_timeout` | 0.25 | 秒 | 仅舵机开。俯仰反馈过期判定 |
| `input_timeout` | 0.25 | 秒 | 目标过期判定 |
| `hold_seconds` | 0.3 | 秒 | 保留参数；当前代码每周期按 ready 条件发布请求，不使用此等待时间 |
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
| `cam_min_deg` | 0.0 | 度 | 相机回正，实际范围按机械极限标定 |
| `cam_max_deg` | 90.0 | 度 | 相机角度上限；正角低头 |
| `grasp_pitch_deg` | 40.0 | 度 | 仅舵机开。抓取时相机先低头到的目标角（0=回正，正=低头），**到位后夹爪才合拢**。须在 `[cam_min_deg, cam_max_deg]` 内，须与 alignment 的 `grasp_pitch` 同一角度（deg = rad×180/π） |
| `grasp_pitch_tol` | 0.0296706 | rad | 仅舵机开。抓取下限容差，约 1.7°：反馈 ≥ grasp_pitch_deg − 容差即可，超过阈值允许抓取；手动 camera 动作仍使用双向到位容差 |
| `claw_init_deg` / `claw_max_deg` | 0.0 / 90.0 | 度 | 同 gripper |
| `left_claw_close_deg` | 0.0 | 度 | 左夹爪抓取闭合角，长期配置，须在 `[claw_init_deg, claw_max_deg]` |
| `right_claw_close_deg` | 5.0 | 度 | 右夹爪抓取闭合角，当前 5°，可单独调整；范围同左侧 |
| `pitch_home` | 0.0 | rad | 仅舵机开。抓取后相机回正的初始角 |
| `pitch_home_tol` | 0.03 | rad | 仅舵机开。回正到位容差，约 1.7° |
| `claw_settle_sec` | 0.5 | 秒 | 视觉张爪、合爪及手动夹爪的等待时间 |
| `pitch_timeout` | 0.25 | 秒 | 仅舵机开。俯仰反馈过期判定 |
| `pitch_joint` | camera_pitch | | 仅舵机开。发布到 `camera/joint_states` 的关节名 |
| `search_pitch_speed_deg` | 20.0 | 度/秒 | 每次丢失后俯仰最多搜索一轮；重新看到目标可再次搜索，任务总次数不清零 |
| `search_turn_sec` / `search_turn_speed_mps` | 0.35 / 0.17 | 秒 / m/s | 俯仰搜索前按目标最后横向位置低速原地转向；速度通过 `min_turn_speed` 门槛 |
| `search_dwell_sec` | 0.15 | 秒 | 转向后停住再做重绑定判断 |
| `search_max_attempts` | 3 | 次/任务 | 目标反复进出画面时的搜索触发上限 |

## 目标型抓取与扫走参数

所有扫走参数位于 actuator 小节；每次扫走开始时固定本次内收角度、转向权重和衰减时间。

| 参数 | 当前值 | 说明 |
|---|---|---|
| `autonomous_target_class` | −1 | 未指定；启动自动抓取必须通过 launch 的 target_class 明确指定非负类别 |
| `claw_open_pitch_deg` | 20° | 相机实际反馈达到阈值时按当前目标宽度张爪一次；应设在抓取角度之前，之后保持开度 |
| `claw_min_deg` | −30° | 扫走和手动夹爪下限，不影响普通张爪起点 |
| `sweep_claw_deg` | −30° | 单侧内收角度 |
| `sweep_overlap` | 0.5 | 横向交集 / 目标宽度，严格超过才加入转向偏置 |
| `sweep_weight` | 0.2 | 初始权重 |
| `sweep_near_multiplier` | 2.0 | 两框有实际面积重叠时的权重倍数 |
| `sweep_max_yaw_rate` | 1.0 rad/s | 权重乘此值产生朝障碍反方向的偏置 |
| `sweep_tau_sec` | 0.33 秒 | 指数衰减时间常数 |
| `sweep_duration_sec` | 1.0 秒 | 累计有效目标控制时间，结束偏置归零 |
| `max_yaw_rate` | 2.0 rad/s | actuator 合成视觉转向与偏置后的上限 |
| `discard_enabled` | false | 兼容字段；true 被拒绝 |
| `discard_speed_mps` | 0.1 m/s | 兼容命名，仍用作手动 move 默认速度 |
| `discard_turn_speed_mps` | 0.17 m/s | 兼容命名，仍用作手动 turn 默认每轮速度 |

丢弃运行入口全部停用。`discard.py` 仅保留旧算法以兼容历史测试，不再接入执行器。`discard_distance_m` 不参与目标抓取。

alignment 新增 `reverse_speed=0.1 m/s` 和 `grasp_pitch_tol=0.0296706 rad`；相机关闭时过近且居中允许倒退。`hold_seconds` 保留兼容但不延迟抓取请求。目标位置、角度和真实观测每周期重新检查，执行器完成扫走及单次张爪等待后才允许抓取。张爪后宽度变化不会重新张爪或重置等待；相机关闭时用目标进入目标带作为单次张爪条件。

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

## 左右夹爪闭合角度与任务记忆

在 `ros2_ws/src/block_bringup/config/perception.yaml` 的 `actuator.ros__parameters` 中修改：

```yaml
left_claw_close_deg: 0.0
right_claw_close_deg: 0.0
```

这是长期保留的配置，目标型和原地抓取均使用它。一次抓取开始时固定本次使用的角度；动作中修改参数不会改变正在执行的合爪目标。临时 `claw` 动作和扫走不会覆盖闭合配置。

旧的 `claw_close_deg` 已拆分为以上两个字段，自定义 YAML 也应更新。`ros2 param set` 只修改本次进程的参数，不会自动写回文件。长期修改请编辑 YAML，然后在项目根目录执行：

```bash
source /opt/ros/humble/setup.bash
cd ros2_ws
colcon build --build-base build_gcxl --install-base install_gcxl --packages-select block_perception
source install_gcxl/setup.bash
```

重启节点后读取新的配置。记忆与配置分开：记忆只存在本次运行内存里，重启清空。
`/robot/memory`（`std_msgs/String`，JSON）提供最新决策状态，晚加入的订阅者也能收到；
`/robot/memory/get`（`std_srvs/Trigger`）返回 JSON 完整历史。
状态包含任务、最近抓取/扫走（旧丢弃字段保留兼容）、夹持指令状态和中断记录，所有结果均不代表物理夹持确认。

## 动作保护参数（2026-11-10）

以下为当前 YAML 或新增参数的初始值，机械相关参数仍需实车标定。软件急停与复位服务见 [动作说明](project_actions.md)。本次修改后需同时重建 `block_perception` 和 `serial_minimal`。

| 节点/参数 | 初始值 | 含义 |
|---|---|---|
| alignment.approach_min_speed | 0.1 m/s | 接近抓取俯仰门槛时的低速 |
| alignment.approach_slow_pitch_deg | 15° | 俯仰距离门槛小于此值时逐步减速 |
| alignment.too_close_y | 0.9 | 舵机模式目标中心达到画面此高度比例时停车等待 |
| actuator.input_timeout | 0.25 s | 接收端 TTL 上限，另检查本地单调时钟年龄 |
| actuator.serial_health_timeout | 0.25 s | 桥接健康消息最大接收年龄 |
| actuator.require_serial_health | launch 的 serial 开关 | 启动只读；手动单独运行 actuator 时默认 false，连接实车须显式设置 true |
| actuator.pitch_feedback_margin_deg | 2° | 相机配置范围外允许的测量余量，不放宽目标角度范围 |
| actuator.feedback_reconnect_sec | 1 s | 反馈中断后允许重建序号基准的间隔 |
| actuator.max_sweep_attempts | 2 | 同一障碍的扫走次数上限 |
| actuator.max_sweeps | 8 | 每任务扫走次数总上限，避免编号变化绕过限制 |
| actuator.max_tracking_motion_sec | 10 s | 对准、接近和扫走累计底盘运动指令时间预算，等待不计入 |
| actuator.max_pitch_rate | 0.6 rad/s | 接收端允许的俯仰速度，与 alignment 的上限保持一致 |
| serial_node.write_timeout | 0.1 s | 帧从入队到完整写出的期限，启动时固定 |

越界控制量、运动/扫障预算耗尽、串口错误或串口健康超时会锁存停车；单纯目标或相机反馈过期会先停车等待。复位不会自动重启旧任务。
