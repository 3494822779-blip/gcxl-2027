"""Exercise real ROS Action goal/result/cancel transport, without hardware."""
import math
import time

import pytest
import rclpy
from action_msgs.msg import GoalStatus
from rclpy.action import ActionClient
from rclpy.executors import SingleThreadedExecutor
from rclpy.parameter import Parameter
from sensor_msgs.msg import JointState
from std_msgs.msg import String
from block_interfaces.action import RobotTask
from block_interfaces.msg import AlignmentCommand
from block_perception.actuator import Actuator
from block_perception.first_grasp_task import FirstGraspTask


@pytest.fixture
def rig():
    rclpy.init()
    n = Actuator()
    n.auto_timer.cancel()
    n.set_parameters([Parameter('claw_settle_sec', value=.01)])
    n.state = 'hold'
    client = ActionClient(n, RobotTask, 'robot/execute')
    executor = SingleThreadedExecutor()
    executor.add_node(n)
    assert client.wait_for_server(timeout_sec=3.)
    yield n, client, executor
    client.destroy()
    n.action_api.server.destroy()
    executor.shutdown()
    n.destroy_node()
    rclpy.shutdown()


def until(rig, condition, timeout=3.):
    deadline = time.monotonic() + timeout
    while not condition() and time.monotonic() < deadline:
        rig[2].spin_once(timeout_sec=.01)
    assert condition(), 'ROS action did not reach expected state'


def goal(rig, operation, **kwargs):
    future = rig[1].send_goal_async(RobotTask.Goal(operation=operation, **kwargs))
    until(rig, future.done)
    handle = future.result()
    if handle.accepted:
        until(rig, lambda: rig[0].action_api.handle is not None)
    return handle


def result(rig, handle, status=GoalStatus.STATUS_SUCCEEDED):
    future = handle.get_result_async()
    until(rig, future.done)
    assert future.result().status == status
    until(rig, lambda: not rig[0].action_api.reserved)
    assert rig[0].state == 'hold'
    assert rig[0].compute(rig[0].get_clock().now().nanoseconds * 1e-9)[:2] == (0., 0.)
    return future.result().result


@pytest.mark.parametrize('op,kwargs', [
    ('stop', {}), ('move', {'distance_m': -.01, 'speed_mps': .2}),
    ('turn', {'angle_deg': 5., 'speed_mps': .2}), ('claw', {'angle_deg': 45.}),
    ('grasp', {}), ('discard', {})])
def test_operations_finish_and_stop(rig, op, kwargs):
    n = rig[0]
    n.set_parameters([Parameter('track_width', value=.001),
                      Parameter('discard_distance_m', value=0.)])
    h = goal(rig, op, **kwargs)
    assert h.accepted
    assert result(rig, h).success
    if op == 'grasp':
        assert n.claw == 0.  # explicit grasp defaults to holding, despite discard_enabled


def test_move_emits_reverse_and_cancel_holds(rig):
    n = rig[0]
    frames = []
    sub = n.create_subscription(String, 'serial_tx', lambda msg: frames.append(msg.data), 10)
    h = goal(rig, 'move', distance_m=-10., speed_mps=.2)
    until(rig, lambda: any(',-200.000,-200.000,' in f for f in frames))
    # Busy goal rejected, including during active execution.
    other = rig[1].send_goal_async(RobotTask.Goal(operation='claw', angle_deg=30.))
    until(rig, other.done)
    assert not other.result().accepted
    cancel = h.cancel_goal_async()
    until(rig, cancel.done)
    assert cancel.result().goals_canceling
    assert not result(rig, h, GoalStatus.STATUS_CANCELED).success
    n.destroy_subscription(sub)


def test_timeout_and_invalid_goals(rig):
    for op, kw in [('unknown', {}), ('move', {'speed_mps': math.nan}),
                   ('claw', {'angle_deg': 100.}), ('camera', {'angle_deg': 0.}),
                   ('first_grasp', {})]:
        assert not goal(rig, op, **kw).accepted
    h = goal(rig, 'approach_grasp', timeout_sec=.1)
    assert 'timeout' in result(rig, h, GoalStatus.STATUS_ABORTED).message


def command(n, **kw):
    msg = AlignmentCommand(valid=True, valid_for_sec=.25, track_id=7, **kw)
    msg.header.stamp = n.get_clock().now().to_msg()
    n.command(msg)


def test_alignment_does_not_close_and_approach_does(rig):
    n = rig[0]
    h = goal(rig, 'align')
    n.claw = 40.
    command(n, aligned=True, grasp_request=True)
    assert n.state == 'track'
    assert result(rig, h).success
    assert n.claw == 40.
    h = goal(rig, 'approach_grasp')
    command(n, aligned=True, grasp_request=True)
    assert n.state == 'grasp'
    assert result(rig, h).track_id == 7
    assert n.claw == 0.


