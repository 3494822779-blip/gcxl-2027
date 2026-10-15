import copy
import time
import pytest
import rclpy
from rclpy.parameter import Parameter
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool
from block_interfaces.msg import Frame, Box, Target
from block_perception.nodes import Selector, Filter, Alignment, Annotations


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
    frame.boxes = [Box(track_id=7, x=300., y=200., width=40., height=80., observed=True),
                   Box(track_id=8, x=50., y=100., width=40., height=80., observed=True)]
    selector.frame(frame)
    raw = selector.pub.messages[-1]
    assert raw.track_id == 7
    filt.target(raw)
    alignment.target(filt.pub.messages[-1])
    alignment.tick()
    assert alignment.pub.messages[-1].aligned
    assert not alignment.pub.messages[-1].grasp_request  # no actual pitch feedback
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
    assert not stop.valid and stop.yaw_rate == stop.pitch_rate == 0
    raw = copy.deepcopy(selector.pub.messages[-1])
    raw.header.stamp.sec += 1
    filt.target(raw)
    assert not filt.pub.messages[-1].valid
    selector.reset(Bool(data=True))
    filt.target(selector.pub.messages[-1])
    assert not filt.pub.messages[-1].locked


def test_grasp_one_shot_feedback_and_dynamic_parameters(nodes):
    _, _, alignment = nodes
    assert not alignment.set_parameters_atomically([Parameter('pitch_min', value=1.), Parameter('pitch_max', value=0.)]).successful
    assert alignment.settings['pitch_min'] == -.8
    assert alignment.set_parameters_atomically([Parameter('deadzone_x', value=.1)]).successful
    m = Target(locked=True, valid=True, observed=True, track_id=12, image_width=640, image_height=480, x=320., y=240.)
    m.header.stamp = alignment.get_clock().now().to_msg()
    alignment.target(m)
    pitch = JointState(name=['camera_pitch'], position=[-.6])
    pitch.header.stamp = m.header.stamp
    alignment.joints(pitch)
    alignment.tick()
    assert not alignment.pub.messages[-1].grasp_request
    alignment.hold_start -= .4
    alignment.tick()
    assert alignment.pub.messages[-1].grasp_request
    alignment.tick()
    assert not alignment.pub.messages[-1].grasp_request
    assert alignment.pub.messages[-1].yaw_rate == 0
    alignment.pitch_time -= 1
    alignment.fired = False
    alignment.hold_start = 0.
    alignment.tick()
    assert not alignment.pub.messages[-1].grasp_request


def test_annotations_exact_join(nodes):
    pytest.importorskip('foxglove_msgs')
    annotation = Annotations()
    annotation.pub = Capture()
    try:
        frame = Frame()
        frame.image.header.stamp.sec = 10
        frame.image.width, frame.image.height = 640, 480
        frame.boxes = [Box(track_id=9, width=20., height=20.)]
        target = Target(valid=True, observed=True, x=10., y=10., raw_x=11., raw_y=12.)
        target.header.stamp.sec = 11
        annotation.frame(frame)
        annotation.target(target)
        assert not annotation.pub.messages
        target.header.stamp.sec = 10
        annotation.target(target)
        assert len(annotation.pub.messages) == 1
        msg = annotation.pub.messages[-1]
        assert len(msg.points) == 4
        assert msg.points[0].timestamp.sec == 10
    finally:
        annotation.destroy_node()
