import copy
import time
import pytest
import rclpy
from rclpy.parameter import Parameter
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool, UInt64
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
    assert alignment.pub.messages[-1].grasp_request  # current policy fires immediately
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
    assert alignment.pub.messages[-1].aligned
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


def test_yaw_gain_fades_as_the_block_gets_closer(nodes):
    _, _, alignment = nodes
    yaw = []
    for y in (100., 300., 420.):          # far -> near; same error_x = 0.3 (outside every deadzone)
        target_at(alignment, y, x=416.)
        alignment.tick()
        yaw.append(abs(alignment.pub.messages[-1].yaw_rate))
    assert yaw[0] > yaw[1] > yaw[2] > 0   # weighted down smoothly, never a hard zero
    # floor = yaw_near_weight (0.2); offset weight at error_x 0.3 = 0.3 / yaw_full_error_x (0.5) = 0.6
    assert yaw[2] == pytest.approx(0.3 * 0.2 * 0.6 * abs(alignment.settings['yaw_gain']))


def test_yaw_gain_scales_with_offset_from_screen_centre(nodes):
    _, _, alignment = nodes
    assert alignment.set_parameters_atomically([Parameter('yaw_near_weight', value=1.)]).successful
    yaw = []
    for x in (352., 416., 480., 600.):   # error_x = 0.1, 0.3, 0.5, 0.875 at the same distance
        target_at(alignment, 100., x=x)
        alignment.tick()
        yaw.append(abs(alignment.pub.messages[-1].yaw_rate))
    gain = abs(alignment.settings['yaw_gain'])
    assert yaw[0] == pytest.approx(0.1 * gain * 0.3)     # floored at yaw_center_weight
    assert yaw[1] == pytest.approx(0.3 * gain * 0.6)     # 0.3 / yaw_full_error_x
    assert yaw[2] == pytest.approx(0.5 * gain)           # full gain from yaw_full_error_x on
    assert yaw[3] == pytest.approx(0.875 * gain)
    assert not alignment.set_parameters_atomically([Parameter('yaw_full_error_x', value=0.)]).successful


def test_x_deadzone_widens_towards_the_target_line(nodes):
    _, _, alignment = nodes
    target_at(alignment, 300., x=365.)   # error_x = 0.14: far, outside the still-narrow deadzone
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert not out.aligned and out.yaw_rate != 0
    target_at(alignment, 432., x=365.)   # same error_x on the target line: inside grasp_error_x
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert out.aligned and out.yaw_rate == 0
    assert alignment.set_parameters_atomically([Parameter('yaw_near_weight', value=1.5)]).successful is False


def test_camera_servo_mode_needs_pitch_and_drives_forward_until_it_is_down(nodes):
    _, _, alignment = nodes
    assert alignment.set_parameters_atomically([Parameter('camera_servo', value=True)]).successful
    assert not alignment.set_parameters_atomically([Parameter('grasp_pitch', value=2.)]).successful
    m = target_at(alignment, 300.)    # bottom edge above the band
    alignment.tick()
    out = alignment.pub.messages[-1]
    assert out.pitch_rate != 0 and out.forward_speed == 0   # no pitch feedback: no forward, no grasp
    pitch = JointState(name=['camera_pitch'], position=[-.4])   # pitch below grasp_pitch (-0.26)
    pitch.header.stamp = m.header.stamp
    alignment.joints(pitch)
    alignment.tick()
    assert alignment.pub.messages[-1].forward_speed == pytest.approx(alignment.settings['forward_speed'])
    assert alignment.pub.messages[-1].pitch_rate > 0  # target above band: visually track upward
    alignment.set_parameters([Parameter('deadzone_y', value=.05)])
    target_at(alignment, 470.)  # approaching makes the target drop below the band
    alignment.tick()
    assert alignment.pub.messages[-1].forward_speed > 0
    assert alignment.pub.messages[-1].pitch_rate < 0  # camera follows it downward
    target_at(alignment, 432.)
    pitch = JointState(name=['camera_pitch'], position=[-.2])
    pitch.header.stamp = alignment.get_clock().now().to_msg()
    alignment.joints(pitch)
    alignment.tick()
    assert alignment.pub.messages[-1].forward_speed == 0
    assert alignment.pub.messages[-1].grasp_request


