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
from block_interfaces.msg import AlignmentCommand, GripperAngles
from block_perception.actuator import Actuator
from block_perception.first_grasp_task import FirstGraspTask


@pytest.fixture
def rig():
    rclpy.init()
    n = Actuator()
    if n.auto_timer is not None:
        n.auto_timer.cancel()
    n.set_parameters([Parameter('claw_settle_sec', value=.01)])
    n.state = 'hold'
    client = ActionClient(n, RobotTask, 'robot/execute')
    executor = SingleThreadedExecutor()
    executor.add_node(n)
    assert client.wait_for_server(timeout_sec=3.)
    yield n, client, executor
    client.destroy()
    n.action_api.destroy()
    executor.shutdown()
    n.destroy_node()
    rclpy.shutdown()


def until(rig, condition, timeout=3.):
    deadline = time.monotonic() + timeout
    while not condition() and time.monotonic() < deadline:
        rig[2].spin_once(timeout_sec=.01)
    assert condition(), 'ROS action did not reach expected state'


def goal(rig, operation, **kwargs):
    if operation in ('target_grasp', 'approach_grasp', 'first_grasp'):
        kwargs.setdefault('has_target_class', True)
        kwargs.setdefault('target_class_id', 2)
    future = rig[1].send_goal_async(RobotTask.Goal(operation=operation, **kwargs))
    until(rig, future.done)
    handle = future.result()
    if handle.accepted and operation != 'stop':
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
    ('grasp', {})])
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
                   ('first_grasp', {'has_target_class': False})]:
        assert not goal(rig, op, **kw).accepted
    h = goal(rig, 'approach_grasp', timeout_sec=.1)
    assert 'timeout' in result(rig, h, GoalStatus.STATUS_ABORTED).message


def command(n, **kw):
    if n.action_api.first_id:
        kw.setdefault('first_grasp_id', n.action_api.first_id)
        kw.setdefault('class_id', n.action_api.goal.target_class_id if n.action_api.goal else 2)
    values = dict(valid=True, valid_for_sec=.25, track_id=7)
    values.update(kw)
    msg = AlignmentCommand(**values)
    msg.header.stamp = n.get_clock().now().to_msg()
    n.grip = GripperAngles(track_id=7, angles_ready=True, valid_for_sec=.25,
        left_angle_rad=math.radians(n.left_claw), right_angle_rad=math.radians(n.right_claw))
    n.grip.header.stamp = msg.header.stamp
    n.opened_at = time.monotonic() - 1.
    n.opening_angles = (n.left_claw, n.right_claw)
    n.opening_track_id = 7
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
    feedback = JointState(name=['camera_pitch'], position=[math.radians(20.)])
    feedback.header.stamp = n.get_clock().now().to_msg()
    n.joints(feedback)
    assert result(rig, h).success
    assert n.cam_target == 20.
    h = goal(rig, 'camera', angle_deg=10., timeout_sec=.1)
    assert not result(rig, h, GoalStatus.STATUS_ABORTED).success
    assert n.cam_target == 10.


def test_target_task_transport_explicit_class_and_cancel(rig):
    n = rig[0]
    selector = FirstGraspTask()
    rig[2].add_node(selector)
    try:
        h = goal(rig, 'target_grasp', target_class_id=3)
        until(rig, lambda: selector.first_id == n.action_api.first_id)
        assert selector.target_class == 3
        command(n, first_grasp_id=n.action_api.first_id + 1, grasp_request=True)
        assert n.state == 'track'
        command(n, class_id=0, grasp_request=True)
        assert n.state == 'track'
        command(n, class_id=3, grasp_request=True)
        assert result(rig, h).success
        until(rig, lambda: not selector.task_active)
        assert n.memory.data['grasps'][-1]['class_id'] == 3
    finally:
        rig[2].remove_node(selector)
        selector.destroy_node()


def test_discard_and_missing_class_are_rejected(rig):
    for op in ('target_grasp', 'approach_grasp', 'first_grasp'):
        assert not goal(rig, op, has_target_class=False).accepted
        assert not goal(rig, op, target_class_id=-1).accepted
    assert not goal(rig, 'discard').accepted
    assert not goal(rig, 'grasp', discard_after=True).accepted