def test_camera_requires_feedback_and_cancel_retains_target(rig):
    n = rig[0]
    n.set_parameters([Parameter('camera_servo', value=True)])
    h = goal(rig, 'camera', angle_deg=20.)
    n.joints(JointState(name=['camera_pitch'], position=[math.radians(20.)]))
    assert result(rig, h).success
    assert n.cam_target == 20.
    h = goal(rig, 'camera', angle_deg=10., timeout_sec=.1)
    assert not result(rig, h, GoalStatus.STATUS_ABORTED).success
    assert n.cam_target == 10.


def test_first_task_correlates_clearance_and_formal_completion(rig):
    n = rig[0]
    n.task_mode = 'first_grasp'
    n.set_parameters([Parameter('track_width', value=.001), Parameter('discard_distance_m', value=0.)])
    first = FirstGraspTask()
    rig[2].add_node(first)
    try:
        h = goal(rig, 'first_grasp')
        until(rig, lambda: first.first_id == n.action_api.first_id)
        task_id = first.first_id
        command(n, first_grasp_id=task_id + 1, grasp_request=True)
        assert n.state == 'track'
        command(n, first_grasp_id=task_id, class_id=2, clearance=True, grasp_request=True)
        until(rig, lambda: n.state == 'track')
        assert not n.action_api.completed
        command(n, first_grasp_id=task_id, class_id=0, grasp_request=True)
        assert result(rig, h).success
        until(rig, lambda: first.first_done)
        h = goal(rig, 'first_grasp')
        until(rig, lambda: first.first_id == n.action_api.first_id)
        cancel = h.cancel_goal_async()
        until(rig, cancel.done)
        result(rig, h, GoalStatus.STATUS_CANCELED)
        until(rig, lambda: first.first_done)
    finally:
        rig[2].remove_node(first)
        first.destroy_node()


def test_autonomous_and_legacy_topic_use_action_goals(rig):
    from std_msgs.msg import Bool
    n = rig[0]
    n.set_parameters([Parameter('discard_enabled', value=False)])
    n.start_autonomous()
    until(rig, lambda: n.action_api.goal is not None)
    assert n.action_api.goal.operation == 'approach_grasp'
    command(n, aligned=True, grasp_request=True)
    until(rig, lambda: not n.action_api.reserved)
    assert n.state == 'hold'
    n.request_discard(Bool(data=True))
    until(rig, lambda: n.action_api.goal is not None)
    assert n.action_api.goal.operation == 'discard'
    # Cancel by the action protocol even when the goal originated from a legacy topic.
    from action_msgs.srv import CancelGoal
    cancel_client = n.create_client(CancelGoal, 'robot/execute/_action/cancel_goal')
    assert cancel_client.wait_for_service(timeout_sec=1.)
    canceled = cancel_client.call_async(CancelGoal.Request())
    until(rig, canceled.done)
    until(rig, lambda: not n.action_api.reserved)
    assert n.state == 'hold'
    n.destroy_client(cancel_client)


@pytest.mark.parametrize('operation', ['grasp', 'discard', 'align'])
def test_cancel_during_substage_and_stale_requests_do_not_restart(rig, operation):
    n = rig[0]
    n.set_parameters([Parameter('camera_servo', value=True)])
    h = goal(rig, operation)
    cancel = h.cancel_goal_async()
    until(rig, cancel.done)
    result(rig, h, GoalStatus.STATUS_CANCELED)
    command(n, aligned=True, grasp_request=True)
    n.tick()
    assert n.state == 'hold'


def test_feedback_and_concurrent_admission(rig):
    feedback = []
    f1 = rig[1].send_goal_async(RobotTask.Goal(operation='move', distance_m=1.),
                               feedback_callback=lambda msg: feedback.append(msg.feedback))
    f2 = rig[1].send_goal_async(RobotTask.Goal(operation='turn', angle_deg=90.))
    until(rig, lambda: f1.done() and f2.done())
    assert sum(h.result().accepted for h in (f1, f2)) == 1
    accepted = f1.result() if f1.result().accepted else f2.result()
    if f1.result().accepted:
        until(rig, lambda: len(feedback) > 0)
        assert feedback[-1].phase == 'move' and feedback[-1].elapsed_sec >= 0
    canceled = accepted.cancel_goal_async()
    until(rig, canceled.done)
    result(rig, accepted, GoalStatus.STATUS_CANCELED)


