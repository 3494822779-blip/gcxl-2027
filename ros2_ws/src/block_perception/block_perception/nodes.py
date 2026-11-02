import copy
import math
import signal
import threading
import rclpy
from rclpy.node import Node
from rclpy.executors import ExternalShutdownException, SingleThreadedExecutor
from rclpy.qos import qos_profile_sensor_data
from rcl_interfaces.msg import SetParametersResult
from std_msgs.msg import Bool
from sensor_msgs.msg import JointState
from block_interfaces.msg import Frame, Target, AlignmentCommand
from .core import Lock, CVFilter, rate, validate


def seconds(stamp):
    return stamp.sec + stamp.nanosec * 1e-9


class ConfigNode(Node):
    def configure(self, defaults):
        self.settings = {k: self.declare_parameter(k, v).value for k, v in defaults.items()}
        validate(self.settings)
        self.add_on_set_parameters_callback(self.update)

    def update(self, params):
        proposed = dict(self.settings)
        proposed.update({p.name: p.value for p in params if p.name in proposed})
        try:
            validate(proposed)
        except (ValueError, TypeError) as e:
            return SetParametersResult(successful=False, reason=str(e))
        self.settings = proposed
        return SetParametersResult(successful=True)


class Selector(Node):
    def __init__(self):
        super().__init__('selector')
        self.lock = Lock()
        self.last_stamp = None
        self.pub = self.create_publisher(Target, 'target/raw', 10)
        self.create_subscription(Frame, 'tracks', self.frame, qos_profile_sensor_data)
        self.create_subscription(Bool, 'target/reset', self.reset, 10)

    def reset(self, msg):
        if msg.data:
            self.lock = Lock()
            self.last_stamp = None
            # Explicit unlock immediately propagates through the filter and decision.
            self.pub.publish(Target(header=self.get_header()))

    def get_header(self):
        from std_msgs.msg import Header
        return Header(stamp=self.get_clock().now().to_msg())

    def frame(self, msg):
        stamp = seconds(msg.image.header.stamp)
        if self.last_stamp is not None and stamp <= self.last_stamp:
            return
        self.last_stamp = stamp
        b = self.lock.select(msg.boxes)
        out = Target(header=msg.image.header, image_width=msg.image.width,
                     image_height=msg.image.height, locked=self.lock.id is not None,
                     track_id=self.lock.id or 0, observed=b is not None, valid=b is not None)
        if b is not None:
            out.class_id = b.class_id
            out.raw_x = out.x = b.x + b.width / 2
            out.raw_y = out.y = b.y + b.height  # bottom edge, not centre
            out.last_observed = msg.image.header.stamp
        self.pub.publish(out)


class Filter(ConfigNode):
    def __init__(self):
        super().__init__('filter')
        self.configure(dict(process_noise=100., measurement_noise=16., initial_variance=100., prediction_timeout=0.4))
        self.kf = CVFilter()
        self.identity = None
        self.last = None
        self.pub = self.create_publisher(Target, 'target/filtered', 10)
        self.create_subscription(Target, 'target/raw', self.target, 10)

    def target(self, msg):
        if not msg.locked or self.identity != msg.track_id:
            self.kf = CVFilter()
            self.last = None
        self.identity = msg.track_id if msg.locked else None
        out = copy.deepcopy(msg)
        if msg.locked:
            t = seconds(msg.header.stamp)
            if self.kf.time is not None and t <= self.kf.time:
                return
            # Reacquisition after a long gap starts a fresh covariance/velocity estimate.
            if msg.observed and self.kf.last_seen is not None and t - self.kf.last_seen > self.settings['prediction_timeout']:
                self.kf = CVFilter()
            xy = self.kf.step(t, (msg.raw_x, msg.raw_y) if msg.observed else None,
                              self.settings['process_noise'], self.settings['measurement_noise'],
                              self.settings['initial_variance'])
            if msg.observed:
                self.last = copy.deepcopy(msg)
            elif self.last is not None:
                out.raw_x, out.raw_y = self.last.raw_x, self.last.raw_y
                out.class_id = self.last.class_id
                out.last_observed = self.last.last_observed
            out.valid = xy is not None and t - self.kf.last_seen <= self.settings['prediction_timeout']
            if xy is not None:
                out.x, out.y = map(float, xy)
        self.pub.publish(out)


