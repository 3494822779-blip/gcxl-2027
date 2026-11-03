import copy
import math
import signal
import threading
import time
import rclpy
from rclpy.node import Node
from rclpy.executors import ExternalShutdownException, SingleThreadedExecutor
from rclpy.qos import qos_profile_sensor_data
from rclpy.signals import SignalHandlerOptions
from rcl_interfaces.msg import SetParametersResult
from std_msgs.msg import Bool
from sensor_msgs.msg import JointState
from block_interfaces.msg import Frame, Target, AlignmentCommand
from .core import Lock, CVFilter, rate, validate
from .debug_log import DebugLog


def seconds(stamp):
    return stamp.sec + stamp.nanosec * 1e-9


# Set by SIGINT/SIGTERM. rclpy's own handler would invalidate the ROS context first, and a node
# can then no longer publish; handling the signals here lets on_stop() still send (actuator: stop frames).
STOP = threading.Event()


def init_ros():
    rclpy.init(signal_handler_options=SignalHandlerOptions.NO)
    for sig in (signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, lambda *_: STOP.set())


def ignore_signals():
    # launch forwards SIGINT again while teardown runs, and rclpy.shutdown() restores the default
    # handler, which would kill the process halfway (exit code -2).
    for sig in (signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, signal.SIG_IGN)


class ConfigNode(Node):
    def configure(self, defaults):
        self.dbg = DebugLog(self)
        self.settings = {k: self.declare_parameter(k, v).value for k, v in defaults.items()}
        validate(self.settings)
        self.add_on_set_parameters_callback(self.update)

    def update(self, params):
        proposed = dict(self.settings)
        proposed.update({p.name: p.value for p in params if p.name in proposed})
        try:
            validate(proposed)
        except (ValueError, TypeError) as e:
            self.dbg.event('param_rejected', params={p.name: p.value for p in params}, reason=str(e))
            return SetParametersResult(successful=False, reason=str(e))
        changed = {k: v for k, v in proposed.items() if self.settings.get(k) != v}
        if changed:
            self.dbg.event('param_changed', **changed)
        self.settings = proposed
        return SetParametersResult(successful=True)


