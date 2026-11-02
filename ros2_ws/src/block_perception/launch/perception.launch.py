import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare

IMAGE_NODES = ['camera', 'detector', 'tracker']


def check_image_size(params_file):
    """camera, detector and tracker must agree on the image size (set once in the yaml)."""
    with open(params_file) as f:
        cfg = yaml.safe_load(f) or {}
    sizes = {n: (cfg[n]['ros__parameters'].get('width'), cfg[n]['ros__parameters'].get('height'))
             for n in IMAGE_NODES if n in cfg and cfg[n]}
    if len(set(sizes.values())) > 1:
        raise RuntimeError(f'width/height differ between nodes in {params_file}: {sizes}')


def launch_nodes(context):
    # 所有节点参数（含 detector/tracker 的 config_file）统一来自 params 指定的 YAML。
    check_image_size(LaunchConfiguration('params').perform(context))
    params = [LaunchConfiguration('params'),
              {'use_sim_time': ParameterValue(LaunchConfiguration('use_sim_time'), value_type=bool)}]
    actions = [Node(package='block_perception', executable='camera', name='camera', parameters=params,
                    condition=IfCondition(LaunchConfiguration('camera')), output='screen')]
    for name in ['detector', 'tracker']:
        actions.append(Node(package='block_deepstream', executable=name, name=name,
                            parameters=params,
                            condition=IfCondition(LaunchConfiguration(name)), output='screen'))
    for name in ['selector', 'filter', 'alignment', 'display', 'gripper', 'actuator']:
        kwargs = {'condition': IfCondition(LaunchConfiguration(name))} if name == 'display' else {}
        actions.append(Node(package='block_perception', executable=name, name=name,
                            parameters=params, output='screen', **kwargs))
    actions.append(Node(package='serial_minimal', executable='serial_node', name='serial_node',
                        parameters=params,
                        condition=IfCondition(LaunchConfiguration('serial')), output='screen'))
    return actions


def generate_launch_description():
    share = FindPackageShare('block_perception')
    defaults = {
        'params': PathJoinSubstitution([share, 'config', 'perception.yaml']),
        'camera': 'false', 'use_sim_time': 'false',
        'detector': 'true', 'tracker': 'true', 'display': 'true', 'serial': 'false',
    }
    actions = [DeclareLaunchArgument(k, default_value=v) for k, v in defaults.items()]
    return LaunchDescription(actions + [OpaqueFunction(function=launch_nodes)])