class Alignment(ConfigNode):
    def __init__(self):
        super().__init__('alignment')
        self.configure(dict(deadzone_x=0.04, deadzone_y=0.2, bottom_target=0.9,
                            forward_speed=0.15, forward_max_error_x=0.3, yaw_gain=-0.5, pitch_gain=-0.5,
                            max_yaw_rate=0.3, max_pitch_rate=0.2, pitch_min=-0.8, pitch_max=-0.5,
                            pitch_timeout=0.25, input_timeout=0.25, hold_seconds=0.3,
                            pitch_joint='camera_pitch'))
        self.target_msg = None
        self.pitch = None
        self.pitch_time = None
        self.hold_start = None
        self.fired = False
        self.identity = None
        self.last_now = None
        self.pub = self.create_publisher(AlignmentCommand, 'alignment/command', 10)
        self.create_subscription(Target, 'target/filtered', self.target, 10)
        self.create_subscription(JointState, 'camera/joint_states', self.joints, qos_profile_sensor_data)
        self.create_timer(0.05, self.tick)

    def update(self, params):
        result = super().update(params)
        if result.successful:
            self.hold_start = None
        return result

    def target(self, msg):
        if self.target_msg is not None and msg.locked and seconds(msg.header.stamp) <= seconds(self.target_msg.header.stamp):
            return
        identity = msg.track_id if msg.locked else None
        if identity != self.identity:
            self.hold_start, self.fired = None, False
        self.identity = identity
        self.target_msg = msg
        if (not msg.valid or not msg.observed or msg.image_width == 0 or msg.image_height == 0
                or abs(msg.x - msg.image_width/2) > self.settings['deadzone_x'] * msg.image_width/2
                or abs(msg.y - self.settings['bottom_target'] * msg.image_height) > self.settings['deadzone_y'] * msg.image_height/2):
            self.hold_start = None

    def joints(self, msg):
        try:
            i = msg.name.index(self.settings['pitch_joint'])
            value = msg.position[i]
            stamp = seconds(msg.header.stamp)
            if math.isfinite(value) and (self.pitch_time is None or stamp > self.pitch_time):
                self.pitch, self.pitch_time = value, stamp
                if not self.settings['pitch_min'] <= value <= self.settings['pitch_max']:
                    self.hold_start = None
        except (ValueError, IndexError):
            pass

    def tick(self):
        now = self.get_clock().now()
        t = now.nanoseconds * 1e-9
        if self.last_now is not None and t < self.last_now:
            self.target_msg = None
            self.pitch_time = None
            self.hold_start = None
        self.last_now = t
        p, m = self.settings, self.target_msg
        out = AlignmentCommand(valid_for_sec=p['input_timeout'], reason='no_target')
        out.header.stamp = now.to_msg()
        fresh = m is not None and 0 <= t - seconds(m.header.stamp) <= p['input_timeout']
        if fresh and m.valid and m.locked and m.image_width > 0 and m.image_height > 0 and all(math.isfinite(v) for v in [m.x, m.y]):
            out.header.frame_id = m.header.frame_id
            out.track_id, out.valid, out.predicted = m.track_id, True, not m.observed
            out.error_x = (m.x - m.image_width/2) / (m.image_width/2)
            out.error_y = (m.y - p['bottom_target'] * m.image_height) / (m.image_height/2)
            out.yaw_rate = rate(out.error_x, p['deadzone_x'], p['yaw_gain'], p['max_yaw_rate'])
            out.pitch_rate = rate(out.error_y, p['deadzone_y'], p['pitch_gain'], p['max_pitch_rate'])
            out.aligned = abs(out.error_x) <= p['deadzone_x'] and abs(out.error_y) <= p['deadzone_y']
            pitch_fresh = self.pitch_time is not None and 0 <= t - self.pitch_time <= p['pitch_timeout']
            ready = out.aligned and m.observed and pitch_fresh and p['pitch_min'] <= self.pitch <= p['pitch_max']
            # Camera not yet down to the grasp band: drive forward (once roughly centred) so the
            # block's bottom edge drops in the image and the camera can keep tilting down.
            if (pitch_fresh and self.pitch > p['pitch_max'] and not self.fired
                    and abs(out.error_x) <= p['forward_max_error_x']):
                out.forward_speed = p['forward_speed']
            out.reason = 'aligning' if not out.aligned else 'aligned_waiting_pitch_or_hold'
            if ready:
                if self.hold_start is None:
                    self.hold_start = t
                if t - self.hold_start >= p['hold_seconds'] and not self.fired:
                    out.grasp_request, self.fired = True, True
            else:
                self.hold_start = None
            if self.fired:
                out.yaw_rate = out.pitch_rate = out.forward_speed = 0.
                out.reason = 'grasp_requested_waiting_reset'
        else:
            self.hold_start = None
            out.reason = 'stale_or_invalid_target' if m is not None else 'no_target'
        self.pub.publish(out)