class Selector(ConfigNode):
    def __init__(self):
        super().__init__('selector')
        self.configure(dict(rebind_iou=0.3, rebind_timeout=3.0))
        self.lock = Lock(self.settings['rebind_iou'], self.settings['rebind_timeout'])
        self.last_stamp = None
        self.pub = self.create_publisher(Target, 'target/raw', 10)
        self.create_subscription(Frame, 'tracks', self.frame, qos_profile_sensor_data)
        self.create_subscription(Bool, 'target/reset', self.reset, 10)

    def update(self, params):
        result = super().update(params)
        if result.successful:
            self.lock.rebind_iou, self.lock.rebind_timeout = self.settings['rebind_iou'], self.settings['rebind_timeout']
        return result

    def reset(self, msg):
        if msg.data:
            self.lock = Lock(self.settings['rebind_iou'], self.settings['rebind_timeout'])
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
        prev_id = self.lock.id
        b = self.lock.select(msg.boxes, stamp)
        if self.lock.id != prev_id:
            self.dbg.event('lock_change', old=prev_id, new=self.lock.id, boxes=len(msg.boxes),
                           rebound=prev_id is not None)
        if self.lock.id is not None and b is None:
            # Locked ID is not in this frame: log what the tracker did output instead
            # [track_id, class_id, confidence, observed, width, height, bottom_y].
            self.dbg.event('lock_lost', every=0.3, lock_id=self.lock.id, n=len(msg.boxes),
                           boxes=[[x.track_id, x.class_id, round(x.confidence, 2), x.observed,
                                   round(x.width), round(x.height), round(x.y + x.height)] for x in msg.boxes[:6]])
        elif b is not None:
            self.dbg.event('lock_seen', every=1., lock_id=b.track_id, class_id=b.class_id,
                           confidence=round(b.confidence, 2), w=round(b.width), h=round(b.height),
                           bottom=round(b.y + b.height), n=len(msg.boxes))
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
    """Yaw centres the block horizontally. The vertical reference is the block's BOTTOM edge.

    camera_servo on : the camera pitch servo moves the bottom edge into the bottom band; the
                      chassis drives forward until the camera has pitched down to grasp_pitch.
    camera_servo off: fixed camera; the chassis drives forward until the bottom edge reaches
                      the bottom band. pitch_rate is 0 and no pitch feedback is needed.
    """
    def __init__(self):
        super().__init__('alignment')
        self.configure(dict(camera_servo=False,
                            deadzone_x=0.04, deadzone_y=0.2, bottom_target=0.9,
                            forward_speed=0.5, forward_max_error_x=0.3, grasp_error_x=0.15,
                            yaw_near_weight=0.2, yaw_center_weight=0.3, yaw_full_error_x=0.5,
                            yaw_gain=-1.0, pitch_gain=-0.5,
                            max_yaw_rate=2.0, max_pitch_rate=0.2, grasp_pitch=-0.26,
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

    def closeness(self, y, height):
        """0 = bottom edge at the top of the image (far), 1 = at the bottom target line (grasp spot)."""
        return max(0., min(1., y / (self.settings['bottom_target'] * height)))

    def x_deadzone(self, y, height):
        """Horizontal deadzone widens from deadzone_x (far) to grasp_error_x (close): late turns matter less."""
        p = self.settings
        return p['deadzone_x'] + max(0., p['grasp_error_x'] - p['deadzone_x']) * self.closeness(y, height)

    def target(self, msg):
        if self.target_msg is not None and msg.locked and seconds(msg.header.stamp) <= seconds(self.target_msg.header.stamp):
            return
        identity = msg.track_id if msg.locked else None
        if identity != self.identity:
            self.hold_start, self.fired = None, False
        self.identity = identity
        self.target_msg = msg
        if (not msg.valid or not msg.observed or msg.image_width == 0 or msg.image_height == 0
                or abs(msg.x - msg.image_width/2) > self.x_deadzone(msg.y, msg.image_height) * msg.image_width/2
                or abs(msg.y - self.settings['bottom_target'] * msg.image_height) > self.settings['deadzone_y'] * msg.image_height/2):
            self.hold_start = None

    def joints(self, msg):
        try:
            i = msg.name.index(self.settings['pitch_joint'])
            value = msg.position[i]
            stamp = seconds(msg.header.stamp)
            if math.isfinite(value) and (self.pitch_time is None or stamp > self.pitch_time):
                self.pitch, self.pitch_time = value, stamp
                if value > self.settings['grasp_pitch']:
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
            # Steering is weighted by distance: full gain while the block is far (turn early), fading
            # to yaw_near_weight as the bottom edge nears the target line, with a deadzone that widens
            # the same way. No hard switch, so there is no latch to chatter on.
            near = self.closeness(m.y, m.image_height)
            dead_x = self.x_deadzone(m.y, m.image_height)
            # A second weight from the offset ratio |error_x|: gentle near the centre, full gain once
            # the block is yaw_full_error_x or more off-centre.
            offset = max(p['yaw_center_weight'], min(1., abs(out.error_x) / p['yaw_full_error_x']))
            gain = p['yaw_gain'] * max(p['yaw_near_weight'], 1. - near) * offset
            out.yaw_rate = rate(out.error_x, dead_x, gain, p['max_yaw_rate'])
            out.aligned = abs(out.error_x) <= dead_x and abs(out.error_y) <= p['deadzone_y']
            centred = abs(out.error_x) <= p['forward_max_error_x'] and not self.fired
            out.reason = 'aligning'
            if p['camera_servo']:
                out.pitch_rate = rate(out.error_y, p['deadzone_y'], p['pitch_gain'], p['max_pitch_rate'])
                pitch_fresh = self.pitch_time is not None and 0 <= t - self.pitch_time <= p['pitch_timeout']
                ready = out.aligned and m.observed and pitch_fresh and self.pitch <= p['grasp_pitch']
                # Camera not yet down to the grasp angle: drive forward (once roughly centred) so the
                # block's bottom edge drops in the image and the camera can keep tilting down.
                if pitch_fresh and self.pitch > p['grasp_pitch'] and centred:
                    out.forward_speed = p['forward_speed']
                    out.reason = 'approaching'
                elif out.aligned:
                    out.reason = 'aligned_waiting_pitch_or_hold'
            else:
                ready = out.aligned and m.observed
                # Bottom edge above the band = block is far: drive forward once roughly centred.
                # Below the band = too close: stop (no reverse).
                if out.error_y < -p['deadzone_y'] and centred:
                    out.forward_speed = p['forward_speed']
                    out.reason = 'approaching'
                elif out.error_y > p['deadzone_y']:
                    out.reason = 'too_close'
                elif out.aligned:
                    out.reason = 'aligned_waiting_hold'
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
        while rclpy.ok() and not self.closing and not STOP.is_set():
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
    """Spin until SIGINT/SIGTERM, then stop in order: on_stop() hook, executor, node, context.

    A repeated signal during teardown only sets the (already set) flag, so it cannot cut teardown
    short. A crash in a callback still runs on_stop() before the exception propagates.
    """
    init_ros()
    node, executor = None, SingleThreadedExecutor()
    try:
        node = cls()
        executor.add_node(node)
        while rclpy.ok() and not STOP.is_set():
            executor.spin_once(timeout_sec=0.1)
    except ExternalShutdownException:
        pass
    finally:
        ignore_signals()
        STOP.set()
        if node is not None:
            hook = getattr(node, 'on_stop', None)
            if hook is not None:
                try:
                    hook()
                except Exception as exc:
                    node.get_logger().error(f'on_stop failed: {exc}')
            dbg = getattr(node, 'dbg', None)
            if dbg is not None:
                dbg.close()
            executor.shutdown()
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


def selector_main(): run(Selector)
def filter_main(): run(Filter)
def alignment_main(): run(Alignment)
def display_main():
    init_ros()
    node = Display()
    executor = SingleThreadedExecutor()
    executor.add_node(node)
    spinner = threading.Thread(target=executor.spin, daemon=True)
    spinner.start()
    try:
        node.loop()
    except ExternalShutdownException:
        pass
    finally:
        ignore_signals()
        STOP.set()
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
        self.source, self.fps = source, fps
        self.is_device = isinstance(source, str) and source.startswith('/dev/video')
        self.has_gst = 'GStreamer:                   YES' in cv2.getBuildInformation()
        self.capture = self.open_capture()
        if not self.capture.isOpened():
            raise RuntimeError(f'Cannot open camera/video source: {source}')
        self.pub = self.create_publisher(Image, 'camera/image_raw', qos_profile_sensor_data)
        # capture.read() can block for ~10 s when a UVC camera stalls (V4L2 select() timeout), so it
        # runs in its own thread: the ROS executor stays responsive and shutdown is never held up.
        self.stopping = threading.Event()
        self.reader = threading.Thread(target=self.read_loop, name='camera-reader', daemon=True)
        self.reader.start()

    def open_capture(self):
        cv2, source, fps = self.cv2, self.source, self.fps
        if self.is_device and self.has_gst:
            pipeline = (f'v4l2src device={source} ! '
                        f'image/jpeg,width={self.width},height={self.height},framerate={int(fps)}/1 ! '
                        'jpegdec ! videoconvert ! video/x-raw,format=BGR ! '
                        'appsink drop=true max-buffers=1 sync=false')
            return cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)
        if self.is_device:
            capture = cv2.VideoCapture(source, cv2.CAP_V4L2)
            capture.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
            capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
            capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
            capture.set(cv2.CAP_PROP_FPS, fps)
            return capture
        return cv2.VideoCapture(source)

    def read_loop(self):
        failures, last, period = 0, 0., 1. / self.fps
        while not self.stopping.is_set():
            ok, bgr = self.capture.read()
            if self.stopping.is_set():
                return
            if ok:
                failures = 0
                # Many UVC modes run faster than the configured fps (this one: ~95 fps at 640x480);
                # keep reading so the driver queue stays drained, but publish at fps only.
                now = time.monotonic()
                if now - last >= period * 0.9:
                    last = now
                    self.publish(bgr)
                continue
            if not self.is_device:
                self.get_logger().error('Video file ended; stopping image source')
                return
            # A USB camera that stalls or drops off the bus: reopen it, backing off up to 2 s.
            failures += 1
            self.get_logger().warning(f'Camera read failed ({failures}); reopening {self.source}')
            self.capture.release()
            self.stopping.wait(min(0.25 * failures, 2.))
            if not self.stopping.is_set():
                self.capture = self.open_capture()

    def publish(self, bgr):
        rgb = self.cv2.cvtColor(self.cv2.resize(bgr, (self.width, self.height)), self.cv2.COLOR_BGR2RGB)
        msg = self.image_type(height=self.height, width=self.width, encoding='rgb8', step=self.width*3,
                              data=rgb.tobytes())
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id
        self.pub.publish(msg)

    def on_stop(self):
        self.stopping.set()
        self.reader.join(1.5)
        # Releasing while read() is still blocked in the reader thread is unsafe; the OS closes the
        # device when the process exits.
        if not self.reader.is_alive():
            self.capture.release()
        else:
            self.get_logger().warning('Camera reader still blocked in read(); leaving it to process exit')


def camera_main(): run(Camera)
