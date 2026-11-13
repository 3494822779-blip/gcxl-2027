"""Target identity and actuator sweep integration, no serial port or robot motion."""
import copy
import math
from types import SimpleNamespace

import pytest
import rclpy
from rclpy.parameter import Parameter
from block_interfaces.action import RobotTask
from block_interfaces.msg import Box, FrameMetadata, TargetTask, AlignmentCommand, GripperAngles
from block_perception.nodes import Selector, Filter, Alignment
from block_perception.actuator import Actuator


class Capture:
    def __init__(self): self.messages = []
    def publish(self, msg): self.messages.append(copy.deepcopy(msg))


@pytest.fixture
def rig(monkeypatch):
    rclpy.init()
    selector, filt, alignment, actuator = Selector(), Filter(), Alignment(), Actuator()
    clock = SimpleNamespace(t=100.)
    monkeypatch.setattr('block_perception.actuator.time.monotonic', lambda: clock.t)
    for n in (selector, filt, alignment): n.pub = Capture()
    actuator.action_api.reserved = True
    actuator.action_api.goal = RobotTask.Goal(operation='target_grasp', has_target_class=True, target_class_id=2)
    actuator.action_api.first_id = 1
    actuator.state = 'track'
    actuator.memory.begin_task('target_grasp', 1)
    selector.task_request(TargetTask(task_id=1, target_class_id=2, active=True))
    yield selector, filt, alignment, actuator, clock
    for n in (selector, filt, alignment, actuator): n.destroy_node()
    rclpy.shutdown()


def send(n, **kw):
    msg = AlignmentCommand(first_grasp_id=1, class_id=2, track_id=9,
                           valid=True, valid_for_sec=.25, **kw)
    msg.header.stamp = n.get_clock().now().to_msg()
    n.command(msg)
    return msg


def compute(n):
    return n.compute(n.get_clock().now().nanoseconds * 1e-9)


def grip(n, angle=40.):
    msg = GripperAngles(track_id=9, angles_ready=True, valid_for_sec=.25,
                        left_angle_rad=math.radians(angle), right_angle_rad=math.radians(angle))
    msg.header.stamp = n.get_clock().now().to_msg()
    n.angles(msg)


def pitch(n, deg):
    from sensor_msgs.msg import JointState
    msg = JointState(name=['camera_pitch'], position=[math.radians(deg)])
    msg.header.stamp = n.get_clock().now().to_msg()
    n.joints(msg)


def test_selector_locks_formal_target_never_obstacle_or_new_id(rig):
    selector, filt, alignment, n, clock = rig
    target = Box(track_id=9, class_id=2, observed=True, x=280., y=250., width=80., height=80.)
    obstacle = Box(track_id=3, class_id=1, observed=True, x=280., y=320., width=80., height=80.)
    frame = FrameMetadata(width=640, height=480, boxes=[target, obstacle])
    def frame_now():
        frame.header.stamp = selector.get_clock().now().to_msg()
        selector.frame(frame)
        raw = selector.pub.messages[-1]
        filt.target(raw)
        alignment.target(filt.pub.messages[-1])
        alignment.tick()
        return raw, alignment.pub.messages[-1]
    raw, cmd = frame_now()
    assert raw.track_id == cmd.track_id == 9 and raw.obstacle_id == 3
    assert cmd.sweep_side == -1 and not cmd.clearance
    assert cmd.class_id == 2
    # A new ID far from the last box (no IoU) must not take over the formal target.
    stranger = copy.deepcopy(target)
    stranger.track_id = 11
    stranger.x = 0.
    frame.boxes = [stranger, obstacle]
    raw, cmd = frame_now()
    assert raw.track_id == 9 and not raw.observed
    assert cmd.forward_speed == cmd.yaw_rate == 0. and not cmd.grasp_request
    # An overlapping same-class replacement (tracker re-created the block, e.g. at the
    # image border) takes over the task target so tracking continues.
    replacement = copy.deepcopy(target)
    replacement.track_id = 10
    frame.boxes = [replacement, obstacle]
    raw, _ = frame_now()
    assert raw.track_id == 10 and raw.observed
    selector.task_request(TargetTask(task_id=1, active=False))
    raw, _ = frame_now()
    assert not raw.valid
    selector.task_request(TargetTask(task_id=1, target_class_id=2, active=True))
    assert not selector.task_active  # delayed duplicate cannot restart completed task


