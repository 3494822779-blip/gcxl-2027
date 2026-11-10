"""AlignmentCommand + GripperAngles -> serial_tx command frames; serial_rx feedback -> camera/joint_states.

Wire format is specified in docs/serial_protocol.md (keep both in sync).
  host -> MCU: $CMD,<seq>,<lw>,<rw>,<lg>,<rg>,<cp_deg>*<XOR>
  MCU -> host: $FB,<seq>,<pitch_deg>*<XOR>
With camera_servo off (fixed camera) cp is the constant camera_fixed_deg and $FB is not needed.
"""
import copy
import json
import math
import time
from rclpy.clock import Clock, ClockType
from rclpy.qos import qos_profile_sensor_data, QoSProfile, DurabilityPolicy
from rcl_interfaces.msg import ParameterDescriptor
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool, String
from std_srvs.srv import Trigger
from block_interfaces.msg import AlignmentCommand, GripperAngles
from .nodes import ConfigNode, run, seconds
from .sweep import Sweep
from .action_api import ActionAPI, TARGET_OPERATIONS
from .task_memory import TaskMemory


def checksum(body):
    value = 0
    for ch in body.encode():
        value ^= ch
    return f'{value:02X}'


def encode_command(seq, lw, rw, lg, rg, cp):
    if not all(math.isfinite(v) for v in (lw, rw, lg, rg, cp)):
        raise ValueError('nonfinite serial command')
    body = f'CMD,{seq},{lw:.3f},{rw:.3f},{lg:.1f},{rg:.1f},{cp:.1f}'
    return f'${body}*{checksum(body)}'


def decode_feedback(line):
    """Return (seq, pitch_deg) for a valid $FB line, else None."""
    line = line.strip()
    if not line.startswith('$') or '*' not in line:
        return None
    body, _, tail = line[1:].partition('*')
    if tail.upper() != checksum(body):
        return None
    fields = body.split(',')
    if len(fields) != 3 or fields[0] != 'FB':
        return None
    try:
        seq, pitch = int(fields[1]), float(fields[2])
    except ValueError:
        return None
    return (seq, pitch) if 0 <= seq < 65536 and math.isfinite(pitch) else None


def clamp(v, limit):
    if not math.isfinite(v):
        raise ValueError('nonfinite control value')
    return max(-limit, min(limit, v))


def wheel_speeds(forward, yaw, track_width, limit, min_turn=0.):
    """Differential drive: forward in m/s, yaw in rad/s (positive = left wheel slower).

    A non-zero yaw is never weaker than min_turn m/s per wheel, so a small error still
    produces a wheel difference the motors can act on.
    """
    half = yaw * track_width / 2
    if yaw and abs(half) < min_turn:
        half = math.copysign(min_turn, half)
    return clamp(forward - half, limit), clamp(forward + half, limit)