def test_autonomous_requires_class(rig):
    n = rig[0]
    class Timer:
        def cancel(self): pass
    n.auto_timer = Timer()
    n.start_autonomous()
    assert n.action_api.goal is None
    n.set_parameters([Parameter('autonomous_target_class', value=4)])
    n.start_autonomous()
    until(rig, lambda: n.action_api.goal is not None)
    assert n.action_api.goal.target_class_id == 4
    command(n, grasp_request=True)
    until(rig, lambda: not n.action_api.reserved)
    assert n.state == 'hold'


@pytest.mark.parametrize('operation', ['grasp', 'target_grasp', 'align'])
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
    h = goal(rig, 'target_grasp')
    cancel = h.cancel_goal_async()
    until(rig, cancel.done)
    result(rig, h, GoalStatus.STATUS_CANCELED)
    assert (n.left_claw, n.right_claw) == (0., 16.)
    assert n.memory.data['grasps'][-1]['status'] == 'completed'
    assert n.memory.data['grasps'][-1]['disposition'] == 'commanded_hold'
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


@pytest.mark.parametrize('ending', ['cancel', 'timeout'])
def test_sweep_action_interruption_stops_and_records_without_grasp(rig, ending):
    n = rig[0]
    h = goal(rig, 'target_grasp', timeout_sec=.15 if ending == 'timeout' else 10.)
    msg = AlignmentCommand(valid=True, valid_for_sec=.25, track_id=7, class_id=2,
        first_grasp_id=n.action_api.first_id, sweep_side=-1, obstacle_id=8,
        obstacle_overlap=.8, forward_speed=.1)
    msg.header.stamp = n.get_clock().now().to_msg()
    n.command(msg)
    assert n.sweep is not None and n.left_claw == -30.
    n.tick()  # the sweep posture has actually been published before interruption
    if ending == 'cancel':
        canceled = h.cancel_goal_async()
        until(rig, canceled.done)
    status = GoalStatus.STATUS_CANCELED if ending == 'cancel' else GoalStatus.STATUS_ABORTED
    assert not result(rig, h, status).success
    assert n.left_claw == -30.
    assert n.memory.data['sweeps'][-1]['status'] == ('canceled' if ending == 'cancel' else 'timeout')
    assert not n.memory.data['grasps'] and not n.memory.data['discards']


def test_complete_target_sweep_through_real_node_topics(rig):
    from block_interfaces.msg import FrameMetadata, Box
    from block_perception.nodes import Selector, Filter, Alignment
    from block_perception.gripper import Gripper
    n = rig[0]
    n.set_parameters_atomically([Parameter('sweep_duration_sec', value=.1),
                                Parameter('sweep_tau_sec', value=.033)])
    pipeline = [Selector(), Filter(), Alignment(), Gripper()]
    for node in pipeline:
        rig[2].add_node(node)
    frames = []
    sub = n.create_subscription(String, 'serial_tx', lambda msg: frames.append(msg.data), 10)
    pub = n.create_publisher(FrameMetadata, 'tracks_meta', 10)
    try:
        h = goal(rig, 'target_grasp', target_class_id=2, timeout_sec=4.)
        result_future = h.get_result_async()
        deadline = time.monotonic() + 4.
        next_frame = 0.
        obstacle_removed = False
        while not result_future.done() and time.monotonic() < deadline:
            if time.monotonic() >= next_frame:
                frame = FrameMetadata(width=640, height=480, boxes=[
                    Box(track_id=7, class_id=2, observed=True, confidence=.9,
                        x=280., y=304., width=80., height=80.),
                    Box(track_id=8, class_id=1, observed=True, confidence=.9,
                        x=280., y=360., width=80., height=80.)])
                # A successful sweep must have an observable result in the next frame.
                obstacle_removed = obstacle_removed or n.sweep is not None and n.sweep.done
                if obstacle_removed:
                    frame.boxes = frame.boxes[:1]
                    frame.boxes[0].y = 200.  # approach brings target centre into the 240 px band
                frame.header.stamp = n.get_clock().now().to_msg()
                pub.publish(frame)
                next_frame = time.monotonic() + .03
            rig[2].spin_once(timeout_sec=.005)
        assert result_future.done()
        assert result_future.result().status == GoalStatus.STATUS_SUCCEEDED
        assert result_future.result().result.track_id == 7
        assert len(n.memory.data['sweeps']) == len(n.memory.data['grasps']) == 1
        assert n.memory.data['sweeps'][0]['obstacle_id'] == 8
        assert n.memory.data['grasps'][0]['class_id'] == 2
        assert not n.memory.data['discards']
        assert any(',-30.0,' in frame for frame in frames)
        assert (n.left_claw, n.right_claw) == (0., 5.)
    finally:
        n.destroy_subscription(sub)
        n.destroy_publisher(pub)
        for node in pipeline:
            rig[2].remove_node(node)
            node.destroy_node()


