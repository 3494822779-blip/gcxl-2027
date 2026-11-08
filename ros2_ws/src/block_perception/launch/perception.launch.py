import fcntl
import os
from pathlib import Path
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, EmitEvent, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare



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


def launch_nodes(context):
    autonomous = LaunchConfiguration('autonomous').perform(context).lower() in ('true', '1')
    target_class = int(LaunchConfiguration('target_class').perform(context))
    if autonomous and target_class < 0:
        raise RuntimeError('autonomous:=true requires target_class:=<nonnegative class>')
    guard_single_instance(context)
    params = [LaunchConfiguration('params')]
    actions = [Node(package='block_deepstream', executable='perception', name='perception',
                    parameters=params + [{
                        'display': ParameterValue(LaunchConfiguration('display'), value_type=bool),
                        'headless': ParameterValue(LaunchConfiguration('headless'), value_type=bool),
                    }], output='screen')]
    task = LaunchConfiguration('task').perform(context)
    if task not in ('normal', 'first_grasp'):
        raise RuntimeError('task must be normal or first_grasp')
    selector = 'first_grasp' if task == 'first_grasp' else 'selector'
    for name in [selector, 'filter', 'alignment', 'gripper', 'actuator']:
        node_params = params
        if name == 'actuator':
            node_params = params + [{'task_mode': task, 'autonomous': ParameterValue(
                LaunchConfiguration('autonomous'), value_type=bool),
                'autonomous_target_class': target_class}]
        actions.append(Node(package='block_perception', executable=name, name=name,
                            parameters=node_params, output='screen'))
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
        'autonomous': 'false', 'task': 'normal', 'target_class': '-1',
        'display': 'true', 'headless': 'false', 'serial': 'false',
    }
    actions = [DeclareLaunchArgument(k, default_value=v) for k, v in defaults.items()]
    return LaunchDescription(actions + [OpaqueFunction(function=launch_nodes)])
