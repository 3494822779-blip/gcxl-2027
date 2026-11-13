# GCXL 2027 工程结构

- `ros2_ws/src/`：唯一运行时源码，按 ROS 2 包组织。
  - `block_interfaces`：消息与 Action 接口
  - `block_core`：无 ROS 的 C++ 算法库
  - `block_perception`：目标选择、滤波、对准和夹爪节点
  - `block_actuator`：执行器、任务 Action、状态机和串口控制
  - `block_deepstream`：相机、推理、跟踪和元数据
  - `serial_minimal`：POSIX 串口桥
  - `block_bringup`：参数、模型配置和 XML 启动
- `block_bringup/config/`：DeepStream/推理配置
- `models/`：模型说明、标签和导出参数（权重/engine 由 LFS 或部署介质提供）
- `third_party/yolo_parser/`：DeepStream 自定义插件源码
- `scripts/`：设备调试、模型转换和性能工具
- `docs/`：协议、参数、验收和设计文档
- `artifacts/`：本地生成物临时目录，不进入版本控制

构建产物统一生成在 `ros2_ws/build`、`ros2_ws/install` 和 `ros2_ws/log`，这些目录已加入忽略规则。
