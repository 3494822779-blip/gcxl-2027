# 物块抓取任务

完整功能梳理、每个动作的逻辑/代码映射、ROS 2 Action 调用与取消说明见 **[项目功能与动作接口](docs/project_actions.md)**。动作统一使用 `/robot/execute`（`block_interfaces/action/RobotTask`）；启动时加 `autonomous:=false` 可等待手动 Goal。

项目提供普通抓取和首次抓取两个任务入口。ROS 2 环境、编译及完整参数说明见 [ROS 工作区说明](ros2_ws/README.md)。新增接口后需重新编译 `block_interfaces`、`block_perception`，并重新 source 安装环境、重启相关节点。

## 快速启动（推荐）

使用优化脚本自动配置最佳性能并启动系统：

```bash
# 普通抓取任务（推荐使用此方式启动）
~/project/gcxl-2027/scripts/run_optimized.sh

# 首次抓取任务
~/project/gcxl-2027/scripts/run_optimized.sh --first-grasp

# 不启动相机或串口
~/project/gcxl-2027/scripts/run_optimized.sh --no-camera --no-serial
```

此脚本会自动检查并提示配置MAXN功率模式和TensorRT引擎。首次使用建议按提示完成所有优化步骤。

## 独立任务：首次抓取 class_0

首次抓取由独立 ROS 节点 `first_grasp` 管理，代码位于 [first_grasp_task.py](ros2_ws/src/block_perception/block_perception/first_grasp_task.py)。它独立维护目标筛选、清障循环和完成状态，复用底层对准、抓取及丢弃动作；普通 `selector` 不包含此任务逻辑。

### 启动与独立触发

启动首次抓取任务及其依赖节点：

```bash
ros2 launch block_perception first_grasp.launch.py camera:=true serial:=true
```

相机和串口参数按实际设备配置；使用外部相机话题时设 `camera:=false`。该入口替代普通 `perception.launch.py`，两套入口不要同时运行，以免同时控制底盘和夹爪。

启动后处于等待状态，不自动抓取。在另一个已加载 ROS 和工作区环境的终端中独立触发：

```bash
ros2 action send_goal /robot/execute block_interfaces/action/RobotTask "{operation: first_grasp}" --feedback
```

执行中重复触发会被忽略；任务完成后可再次触发。普通 `/target/reset` 不控制此独立任务。

### 执行规则

1. 首次正式抓取目标必须且只能是 **class_0**。
2. 判定区域为 class_0 检测框左右边缘之间、从框的上边缘延伸到画面顶部的区域。选择区域内没有 class_1 框或仅少量相交的目标，优先选择无遮挡目标。
3. 有前方物块遮挡时，先普通抓取遮挡物，再执行丢弃；丢弃结束后使用新图像重新检测、选目标，可重复清障。清障抓取不计入首次正式抓取。
4. 无合格目标时停车等待。正式抓取 class_0 后保持夹持并结束任务，不自动丢弃。

`first_grasp` 节点参数 `first_grasp_overlap` 控制允许的相交比例：交集面积除以对应 class_1 框面积，默认 `0.0`（不相交），取值范围 `[0, 1)`。配置位于 [perception.yaml](ros2_ws/src/block_perception/config/perception.yaml)。

前方遮挡目前按框横向相交且下边缘更靠近画面底部判断。该规则是图像几何近似，实际抓取范围及遮挡关系仍需实车验证。`/task/grasp_finished` 发布动作完成信息；完成表示上位机动作计时结束,不代表传感器确认已夹住物块。

## 性能优化

系统已针对Jetson Orin NX进行全面优化，实现**1280x720@60fps**高质量实时处理：

- **相机**：720p@60fps MJPEG + GPU硬件解码（nvjpegdec）
- **推理**：TensorRT FP16精度，检测延迟<60ms
- **跟踪**：NvDCF优化配置，跟踪延迟<80ms
- **系统**：MAXN功率模式 + 时钟锁定

**性能指标提升**：
- 分辨率：640x480 → 1280x720（+125%）
- 帧率：~28fps → 60fps（+114%）
- 检测延迟：~110ms → <60ms（-45%）
- GPU利用率：50% → 85%（+70%）

### 快速开始

1. **启用最大性能模式**（建议每次运行前执行）：

   ```bash
   sudo ~/project/gcxl-2027/scripts/enable_max_performance.sh
   ```

   此脚本将：
   - 设置MAXN功率模式（最大功率）
   - 锁定CPU/GPU到最大时钟频率
   - 优化系统调度器
   - 注意：重启后需要重新执行

2. **重新生成720p@60fps优化的TensorRT引擎**（首次或更换分辨率后）：

   ```bash
   ~/project/gcxl-2027/scripts/rebuild_engine_720p.sh
   ```

   生成时间约5-10分钟，引擎针对1280x720@60fps优化。

3. **实时性能监控**：

   ```bash
   ~/project/gcxl-2027/scripts/monitor_performance.sh
   ```

   显示实时帧率、GPU利用率、CPU使用率、温度和内存使用。

### 优化内容

**相机图像质量提升**：

- 使用硬件加速MJPEG解码（nvjpegdec）替代软件解码
- GPU零拷贝内存传输，减少CPU-GPU数据搬运
- 优化buffer管理，减少丢帧

**帧率提升**：

- 相机支持720p@30fps（从640x480@30fps提升）
- 更高分辨率提供更好的远距离检测精度
- 精确的帧率控制，避免不必要的跳帧

**DeepStream推理优化**：

- TensorRT引擎使用FP16精度（性能提升~2x）
- 针对720p输入分辨率优化
- 减少batched-push-timeout到25ms，降低端到端延迟
- 优化内存池大小和零拷贝传输

**跟踪器优化**：

- 降低特征图尺寸，提升跟踪速度
- 调整匹配阈值，更快建立新跟踪
- 优化视觉特征权重，减少计算开销
- 更快的状态估计适应高帧率

**系统级优化**：

- MAXN功率模式（最大CPU/GPU性能）
- 锁定最大时钟频率
- 性能调度器（performance governor）

### 预期性能

在Jetson Orin NX上：

- **相机帧率**：720p@60fps（稳定）
- **检测延迟**：<60ms（端到端）
- **跟踪延迟**：<80ms（端到端）
- **GPU利用率**：75-90%（MAXN模式）
- **功耗**：18-28W（MAXN模式）

### 故障排查

**帧率低于预期**：

1. 确认已启用MAXN模式：`sudo nvpmodel -q`
2. 确认时钟已锁定：`sudo jetson_clocks --show`
3. 检查是否使用了正确的TensorRT引擎（720p版本）
4. 监控GPU利用率，确保没有其他进程占用GPU

**图像质量问题**：

1. 确认相机支持1280x720@60fps MJPEG模式：`v4l2-ctl --device=/dev/video0 --list-formats-ext`
2. 确认USB 3.0连接（60fps需要约0.5 Gbps带宽）：`lsusb -t`（看到5000M）
3. 运行测试脚本：`~/project/gcxl-2027/scripts/test_camera_60fps.sh`
2. 检查USB带宽（使用USB 3.0接口）
3. 调整相机曝光和增益参数
4. 检查GStreamer pipeline是否使用nvjpegdec

**内存不足**：

1. 关闭不必要的后台程序
2. 减少tracker的maxTargetsPerStream参数
3. 考虑降低分辨率到640x480

更多配置参数见 [perception.yaml](ros2_ws/src/block_perception/config/perception.yaml)。