class Display(Node):
    """Shows the tracked frame with boxes and target markers in an on-screen window.

    Double buffered: ROS callbacks render into the back buffer, then swap it with
    the front buffer. The GUI loop (main thread) shows the front buffer as soon as
    a new frame is swapped in, so drawing and window refresh never block each other.
    """
    def __init__(self):
        super().__init__('display')
        self.window = self.declare_parameter('window', 'block_perception').value
        self.frames = {}
        self.targets = {}
        self.buffers = [None, None]
        self.front = 0              # index of the last completed buffer
        self.showing = None         # index the GUI thread is currently showing
        self.fresh = False
        self.shown = False
        self.closing = False
        self.cond = threading.Condition()
        self.draw_ns = None
        self.fps = 0.
        self.create_subscription(Frame, 'tracks', self.frame, qos_profile_sensor_data)
        self.create_subscription(Target, 'target/filtered', self.target, 10)

    def frame(self, msg):
        key = (msg.image.header.stamp.sec, msg.image.header.stamp.nanosec)
        self.frames[key] = msg
        self.draw(key)
        while len(self.frames) > 30:
            self.frames.pop(next(iter(self.frames)))

    def target(self, msg):
        key = (msg.header.stamp.sec, msg.header.stamp.nanosec)
        self.targets[key] = msg
        self.draw(key)
        while len(self.targets) > 30:
            self.targets.pop(next(iter(self.targets)))

    def draw(self, key):
        # Exact timestamp join: never overlay a previous frame's target on a new image.
        if key not in self.frames or key not in self.targets:
            return
        import cv2, numpy as np
        frame, target = self.frames.pop(key), self.targets.pop(key)
        img = frame.image
        if img.width == 0 or img.height == 0 or not img.data:
            return
        with self.cond:
            back = 1 - self.front
            if self.showing == back:
                return  # GUI still holds the buffer we would overwrite; drop this frame
        channels = 1 if img.encoding in ('mono8', '8UC1') else 3
        src = np.frombuffer(img.data, dtype=np.uint8).reshape(img.height, img.width, channels)
        buf = self.buffers[back]
        if buf is None or buf.shape != (img.height, img.width, 3):
            buf = self.buffers[back] = np.empty((img.height, img.width, 3), dtype=np.uint8)
        if channels == 1:
            cv2.cvtColor(src, cv2.COLOR_GRAY2BGR, dst=buf)
        elif img.encoding == 'rgb8':
            cv2.cvtColor(src, cv2.COLOR_RGB2BGR, dst=buf)
        else:
            np.copyto(buf, src)
        green, yellow, cyan = (0, 255, 0), (0, 255, 255), (255, 255, 0)
        font = cv2.FONT_HERSHEY_SIMPLEX
        def pt(x, y):
            return int(round(x)), int(round(y))
        for b in frame.boxes:
            cv2.rectangle(buf, pt(b.x, b.y), pt(b.x + b.width, b.y + b.height), green, 2)
            cv2.putText(buf, f'class={b.class_id} id={b.track_id} p={b.confidence:.2f}',
                        pt(b.x, max(12., b.y - 6)), font, 0.5, green, 1, cv2.LINE_AA)
        if target.valid:
            if target.observed:
                cv2.circle(buf, pt(target.raw_x, target.raw_y), 5, yellow, -1)
            cv2.circle(buf, pt(target.x, target.y), 5, cyan, -1)
            cx, cy = img.width / 2, img.height / 2
            cv2.line(buf, pt(cx, cy), pt(target.x, target.y), cyan, 2)
            cv2.putText(buf, f'id={target.track_id} dx={target.x-cx:.1f}px dy={target.y-cy:.1f}px predicted={not target.observed}',
                        (8, 20), font, 0.6, green, 2, cv2.LINE_AA)
        now = self.get_clock().now().nanoseconds
        if self.draw_ns is not None and now > self.draw_ns:
            inst = 1e9 / (now - self.draw_ns)
            self.fps = 0.9 * self.fps + 0.1 * inst if self.fps else inst
        self.draw_ns = now
        cv2.putText(buf, f'{self.fps:.1f} fps', (8, img.height - 10), font, 0.6, green, 2, cv2.LINE_AA)
        with self.cond:
            self.front, self.fresh = back, True   # swap: finished back buffer becomes the front
            self.cond.notify()
        self.get_logger().info(f'display fps={self.fps:.1f}', throttle_duration_sec=5.)

    def loop(self):
        """GUI loop; must run in the main thread (OpenCV/Qt window calls)."""
        import cv2
        while rclpy.ok() and not self.closing:
            with self.cond:
                self.cond.wait_for(lambda: self.fresh or self.closing, timeout=0.05)
                index = None
                if self.fresh:
                    index, self.fresh = self.front, False
                    self.showing = index
            if index is None:
                if self.shown:
                    cv2.waitKey(1)   # keep the window responsive while idle
                continue
            try:
                cv2.imshow(self.window, self.buffers[index])
                cv2.waitKey(1)
                self.shown = True
            except cv2.error as exc:
                self.get_logger().error(f'Cannot open display window (is DISPLAY set?): {exc}')
                self.closing = True
            finally:
                with self.cond:
                    self.showing = None
        if self.shown:
            cv2.destroyAllWindows()


