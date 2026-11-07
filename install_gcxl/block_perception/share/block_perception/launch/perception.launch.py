import fcntl
import os
from pathlib import Path
import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, EmitEvent, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare

IMAGE_NODES = ['camera', 'detector', 'tracker']


def guard_single_instance(context):
    # Keep the descriptor in the launch context for the whole launch lifetime.
    # flock also closes the race between two simultaneous launch commands.
    lock = open(f'/tmp/block_perception_launch_{os.getuid()}.lock', 'a+')
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        # A killed launch parent can leave children alive after its lock is released.
        for proc in Path('/proc').iterdir():
            if not proc.name.isdigit():
                continue
            try:
                # A process can disappear between iterating /proc and reading
                # its files.  Zombies also retain a cmdline entry briefly and
                # must not block a new launch after the previous launch exits.
                state = (proc / 'stat').read_text().split()[2]
                if state == 'Z':
                    continue
                args = (proc / 'cmdline').read_bytes().split(b'\0')
            except (OSError, ProcessLookupError, IndexError, ValueError):
                continue
            if any(b'/lib/block_perception/' in arg or
                   b'/lib/block_deepstream/' in arg for arg in args):
                raise RuntimeError(f'Existing perception process PID {proc.name}; '
                                   'stop the previous instance before launching again.')
    except Exception:
        lock.close()
        raise
    context.extend_locals({'block_perception_instance_lock': lock})


def check_image_size(params_file):
    """camera, detector and tracker must agree on the image size (set once in the yaml)."""
    with open(params_file) as f:
        cfg = yaml.safe_load(f) or {}
    sizes = {n: (cfg[n]['ros__parameters'].get('width'), cfg[n]['ros__parameters'].get('height'))
             for n in IMAGE_NODES if n in cfg and cfg[n]}
    if len(set(sizes.values())) > 1:
        raise RuntimeError(f'width/height differ between nodes in {params_file}: {sizes}')


def launch_nodes(context):
    guard_single_instance(context)
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
    task = LaunchConfiguration('task').perform(context)
    if task not in ('normal', 'first_grasp'):
        raise RuntimeError('task must be normal or first_grasp')
    selector = 'first_grasp' if task == 'first_grasp' else 'selector'
    for name in [selector, 'filter', 'alignment', 'display', 'gripper', 'actuator']:
        node_params = params
        if name == 'actuator':
            node_params = params + [{'task_mode': task, 'autonomous': ParameterValue(
                LaunchConfiguration('autonomous'), value_type=bool)}]
        kwargs = {'condition': IfCondition(LaunchConfiguration(name))} if name == 'display' else {}
        actions.append(Node(package='block_perception', executable=name, name=name,
                            parameters=node_params, output='screen', **kwargs))
    actions.append(Node(package='serial_minimal', executable='serial_node', name='serial_node',
                        parameters=params,
                        condition=IfCondition(LaunchConfiguration('serial')), output='screen'))
    # A failed camera or controller must not leave the rest of this stack running.
    handlers = [RegisterEventHandler(OnProcessExit(
        target_action=node,
        on_exit=[EmitEvent(event=Shutdown(reason='Perception node exited; stopping stack'))]))
        for node in actions]
    return handlers + actions


def generate_launch_description():
    share = FindPackageShare('block_perception')
    defaults = {
        'params': PathJoinSubstitution([share, 'config', 'perception.yaml']),
        'autonomous': 'true', 'camera': 'false', 'use_sim_time': 'false', 'task': 'normal',
        'detector': 'true', 'tracker': 'true', 'display': 'true', 'serial': 'false',
    }
    actions = [DeclareLaunchArgument(k, default_value=v) for k, v in defaults.items()]
    return LaunchDescription(actions + [OpaqueFunction(function=launch_nodes)])
