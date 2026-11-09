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
    return (seq, pitch) if math.isfinite(pitch) else None


def clamp(v, limit):
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
                            cam_min_deg=0., cam_max_deg=90.,
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
                            claw_settle_sec=0.5, claw_deadband_deg=0., pitch_timeout=0.25, pitch_joint='camera_pitch'))
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

    def begin_grasp(self, target=None):
        self.action = copy.deepcopy(target)
        self.grasp_close = (self.settings['left_claw_close_deg'], self.settings['right_claw_close_deg'])
        self.state, self.close_time = 'grasp', None
        self.memory.begin_grasp(target, *self.grasp_close)
        self.publish_memory()
        self.dbg.event('state', state='grasp')

    def start_autonomous(self):
        self.auto_timer.cancel()
        if not self.action_api.reserved:
            cls = self.settings['autonomous_target_class']
            if cls < 0:
                self.get_logger().error('autonomous requires autonomous_target_class; staying stopped')
                return
            self.action_api.submit('target_grasp', has_target_class=True, target_class_id=cls)

    def command(self, msg):
        if not self.action_api.allows_command(msg):
            return
        now = self.get_clock().now().nanoseconds * 1e-9
        if seconds(msg.header.stamp) <= self.resume_after:
            return
        self.cmd = msg
        if self.state != 'track' or not msg.valid or msg.predicted or not self.fresh(msg, now):
            return
        if not self.action_api.allows_grasp():
            return
        if self.formal_track_id is None:
            self.formal_track_id = msg.track_id
        if msg.track_id != self.formal_track_id:
            self.cmd = None
            return
        if (self.sweep is None and msg.sweep_side in (-1, 1)
                and msg.obstacle_id != self.last_swept_obstacle_id):
            self.sweep = Sweep(msg.sweep_side, msg.obstacle_overlap, msg.obstacle_near, self.settings)
            if msg.sweep_side < 0:
                self.left_claw = self.sweep.angle
            else:
                self.right_claw = self.sweep.angle
            self.opened_at = None
            self.memory.begin_sweep(msg.track_id, msg.class_id, msg.obstacle_id, msg.sweep_side)
            self.publish_memory()
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
        if msg is None:
            return 'waiting_for_target'
        if not self.fresh(msg, now) or not msg.valid:
            return 'stale_or_invalid_target'
        if msg.predicted:
            return 'waiting_for_locked_target'
        if self.sweep is not None and not self.sweep.done:
            return 'sweeping'
        if not msg.grasp_request:
            return msg.reason or 'aligning'
        if self.opening_track_id != msg.track_id or self.opening_angles is None:
            if p['camera_servo'] and (self.pitch_time is None or
                    not 0 <= now - self.pitch_time <= p['pitch_timeout'] or
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
        self.formal_track_id = None
        self.opened_at = None
        self.opening_angles = None
        self.opening_track_id = None

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
        self.grip = msg

    def joints(self, msg):
        try:
            value = msg.position[msg.name.index(self.settings['pitch_joint'])]
        except (ValueError, IndexError):
            return
        if math.isfinite(value):
            self.pitch, self.pitch_time = value, self.get_clock().now().nanoseconds * 1e-9

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
            self.dbg.event('rx_feedback', every=0.5, seq=fb[0], pitch_deg=fb[1])
            joint = JointState(name=[self.settings['pitch_joint']], position=[math.radians(fb[1])])
            joint.header.stamp = self.get_clock().now().to_msg()
            self.joints(joint)
            self.joint_pub.publish(joint)

    def fresh(self, msg, now):
        return msg is not None and 0 <= now - seconds(msg.header.stamp) <= msg.valid_for_sec

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
        pitch_fresh = servo and self.pitch_time is not None and now - self.pitch_time <= p['pitch_timeout']
        if not servo:
            self.cam_target = p['camera_fixed_deg']
        elif self.cam_target is None:   # start from where the camera really is, else from home
            self.cam_target = math.degrees(self.pitch if pitch_fresh else p['pitch_home'])
        if self.state == 'track':
            c = self.cmd
            steady = time.monotonic()
            step = 0. if self.sweep_last_time is None else min(max(steady - self.sweep_last_time, 0.), 0.1)
            self.sweep_last_time = steady
            valid = self.fresh(c, now) and c.valid and (not c.first_grasp_id or not c.predicted)
            sweeping = self.sweep is not None and not self.sweep.done
            if valid:
                bias = 0.
                if sweeping:
                    self.sweep.advance(step)
                    bias = self.sweep.yaw()
                    if self.sweep.done:
                        self.last_swept_obstacle_id = c.obstacle_id
                        self.memory.complete_sweep()
                        self.opened_at = None
                # Apply minimum turn to the visual correction only. The sweep bias must
                # decay continuously rather than being raised to minimum motor speed.
                half = c.yaw_rate * p['track_width'] / 2
                if half and abs(half) < p['min_turn_speed']:
                    half = math.copysign(p['min_turn_speed'], half)
                yaw = clamp(2 * half / p['track_width'] + bias, p['max_yaw_rate'])
                forward = 0. if c.grasp_request else c.forward_speed
                lw, rw = wheel_speeds(forward, yaw, p['track_width'], p['max_wheel_speed'])
                if servo:
                    self.cam_target += math.degrees(c.pitch_rate) * dt
            # Capture width exactly once for this target. Subsequent jitter, missing
            # width proposals and pitch crossings do not change the saved opening.
            if valid and self.opening_track_id is not None and c.track_id != self.opening_track_id:
                self.opening_track_id = self.opening_angles = self.opened_at = None
            trigger = (pitch_fresh and self.pitch >= math.radians(p['claw_open_pitch_deg'])
                       if servo else valid and c.open_allowed)
            if valid and not c.predicted and not sweeping and self.opened_at is None:
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
        elif self.state == 'grasp':
            # Wheels stopped. With a camera servo: phase 1 pitches the camera down at the block (claws
            # stay open), phase 2 closes the claws once the grasp angle is reached.
            reached = True
            if servo:
                self.cam_target = p['grasp_pitch_deg']
                reached = pitch_fresh and self.pitch >= math.radians(p['grasp_pitch_deg']) - p['grasp_pitch_tol']
            if reached:
                self.left_claw, self.right_claw = self.grasp_close
                self.memory.phase('claw_close')
                if self.close_time is None:
                    self.close_time = now
                if now - self.close_time >= p['claw_settle_sec']:
                    if servo:
                        self.cam_target = math.degrees(p['pitch_home'])
                    self.memory.complete_grasp()
                    self.state = 'done'
                    self.dbg.event('state', state='done')
                    self.finish_first(now)
            else:
                self.close_time = None
        elif self.state == 'done' and servo:
            # Grasp finished; command the camera back to its initial (home) pitch.
            self.cam_target = math.degrees(p['pitch_home'])
        self.cam_target = max(p['cam_min_deg'], min(p['cam_max_deg'], self.cam_target))
        return lw * p['wheel_speed_scale'], rw * p['wheel_speed_scale'], self.left_claw, self.right_claw, self.cam_target

    def tick(self):
        wheels = self.action_api.before_tick()
        lw, rw, lg, rg, cp = self.compute(self.get_clock().now().nanoseconds * 1e-9)
        if wheels is not None:
            lw, rw = (v * self.settings['wheel_speed_scale'] for v in wheels)
        frame = encode_command(self.seq, lw, rw, lg, rg, cp)
        self.pub.publish(String(data=frame))
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
        cam = self.cam_target if self.cam_target is not None else self.settings['camera_fixed_deg']
        for _ in range(repeats):
            self.pub.publish(String(data=encode_command(self.seq, 0., 0., self.left_claw, self.right_claw, cam)))
            self.seq = (self.seq + 1) % 65536
            time.sleep(period)


def main():
    run(Actuator)