def test_timeout_still_stops_when_ros_simulation_clock_is_paused(rig):
    n = rig[0]
    n.set_parameters([Parameter('use_sim_time', value=True)])
    assert n.get_clock().now().nanoseconds == 0
    h = goal(rig, 'approach_grasp', timeout_sec=.1)
    assert 'timeout' in result(rig, h, GoalStatus.STATUS_ABORTED).message


def test_memory_query_latched_context_and_independent_claw_cancel(rig):
    import json
    from std_srvs.srv import Trigger
    from rclpy.qos import QoSProfile, DurabilityPolicy
    n = rig[0]
    n.set_parameters([Parameter('right_claw_close_deg', value=16.)])
    h = goal(rig, 'grasp')
    assert result(rig, h).success
    assert (n.left_claw, n.right_claw) == (0., 16.)
    query = n.create_client(Trigger, 'robot/memory/get')
    assert query.wait_for_service(timeout_sec=1.)
    answer = query.call_async(Trigger.Request())
    until(rig, answer.done)
    snapshot = json.loads(answer.result().message)
    assert snapshot['history']['grasps'][-1]['class_id'] is None
    assert snapshot['context']['held']['state'] == 'commanded_hold'
    # Late subscribers receive the current decision context without waiting for a new task.
    updates = []
    sub = n.create_subscription(String, 'robot/memory', lambda msg: updates.append(json.loads(msg.data)),
                                QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
    until(rig, lambda: bool(updates))
    assert updates[-1]['task']['status'] == 'completed'
    h = goal(rig, 'discard')
    cancel = h.cancel_goal_async()
    until(rig, cancel.done)
    result(rig, h, GoalStatus.STATUS_CANCELED)
    assert (n.left_claw, n.right_claw) == (0., 16.)
    assert n.memory.data['grasps'][-1]['status'] == 'completed'
    assert n.memory.data['grasps'][-1]['disposition'] == 'discard_interrupted'
    assert n.memory.decision_context()['task']['status'] == 'canceled'
    n.destroy_subscription(sub)
    n.destroy_client(query)


def test_claw_action_does_not_change_long_term_closure_settings(rig):
    n = rig[0]
    n.set_parameters([Parameter('right_claw_close_deg', value=18.)])
    assert result(rig, goal(rig, 'claw', angle_deg=80.)).success
    assert n.settings['right_claw_close_deg'] == 18.
    assert result(rig, goal(rig, 'grasp')).success
    assert (n.left_claw, n.right_claw) == (0., 18.)


def test_first_selector_uses_memory_as_well_as_current_images(rig):
    import json
    from block_interfaces.msg import Frame, Box
    from std_msgs.msg import UInt64
    n = rig[0]
    first = FirstGraspTask()
    try:
        first.start_action(UInt64(data=123))
        n.memory.begin_task('first_grasp', 123)
        n.memory.begin_grasp(AlignmentCommand(first_grasp_id=123, class_id=0, track_id=3))
        n.memory.complete_grasp()
        first.remember(String(data=json.dumps(n.memory.decision_context())))
        assert not first.memory_allows_selection()  # formal grasp completed; no duplicate selection
        outputs = []
        class Capture:
            def publish(self, msg):
                outputs.append(msg)
        first.pub = Capture()
        frame = Frame()
        frame.image.width, frame.image.height = 640, 480
        frame.boxes = [Box(track_id=3, class_id=0, x=300., y=200., width=40., height=40., observed=True)]
        frame.image.header.stamp = first.get_clock().now().to_msg()
        first.frame(frame)
        assert not outputs[-1].valid  # visible target alone must not repeat a completed task
        first.start_action(UInt64(data=124))
        assert first.memory_allows_selection()  # old task cannot mark a new task completed
        n.memory.begin_task('first_grasp', 124)
        n.memory.begin_grasp(AlignmentCommand(first_grasp_id=124, class_id=2, track_id=4, clearance=True))
        n.memory.complete_grasp()
        n.memory.begin_discard()
        first.remember(String(data=json.dumps(n.memory.decision_context())))
        assert not first.memory_allows_selection()  # wait until the obstacle disposal finishes
        n.memory.complete_discard()
        first.remember(String(data=json.dumps(n.memory.decision_context())))
        assert first.memory_allows_selection()  # clearance is not the formal grasp
        frame.image.header.stamp = first.get_clock().now().to_msg()
        first.frame(frame)
        assert outputs[-1].valid and outputs[-1].class_id == 0
        n.memory.finish_task('timeout', 'track')
        first.remember(String(data=json.dumps(n.memory.decision_context())))
        assert not first.memory_allows_selection()
    finally:
        first.destroy_node()
