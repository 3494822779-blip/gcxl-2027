"""Opt-in on-device nvtracker test: BLOCK_TEST_DEEPSTREAM=1 pytest ..."""
import os
import subprocess
import time
from pathlib import Path
import pytest


@pytest.mark.skipif(os.environ.get('BLOCK_TEST_DEEPSTREAM') != '1', reason='requires DeepStream GPU runtime')
def test_detector_ros_roundtrip(tmp_path):
    """Exercise model loading, image preprocessing, parser and ROS timestamp preservation."""
    import rclpy
    from rclpy.qos import qos_profile_sensor_data
    from block_interfaces.msg import Frame
    from sensor_msgs.msg import Image
    config = Path(__file__).resolve().parents[1] / 'config/infer_blocks.txt'
    with open(tmp_path / 'detector.log', 'w+') as log:
        process = subprocess.Popen(
            ['ros2', 'run', 'block_deepstream', 'detector', '--ros-args',
             '-p', 'config_file:=' + str(config)], stdout=log, stderr=subprocess.STDOUT)
        rclpy.init()
        node = rclpy.create_node('detector_smoke_test')
        pub = node.create_publisher(Image, 'camera/image_raw', qos_profile_sensor_data)
        outputs = []
        node.create_subscription(Frame, 'detections', outputs.append, qos_profile_sensor_data)
        stamps = set()
        try:
            deadline = time.monotonic() + 30
            while pub.get_subscription_count() == 0 and time.monotonic() < deadline and process.poll() is None:
                rclpy.spin_once(node, timeout_sec=.1)
            assert pub.get_subscription_count() > 0, 'detector did not subscribe'
            image = Image(width=640, height=480, encoding='rgb8', step=1920,
                          data=bytes(640 * 480 * 3))
            sample = os.environ.get('BLOCK_TEST_IMAGE')
            if sample:
                import cv2
                pixels = cv2.imread(sample)
                assert pixels is not None, 'cannot read BLOCK_TEST_IMAGE'
                pixels = cv2.cvtColor(cv2.resize(pixels, (640, 480)), cv2.COLOR_BGR2RGB)
                image.data = pixels.tobytes()
            image.header.frame_id = 'camera_optical_frame'
            deadline = time.monotonic() + 30
            while len(outputs) < 10 and time.monotonic() < deadline and process.poll() is None:
                image.header.stamp = node.get_clock().now().to_msg()
                stamps.add((image.header.stamp.sec, image.header.stamp.nanosec))
                pub.publish(image)
                until = time.monotonic() + .1
                while time.monotonic() < until:
                    rclpy.spin_once(node, timeout_sec=.01)
            assert len(outputs) >= 10, 'detector did not return frames'
            assert all((f.image.header.stamp.sec, f.image.header.stamp.nanosec) in stamps for f in outputs)
            assert all(f.image.width == 640 and f.image.height == 480 for f in outputs)
            assert all(0 <= b.class_id < 4 and b.observed for f in outputs for b in f.boxes)
            if sample:
                assert any(f.boxes for f in outputs), 'no detections on supplied positive sample'
                print('Sample detections:', [(b.class_id, round(b.confidence, 3))
                                             for f in outputs[:1] for b in f.boxes])
        finally:
            node.destroy_node()
            rclpy.shutdown()
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            log.seek(0)
            print(log.read())


@pytest.mark.skipif(os.environ.get('BLOCK_TEST_DEEPSTREAM') != '1', reason='requires DeepStream GPU runtime')
def test_nvtracker_ros_roundtrip(tmp_path):
    import rclpy
    from rclpy.qos import qos_profile_sensor_data
    from block_interfaces.msg import Frame, Box
    from sensor_msgs.msg import Image
    log = open(tmp_path / 'tracker.log', 'w+')
    process = subprocess.Popen(['ros2', 'run', 'block_deepstream', 'tracker', '--ros-args',
                                '-p', 'config_file:=' + str(Path(__file__).resolve().parents[1] / 'config/tracker_blocks.yml')], stdout=log, stderr=subprocess.STDOUT)
    rclpy.init()
    node = rclpy.create_node('tracker_smoke_test')
    pub = node.create_publisher(Frame, 'detections', qos_profile_sensor_data)
    outputs = []
    node.create_subscription(Frame, 'tracks', outputs.append, qos_profile_sensor_data)
    try:
        deadline = time.monotonic() + 20
        while pub.get_subscription_count() == 0 and time.monotonic() < deadline and process.poll() is None:
            rclpy.spin_once(node, timeout_sec=.1)
        assert pub.get_subscription_count() > 0, 'tracker did not subscribe'
        import numpy as np
        rgb = np.zeros((480, 640, 3), dtype=np.uint8)
        rng = np.random.default_rng(3)
        rgb[200:280, 300:340] = rng.integers(0, 255, (80, 40, 3), dtype=np.uint8)
        image = Image(width=640, height=480, encoding='rgb8', step=1920, data=rgb.tobytes())
        observed_stamps = set()
        deadline = time.monotonic() + 20
        while len(outputs) < 15 and time.monotonic() < deadline and process.poll() is None:
            image.header.stamp = node.get_clock().now().to_msg()
            image.header.frame_id = 'camera_optical_frame'
            observed_stamps.add((image.header.stamp.sec, image.header.stamp.nanosec))
            pub.publish(Frame(image=image, boxes=[Box(class_id=0, confidence=.95, x=300., y=200., width=40., height=80., observed=True)]))
            until = time.monotonic() + .08
            while time.monotonic() < until:
                rclpy.spin_once(node, timeout_sec=.01)
        assert len(outputs) >= 10
        boxes = [f.boxes[0] for f in outputs if f.boxes]
        assert len(boxes) >= 10
        assert len({b.track_id for b in boxes}) == 1
        assert all(b.observed for b in boxes)
        assert all((f.image.header.stamp.sec, f.image.header.stamp.nanosec) in observed_stamps for f in outputs)
        # Tracker-only continuation must never be advertised as a fresh detector observation.
        empty_stamps = set()
        for _ in range(3):
            image.header.stamp = node.get_clock().now().to_msg()
            stamp = (image.header.stamp.sec, image.header.stamp.nanosec)
            empty_stamps.add(stamp)
            pub.publish(Frame(image=image, boxes=[]))
            until = time.monotonic() + 3
            while time.monotonic() < until:
                rclpy.spin_once(node, timeout_sec=.01)
                if any((f.image.header.stamp.sec, f.image.header.stamp.nanosec) == stamp for f in outputs):
                    break
        empty_outputs = [f for f in outputs
                         if (f.image.header.stamp.sec, f.image.header.stamp.nanosec) in empty_stamps]
        assert empty_outputs
        assert all(not b.observed for f in empty_outputs for b in f.boxes)

    finally:
        node.destroy_node()
        rclpy.shutdown()
        process.terminate()
        try: process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        log.seek(0)
        print(log.read())
        log.close()