def test_selector_follows_block_after_tracker_id_change(nodes):
    selector = nodes[0]
    def send(sec, boxes):
        f = Frame()
        f.image.width, f.image.height = 640, 480
        f.image.header.stamp.sec = sec
        f.boxes = boxes
        selector.frame(f)
        return selector.pub.messages[-1]
    block = dict(class_id=2, x=100., y=300., width=300., height=178., observed=True)
    assert send(10, [Box(track_id=21, **block)]).track_id == 21
    lost = send(11, [])
    assert lost.locked and not lost.observed and lost.track_id == 21
    again = send(12, [Box(track_id=22, **block)])
    assert again.observed and again.track_id == 22
    assert selector.set_parameters_atomically([Parameter('rebind_iou', value=2.)]).successful is False


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


def test_first_task_independent_trigger_clearance_and_completion(first_nodes):
    from block_interfaces.msg import AlignmentCommand
    selector, filt, alignment = first_nodes
    assert selector.first_id == 0
    selector.start_action(UInt64(data=selector.get_clock().now().nanoseconds))
    task_id = selector.first_id
    # Duplicate external goals are rejected by ActionAPI (covered in test_actions).
    assert selector.first_id == task_id
    frame = Frame()
    frame.image.width, frame.image.height = 640, 480
    frame.boxes = [Box(track_id=1, class_id=0, x=300., y=200., width=40., height=40., observed=True),
                   Box(track_id=2, class_id=2, x=300., y=300., width=40., height=40., observed=True)]
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    selector.frame(frame)
    target = selector.pub.messages[-1]
    assert target.track_id == 2 and target.clearance and target.first_grasp_id == task_id
    request = AlignmentCommand(first_grasp_id=task_id, track_id=2, clearance=True, grasp_request=True)
    selector.first_command(request)
    selector.reset(Bool(data=True))
    assert selector.first_busy is not None
    request.header.stamp = selector.get_clock().now().to_msg()
    selector.first_finished(request)
    assert selector.first_busy is None and not selector.first_done
    frame.boxes = frame.boxes[:1]
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    selector.frame(frame)
    target = selector.pub.messages[-1]
    assert target.track_id == 1 and not target.clearance
    request.track_id, request.clearance = 1, False
    selector.first_command(request)
    request.header.stamp = selector.get_clock().now().to_msg()
    selector.first_finished(request)
    assert selector.first_done
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    selector.frame(frame)
    assert not selector.pub.messages[-1].valid


def test_first_metadata_reaches_grasp_command_and_hazard_stops(first_nodes):
    selector, filt, alignment = first_nodes
    selector.start_action(UInt64(data=selector.get_clock().now().nanoseconds))
    frame = Frame()
    frame.image.width, frame.image.height = 640, 480
    frame.boxes = [Box(track_id=1, class_id=0, x=300., y=392., width=40., height=40., observed=True)]
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    selector.frame(frame)
    filt.target(selector.pub.messages[-1])
    alignment.target(filt.pub.messages[-1])
    alignment.tick()
    cmd = alignment.pub.messages[-1]
    assert cmd.grasp_request and cmd.first_grasp_id == selector.first_id
    assert not cmd.clearance and cmd.class_id == 0
    # A fresh hazard frame must not be converted to a predicted approach target.
    frame.boxes.append(Box(track_id=2, class_id=1, x=310., y=100., width=40., height=40., observed=True))
    frame.image.header.stamp = selector.get_clock().now().to_msg()
    selector.frame(frame)
    filt.target(selector.pub.messages[-1])
    alignment.target(filt.pub.messages[-1])
    alignment.tick()
    assert not alignment.pub.messages[-1].valid
    assert alignment.pub.messages[-1].forward_speed == 0.


@pytest.fixture
def first_nodes():
    from block_perception.first_grasp_task import FirstGraspTask
    rclpy.init()
    instances = [FirstGraspTask(), Filter(), Alignment()]
    for n in instances:
        n.pub = Capture()
    yield instances
    for n in instances:
        n.destroy_node()
    rclpy.shutdown()


def test_independent_task_idle_until_trigger_and_normal_selector_has_no_task(first_nodes):
    assert not hasattr(Selector, 'start_first')
    task = first_nodes[0]
    frame = Frame()
    frame.image.header.stamp = task.get_clock().now().to_msg()
    frame.boxes = [Box(track_id=1, class_id=0, x=100., y=100., width=40., height=40., observed=True)]
    task.frame(frame)
    assert not task.pub.messages[-1].valid
