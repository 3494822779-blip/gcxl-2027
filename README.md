# GCXL 2027

ROS 2 Humble / C++17 视觉与物块抓取系统。C++ 行为迁移尚在回归验证阶段，当前测试范围见 [迁移记录](docs/cpp_migration.md)。

## 构建

```bash
source /opt/ros/humble/setup.bash
cd ros2_ws
colcon build --executor sequential --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo
source install/setup.bash
```

## 启动

先准备 YOLO parser 动态库和模型文件（[模型说明](models/README.md)）。从仓库根目录执行：

```bash
source /opt/ros/humble/setup.bash
source ros2_ws/install/setup.bash
ros2 launch block_bringup perception.xml \
  model_dir:="$PWD/models/yolov8n_gcxl_aug" \
  headless:=true display:=false serial:=false
```

`serial:=true` 连接串口。默认等待 Action 请求；自动任务使用 `autonomous:=true target_class:=2`。串口与控制参数集中在 `ros2_ws/src/block_bringup/config/perception.yaml`。

## 测试

```bash
cd ros2_ws
colcon test --packages-select block_core block_perception block_actuator block_deepstream
colcon test-result --verbose
```

算法库也可以完全脱离 ROS 构建：

```bash
cmake -S ros2_ws/src/block_core -B /tmp/gcxl-core \
  -DCMAKE_DISABLE_FIND_PACKAGE_ament_cmake=ON -DBUILD_TESTING=ON
cmake --build /tmp/gcxl-core -j2
ctest --test-dir /tmp/gcxl-core --output-on-failure
```

依赖 C++17、JsonCpp、GoogleTest。更多说明：[工程结构](docs/architecture.md)、[参数](docs/parameters.md)、[串口协议](docs/serial_protocol.md)、[Action 行为](docs/project_actions.md)。