def test_single_sweep_decay_pause_restore_then_grasp_same_target(rig):
    _, _, _, n, clock = rig
    msg = send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.75,
               obstacle_near=True, forward_speed=.1)
    assert compute(n) == (0., 0., -30., 0., 0.)
    clock.t += .6  # let the selected claw reach its sweep position before driving
    send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.75, obstacle_near=True, forward_speed=.1)
    initial = compute(n)
    assert initial[0] > initial[1]
    assert not n.memory.data['grasps'] and len(n.memory.data['sweeps']) == 1
    clock.t += .1
    send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.75, forward_speed=.1)
    decayed = compute(n)
    assert 0 < decayed[0] - decayed[1] < initial[0] - initial[1]
    # Invalid/predicted target stops wheels and pauses sweep progress.
    elapsed = n.sweep.elapsed
    clock.t += .1
    send(n, predicted=True, forward_speed=.2)
    assert compute(n)[:2] == (0., 0.) and n.sweep.elapsed == elapsed
    clock.t += .1
    n.cmd.header.stamp.sec -= 1
    assert compute(n)[:2] == (0., 0.) and n.sweep.elapsed == elapsed
    # Readiness during sweep does not close the claw; the obstacle is never grasped.
    for _ in range(12):
        clock.t += .1
        grip(n)
        send(n, grasp_request=True, open_allowed=True)
        compute(n)
        assert n.state == 'track'
    assert n.sweep.done and len(n.memory.data['sweeps']) == 1
    assert n.memory.data['sweeps'][0]['status'] == 'completed'
    assert (n.left_claw, n.right_claw) == (40., 40.)
    clock.t += .6
    grip(n)
    send(n, grasp_request=True, open_allowed=True)
    assert n.state == 'grasp' and n.action.track_id == 9 and not n.action.clearance
    assert n.memory.data['grasps'][0]['track_id'] == 9


def test_fixed_camera_opens_once_and_width_does_not_restart_wait(rig):
    _, _, _, n, clock = rig
    grip(n)
    send(n, forward_speed=.1)
    assert compute(n)[2:4] == (0., 0.)
    send(n, open_allowed=True, grasp_request=True)
    compute(n)
    assert n.state == 'track' and n.opened_at == clock.t
    clock.t += .3
    grip(n, 50.)
    send(n, open_allowed=True, grasp_request=True)
    compute(n)
    assert n.state == 'track' and n.opened_at == clock.t - .3
    assert (n.left_claw, n.right_claw) == (40., 40.)
    clock.t += .6
    grip(n, 50.)
    send(n, open_allowed=True, grasp_request=True)
    assert n.state == 'grasp'


def test_sweep_memory_interruption_not_grasp_success(rig):
    n = rig[3]
    send(n, sweep_side=1, obstacle_id=6, obstacle_overlap=.7)
    n.memory.finish_task('canceled', 'sweeping')
    n.action_api.hold()
    assert compute(n)[:2] == (0., 0.)
    assert n.memory.data['sweeps'][0]['status'] == 'canceled'
    assert n.memory.data['grasps'] == []
    assert (n.left_claw, n.right_claw) == (0., -30.)


def test_negative_claw_limits_and_discard_config(rig):
    n = rig[3]
    assert not n.set_parameters_atomically([Parameter('sweep_claw_deg', value=-31.)]).successful
    assert not n.set_parameters_atomically([Parameter('discard_enabled', value=True)]).successful
    assert not n.set_parameters_atomically([Parameter('sweep_tau_sec', value=0.)]).successful
    n.action_api.validate(RobotTask.Goal(operation='claw', angle_deg=-30.))
    with pytest.raises(ValueError):
        n.action_api.validate(RobotTask.Goal(operation='claw', angle_deg=-31.))


