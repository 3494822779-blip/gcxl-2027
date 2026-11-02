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


def test_claws_follow_angles_but_not_below_init(node):
    g = GripperAngles(angles_ready=True, left_angle_rad=math.radians(45.), right_angle_rad=math.radians(45.), valid_for_sec=.25)
    g.header.stamp = node.get_clock().now().to_msg()
    node.angles(g)
    assert node.compute(now(node))[2] == pytest.approx(45.)
    g.left_angle_rad = math.radians(-5.)
    node.angles(g)
    assert node.compute(now(node))[2] == 0.


def test_fixed_camera_sends_constant_angle_and_grasp_closes_claws(node):
    node.set_parameters([Parameter('claw_close_deg', value=25.), Parameter('camera_fixed_deg', value=5.)])
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
    node.set_parameters([Parameter('claw_close_deg', value=25.)])
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
