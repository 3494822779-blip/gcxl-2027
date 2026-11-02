import copy
import time
import pytest
import rclpy
from rclpy.parameter import Parameter
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool
from block_interfaces.msg import Frame, Box, Target
from block_perception.nodes import Selector, Filter, Alignment, Display


class Capture:
    def __init__(self): self.messages = []
    def publish(self, msg): self.messages.append(copy.deepcopy(msg))


@pytest.fixture
def nodes():
    rclpy.init()
    instances = [Selector(), Filter(), Alignment()]
    for n in instances: n.pub = Capture()
    yield instances
    for n in instances: n.destroy_node()
    rclpy.shutdown()


def test_chain_lock_prediction_timeout_reset(nodes):
    selector, filt, alignment = nodes
    frame = Frame()
    frame.image.width, frame.image.height = 640, 480
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    frame.boxes = [Box(track_id=7, x=300., y=352., width=40., height=80., observed=True),
                   Box(track_id=8, x=50., y=100., width=40., height=80., observed=True)]
    selector.frame(frame)
    raw = selector.pub.messages[-1]
    assert raw.track_id == 7
    filt.target(raw)
    alignment.target(filt.pub.messages[-1])
    alignment.tick()
    assert alignment.pub.messages[-1].aligned
    assert not alignment.pub.messages[-1].grasp_request  # hold time not reached yet
    time.sleep(.01)
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    frame.boxes = [frame.boxes[1]]
    selector.frame(frame)
    filt.target(selector.pub.messages[-1])
    predicted = filt.pub.messages[-1]
    assert predicted.track_id == 7 and predicted.valid and not predicted.observed
    alignment.target(predicted)
    alignment.tick()
    assert alignment.pub.messages[-1].predicted
    assert not alignment.pub.messages[-1].grasp_request
    predicted.header.stamp.sec -= 1
    alignment.target_msg = predicted
    alignment.tick()
    stop = alignment.pub.messages[-1]
    assert not stop.valid and stop.yaw_rate == stop.pitch_rate == stop.forward_speed == 0
    raw = copy.deepcopy(selector.pub.messages[-1])
    raw.header.stamp.sec += 1
    filt.target(raw)
    assert not filt.pub.messages[-1].valid
    selector.reset(Bool(data=True))
    filt.target(selector.pub.messages[-1])
    assert not filt.pub.messages[-1].locked


def target_at(alignment, y, x=320.):
    m = Target(locked=True, valid=True, observed=True, track_id=12, image_width=640, image_height=480, x=x, y=y)
    m.header.stamp = alignment.get_clock().now().to_msg()
    alignment.target(m)
    return m


def test_grasp_one_shot_and_dynamic_parameters(nodes):
    _, _, alignment = nodes
    assert not alignment.set_parameters_atomically([Parameter('bottom_target', value=0.)]).successful
    assert alignment.settings['bottom_target'] == pytest.approx(.9)
    assert alignment.set_parameters_atomically([Parameter('deadzone_x', value=.1)]).successful
    target_at(alignment, 432.)   # bottom edge on the target line (0.9 * 480)
    alignment.tick()
    assert alignment.pub.messages[-1].aligned and not alignment.pub.messages[-1].grasp_request
    alignment.hold_start -= .4
    alignment.tick()
    assert alignment.pub.messages[-1].grasp_request
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert not out.grasp_request and out.yaw_rate == out.forward_speed == 0   # one shot, then frozen


def test_fixed_camera_drives_forward_until_bottom_edge_in_band(nodes):
    _, _, alignment = nodes
    target_at(alignment, 300.)    # bottom edge well above the band: far away
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert out.forward_speed == pytest.approx(alignment.settings['forward_speed'])
    assert not out.aligned and not out.grasp_request and out.reason == 'approaching'
    target_at(alignment, 300., x=600.)   # far off-centre: turn first, no forward
    alignment.tick()
    assert alignment.pub.messages[-1].forward_speed == 0 and alignment.pub.messages[-1].yaw_rate != 0
    target_at(alignment, 490.)    # predicted bottom edge past the image bottom: too close, stop (no reverse)
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert out.forward_speed == 0 and out.reason == 'too_close' and not out.aligned


def test_camera_servo_mode_needs_pitch_and_drives_forward_until_it_is_down(nodes):
    _, _, alignment = nodes
    assert alignment.set_parameters_atomically([Parameter('camera_servo', value=True)]).successful
    assert not alignment.set_parameters_atomically([Parameter('grasp_pitch', value=2.)]).successful
    m = target_at(alignment, 300.)    # bottom edge above the band
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert out.pitch_rate != 0 and out.forward_speed == 0   # no pitch feedback: no forward, no grasp
    pitch = JointState(name=['camera_pitch'], position=[-.1])   # camera not yet down to grasp_pitch (-0.26)
    pitch.header.stamp = m.header.stamp
    alignment.joints(pitch)
    alignment.tick()
    assert alignment.pub.messages[-1].forward_speed == pytest.approx(alignment.settings['forward_speed'])
    target_at(alignment, 432.)
    pitch = JointState(name=['camera_pitch'], position=[-.3])
    pitch.header.stamp = alignment.get_clock().now().to_msg()
    alignment.joints(pitch)
    alignment.tick()
    assert not alignment.pub.messages[-1].grasp_request and alignment.pub.messages[-1].forward_speed == 0
    alignment.hold_start -= .4
    alignment.tick()
    assert alignment.pub.messages[-1].grasp_request


def test_display_exact_join(nodes):
    display = Display()
    try:
        frame = Frame()
        frame.image.header.stamp.sec = 10
        frame.image.width, frame.image.height = 64, 48
        frame.image.encoding, frame.image.step = 'rgb8', 64 * 3
        frame.image.data = bytes(64 * 48 * 3)
        frame.boxes = [Box(track_id=9, x=5., y=5., width=20., height=20.)]
        target = Target(valid=True, observed=True, x=10., y=10., raw_x=11., raw_y=12.)
        target.header.stamp.sec = 11
        display.frame(frame)
        display.target(target)
        assert not display.fresh
        target.header.stamp.sec = 10
        display.target(target)
        assert display.fresh and display.front == 1
        front = display.buffers[display.front]
        assert front.shape == (48, 64, 3)
        assert front[5, 5].tolist() == [0, 255, 0]
        # a second joined frame renders into the other buffer and swaps back
        frame.image.header.stamp.sec = 12
        target.header.stamp.sec = 12
        display.frame(frame)
        display.target(target)
        assert display.front == 0 and display.buffers[0] is not display.buffers[1]
        # GUI still showing the buffer that would be overwritten: frame is dropped
        display.showing = 1
        frame.image.header.stamp.sec = 13
        target.header.stamp.sec = 13
        display.frame(frame)
        display.target(target)
        assert display.front == 0
    finally:
        display.destroy_node()
