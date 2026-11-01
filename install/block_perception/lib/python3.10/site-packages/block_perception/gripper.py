"""Locked target bounding edges -> gripper angle proposal (formula intentionally absent)."""
import copy
import math
from block_interfaces.msg import Frame, Target, GripperAngles
from rclpy.qos import qos_profile_sensor_data
from .nodes import ConfigNode, run


def calculate_angles(left_edge_px, right_edge_px, image_width, image_height):
    """Return (left_rad, right_rad), or None if no calibrated formula is available.

    Inputs are pixel boundaries in the original image, not normalized errors.
    TODO: implement the user's geometric/calibrated calculation here.
    """
    return None


def stamp_ns(header):
    return header.stamp.sec * 1_000_000_000 + header.stamp.nanosec


class Gripper(ConfigNode):
    def __init__(self):
        super().__init__('gripper')
        self.configure(dict(input_timeout=0.25))
        self.frames = {}
        self.current = None
        self.last_time = None
        self.pub = self.create_publisher(GripperAngles, 'gripper/angles', 10)
        self.create_subscription(Frame, 'tracks', self.frame, qos_profile_sensor_data)
        self.create_subscription(Target, 'target/raw', self.target, 10)
        self.create_timer(0.05, self.watchdog)

    def invalid(self, reason, target=None):
        out = GripperAngles(left_angle_rad=math.nan, right_angle_rad=math.nan,
                            valid_for_sec=self.settings['input_timeout'], reason=reason)
        if target is not None:
            out.header = copy.deepcopy(target.header)
            out.track_id = target.track_id
            out.image_width, out.image_height = target.image_width, target.image_height
        else:
            out.header.stamp = self.get_clock().now().to_msg()
        return out

    def frame(self, msg):
        # Cache metadata only, never retain the large RGB payload.
        key = (stamp_ns(msg.image.header), msg.image.header.frame_id)
        self.frames[key] = (msg.image.width, msg.image.height, msg.boxes)
        while len(self.frames) > 30:
            self.frames.pop(next(iter(self.frames)))
        self.match()

    def target(self, msg):
        if self.current is not None and stamp_ns(msg.header) <= stamp_ns(self.current.header):
            return
        self.current = msg
        if not msg.locked or not msg.valid or not msg.observed:
            self.pub.publish(self.invalid('target_not_observed', msg))
            return
        self.pub.publish(self.invalid('waiting_for_matching_frame', msg))
        self.match()

    def match(self):
        target = self.current
        if target is None or not target.locked or not target.valid or not target.observed:
            return
        key = (stamp_ns(target.header), target.header.frame_id)
        metadata = self.frames.pop(key, None)
        if metadata is None:
            return
        out = self.invalid('invalid_bounds', target)
        age = (self.get_clock().now().nanoseconds - key[0]) * 1e-9
        if not 0 <= age <= self.settings['input_timeout']:
            out.reason = 'stale_target'
        else:
            width, height, boxes = metadata
            matches = [b for b in boxes if b.track_id == target.track_id and b.observed]
            if len(matches) == 1 and width == target.image_width and height == target.image_height:
                b = matches[0]
                left, right = float(b.x), float(b.x + b.width)
                if height > 0 and math.isfinite(left) and math.isfinite(right) and 0 <= left < right <= width:
                    out.bounds_valid = True
                    out.left_edge_px, out.right_edge_px = left, right
                    angles = calculate_angles(left, right, width, height)
                    out.reason = 'calculation_not_implemented'
                    if angles is not None:
                        if len(angles) == 2 and all(math.isfinite(a) for a in angles):
                            out.left_angle_rad, out.right_angle_rad = map(float, angles)
                            out.angles_ready, out.reason = True, 'ready'
                        else:
                            out.reason = 'invalid_calculation_result'
        self.pub.publish(out)

    def watchdog(self):
        now = self.get_clock().now().nanoseconds
        if self.last_time is not None and now < self.last_time:
            self.frames.clear()
            self.current = None
        self.last_time = now
        if self.current is None:
            self.pub.publish(self.invalid('no_target'))
        elif not 0 <= (now - stamp_ns(self.current.header)) * 1e-9 <= self.settings['input_timeout']:
            self.pub.publish(self.invalid('stale_target', self.current))


def main():
    run(Gripper)