def test_ready_is_rechecked_but_saved_width_is_not_recomputed(rig):
    _, _, _, n, clock = rig
    grip(n)
    send(n, open_allowed=True, grasp_request=True)
    compute(n)
    clock.t += .6
    grip(n, 60.)
    send(n, open_allowed=True, grasp_request=False)
    compute(n)
    assert n.state == 'track' and n.opening_angles == (40., 40.)
    assert (n.left_claw, n.right_claw) == (40., 40.)
    n.grip.header.stamp.sec -= 1  # saved width needs no refresh once opening was issued
    send(n, open_allowed=True, grasp_request=True)
    assert n.state == 'grasp'


def test_positive_pitch_tolerance_and_stale_feedback(rig):
    from sensor_msgs.msg import JointState
    from block_interfaces.msg import Target
    alignment = rig[2]
    alignment.set_parameters([Parameter('camera_servo', value=True)])
    for pitch, ready in ((38., False), (38.4, True), (40., True), (41.6, True), (42., True), (44.3, True), (55., True)):
        target = Target(locked=True, valid=True, observed=True, track_id=9, first_grasp_id=1,
            class_id=2, image_width=640, image_height=480, x=320., y=240.)
        target.header.stamp = alignment.get_clock().now().to_msg()
        alignment.target(target)
        feedback = JointState(name=['camera_pitch'], position=[math.radians(pitch)])
        feedback.header.stamp = target.header.stamp
        alignment.joints(feedback)
        alignment.tick()
        assert alignment.pub.messages[-1].grasp_request == ready
    alignment.pitch = math.radians(40.)
    alignment.pitch_time -= 1.
    alignment.tick()
    assert not alignment.pub.messages[-1].grasp_request


def test_temporary_frame_matching_does_not_starve_closure(rig):
    _, _, _, n, clock = rig
    grip(n, 58.2)
    send(n, open_allowed=True, grasp_request=True)
    compute(n)
    assert n.state == 'track'
    clock.t += .6
    # Separate metadata and target topics can deliver this temporary status just
    # before every alignment callback. The previous valid proposal is still fresh.
    waiting = GripperAngles(track_id=9, valid_for_sec=.25, reason='waiting_for_matching_frame')
    waiting.header.stamp = n.get_clock().now().to_msg()
    n.angles(waiting)
    send(n, open_allowed=True, grasp_request=True)
    assert n.state == 'grasp'
    assert compute(n)[2:4] == (0., 5.)


def test_stale_target_command_keeps_identity_and_clears_ready_phase(rig):
    from block_interfaces.msg import Target
    alignment, n = rig[2], rig[3]
    m = Target(locked=True, observed=True, valid=True, track_id=9, first_grasp_id=1,
               class_id=2, image_width=640, image_height=480, x=320., y=240.)
    m.header.stamp = alignment.get_clock().now().to_msg()
    alignment.target(m)
    alignment.tick()
    n.command(alignment.pub.messages[-1])
    m.header.stamp.sec -= 1
    alignment.target_msg = m
    alignment.tick()
    invalid = alignment.pub.messages[-1]
    assert invalid.first_grasp_id == 1 and invalid.class_id == 2 and invalid.track_id == 9
    n.command(invalid)
    assert not n.cmd.valid
    assert n.action_api.current_phase() == 'stale_or_invalid_target'
    assert compute(n)[:2] == (0., 0.)


@pytest.mark.parametrize('case', ['expired', 'other_target', 'invalid_bounds', 'lost_target'])
def test_temporary_matching_never_reuses_unsafe_proposal(rig, case):
    n = rig[3]
    grip(n)
    msg = GripperAngles(track_id=9, valid_for_sec=.25, reason='waiting_for_matching_frame')
    msg.header.stamp = n.get_clock().now().to_msg()
    if case == 'expired':
        n.grip.header.stamp.sec -= 1
    elif case == 'other_target':
        msg.track_id = 10
    elif case == 'invalid_bounds':
        msg.reason = 'invalid_bounds'
    else:
        msg.reason = 'target_not_observed'
    n.angles(msg)
    assert not n.grip.angles_ready
    send(n, open_allowed=True, grasp_request=True)
    assert n.state == 'track'
    assert n.action_api.current_phase() == 'waiting_for_gripper_angles'


