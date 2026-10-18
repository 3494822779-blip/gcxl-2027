from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch_xml.launch_description_sources import XMLLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    share = FindPackageShare('block_perception')
    defaults = {
        'params': PathJoinSubstitution([share, 'config', 'perception.yaml']),
        'infer_config': PathJoinSubstitution([share, 'config', 'infer_blocks.txt']),
        'tracker_config': PathJoinSubstitution([share, 'config', 'tracker_blocks.yml']),
        'camera': 'false', 'source': '/dev/video0', 'width': '640', 'height': '480', 'use_sim_time': 'false',
        'detector': 'true', 'tracker': 'true', 'annotations': 'true', 'foxglove': 'true',
    }
    actions = [DeclareLaunchArgument(k, default_value=v) for k, v in defaults.items()]
    common = {'use_sim_time': ParameterValue(LaunchConfiguration('use_sim_time'), value_type=bool)}
    dimensions = {k: ParameterValue(LaunchConfiguration(k), value_type=int) for k in ['width', 'height']}
    actions.append(Node(package='block_perception', executable='camera', name='camera',
                        parameters=[common, dimensions, {'source': LaunchConfiguration('source')}],
                        condition=IfCondition(LaunchConfiguration('camera')), output='screen'))
    for name, cfg in [('detector', 'infer_config'), ('tracker', 'tracker_config')]:
        actions.append(Node(package='block_deepstream', executable=name, name=name,
                            parameters=[LaunchConfiguration('params'), common, dimensions,
                                        {'config_file': LaunchConfiguration(cfg)}],
                            condition=IfCondition(LaunchConfiguration(name)), output='screen'))
    for name in ['selector', 'filter', 'alignment', 'annotations', 'gripper']:
        kwargs = {'condition': IfCondition(LaunchConfiguration(name))} if name == 'annotations' else {}
        actions.append(Node(package='block_perception', executable=name, name=name,
                            parameters=[LaunchConfiguration('params'), common], output='screen', **kwargs))
    actions.append(IncludeLaunchDescription(
        XMLLaunchDescriptionSource(PathJoinSubstitution([FindPackageShare('foxglove_bridge'), 'launch', 'foxglove_bridge_launch.xml'])),
        condition=IfCondition(LaunchConfiguration('foxglove')),
        launch_arguments={'use_sim_time': LaunchConfiguration('use_sim_time')}.items()))
    return LaunchDescription(actions)