def test_stop_preempts_move_and_does_not_latch(rig):
    n = rig[0]
    moving = goal(rig, 'move', distance_m=10., speed_mps=.2)
    stopped = goal(rig, 'stop')
    assert result(rig, stopped).success
    assert not result(rig, moving, GoalStatus.STATUS_ABORTED).success
    assert not n.safety_reason
    assert result(rig, goal(rig, 'move', distance_m=0.)).success


def test_stop_invalidates_a_goal_accepted_before_execute(rig):
    from types import SimpleNamespace
    from rclpy.action import GoalResponse
    n = rig[0]
    request = RobotTask.Goal(operation='claw', angle_deg=80.)
    assert n.action_api.accept(request) == GoalResponse.ACCEPT
    assert n.action_api.accept(RobotTask.Goal(operation='stop')) == GoalResponse.ACCEPT
    aborted = []
    pending = SimpleNamespace(request=request, abort=lambda: aborted.append(True))
    future = rig[2].create_task(n.action_api.execute, pending)
    until(rig, future.done)
    assert not future.result().success and aborted
    assert n.claw == 0. and not n.action_api.reserved


def test_emergency_stop_service_requires_explicit_reset_and_new_goal(rig):
    from std_srvs.srv import Trigger
    n = rig[0]
    moving = goal(rig, 'move', distance_m=10.)
    client = n.create_client(Trigger, 'robot/emergency_stop')
    assert client.wait_for_service(timeout_sec=1.)
    future = client.call_async(Trigger.Request())
    until(rig, future.done)
    assert future.result().success
    result(rig, moving, GoalStatus.STATUS_ABORTED)
    assert not goal(rig, 'claw', angle_deg=80.).accepted
    assert result(rig, goal(rig, 'stop')).success
    reset = n.create_client(Trigger, 'robot/reset_safety')
    assert reset.wait_for_service(timeout_sec=1.)
    future = reset.call_async(Trigger.Request())
    until(rig, future.done)
    assert future.result().success and n.state == 'hold'
    assert result(rig, goal(rig, 'move', distance_m=0.)).success
    n.destroy_client(client)
    n.destroy_client(reset)


def test_serial_fault_latches_even_after_link_recovers(rig):
    from std_msgs.msg import Bool
    from std_srvs.srv import Trigger
    n = rig[0]
    n.require_serial_health = True
    assert not goal(rig, 'move', distance_m=1.).accepted
    n.link_health(Bool(data=True))
    moving = goal(rig, 'move', distance_m=10.)
    n.link_health(Bool(data=False))
    result(rig, moving, GoalStatus.STATUS_ABORTED)
    assert not n.reset_safety(None, Trigger.Response()).success
    n.link_health(Bool(data=True))
    assert not goal(rig, 'move', distance_m=1.).accepted
    assert n.reset_safety(None, Trigger.Response()).success
    assert n.state == 'hold'


def test_align_never_changes_claw_opening(rig):
    n = rig[0]
    h = goal(rig, 'align')
    n.claw = 35.
    c = AlignmentCommand(valid=True, valid_for_sec=.25, track_id=7, open_allowed=True)
    c.header.stamp = n.get_clock().now().to_msg()
    n.angles(GripperAngles(header=c.header, track_id=7, angles_ready=True,
        valid_for_sec=.25, left_angle_rad=math.radians(65.), right_angle_rad=math.radians(65.)))
    n.command(c)
    n.tick()
    assert n.claw == 35. and n.opening_angles is None
    canceled = h.cancel_goal_async()
    until(rig, canceled.done)
    result(rig, h, GoalStatus.STATUS_CANCELED)


