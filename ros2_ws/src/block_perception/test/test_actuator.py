import math
import pytest
import rclpy
from sensor_msgs.msg import JointState
from block_interfaces.msg import AlignmentCommand, GripperAngles
from block_perception.actuator import Actuator, wheel_speeds


@pytest.fixture
def node():
    rclpy.init()
    n = Actuator()
    yield n
    n.destroy_node()
    rclpy.shutdown()


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


def test_track_integrates_pitch_rate_into_angle_and_clamps(node):
    node.command(cmd(node, forward_speed=.15, yaw_rate=.2, pitch_rate=-.1))
    t = now(node)
    node.compute(t)   # first call: dt = 0, target starts at home (0 deg)
    lw, rw, lg, rg, cp = node.compute(t + .05)
    assert rw > lw and lg == rg == 20.
    assert cp == pytest.approx(math.degrees(-.1) * .05)   # -0.286 deg
    c = cmd(node, pitch_rate=-.1)
    for i in range(2, 400):
        c.header.stamp.sec, c.header.stamp.nanosec = int((t + .05 * i) // 1), int(((t + .05 * i) % 1) * 1e9)
        node.command(c)   # keep the command fresh at simulated time
        out = node.compute(t + .05 * i)
    assert out[4] == -60.   # clamped at cam_min_deg
    stale = node.compute(t + 100.)
    assert stale[:2] == (0., 0.) and stale[4] == -60.   # stale command: hold angle


def test_camera_starts_from_feedback(node):
    node.joints(JointState(name=['camera_pitch'], position=[math.radians(-20.)]))
    assert node.compute(now(node))[4] == pytest.approx(-20.)


def test_claws_follow_angles_but_not_below_init(node):
    g = GripperAngles(angles_ready=True, left_angle_rad=math.radians(45.), right_angle_rad=math.radians(45.), valid_for_sec=.25)
    g.header.stamp = node.get_clock().now().to_msg()
    node.angles(g)
    assert node.compute(now(node))[2] == pytest.approx(45.)
    g.left_angle_rad = math.radians(5.)
    node.angles(g)
    assert node.compute(now(node))[2] == 20.


def test_grasp_retracts_camera_then_closes_and_finishes(node):
    node.set_parameters([rclpy.parameter.Parameter('claw_close_deg', value=25.)])
    node.command(cmd(node, grasp_request=True, forward_speed=.15, yaw_rate=.3))
    assert node.state == 'grasp'
    j = JointState(name=['camera_pitch'], position=[-.6])
    node.joints(j)
    lw, rw, lg, rg, cp = node.compute(now(node))
    assert lw == rw == 0. and lg == rg == 25. and cp == 0.   # commanded back to home angle
    node.joints(JointState(name=['camera_pitch'], position=[0.]))
    node.compute(now(node))
    assert node.state == 'grasp'
    node.close_time -= 1.
    node.compute(now(node))
    assert node.state == 'done'
    assert node.compute(now(node))[4] == 0.


def test_command_frame_and_feedback_roundtrip(node):
    from block_perception.actuator import encode_command, decode_feedback, checksum
    f = encode_command(7, .1, -.1, 20., 45.5, -25.5)
    body, _, tail = f[1:].partition('*')
    assert f.startswith('$CMD,7,0.100,-0.100,20.0,45.5,-25.5*') and tail == checksum(body)
    fb = '$FB,3,-30.5*' + checksum('FB,3,-30.5')
    assert decode_feedback(fb + '\r') == (3, -30.5)
    assert decode_feedback('$FB,3,-30.5*00') is None and decode_feedback('garbage') is None


def test_serial_rx_reassembles_lines_and_sets_pitch(node):
    from std_msgs.msg import String
    from block_perception.actuator import checksum
    line = '$FB,1,-30.0*' + checksum('FB,1,-30.0') + '\n'
    node.serial_rx(String(data=line[:6]))
    assert node.pitch is None
    node.serial_rx(String(data=line[6:]))
    assert node.pitch == pytest.approx(math.radians(-30.))
