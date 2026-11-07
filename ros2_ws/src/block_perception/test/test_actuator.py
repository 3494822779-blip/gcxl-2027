import math
import pytest
import rclpy
from rclpy.parameter import Parameter
from sensor_msgs.msg import JointState
from std_msgs.msg import String
from block_interfaces.msg import AlignmentCommand, GripperAngles
from block_perception.actuator import Actuator, wheel_speeds, encode_command, decode_feedback, checksum


@pytest.fixture
def node():
    rclpy.init()
    n = Actuator()
    n.auto_timer.cancel()
    n.state = 'track'
    yield n
    n.destroy_node()
    rclpy.shutdown()


@pytest.fixture
def servo(node):
    assert node.set_parameters_atomically([Parameter('camera_servo', value=True)]).successful
    return node


def now(n):
    return n.get_clock().now().nanoseconds * 1e-9


def cmd(n, **kw):
    c = AlignmentCommand(valid=True, valid_for_sec=.25, **kw)
    c.header.stamp = n.get_clock().now().to_msg()
    return c


def test_wheel_mix_and_limit():
    assert wheel_speeds(.1, 0., .2, 1.) == (.1, .1)
    l, r = wheel_speeds(0., 1., .2, 1.)
    assert l == pytest.approx(-.1) and r == pytest.approx(.1)
    assert wheel_speeds(5., 0., .2, 1.) == (1., 1.)
    # tiny yaw is raised to min_turn per wheel; zero yaw stays straight; sign is kept
    assert wheel_speeds(0., .01, .2, 1., min_turn=.1) == pytest.approx((-.1, .1))
    assert wheel_speeds(0., -.01, .2, 1., min_turn=.1) == pytest.approx((.1, -.1))
    assert wheel_speeds(.3, 0., .2, 1., min_turn=.1) == (.3, .3)
    assert wheel_speeds(0., 2., .2, 1., min_turn=.1) == pytest.approx((-.2, .2))   # big yaw unchanged


def test_claws_follow_angles_but_not_below_init(node):
    g = GripperAngles(angles_ready=True, left_angle_rad=math.radians(45.), right_angle_rad=math.radians(45.), valid_for_sec=.25)
    g.header.stamp = node.get_clock().now().to_msg()
    node.angles(g)
    assert node.compute(now(node))[2] == pytest.approx(45.)
    g.left_angle_rad = math.radians(-5.)
    node.angles(g)
    assert node.compute(now(node))[2] == 0.


def test_fixed_camera_sends_constant_angle_and_grasp_closes_claws(node):
    node.set_parameters([Parameter('discard_enabled', value=False), Parameter('left_claw_close_deg', value=25.), Parameter('right_claw_close_deg', value=25.), Parameter('camera_fixed_deg', value=5.)])
    node.command(cmd(node, forward_speed=.15, yaw_rate=.2, pitch_rate=-.1))   # pitch_rate is ignored
    lw, rw, lg, rg, cp = node.compute(now(node) + .05)
    assert rw > lw and lg == rg == 0. and cp == 5.
    assert node.compute(now(node) + 100.) == (0., 0., 0., 0., 5.)   # stale command: stop, same cp
    node.command(cmd(node, grasp_request=True))
    assert node.state == 'grasp'
    assert node.compute(now(node)) == (0., 0., 25., 25., 5.)   # no camera phase: claws close at once
    node.close_time -= 1.
    node.compute(now(node))
    assert node.state == 'done'
    assert node.compute(now(node)) == (0., 0., 25., 25., 5.)


@pytest.mark.parametrize('pitch_rate', [-.2, 0., .2])
def test_forward_preserves_visual_pitch_tracking(servo, pitch_rate):
    servo.cam_target = 20.
    t = now(servo)
    servo.cam_time = t - .05
    servo.command(cmd(servo, forward_speed=.15, pitch_rate=pitch_rate))
    out = servo.compute(t + .01)
    assert out[0] > 0 and out[1] > 0
    assert out[4] == pytest.approx(20. + math.degrees(pitch_rate) * .06, abs=1e-5)