def test_target_loss_before_closure_and_dropout_after_closure(rig):
    n = rig[0]
    n.set_parameters([Parameter('camera_servo', value=True)])
    h = goal(rig, 'target_grasp')
    n.claw = 40.
    command(n, grasp_request=True)
    assert n.state == 'grasp' and n.close_time is None
    command(n, valid=False)
    pitch = JointState(name=['camera_pitch'], position=[math.radians(45.)])
    pitch.header.stamp = n.get_clock().now().to_msg()
    n.joints(pitch)
    n.tick()
    assert n.claw == 40. and n.close_time is None
    command(n, grasp_request=True)
    n.tick()
    assert n.claw == 0. and n.close_time is not None
    n.pitch_received -= 1.
    n.close_time -= 1.
    assert result(rig, h).success


@pytest.mark.parametrize('field,value', [('forward_speed', math.nan), ('yaw_rate', math.inf),
    ('pitch_rate', math.nan), ('obstacle_overlap', 2.), ('valid_for_sec', 0.),
    ('forward_speed', .6), ('yaw_rate', 3.), ('pitch_rate', 1.)])
def test_invalid_command_stops_and_latches(rig, field, value):
    n = rig[0]
    h = goal(rig, 'target_grasp')
    command(n, **{field: value})
    assert n.safety_reason and n.state == 'hold'
    result(rig, h, GoalStatus.STATUS_ABORTED)


def test_local_command_timeout_works_without_ros_clock_progress(rig):
    n = rig[0]
    h = goal(rig, 'target_grasp')
    command(n, forward_speed=.2, valid_for_sec=900.)
    assert n.compute(n.get_clock().now().nanoseconds * 1e-9)[0] > 0.
    n.cmd_received -= 1.
    assert n.compute(n.get_clock().now().nanoseconds * 1e-9)[:2] == (0., 0.)
    canceled = h.cancel_goal_async()
    until(rig, canceled.done)
    result(rig, h, GoalStatus.STATUS_CANCELED)


def test_pitch_rejects_replay_impossible_angle_and_old_joint_timestamp(rig):
    from block_perception.actuator import checksum
    n = rig[0]
    def feedback(seq, angle):
        body = f'FB,{seq},{angle}'
        n.serial_rx(String(data=f'${body}*{checksum(body)}\n'))
    feedback(65534, 20.)
    assert n.pitch is None
    feedback(65535, 20.)
    first = n.pitch_received
    feedback(65535, 30.)
    feedback(0, 999.)
    assert n.pitch_received == first and n.pitch == math.radians(20.)
    feedback(0, 25.)
    assert n.pitch == math.radians(25.)  # sequence wrap is valid
    msg = JointState(name=['camera_pitch'], position=[math.radians(40.)])
    msg.header.stamp = n.get_clock().now().to_msg()
    msg.header.stamp.sec -= 1
    n.joints(msg)
    assert n.pitch == math.radians(25.)
    received = n.pitch_received
    n.last_fb_time -= 2.
    feedback(0, 40.)
    assert n.pitch == math.radians(25.) and n.pitch_received == received
    feedback(1, 30.)
    assert n.pitch == math.radians(30.)


def test_stop_does_not_apply_an_unpublished_servo_goal(rig):
    from std_srvs.srv import Trigger
    n = rig[0]
    n.tick()
    previous = n.last_safe_servos
    n.claw = 80.
    n.emergency_stop(None, Trigger.Response())
    assert (n.left_claw, n.right_claw, n.cam_target) == previous


def test_tracking_motion_budget_counts_movement_and_stops(rig):
    n = rig[0]
    n.set_parameters([Parameter('max_tracking_motion_sec', value=.1)])
    h = goal(rig, 'target_grasp')
    command(n, forward_speed=.2)
    n.tick()
    n.motion_time -= .15
    n.tick()
    assert n.safety_reason == 'tracking_motion_budget_exceeded'
    result(rig, h, GoalStatus.STATUS_ABORTED)
