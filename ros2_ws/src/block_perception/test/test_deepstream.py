"""Opt-in real-camera NVMM test: BLOCK_TEST_DEEPSTREAM=1 pytest this file."""
import os
import signal
import subprocess
import time
from pathlib import Path
import pytest


@pytest.mark.skipif(os.environ.get('BLOCK_TEST_DEEPSTREAM') != '1', reason='requires Jetson and USB camera')
def test_nvmm_metadata_and_osd(tmp_path):
    import rclpy
    from rclpy.qos import qos_profile_sensor_data
    from block_interfaces.msg import FrameMetadata, Target
    from ament_index_python.packages import get_package_share_directory, get_package_prefix
    config = Path(get_package_share_directory('block_perception')) / 'config/perception.yaml'
    with open(tmp_path / 'perception.log', 'w+') as log:
        binary = str(Path(get_package_prefix('block_deepstream')) / 'lib/block_deepstream/perception')
        process = subprocess.Popen([binary, '--ros-args',
            '--params-file', str(config), '-p', 'headless:=true'], stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        rclpy.init()
        node = rclpy.create_node('nvmm_metadata_test')
        seen = {'detections_meta': [], 'tracks_meta': []}
        target_pub = node.create_publisher(Target, 'target/filtered', qos_profile_sensor_data)
        def receive(topic, frame):
            assert frame.width == 1280 and frame.height == 720
            assert not hasattr(frame, 'image')
            seen[topic].append(frame)
            if topic == 'tracks_meta':
                target_pub.publish(Target(header=frame.header, image_width=frame.width,
                    image_height=frame.height, valid=True, observed=True, track_id=1,
                    x=640., y=360., raw_x=630., raw_y=350.))
        for topic in seen:
            node.create_subscription(FrameMetadata, topic,
                lambda frame, name=topic: receive(name, frame), qos_profile_sensor_data)
        try:
            deadline = time.monotonic() + 20
            while time.monotonic() < deadline and process.poll() is None:
                rclpy.spin_once(node, timeout_sec=.05)
            assert process.poll() is None
            assert min(map(len, seen.values())) > 100
            for frames in seen.values():
                stamps = [f.header.stamp.sec * 10**9 + f.header.stamp.nanosec for f in frames]
                assert stamps == sorted(set(stamps))
                assert all(f.header.frame_id == 'camera_optical_frame' for f in frames)
            detected = {(f.header.stamp.sec, f.header.stamp.nanosec) for f in seen['detections_meta']}
            matched = sum((f.header.stamp.sec, f.header.stamp.nanosec) in detected for f in seen['tracks_meta'])
            assert matched > 100
        finally:
            node.destroy_node(); rclpy.shutdown()
            if process.poll() is None: os.killpg(process.pid, signal.SIGINT)
            try: process.wait(timeout=8)
            except subprocess.TimeoutExpired: os.killpg(process.pid, signal.SIGKILL); process.wait()
        log.seek(0)
        output = log.read()
        assert process.returncode == 0, output
        for element in ('decode', 'convert', 'mux', 'infer', 'tracker', 'osd'):
            assert f'NVMM verified at {element}:' in output, output
        assert 'osd_match=' in output
        assert 'Non-NVMM' not in output
