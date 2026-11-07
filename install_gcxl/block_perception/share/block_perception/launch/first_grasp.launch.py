"""Launch the independent first-grasp task with shared perception/action nodes."""
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription([IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([
            FindPackageShare('block_perception'), 'launch', 'perception.launch.py'])),
        launch_arguments={'task': 'first_grasp'}.items())])