def test_open_once_at_measured_twenty_degrees_then_hold_across_pitch_and_width_changes(rig):
    from sensor_msgs.msg import JointState
    n, clock = rig[3], rig[4]
    n.set_parameters([Parameter('camera_servo', value=True)])
    grip(n, 42.)
    send(n, open_allowed=True)
    n.cam_target = 30.  # command alone must not open the claws without actual feedback
    assert compute(n)[2:4] == (0., 0.)
    pitch(n, 19.9)
    assert compute(n)[2:4] == (0., 0.)
    pitch(n, 20.)
    send(n, open_allowed=False)  # image band is not the trigger in servo mode
    assert compute(n)[2:4] == (42., 42.)
    opened = n.opened_at
    for deg in (19., 21., 30., 40.):
        clock.t += .1
        pitch(n, deg)
        grip(n, 65.)
        send(n)
        assert compute(n)[2:4] == (42., 42.)
        assert n.opened_at == opened
    # A brief loss holds the opening and does not reset the dwell timer.
    send(n, predicted=True)
    assert compute(n)[:2] == (0., 0.)
    assert n.opened_at == opened and n.opening_angles == (42., 42.)
    clock.t += .2
    pitch(n, 40.)
    n.grip = None
    send(n, grasp_request=True, open_allowed=True)
    assert n.state == 'grasp'
    assert compute(n)[2:4] == (0., 5.)


def test_configurable_open_pitch_feedback_expiry_and_next_task_reset(rig):
    from sensor_msgs.msg import JointState
    n = rig[3]
    assert n.set_parameters_atomically([Parameter('camera_servo', value=True),
        Parameter('claw_open_pitch_deg', value=25.)]).successful
    assert not n.set_parameters_atomically([Parameter('claw_open_pitch_deg', value=100.)]).successful
    grip(n)
    send(n)
    pitch(n, 24.)
    assert compute(n)[2:4] == (0., 0.)
    pitch(n, 25.)
    n.pitch_time -= 1.
    assert compute(n)[2:4] == (0., 0.)
    pitch(n, 25.)
    assert compute(n)[2:4] == (40., 40.)
    n.reset_sweep()  # ActionAPI uses this reset at every new goal.
    grip(n, 60.)
    assert compute(n)[2:4] == pytest.approx((60., 60.))


def test_sweep_restores_saved_opening_without_remeasuring_width(rig):
    n, clock = rig[3], rig[4]
    grip(n, 45.)
    send(n, open_allowed=True)
    compute(n)
    send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.7)
    assert n.left_claw == -30.
    for _ in range(18):
        clock.t += .1
        grip(n, 70.)
        send(n)
        compute(n)
    assert n.sweep.done
    assert (n.left_claw, n.right_claw) == (45., 45.)
    assert n.opening_angles == (45., 45.)


def test_recorded_44_degree_feedback_triggers_and_closes_grasp(rig):
    from sensor_msgs.msg import JointState
    from block_interfaces.msg import Target
    alignment, n, clock = rig[2], rig[3], rig[4]
    alignment.set_parameters([Parameter('camera_servo', value=True)])
    n.set_parameters([Parameter('camera_servo', value=True)])
    feedback = JointState(name=['camera_pitch'], position=[math.radians(44.3)])
    feedback.header.stamp = alignment.get_clock().now().to_msg()
    alignment.joints(feedback)
    n.joints(feedback)
    target = Target(locked=True, observed=True, valid=True, track_id=9, first_grasp_id=1,
        class_id=2, image_width=640, image_height=480, x=320., y=240.)
    target.header.stamp = alignment.get_clock().now().to_msg()
    alignment.target(target)
    alignment.tick()
    cmd = alignment.pub.messages[-1]
    assert cmd.grasp_request and cmd.forward_speed == 0.
    grip(n, 48.3)
    n.command(cmd)
    compute(n)
    assert n.state == 'track'  # opening must still settle
    clock.t += .6
    feedback.header.stamp = n.get_clock().now().to_msg()
    n.joints(feedback)
    cmd.header.stamp = n.get_clock().now().to_msg()
    n.command(cmd)
    assert n.state == 'grasp'
    assert compute(n)[2:4] == (0., 5.)  # no need to wait for pitch to fall back to 40


