import math
import pytest
import rclpy
from block_interfaces.msg import Frame, Box, Target
from block_perception.gripper import Gripper, calculate_angles


class Capture:
    def __init__(self): self.messages = []
    def publish(self, msg): self.messages.append(msg)


@pytest.fixture
def node():
    rclpy.init()
    n = Gripper()
    n.pub = Capture()
    yield n
    n.destroy_node()
    rclpy.shutdown()


def inputs(node):
    f = Frame()
    f.image.header.stamp = node.get_clock().now().to_msg()
    f.image.header.frame_id = 'camera'
    f.image.width, f.image.height = 640, 480
    f.boxes = [Box(track_id=3, x=100., width=80., observed=True),
               Box(track_id=4, x=300., width=90., observed=True)]
    t = Target(header=f.image.header, track_id=4, locked=True, valid=True, observed=True,
               image_width=640, image_height=480)
    return f, t


@pytest.mark.parametrize('frame_first', [True, False])
def test_exact_locked_edges_and_no_fake_angles(node, frame_first):
    f, t = inputs(node)
    if frame_first:
        node.frame(f)
        node.target(t)
    else:
        node.target(t)
        node.frame(f)
    out = node.pub.messages[-1]
    assert out.track_id == 4 and out.bounds_valid
    assert (out.left_edge_px, out.right_edge_px) == (300., 390.)
    assert not out.angles_ready and math.isnan(out.left_angle_rad) and math.isnan(out.right_angle_rad)
    assert out.reason == 'calculation_not_implemented'
    assert calculate_angles(300., 390., 640, 480) is None


def test_prediction_wrong_frame_and_timeout(node):
    f, t = inputs(node)
    t.observed = False
    node.frame(f)
    node.target(t)
    assert not node.pub.messages[-1].bounds_valid
    f, t = inputs(node)
    f.image.header.frame_id = 'other_camera'
    # Headers share a Python object; restore only target's frame ID with a new header.
    from std_msgs.msg import Header
    t.header = Header(stamp=f.image.header.stamp, frame_id='camera')
    node.target(t)
    node.frame(f)
    assert not node.pub.messages[-1].bounds_valid
    node.current.header.stamp.sec -= 1
    node.watchdog()
    assert node.pub.messages[-1].reason == 'stale_target'
    assert not node.pub.messages[-1].angles_ready


@pytest.mark.parametrize('x,width', [(620., 80.), (math.nan, 80.), (300., -10.)])
def test_invalid_edges(node, x, width):
    f, t = inputs(node)
    f.boxes[1].x, f.boxes[1].width = x, width
    node.frame(f)
    node.target(t)
    assert not node.pub.messages[-1].bounds_valid
    assert not node.pub.messages[-1].angles_ready
