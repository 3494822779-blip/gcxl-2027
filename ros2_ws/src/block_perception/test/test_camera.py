import threading
import time
from types import SimpleNamespace
from unittest.mock import Mock

import pytest
import rclpy

from block_perception.nodes import Camera
from block_perception import v4l2_capture


def test_device_disconnect_reopens_and_releases():
    stop = threading.Event()
    first, second = Mock(), Mock()
    first.sequence_gaps = second.sequence_gaps = 0
    first.error_frames = second.error_frames = 0
    first.read.side_effect = OSError('disconnected')

    def read():
        stop.set()
        return b'jpeg'

    second.read.side_effect = read
    node = SimpleNamespace(stopping=stop, is_device=True, capture=first,
                           read_errors=0, read_count=0, sequence_gaps=0, error_frames=0,
                           overwritten=0, latest=None, cond=threading.Condition(),
                           get_logger=lambda: Mock(), open_capture=Mock(return_value=second))
    node.close_capture = lambda: Camera.close_capture(node)
    Camera.read_loop(node)
    node.open_capture.assert_called_once()
    first.release.assert_called_once()
    second.release.assert_called_once()
    assert node.read_errors == 1 and node.capture is None


@pytest.mark.parametrize('exposure', [0, 100, -1])
def test_legacy_exposure_rejected_before_open(monkeypatch, exposure):
    open_capture = Mock()
    monkeypatch.setattr(Camera, 'open_capture', open_capture)
    rclpy.init(args=['--ros-args', '-p', f'exposure:={exposure}'])
    try:
        with pytest.raises(ValueError, match='removed'):
            Camera()
        open_capture.assert_not_called()
    finally:
        rclpy.shutdown()


def test_camera_decodes_rgb_and_stops_workers(monkeypatch):
    import cv2
    import numpy as np
    from sensor_msgs.msg import Image
    from rclpy.qos import qos_profile_sensor_data

    ok, jpeg = cv2.imencode('.jpg', np.full((720, 1280, 3), (0, 0, 240), dtype=np.uint8))
    assert ok
    capture = Mock(sequence_gaps=0, error_frames=0)

    def read():
        time.sleep(.02)
        return jpeg.tobytes()

    capture.read.side_effect = read
    monkeypatch.setattr(v4l2_capture, 'V4L2Capture', Mock(return_value=capture))
    rclpy.init()
    camera = Camera()
    subscriber = rclpy.create_node('camera_test_subscriber')
    frames = []
    subscriber.create_subscription(Image, 'camera/image_raw', frames.append, qos_profile_sensor_data)
    try:
        deadline = time.monotonic() + 5
        while not frames and time.monotonic() < deadline:
            rclpy.spin_once(subscriber, timeout_sec=.1)
        assert frames
        frame = frames[-1]
        assert (frame.width, frame.height, frame.encoding, frame.step) == (1280, 720, 'rgb8', 3840)
        assert len(frame.data) == 1280 * 720 * 3
        assert frame.data[0] > 200 and frame.data[2] < 10
    finally:
        camera.on_stop()
        subscriber.destroy_node()
        camera.destroy_node()
        rclpy.shutdown()
    assert not camera.reader.is_alive() and not camera.publisher.is_alive()
    capture.release.assert_called_once()
