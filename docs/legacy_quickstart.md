# 迁移前使用说明（历史参考）

以下内容来自重构前 README，不作为当前行为验收依据。

# 物块抓取任务

> 目标抓取接口已更新：启动默认等待人工任务；`target_grasp`（以及 `approach_grasp`、`first_grasp`）必须显式提供 `has_target_class: true, target_class_id: <类别>`。自动运行使用 `autonomous:=true target_class:=<类别>`。清障采用有次数预算的单爪逐个扫走，丢弃已停用。完整规则和当前命令以 [动作说明](docs/project_actions.md) 为准。接口变更后须重建全部依赖包并重启节点。


完整功能梳理、每个动作的逻辑/代码映射、ROS 2 Action 调用与取消说明见 **[项目功能与动作接口](docs/project_actions.md)**。动作统一使用 `/robot/execute`（`block_interfaces/action/RobotTask`）；启动时加 `autonomous:=false` 可等待手动 Goal。

项目提供普通抓取和首次抓取两个任务入口。ROS 2 环境、编译及完整参数说明见 [ROS 工作区说明](ros2_ws/README.md)。新增接口后需重新编译 `block_interfaces`、`block_perception`，并重新 source 安装环境、重启相关节点。

## 快速启动（推荐）

使用优化脚本自动配置最佳性能并启动系统：

```bash
# 普通抓取任务（推荐使用此方式启动）
~/project/gcxl-2027/scripts/run/run_optimized.sh

# 首次抓取任务
~/project/gcxl-2027/scripts/run/run_optimized.sh --first-grasp

# 关闭显示和串口
~/project/gcxl-2027/scripts/run/run_optimized.sh --no-display --no-serial
```

此脚本会自动检查并提示配置MAXN功率模式。首次使用建议按提示完成所有优化步骤。

## 目标型抓取与单爪扫走

普通启动默认等待任务；`first_grasp.launch.py` 是同一选择逻辑的兼容入口，不再只选 class_0。

```bash
ros2 launch block_bringup perception.xml serial:=true
ros2 action send_goal /robot/execute block_interfaces/action/RobotTask "{operation: target_grasp, has_target_class: true, target_class_id: 2}" --feedback
```

必须显式指定类别，没有默认类别。`approach_grasp` 和 `first_grasp` 是相同流程的兼容名称，同样要求类别。
选中指定类别中下边缘最低的真实目标后锁定；优先处理前方下边缘最低、与目标横向相交的障碍，单爪内收 −30°，等待后继续接近正式目标并叠加指数衰减的反向转向偏置。
扫完恢复爪位并依据新观测复查，默认同一障碍最多两次、每任务总共八次；仍有障碍则停车，清障后抓取原目标。目标丢失时停车，可按同类别框几何重叠接管跟踪器新编号。
抓取结束保持夹持、停车并发出相机回正指令。`discard` 和 `discard_after: true` 已停用并拒绝执行。
详细参数、取消和记忆接口见 [动作说明](docs/project_actions.md)。

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
   sudo ~/project/gcxl-2027/scripts/tune/enable_max_performance.sh
   ```

   此脚本将：
   - 设置MAXN功率模式（最大功率）
   - 锁定CPU/GPU到最大时钟频率
   - 优化系统调度器
   - 注意：重启后需要重新执行

2. **重新生成720p@60fps优化的TensorRT引擎**（首次或更换分辨率后）：

   ```bash
   ~/project/gcxl-2027/scripts/model/rebuild_engine_720p.sh
   ```

   生成时间约5-10分钟，引擎针对1280x720@60fps优化。

3. **实时性能监控**：

   ```bash
   ~/project/gcxl-2027/scripts/tune/monitor_performance.sh
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
3. 运行测试脚本：`~/project/gcxl-2027/scripts/hardware/test_camera_60fps.sh`
2. 检查USB带宽（使用USB 3.0接口）
3. 调整相机曝光和增益参数
4. 检查GStreamer pipeline是否使用nvjpegdec

**内存不足**：

1. 关闭不必要的后台程序
2. 减少tracker的maxTargetsPerStream参数
3. 考虑降低分辨率到640x480

更多配置参数见 [perception.yaml](ros2_ws/src/block_bringup/config/perception.yaml)。
