"""Exercise the real bridge with a pseudo terminal, never a physical serial device."""
import os
import time
import rclpy
from rclpy.action import ActionClient
from rclpy.executors import SingleThreadedExecutor
from block_interfaces.action import RobotTask
from block_perception.actuator import Actuator
from serial_minimal.serial_node import SerialNode


def test_bridge_failure_aborts_motion_using_real_health_topic():
    master, slave = os.openpty()
    port = os.ttyname(slave)
    rclpy.init(args=['--ros-args', '-p', f'port:={port}', '-p', 'require_serial_health:=true'])
    actuator = bridge = client = None
    executor = SingleThreadedExecutor()
    def until(condition):
        deadline = time.monotonic() + 3.
        while not condition() and time.monotonic() < deadline:
            executor.spin_once(timeout_sec=.01)
        assert condition()
    try:
        actuator, bridge = Actuator(), SerialNode()
        executor.add_node(actuator)
        executor.add_node(bridge)
        client = ActionClient(actuator, RobotTask, 'robot/execute')
        assert client.wait_for_server(timeout_sec=1.)
        until(actuator.link_ready)
        future = client.send_goal_async(RobotTask.Goal(operation='move', distance_m=10.))
        until(future.done)
        handle = future.result()
        assert handle.accepted
        until(lambda: actuator.action_api.handle is not None)
        result = handle.get_result_async()
        os.close(master)
        master = None
        until(result.done)
        assert not result.result().result.success
        assert actuator.safety_reason == 'serial_link_lost'
        assert actuator.state == 'hold'
    finally:
        if client is not None:
            client.destroy()
        executor.shutdown()
        if bridge is not None:
            bridge.destroy_node()
        if actuator is not None:
            actuator.action_api.destroy()
            actuator.destroy_node()
        rclpy.shutdown()
        if master is not None:
            os.close(master)
        os.close(slave)