def run(cls):
    rclpy.init()
    node = cls()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        # launch can forward SIGINT again while node resources are being destroyed.
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


def selector_main(): run(Selector)
def filter_main(): run(Filter)
def alignment_main(): run(Alignment)
def display_main():
    rclpy.init()
    node = Display()
    executor = SingleThreadedExecutor()
    executor.add_node(node)
    spinner = threading.Thread(target=executor.spin, daemon=True)
    spinner.start()
    try:
        node.loop()
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        # launch can forward SIGINT again while node resources are being destroyed.
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        node.closing = True
        executor.shutdown()
        spinner.join(1.)
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


class Camera(Node):
    """Optional USB/file source. Hardware drivers can replace this node unchanged."""
    def __init__(self):
        super().__init__('camera')
        import cv2
        from rcl_interfaces.msg import ParameterDescriptor
        from sensor_msgs.msg import Image
        self.cv2, self.image_type = cv2, Image
        fixed = ParameterDescriptor(read_only=True)
        source = self.declare_parameter('source', '/dev/video0', fixed).value
        self.width = self.declare_parameter('width', 640, fixed).value
        self.height = self.declare_parameter('height', 480, fixed).value
        fps = self.declare_parameter('fps', 30., fixed).value
        self.frame_id = self.declare_parameter('frame_id', 'camera_optical_frame', fixed).value
        if self.width <= 0 or self.height <= 0 or not math.isfinite(fps) or fps <= 0:
            raise ValueError('Invalid camera dimensions/fps')
        # Match rune_aim's USB camera path: many UVC cameras expose a valid
        # MJPEG mode but produce a blank/invalid stream when OpenCV selects
        # the default raw format. Use an explicit GStreamer pipeline.
        # A pip-installed OpenCV in ~/.local may shadow the system one and lacks
        # GStreamer; fall back to V4L2 with MJPG forced (YUYV is only 9 fps at 720p).
        is_device = isinstance(source, str) and source.startswith('/dev/video')
        has_gst = 'GStreamer:                   YES' in cv2.getBuildInformation()
        if is_device and has_gst:
            pipeline = (f'v4l2src device={source} ! '
                        f'image/jpeg,width={self.width},height={self.height},framerate={int(fps)}/1 ! '
                        'jpegdec ! videoconvert ! video/x-raw,format=BGR ! '
                        'appsink drop=true max-buffers=1 sync=false')
            self.capture = cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)
        elif is_device:
            self.capture = cv2.VideoCapture(source, cv2.CAP_V4L2)
            self.capture.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
            self.capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
            self.capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
            self.capture.set(cv2.CAP_PROP_FPS, fps)
        else:
            self.capture = cv2.VideoCapture(source)
        if not self.capture.isOpened():
            raise RuntimeError(f'Cannot open camera/video source: {source}')
        self.pub = self.create_publisher(Image, 'camera/image_raw', qos_profile_sensor_data)
        self.timer = self.create_timer(1/fps, self.read)

    def read(self):
        ok, bgr = self.capture.read()
        if not ok:
            self.get_logger().error('Camera read failed or file ended; stopping image source')
            self.timer.cancel()
            return
        rgb = self.cv2.cvtColor(self.cv2.resize(bgr, (self.width, self.height)), self.cv2.COLOR_BGR2RGB)
        msg = self.image_type(height=self.height, width=self.width, encoding='rgb8', step=self.width*3,
                              data=rgb.tobytes())
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id
        self.pub.publish(msg)

    def destroy_node(self):
        self.capture.release()
        return super().destroy_node()


def camera_main(): run(Camera)