class Actuator(ConfigNode):
    def __init__(self):
        super().__init__('actuator')
        self.configure(dict(camera_servo=False, camera_fixed_deg=0.,
                            track_width=0.2, wheel_speed_scale=1000., max_wheel_speed=0.5, min_turn_speed=0.1,
                            cam_min_deg=0., cam_max_deg=90., search_pitch_speed_deg=20.,
                            grasp_pitch_deg=40., grasp_pitch_tol=math.radians(1.7), claw_open_pitch_deg=20.,
                            claw_init_deg=0., claw_max_deg=90., claw_min_deg=-30.,
                            sweep_claw_deg=-30., sweep_overlap=0.5, sweep_weight=0.2,
                            sweep_near_multiplier=2., sweep_max_yaw_rate=1.,
                            sweep_tau_sec=0.33, sweep_duration_sec=1.,
                            max_yaw_rate=2., autonomous_target_class=-1,
                            left_claw_close_deg=0., right_claw_close_deg=5.,
                            pitch_home=0.0, pitch_home_tol=0.03,
                            discard_enabled=False, discard_distance_m=0.2,
                            discard_speed_mps=0.1, discard_turn_speed_mps=0.17,
                            claw_settle_sec=0.5, claw_deadband_deg=0., pitch_timeout=0.25, pitch_joint='camera_pitch',
                            input_timeout=0.25, serial_health_timeout=0.25,
                            pitch_feedback_margin_deg=2., feedback_reconnect_sec=1.,
                            max_sweep_attempts=2, max_sweeps=8, max_pitch_rate=0.6,
                            max_tracking_motion_sec=10.))
        self.declare_parameter('require_serial_health', False, ParameterDescriptor(read_only=True))
        self.require_serial_health = self.get_parameter('require_serial_health').value
        self.safety_reason = ''
        self.serial_healthy = False
        self.serial_health_time = None
        self.cmd_received = self.grip_received = self.pitch_received = None
        self.cmd_stamp = None
        self.last_fb_seq = self.last_fb_time = None
        self.declare_parameter('task_mode', 'normal', ParameterDescriptor(read_only=True))
        self.task_mode = self.get_parameter('task_mode').value
        self.action = None
        self.resume_after = 0.
        self.finished_pub = self.create_publisher(AlignmentCommand, 'task/grasp_finished', 10)
        self.cmd = None
        self.grip = None
        self.pitch = None
        self.pitch_time = None
        self.declare_parameter('autonomous', False, ParameterDescriptor(read_only=True))
        self.state = 'hold'
        self.reset_sweep()
        self.claw = self.settings['claw_init_deg']
        self.close_time = None
        self.cam_target = None      # absolute camera pitch command, degrees
        self.cam_time = None
        self.seq = 0
        self.rx_buf = ''
        self.pub = self.create_publisher(String, 'serial_tx', 10)
        self.last_safe_servos = (self.left_claw, self.right_claw,
            max(self.settings['cam_min_deg'], min(self.settings['cam_max_deg'], self.settings['camera_fixed_deg'])))
        self.safety_pub = self.create_publisher(String, 'robot/safety',
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
        self.safety_pub.publish(String(data='ready'))
        self.create_service(Trigger, 'robot/emergency_stop', self.emergency_stop)
        self.create_service(Trigger, 'robot/reset_safety', self.reset_safety)
        self.create_subscription(Bool, 'serial/healthy', self.link_health,
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
        self.joint_pub = self.create_publisher(JointState, 'camera/joint_states', qos_profile_sensor_data)
        self.create_subscription(Bool, 'actuator/discard', self.request_discard, 10)
        self.create_subscription(String, 'serial_rx', self.serial_rx, 10)
        self.create_subscription(AlignmentCommand, 'alignment/command', self.command, 10)
        self.create_subscription(GripperAngles, 'gripper/angles', self.angles, 10)
        self.create_subscription(JointState, 'camera/joint_states', self.joints, qos_profile_sensor_data)
        self.memory = TaskMemory()
        self.memory_pub = self.create_publisher(String, 'robot/memory',
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
        self.memory_service = self.create_service(Trigger, 'robot/memory/get', self.get_memory)
        self.memory_revision = -1
        self.publish_memory()
        self.action_api = ActionAPI(self)
        self.control_clock = Clock(clock_type=ClockType.STEADY_TIME)
        self.create_timer(0.05, self.tick, clock=self.control_clock)
        self.auto_timer = None
        if self.get_parameter('autonomous').value:
            self.auto_timer = self.create_timer(0.1, self.start_autonomous, clock=self.control_clock)

    @property
    def claw(self):
        """Legacy common-angle accessor; grasp and hold use the independent targets."""
        return self.left_claw

    @claw.setter
    def claw(self, angle):
        self.left_claw = self.right_claw = angle

    def publish_memory(self):
        if self.memory_revision != self.memory.revision:
            self.memory_pub.publish(String(data=json.dumps(self.memory.decision_context(), ensure_ascii=False)))
            self.memory_revision = self.memory.revision

    def get_memory(self, request, response):
        response.success = True
        response.message = json.dumps(self.memory.snapshot(), ensure_ascii=False)
        return response

    def link_ready(self):
        return (not self.require_serial_health or
                (self.serial_healthy and self.serial_health_time is not None and
                 0 <= time.monotonic() - self.serial_health_time <= self.settings['serial_health_timeout']))

    def link_health(self, msg):
        self.serial_healthy, self.serial_health_time = msg.data, time.monotonic()
        if self.require_serial_health and not msg.data and self.action_api.reserved:
            self.trip_safety('serial_link_lost')

    def trip_safety(self, reason):
        if not self.safety_reason:
            self.safety_reason = reason
            self.safety_pub.publish(String(data=reason))
            self.get_logger().error(f'Safety stop: {reason}')
        self.action_api.interrupt(reason)

    def emergency_stop(self, request, response):
        self.trip_safety('emergency_stop')
        response.success, response.message = True, 'stopped; explicit reset required'
        return response

    def reset_safety(self, request, response):
        if self.action_api.reserved or not self.link_ready():
            response.success, response.message = False, 'task still ending or serial link unavailable'
            return response
        self.safety_reason = ''
        self.action_api.hold()
        self.cmd_stamp = None
        self.safety_pub.publish(String(data='ready'))
        response.success, response.message = True, 'ready; submit a new task to move'
        return response

    def send_stop(self):
        # Do not apply a pending claw/camera target for the first time in a stop frame.
        servos = self.last_safe_servos
        self.left_claw, self.right_claw, self.cam_target = servos
        self.pub.publish(String(data=encode_command(self.seq, 0., 0., *servos)))
        self.seq = (self.seq + 1) % 65536

    def pitch_fresh(self, now):
        return (self.pitch is not None and self.pitch_time is not None and
                0 <= now - self.pitch_time <= self.settings['pitch_timeout'] and
                self.pitch_received is not None and
                0 <= time.monotonic() - self.pitch_received <= self.settings['pitch_timeout'])

    def close_target_ready(self, now):
        # Manual grasp has no target. A target task must recheck immediately before closure.
        if self.action is None:
            return True
        c, a = self.cmd, self.action
        return (self.fresh(c, now) and c.valid and not c.predicted and c.grasp_request and
                c.track_id == a.track_id and c.class_id == a.class_id and
                c.first_grasp_id == a.first_grasp_id and not c.sweep_side)

    def begin_grasp(self, target=None):
        self.action = copy.deepcopy(target)
        self.grasp_close = (self.settings['left_claw_close_deg'], self.settings['right_claw_close_deg'])
        self.state, self.close_time = 'grasp', None
        self.memory.begin_grasp(target, *self.grasp_close)
        self.publish_memory()
        self.dbg.event('state', state='grasp')

    def start_autonomous(self):
        if not self.link_ready():
            return
        self.auto_timer.cancel()
        if not self.action_api.reserved:
            cls = self.settings['autonomous_target_class']
            if cls < 0:
                self.get_logger().error('autonomous requires autonomous_target_class; staying stopped')
                return
            self.action_api.submit('target_grasp', has_target_class=True, target_class_id=cls)

    def command(self, msg):
        if not self.action_api.allows_command(msg):
            self.dbg.event('command_rejected', every=0.5, reason='task_or_class_mismatch',
                           track_id=msg.track_id, task_id=msg.first_grasp_id, class_id=msg.class_id)
            return
        now = self.get_clock().now().nanoseconds * 1e-9
        if seconds(msg.header.stamp) <= self.resume_after:
            return
        if self.safety_reason:
            return
        if (not all(math.isfinite(v) for v in (msg.yaw_rate, msg.pitch_rate, msg.forward_speed,
                msg.error_x, msg.error_y, msg.valid_for_sec, msg.obstacle_overlap)) or
                msg.valid_for_sec <= 0 or not 0 <= msg.obstacle_overlap <= 1 or
                msg.sweep_side not in (-1, 0, 1) or
                abs(msg.forward_speed) > self.settings['max_wheel_speed'] or
                abs(msg.yaw_rate) > self.settings['max_yaw_rate'] or
                abs(msg.pitch_rate) > self.settings['max_pitch_rate']):
            self.trip_safety('invalid_alignment_command')
            return
        stamp = seconds(msg.header.stamp)
        if self.cmd_stamp is not None and stamp <= self.cmd_stamp:
            return
        self.cmd_stamp, self.cmd_received = stamp, time.monotonic()
        self.cmd = msg
        if self.state != 'track' or not msg.valid or msg.predicted or not self.fresh(msg, now):
            return
        if not self.action_api.allows_grasp():
            return
        if self.formal_track_id is None:
            self.formal_track_id = msg.track_id
        if msg.track_id != self.formal_track_id:
            # Within one task the target node only changes ID by rebinding the same block
            # (same class, IoU with last box), so follow it instead of losing the target.
            self.dbg.event('track_rebound', old=self.formal_track_id, new=msg.track_id)
            if self.opening_track_id == self.formal_track_id:
                self.opening_track_id = msg.track_id
            self.formal_track_id = msg.track_id
        recovery_done = (self.sweep is None or self.sweep.done and self.sweep_recovered_at is not None and
                         time.monotonic() - self.sweep_recovered_at >= self.settings['claw_settle_sec'] and
                         stamp > self.sweep_recovered_stamp)
        if recovery_done and msg.sweep_side in (-1, 1):
            attempts = self.sweep_attempts.get(msg.obstacle_id, 0)
            if attempts >= self.settings['max_sweep_attempts'] or self.sweep_count >= self.settings['max_sweeps']:
                self.trip_safety('sweep_no_progress')
                return
            self.sweep_attempts[msg.obstacle_id] = attempts + 1
            self.sweep_count += 1
            self.sweep_obstacle_id = msg.obstacle_id
            self.sweep_started_at = time.monotonic()
            self.sweep_recovered_at = None
            self.sweep_restore_angles = (self.left_claw, self.right_claw)
            self.sweep = Sweep(msg.sweep_side, msg.obstacle_overlap, msg.obstacle_near, self.settings)
            if msg.sweep_side < 0:
                self.left_claw = self.sweep.angle
            else:
                self.right_claw = self.sweep.angle
            self.opened_at = None
            self.memory.begin_sweep(msg.track_id, msg.class_id, msg.obstacle_id, msg.sweep_side)
            self.publish_memory()
        if self.search_direction:
            self.dbg.event('target_search', phase='reacquired', track_id=msg.track_id)
            self.search_direction = 0
        reason = self.grasp_blocker(now)
        if reason:
            self.dbg.event('grasp_wait', every=0.5, reason=reason, track_id=msg.track_id,
                           request=msg.grasp_request,
                           grip_reason=self.grip.reason if self.grip is not None else 'no_gripper',
                           left_deg=self.left_claw, right_deg=self.right_claw)
            return
        self.begin_grasp(msg)

    def grasp_blocker(self, now):
        """Report actual executor admission, rather than the last visual readiness label."""
        msg, g, p = self.cmd, self.grip, self.settings
        if self.search_completed and (msg is None or not self.fresh(msg, now) or not msg.valid or msg.predicted):
            return 'search_exhausted'
        if self.search_direction:
            return 'searching_down' if self.search_direction > 0 else 'searching_up'
        if msg is None:
            return 'waiting_for_target'
        if not self.fresh(msg, now) or not msg.valid:
            return 'stale_or_invalid_target'
        if msg.predicted:
            return 'waiting_for_locked_target'
        if self.sweep is not None and not self.sweep.done:
            return 'sweeping'
        if (self.sweep is not None and self.sweep.done and
                seconds(msg.header.stamp) <= self.sweep_recovered_stamp):
            return 'waiting_for_post_sweep_observation'
        if msg.sweep_side:
            return 'waiting_for_sweep'
        if not msg.grasp_request:
            return msg.reason or 'aligning'
        if self.opening_track_id != msg.track_id or self.opening_angles is None:
            if p['camera_servo'] and (not self.pitch_fresh(now) or
                    self.pitch < math.radians(p['claw_open_pitch_deg'])):
                return 'waiting_for_open_pitch'
            if not self.fresh(g, now) or not g.angles_ready or g.track_id != msg.track_id:
                return 'waiting_for_gripper_angles'
            return 'waiting_for_opening'
        if self.opened_at is None:
            return 'waiting_for_opening'
        if time.monotonic() - self.opened_at < p['claw_settle_sec']:
            return 'opening'
        return ''

    def reset_sweep(self):
        self.sweep = None
        self.last_swept_obstacle_id = 0
        self.sweep_last_time = None
        self.search_direction = 0
        self.search_completed = False
        self.formal_track_id = None
        self.opened_at = None
        self.opening_angles = None
        self.opening_track_id = None
        self.sweep_attempts = {}
        self.sweep_count = 0
        self.sweep_obstacle_id = None
        self.sweep_started_at = self.sweep_recovered_at = None
        self.sweep_recovered_stamp = None
        self.sweep_restore_angles = None
        self.tracking_motion = 0.
        self.motion_time = None
        self.motion_active = False

    def finish_first(self, now):
        if (self.action_api.goal is not None and
                self.action_api.goal.operation in TARGET_OPERATIONS and self.action is not None and
                self.action.first_grasp_id == self.action_api.first_id and not self.action.clearance):
            self.action_api.completed = True
        if self.action is None or not self.action.first_grasp_id:
            return
        result = copy.deepcopy(self.action)
        result.header.stamp = self.get_clock().now().to_msg()
        self.finished_pub.publish(result)
        self.resume_after = max(now, seconds(result.header.stamp))
        self.cmd = self.grip = None
        if result.clearance:
            self.state = 'track'
            self.close_time = None

    def angles(self, msg):
        if seconds(msg.header.stamp) <= self.resume_after:
            return
        now = self.get_clock().now().nanoseconds * 1e-9
        # Frame and target topics arrive independently. A temporary matching status
        # is not a negative observation; retain the last same-target proposal only
        # for its original TTL. Actual invalid bounds/lost targets still invalidate it.
        if (not msg.angles_ready and msg.reason == 'waiting_for_matching_frame' and
                self.fresh(self.grip, now) and self.grip.angles_ready and
                self.grip.track_id == msg.track_id):
            return
        if self.grip is None or seconds(msg.header.stamp) > seconds(self.grip.header.stamp):
            self.grip_received = time.monotonic()
        self.grip = msg

    def joints(self, msg):
        try:
            value = msg.position[msg.name.index(self.settings['pitch_joint'])]
        except (ValueError, IndexError):
            return
        stamp = seconds(msg.header.stamp)
        now = self.get_clock().now().nanoseconds * 1e-9
        p = self.settings
        deg = math.degrees(value)
        if (math.isfinite(value) and
                p['cam_min_deg'] - p['pitch_feedback_margin_deg'] <= deg <= p['cam_max_deg'] + p['pitch_feedback_margin_deg'] and
                0 <= now - stamp <= p['pitch_timeout'] and
                (self.pitch_time is None or stamp > self.pitch_time)):
            self.pitch, self.pitch_time, self.pitch_received = value, stamp, time.monotonic()

    def serial_rx(self, msg):
        # serial_node forwards raw chunks; reassemble complete lines before parsing.
        self.rx_buf = (self.rx_buf + msg.data)[-512:]
        *lines, self.rx_buf = self.rx_buf.split('\n')
        for line in lines:
            fb = decode_feedback(line)
            if fb is None:
                if line.strip():
                    self.dbg.event('rx_bad_frame', every=1., line=line.strip()[:80])
                continue
            steady = time.monotonic()
            p = self.settings
            if not p['cam_min_deg'] - p['pitch_feedback_margin_deg'] <= fb[1] <= p['cam_max_deg'] + p['pitch_feedback_margin_deg']:
                continue
            if self.last_fb_seq is None or steady - self.last_fb_time >= p['feedback_reconnect_sec']:
                # Establish a baseline only. A second progressing frame is needed
                # before feedback may authorize motion or closure after reconnect.
                self.last_fb_seq, self.last_fb_time = fb[0], steady
                continue
            if not 0 < (fb[0] - self.last_fb_seq) % 65536 < 32768:
                continue
            self.last_fb_seq, self.last_fb_time = fb[0], steady
            self.dbg.event('rx_feedback', every=0.5, seq=fb[0], pitch_deg=fb[1])
            joint = JointState(name=[self.settings['pitch_joint']], position=[math.radians(fb[1])])
            joint.header.stamp = self.get_clock().now().to_msg()
            self.joints(joint)
            self.joint_pub.publish(joint)

    def fresh(self, msg, now):
        if msg is None or not math.isfinite(msg.valid_for_sec) or msg.valid_for_sec <= 0:
            return False
        ttl = min(msg.valid_for_sec, self.settings['input_timeout'])
        received = self.cmd_received if msg is self.cmd else self.grip_received if msg is self.grip else None
        return (0 <= now - seconds(msg.header.stamp) <= ttl and
                (received is None or 0 <= time.monotonic() - received <= ttl))

    def request_discard(self, msg):
        # Legacy topic is an Action client, with the same arbitration and timeout.
        if msg.data:
            self.action_api.submit('discard')

    def compute(self, now):
        """Return (left_wheel, right_wheel, left_claw_deg, right_claw_deg, camera_angle_deg).

        camera_servo on : the camera servo takes an absolute angle; the alignment pitch rate (rad/s)
                          is integrated into cam_target, clamped to [cam_min_deg, cam_max_deg].
        camera_servo off: the camera is fixed; cp is the constant camera_fixed_deg and a grasp
                          closes the claws immediately.
        """
        p = self.settings
        servo = p['camera_servo']
        lw = rw = 0.
        dt = 0. if self.cam_time is None else min(max(now - self.cam_time, 0.), 0.1)
        self.cam_time = now
        pitch_fresh = servo and self.pitch_fresh(now)
        if not servo:
            self.cam_target = p['camera_fixed_deg']
        elif self.cam_target is None:   # start from where the camera really is, else from home
            self.cam_target = math.degrees(self.pitch if pitch_fresh else p['pitch_home'])
        if self.safety_reason:
            self.state = 'hold'
        if self.state == 'track':
            c = self.cmd
            steady = time.monotonic()
            step = 0. if self.sweep_last_time is None else min(max(steady - self.sweep_last_time, 0.), 0.1)
            self.sweep_last_time = steady
            valid = self.fresh(c, now) and c.valid and (not c.first_grasp_id or not c.predicted)
            sweeping = self.sweep is not None and not self.sweep.done
            if self.action_api.allows_grasp() and self.formal_track_id is not None:
                valid = valid and c.track_id == self.formal_track_id
            if servo and not pitch_fresh:
                valid = False
            if not valid and servo and pitch_fresh and self.action_api.allows_grasp() and self.formal_track_id is not None and not self.search_completed:
                if not self.search_direction:
                    self.search_direction = 1
                    self.dbg.event('target_search', phase='searching_down', track_id=self.formal_track_id)
                self.cam_target += self.search_direction * p['search_pitch_speed_deg'] * dt
                limit = p['cam_max_deg'] if self.search_direction > 0 else p['cam_min_deg']
                if ((self.search_direction > 0 and self.cam_target >= limit) or
                        (self.search_direction < 0 and self.cam_target <= limit)):
                    self.cam_target = limit
                    if self.search_direction < 0:
                        self.search_direction = 0
                        self.search_completed = True
                    else:
                        self.search_direction = -1
                    self.dbg.event('target_search', phase='search_exhausted' if self.search_completed else 'searching_up',
                                   track_id=self.formal_track_id)
            else:
                if self.search_direction:
                    self.dbg.event('target_search', phase='reacquired', track_id=self.formal_track_id)
                self.search_direction = 0
            if valid:
                bias = 0.
                settling = False
                if sweeping:
                    settling = steady - self.sweep_started_at < p['claw_settle_sec']
                    if not settling:
                        self.sweep.advance(step)
                    bias = self.sweep.yaw()
                    if self.sweep.done:
                        self.last_swept_obstacle_id = self.sweep_obstacle_id
                        self.memory.complete_sweep()
                        self.left_claw, self.right_claw = self.sweep_restore_angles
                        self.sweep_recovered_at = steady
                        self.sweep_recovered_stamp = now
                        self.opened_at = steady if self.opening_angles is not None else None
                recovering = (self.sweep_recovered_at is not None and
                              steady - self.sweep_recovered_at < p['claw_settle_sec'])
                # Apply minimum turn to the visual correction only. The sweep bias must
                # decay continuously rather than being raised to minimum motor speed.
                half = c.yaw_rate * p['track_width'] / 2
                if half and abs(half) < p['min_turn_speed']:
                    half = math.copysign(p['min_turn_speed'], half)
                yaw = clamp(2 * half / p['track_width'] + bias, p['max_yaw_rate'])
                forward = 0. if c.grasp_request else c.forward_speed
                lw, rw = wheel_speeds(forward, yaw, p['track_width'], p['max_wheel_speed'])
                if settling or recovering or (not sweeping and c.sweep_side):
                    lw = rw = 0.
                if servo:
                    self.cam_target += math.degrees(c.pitch_rate) * dt
            # Capture width exactly once for this target. Subsequent jitter, missing
            # width proposals and pitch crossings do not change the saved opening.
            if valid and self.opening_track_id is not None and c.track_id != self.opening_track_id:
                self.opening_track_id = self.opening_angles = self.opened_at = None
            trigger = (pitch_fresh and self.pitch >= math.radians(p['claw_open_pitch_deg'])
                       if servo else valid and c.open_allowed)
            if valid and self.action_api.allows_grasp() and not c.predicted and not sweeping and self.opened_at is None:
                g = self.grip
                if (self.opening_angles is None and trigger and self.fresh(g, now) and
                        g.angles_ready and g.track_id == c.track_id and
                        all(math.isfinite(v) for v in (g.left_angle_rad, g.right_angle_rad))):
                    self.opening_angles = tuple(max(p['claw_init_deg'], min(p['claw_max_deg'],
                        math.degrees(angle))) for angle in (g.left_angle_rad, g.right_angle_rad))
                    self.opening_track_id = c.track_id
                if self.opening_angles is not None and self.opening_track_id == c.track_id:
                    # A later sweep may temporarily override one claw; restore the same
                    # saved opening afterwards without measuring width a second time.
                    self.memory.uncertain_hold('visual_gripper_open_once')
                    self.left_claw, self.right_claw = self.opening_angles
                    self.opened_at = steady
                    self.dbg.event('claw_open_once', track_id=c.track_id,
                                   left_deg=self.left_claw, right_deg=self.right_claw,
                                   trigger_pitch_deg=p['claw_open_pitch_deg'])
            if self.opened_at is not None and steady - self.opened_at < p['claw_settle_sec']:
                lw = rw = 0.
        elif self.state == 'grasp':
            # Wheels stopped. With a camera servo: phase 1 pitches the camera down at the block (claws
            # stay open), phase 2 closes the claws once the grasp angle is reached.
            reached = self.close_target_ready(now)
            if servo:
                self.cam_target = p['grasp_pitch_deg']
                reached = reached and pitch_fresh and self.pitch >= math.radians(p['grasp_pitch_deg']) - p['grasp_pitch_tol']
            # After closure has started, keep holding and finish on a monotonic timer;
            # a later visual or pitch dropout must not restart the closure dwell.
            if reached or self.close_time is not None:
                self.left_claw, self.right_claw = self.grasp_close
                self.memory.phase('claw_close')
                if self.close_time is None:
                    self.close_time = time.monotonic()
                if time.monotonic() - self.close_time >= p['claw_settle_sec']:
                    if servo:
                        self.cam_target = math.degrees(p['pitch_home'])
                    self.memory.complete_grasp()
                    self.state = 'done'
                    self.dbg.event('state', state='done')
                    self.finish_first(now)
        elif self.state == 'done' and servo:
            # Grasp finished; command the camera back to its initial (home) pitch.
            self.cam_target = math.degrees(p['pitch_home'])
        self.cam_target = max(p['cam_min_deg'], min(p['cam_max_deg'], self.cam_target))
        return lw * p['wheel_speed_scale'], rw * p['wheel_speed_scale'], self.left_claw, self.right_claw, self.cam_target

    def tick(self):
        wheels = self.action_api.before_tick()
        try:
            lw, rw, lg, rg, cp = self.compute(self.get_clock().now().nanoseconds * 1e-9)
            if wheels is not None and not self.safety_reason:
                lw, rw = (v * self.settings['wheel_speed_scale'] for v in wheels)
            p = self.settings
            steady = time.monotonic()
            if self.motion_active and self.motion_time is not None:
                self.tracking_motion += min(max(steady - self.motion_time, 0.), p['input_timeout'])
            self.motion_time = steady
            self.motion_active = self.state == 'track' and (lw != 0. or rw != 0.)
            if self.motion_active and self.tracking_motion >= p['max_tracking_motion_sec']:
                raise ValueError('tracking_motion_budget_exceeded')
            if (not all(math.isfinite(v) for v in (lw, rw, lg, rg, cp)) or
                    max(abs(lw), abs(rw)) > p['max_wheel_speed'] * p['wheel_speed_scale'] + 1e-9 or
                    not p['claw_min_deg'] <= lg <= p['claw_max_deg'] or
                    not p['claw_min_deg'] <= rg <= p['claw_max_deg'] or
                    not p['cam_min_deg'] <= cp <= p['cam_max_deg']):
                raise ValueError('invalid actuator output')
            frame = encode_command(self.seq, lw, rw, lg, rg, cp)
        except ValueError as exc:
            self.trip_safety(str(exc))
            return
        self.pub.publish(String(data=frame))
        self.last_safe_servos = (lg, rg, cp)
        self.dbg.event('tx_command', every=0.5, state=self.state, frame=frame, pitch=self.pitch)
        self.seq = (self.seq + 1) % 65536
        self.publish_memory()

    def on_stop(self, repeats=5, period=0.02):
        """Graceful exit: stop the wheels now instead of waiting for the MCU's 250 ms timeout.

        Claws and camera keep their last target (docs/serial_protocol.md section 6: they hold
        position on timeout, so they do here too). Several frames, because a single serial_tx
        message can be lost while the other nodes are shutting down.
        """
        if self.action_api.handle is not None:
            self.action_api.finish(False, 'actuator shutting down')
        for _ in range(repeats):
            self.send_stop()
            time.sleep(period)

    def destroy_node(self):
        self.action_api.destroy()
        return super().destroy_node()


def main():
    run(Actuator)