def test_overshoot_does_not_bypass_missing_pitch_feedback(rig):
    from sensor_msgs.msg import JointState
    n = rig[3]
    n.set_parameters([Parameter('camera_servo', value=True)])
    n.claw = 48.3
    n.begin_grasp()
    pitch(n, 44.3)
    n.pitch_time -= 1.
    assert compute(n)[2:4] == (48.3, 48.3)


def test_lost_target_search_reverses_and_stops_on_reacquisition(rig):
    _, _, _, n, _ = rig
    n.set_parameters([Parameter('camera_servo', value=True)])
    n.formal_track_id = 9
    n.cam_target = 89.
    n.cmd = None
    n.cam_time = 100.
    n.pitch, n.pitch_time, n.pitch_received = math.radians(89.), 100., 100.
    wheels = n.compute(100.1)
    assert wheels[:2] == (0., 0.)
    assert n.cam_target == 90. and n.search_direction == -1
    n.compute(100.2)
    assert n.cam_target < 90.
    assert n.grasp_blocker(100.2) == 'searching_up'
    send(n)
    pitch(n, 89.)
    compute(n)
    assert n.search_direction == 0
    n.cmd = None
    n.pitch_time = n.cam_time
    n.compute(n.cam_time + .1)
    assert n.search_direction != 0
    n.action_api.hold()
    angle = n.cam_target
    n.compute(n.cam_time + .1)
    assert n.cam_target == angle and n.search_direction == 0


def test_no_search_before_lock_or_with_fixed_camera(rig):
    _, _, _, n, _ = rig
    n.set_parameters([Parameter('camera_servo', value=True)])
    n.cam_target = 30.
    n.cam_time = 100.
    n.compute(100.1)
    assert n.cam_target == 30. and n.search_direction == 0
    n.formal_track_id = 9
    n.set_parameters([Parameter('camera_servo', value=False)])
    n.compute(100.2)
    assert n.search_direction == 0


@pytest.mark.parametrize('target_x', [160., 480.])
@pytest.mark.parametrize('yaw_gain', [-1.25, 1.25])
@pytest.mark.parametrize('min_turn', [0., .17])
def test_search_turn_matches_tracking_yaw_sign_and_motor_threshold(rig, target_x, yaw_gain, min_turn):
    from block_interfaces.msg import Target
    alignment, n = rig[2], rig[3]
    alignment.set_parameters([Parameter('yaw_gain', value=yaw_gain)])
    n.set_parameters([Parameter('camera_servo', value=True),
                      Parameter('min_turn_speed', value=min_turn),
                      Parameter('search_turn_speed_mps', value=.08)])
    n.cam_target = 30.
    target = Target(locked=True, valid=True, observed=True, first_grasp_id=1,
                    class_id=2, track_id=9, image_width=640, image_height=480,
                    x=target_x, y=240.)
    target.header.stamp = alignment.get_clock().now().to_msg()
    alignment.target(target)
    alignment.tick()
    n.command(alignment.pub.messages[-1])
    pitch(n, 30.)
    tracking = compute(n)
    # Lost messages can contain zero error/yaw. Search must retain the actual
    # last real steering side, including installations with inverted gain.
    send(n, predicted=True)
    left, right, *_ = compute(n)
    assert (right - left) * (tracking[1] - tracking[0]) > 0.
    expected = max(.08, min_turn) * n.settings['wheel_speed_scale']
    assert abs(left) == pytest.approx(expected)
    assert abs(right) == pytest.approx(expected)
    assert left == -right


def test_search_attempts_are_bounded_per_task(rig):
    n, clock = rig[3], rig[4]
    n.set_parameters([Parameter('camera_servo', value=True)])
    n.cam_target = 45.
    for attempt in range(n.settings['search_max_attempts']):
        send(n, error_x=.5, yaw_rate=-.5)
        pitch(n, 45.)
        compute(n)
        send(n, predicted=True)
        assert compute(n)[:2] != (0., 0.)
        assert n.search_attempts == attempt + 1
        clock.t += .05
    # Seeing the target again resets the pass, but never resets the task count.
    send(n, error_x=.5, yaw_rate=-.5)
    compute(n)
    send(n, predicted=True)
    assert compute(n)[:2] == (0., 0.)
    angle = n.cam_target
    clock.t += .05
    assert compute(n)[:2] == (0., 0.) and n.cam_target == angle
    assert n.search_completed and n.search_direction == 0
    n.reset_sweep()
    assert n.search_attempts == 0