def test_servo_track_integrates_pitch_rate_into_angle_and_clamps(servo):
    node = servo
    node.command(cmd(node, forward_speed=.15, yaw_rate=.2, pitch_rate=-.1))
    t = now(node)
    node.compute(t)   # first call: dt = 0, target starts at home (0 deg)
    lw, rw, lg, rg, cp = node.compute(t + .05)
    assert rw > lw and lg == rg == 0.
    assert cp == pytest.approx(math.degrees(-.1) * .05)
    c = cmd(node, pitch_rate=-.1)
    for i in range(2, 400):
        c.header.stamp.sec, c.header.stamp.nanosec = int((t + .05 * i) // 1), int(((t + .05 * i) % 1) * 1e9)
        node.command(c)   # keep the command fresh at simulated time
        out = node.compute(t + .05 * i)
    assert out[4] == -15.   # clamped at cam_min_deg
    stale = node.compute(t + 100.)
    assert stale[:2] == (0., 0.) and stale[4] == -15.   # stale command: hold angle


def test_servo_camera_starts_from_feedback(servo):
    servo.joints(JointState(name=['camera_pitch'], position=[math.radians(-10.)]))
    assert servo.compute(now(servo))[4] == pytest.approx(-10.)


def test_servo_grasp_pitches_down_then_closes_then_homes(servo):
    node = servo
    node.set_parameters([Parameter('discard_enabled', value=False), Parameter('left_claw_close_deg', value=25.), Parameter('right_claw_close_deg', value=25.)])
    node.command(cmd(node, grasp_request=True, forward_speed=.15, yaw_rate=.3))
    assert node.state == 'grasp'
    # Phase 1: camera commanded down to the grasp angle; claws stay open at the initial angle.
    node.joints(JointState(name=['camera_pitch'], position=[-.3]))
    lw, rw, lg, rg, cp = node.compute(now(node))
    assert lw == rw == 0. and lg == rg == 0. and cp == -15.
    # Phase 2: pitch reaches the grasp angle -> claws close.
    node.joints(JointState(name=['camera_pitch'], position=[math.radians(-15.)]))
    assert node.compute(now(node))[2:5] == (25., 25., -15.)
    # Phase 3: settle elapses -> done; camera commanded back home.
    node.close_time -= 1.
    node.compute(now(node))
    assert node.state == 'done'
    assert node.compute(now(node))[4] == 0.


def test_command_frame_and_feedback_roundtrip():
    f = encode_command(7, .1, -.1, 20., 45.5, -25.5)
    body, _, tail = f[1:].partition('*')
    assert f.startswith('$CMD,7,0.100,-0.100,20.0,45.5,-25.5*') and tail == checksum(body)
    fb = '$FB,3,-30.5*' + checksum('FB,3,-30.5')
    assert decode_feedback(fb + '\r') == (3, -30.5)
    assert decode_feedback('$FB,3,-30.5*00') is None and decode_feedback('garbage') is None


def test_serial_rx_reassembles_lines_and_sets_pitch(node):
    line = '$FB,1,-30.0*' + checksum('FB,1,-30.0') + '\n'
    node.serial_rx(String(data=line[:6]))
    assert node.pitch is None
    node.serial_rx(String(data=line[6:]))
    assert node.pitch == pytest.approx(math.radians(-30.))


def test_discard_auto_and_manual_entry(node):
    from std_msgs.msg import Bool
    node.command(cmd(node, grasp_request=True))
    t = now(node)
    node.compute(t)
    node.compute(t + 1.)
    assert node.state == 'discard'
    task = node.discard_task
    node.request_discard(Bool(data=False))
    assert node.discard_task is task
    node.command(cmd(node, forward_speed=.4, grasp_request=True))
    lw, rw, *_ = node.compute(t + 1.05)
    assert lw < 0 < rw
    for _ in range(12):
        t += 100.
        node.compute(t)
    assert node.state == 'done'
    assert node.compute(t + 1.)[:4] == (0., 0., 90., 90.)
    node.set_parameters([Parameter('discard_enabled', value=False)])
    node.request_discard(Bool(data=True))
    for _ in range(100):
        if node.state == 'discard':
            break
        rclpy.spin_once(node, timeout_sec=.01)
    assert node.state == 'discard' and node.discard_task is not task


def test_manual_discard_during_tracking(node):
    from std_msgs.msg import Bool
    node.request_discard(Bool(data=False))
    assert node.state == 'track'
    node.request_discard(Bool(data=True))
    for _ in range(100):
        if node.state == 'discard':
            break
        rclpy.spin_once(node, timeout_sec=.01)
    assert node.state == 'discard'


def test_first_formal_grasp_holds_despite_auto_discard(node):
    node.command(cmd(node, grasp_request=True, first_grasp_id=100, class_id=0))
    t = now(node)
    node.compute(t)
    node.compute(t + 1.)
    assert node.state == 'done'
    assert node.claw == node.settings['left_claw_close_deg']


def test_first_rejects_nonzero_formal_and_stale_requests(node):
    node.command(cmd(node, grasp_request=True, first_grasp_id=100, class_id=1))
    assert node.state == 'track'
    node.command(cmd(node, grasp_request=True, first_grasp_id=100, predicted=True))
    assert node.state == 'track'


def test_clearance_forces_discard_and_rearms_without_stale_command(node):
    assert node.set_parameters_atomically([Parameter('discard_enabled', value=False)]).successful
    request = cmd(node, grasp_request=True, first_grasp_id=100, class_id=2, clearance=True)
    node.command(request)
    t = now(node)
    node.compute(t)
    node.compute(t + 1.)
    assert node.state == 'discard'
    for i in range(20):
        node.compute(t + 2. + i * 10.)
        if node.state == 'track':
            break
    assert node.state == 'track' and node.cmd is None
    node.command(request)
    assert node.state == 'track' and node.cmd is None


def test_new_first_task_can_approach_after_done_but_same_task_cannot(node):
    node.action = cmd(node, first_grasp_id=100)
    node.state = 'done'
    node.command(cmd(node, first_grasp_id=100, forward_speed=.1))
    assert node.state == 'done'
    node.command(cmd(node, first_grasp_id=101, forward_speed=.1))
    assert node.state == 'track'
    lw, rw, *_ = node.compute(now(node))
    assert lw > 0 and rw > 0


def test_independent_closure_holds_through_discard_first_turn_and_shutdown(node):
    node.set_parameters([Parameter('left_claw_close_deg', value=0.),
                         Parameter('right_claw_close_deg', value=17.)])
    node.command(cmd(node, grasp_request=True))
    t = now(node)
    assert node.compute(t)[2:4] == (0., 17.)
    node.compute(t + 1.)
    assert node.state == 'discard'
    assert node.compute(t + 1.05)[2:4] == (0., 17.)
    frames = []
    class Capture:
        def publish(self, msg):
            frames.append(msg.data)
    node.pub = Capture()
    node.on_stop(repeats=1, period=0.)
    assert ',0.000,0.000,0.0,17.0,' in frames[0]
    node.compute(t + 10.)  # stop between turn and open
    assert node.compute(t + 10.05)[2:4] == (90., 90.)
    assert node.settings['right_claw_close_deg'] == 17.


@pytest.mark.parametrize('name,value', [('left_claw_close_deg', -1.),
    ('right_claw_close_deg', 91.), ('right_claw_close_deg', math.nan)])
def test_closure_angle_limits(node, name, value):
    assert not node.set_parameters_atomically([Parameter(name, value=value)]).successful


def test_closure_parameters_snapshot_for_entire_grasp(node):
    node.set_parameters([Parameter('right_claw_close_deg', value=12.), Parameter('discard_enabled', value=False)])
    node.command(cmd(node, grasp_request=True))
    node.set_parameters([Parameter('right_claw_close_deg', value=19.)])
    assert node.compute(now(node))[2:4] == (0., 12.)
    assert node.memory.grasp['right_close_deg'] == 12.
    assert node.settings['right_claw_close_deg'] == 19.


def test_config_survives_node_restart_but_task_memory_does_not(tmp_path):
    params = tmp_path / 'closure.yaml'
    params.write_text('''actuator:
  ros__parameters:
    autonomous: false
    left_claw_close_deg: 0.0
    right_claw_close_deg: 13.0
''')
    sessions = []
    for _ in range(2):
        rclpy.init(args=['--ros-args', '--params-file', str(params)])
        n = Actuator()
        try:
            assert n.settings['left_claw_close_deg'] == 0.
            assert n.settings['right_claw_close_deg'] == 13.
            assert n.memory.data['grasps'] == []
            sessions.append(n.memory.session_id)
            n.memory.begin_task('grasp')
            n.begin_grasp()
            assert n.compute(now(n))[2:4] == (0., 13.)
            n.claw = 90.  # a temporary opening must not rewrite the persistent configuration
        finally:
            n.action_api.server.destroy()
            n.destroy_node()
            rclpy.shutdown()
    assert sessions[0] != sessions[1]