def test_search_finishes_after_one_pass(rig):
    n = rig[3]
    n.set_parameters([Parameter('camera_servo', value=True)])
    n.formal_track_id = 9
    n.cam_target = 89.
    n.cam_time = 100.
    for i in range(100):
        n.pitch, n.pitch_time, n.pitch_received = 0., 100. + (i + 1) * .1, 100.
        assert n.compute(100. + (i + 1) * .1)[:2] == (0., 0.)
    assert n.search_completed and n.search_direction == 0
    assert n.cam_target == n.settings['cam_min_deg']
    assert n.grasp_blocker(110.) == 'search_exhausted'
    send(n)
    pitch(n, 0.)
    compute(n)
    n.cmd = None
    angle = n.cam_target
    n.compute(n.cam_time + .1)
    # A fresh observation starts a new search budget; a later loss is allowed
    # to search again instead of remaining permanently exhausted.
    assert n.cam_target > angle and n.search_direction != 0
    n.reset_sweep()
    assert not n.search_completed


def test_multiple_obstacles_recover_claw_before_next_sweep(rig):
    n, clock = rig[3], rig[4]
    n.set_parameters([Parameter('sweep_duration_sec', value=.2), Parameter('claw_settle_sec', value=.1)])
    send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.8)
    assert compute(n)[:2] == (0., 0.)
    for _ in range(4):
        clock.t += .1
        send(n, sweep_side=1, obstacle_id=4, forward_speed=.2)
        compute(n)
    assert n.sweep.done and n.last_swept_obstacle_id == 3
    assert len(n.memory.data['sweeps']) == 1
    assert (n.left_claw, n.right_claw) == (0., 0.)
    clock.t += .2
    send(n, sweep_side=1, obstacle_id=4, obstacle_overlap=.8)
    assert n.sweep.side == 1 and n.sweep_obstacle_id == 4
    assert n.memory.data['sweeps'][-1]['obstacle_id'] == 4
    assert compute(n)[:2] == (0., 0.)  # right claw now settling


def test_persistent_obstacle_exhausts_budget_without_closing(rig):
    n, clock = rig[3], rig[4]
    n.set_parameters([Parameter('sweep_duration_sec', value=.1), Parameter('claw_settle_sec', value=.05)])
    for _ in range(20):
        send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.8, grasp_request=True)
        compute(n)
        if n.safety_reason:
            break
        clock.t += .1
    assert n.safety_reason == 'sweep_no_progress'
    assert n.sweep_count == 2 and not n.memory.data['grasps']


def test_track_rebind_preserves_saved_opening(rig):
    n, clock = rig[3], rig[4]
    grip(n, 45.)
    send(n, open_allowed=True)
    compute(n)
    first_opened = n.opened_at
    clock.t += .1
    msg = AlignmentCommand(first_grasp_id=1, class_id=2, track_id=10,
        valid=True, valid_for_sec=.25)
    msg.header.stamp = n.get_clock().now().to_msg()
    n.command(msg)
    compute(n)
    assert n.opening_track_id == 10 and n.opening_angles == (45., 45.)
    assert n.opened_at == first_opened


def test_grasp_requires_an_observation_after_sweep_completion(rig):
    n, clock = rig[3], rig[4]
    grip(n, 45.)
    send(n, open_allowed=True)
    compute(n)
    send(n, sweep_side=-1, obstacle_id=3, obstacle_overlap=.8)
    n.sweep.advance(n.sweep.duration)
    n.sweep_recovered_stamp = n.get_clock().now().nanoseconds * 1e-9 + .1
    n.opened_at = clock.t - 1.
    send(n, grasp_request=True)
    assert n.state == 'track'
    assert n.grasp_blocker(n.get_clock().now().nanoseconds * 1e-9) == 'waiting_for_post_sweep_observation'
