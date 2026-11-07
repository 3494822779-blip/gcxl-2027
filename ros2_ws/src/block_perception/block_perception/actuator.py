"""AlignmentCommand + GripperAngles -> serial_tx command frames; serial_rx feedback -> camera/joint_states.

Wire format is specified in docs/serial_protocol.md (keep both in sync).
  host -> MCU: $CMD,<seq>,<lw>,<rw>,<lg>,<rg>,<cp_deg>*<XOR>
  MCU -> host: $FB,<seq>,<pitch_deg>*<XOR>
With camera_servo off (fixed camera) cp is the constant camera_fixed_deg and $FB is not needed.
"""
import copy
import math
import time
from rclpy.clock import Clock, ClockType
from rclpy.qos import qos_profile_sensor_data
from rcl_interfaces.msg import ParameterDescriptor
from sensor_msgs.msg import JointState
from std_msgs.msg import Bool, String
from block_interfaces.msg import AlignmentCommand, GripperAngles
from .nodes import ConfigNode, run, seconds
from .discard import DiscardTask
from .action_api import ActionAPI


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
                            cam_min_deg=-15., cam_max_deg=30.,
                            grasp_pitch_deg=-15., grasp_pitch_tol=0.03,
                            claw_init_deg=0., claw_max_deg=90., claw_close_deg=0.,
                            pitch_home=0.0, pitch_home_tol=0.03,
                            discard_enabled=True, discard_distance_m=0.2,
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
        self.declare_parameter('autonomous', True, ParameterDescriptor(read_only=True))
        self.state = 'hold'
        self.discard_task = None
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
        self.action_api = ActionAPI(self)
        self.control_clock = Clock(clock_type=ClockType.STEADY_TIME)
        self.create_timer(0.05, self.tick, clock=self.control_clock)
        self.auto_timer = None
        if self.get_parameter('autonomous').value and self.task_mode == 'normal':
            self.auto_timer = self.create_timer(0.1, self.start_autonomous, clock=self.control_clock)

    def start_autonomous(self):
        self.auto_timer.cancel()
        if not self.action_api.reserved:
            self.action_api.submit('approach_grasp', discard_after=self.settings['discard_enabled'])

    def command(self, msg):
        if not self.action_api.allows_command(msg):
            return
        now = self.get_clock().now().nanoseconds * 1e-9
        if seconds(msg.header.stamp) <= self.resume_after:
            return
        self.cmd = msg
        # 调试：记录所有收到的alignment命令
        self.dbg.event('alignment_cmd', every=0.5,
                       grasp_request=msg.grasp_request,
                       valid=msg.valid,
                       aligned=msg.aligned,
                       reason=msg.reason,
                       track_id=msg.track_id,
                       first_grasp_id=msg.first_grasp_id,
                       state=self.state)
        if (self.state == 'done' and msg.first_grasp_id and msg.valid and
                self.fresh(msg, now) and
                (self.action is None or self.action.first_grasp_id != msg.first_grasp_id)):
            self.state, self.close_time = 'track', None
        available = self.state == 'track'
        if msg.grasp_request and available and self.action_api.allows_grasp():
            if not msg.valid or msg.predicted or not self.fresh(msg, now):
                self.dbg.event('grasp_rejected', reason='not_valid_or_predicted_or_stale',
                               valid=msg.valid, predicted=msg.predicted, fresh=self.fresh(msg, now))
                return
            if msg.first_grasp_id:
                if not msg.clearance and msg.class_id != 0:
                    self.dbg.event('grasp_rejected', reason='non_clearance_requires_class_0',
                                   clearance=msg.clearance, class_id=msg.class_id)
                    return
                if msg.clearance and not self.settings['claw_init_deg'] <= 90. <= self.settings['claw_max_deg']:
                    self.dbg.event('grasp_rejected', reason='clearance_needs_90deg_support',
                                   claw_init=self.settings['claw_init_deg'],
                                   claw_max=self.settings['claw_max_deg'])
                    return
            self.action = copy.deepcopy(msg)
            self.state, self.close_time = 'grasp', None
            self.dbg.event('state', state='grasp')
        elif msg.grasp_request and not available:
            self.dbg.event('grasp_rejected', reason='state_not_track', state=self.state)

    def finish_first(self, now):
        if (self.action_api.goal is not None and
                self.action_api.goal.operation == 'first_grasp' and self.action is not None and
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
        if seconds(msg.header.stamp) > self.resume_after:
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

    def start_discard(self):
        self.discard_task = DiscardTask(self.settings, self.claw)
        self.state = 'discard'
        self.dbg.event('state', state='discard')

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
            if self.fresh(c, now) and c.valid:
                lw, rw = wheel_speeds(c.forward_speed, c.yaw_rate, p['track_width'], p['max_wheel_speed'],
                                      p['min_turn_speed'])
                if servo:
                    self.cam_target += math.degrees(c.pitch_rate) * dt
            g = self.grip
            if self.fresh(g, now) and g.angles_ready:
                target = max(p['claw_init_deg'], min(p['claw_max_deg'], math.degrees(g.left_angle_rad)))
                # Box width jitters by a few px every frame; ignore sub-deadband changes so the claw servo holds still.
                if abs(target - self.claw) > p['claw_deadband_deg']:
                    self.claw = target
        elif self.state == 'grasp':
            # Wheels stopped. With a camera servo: phase 1 pitches the camera down at the block (claws
            # stay open), phase 2 closes the claws once the grasp angle is reached.
            reached = True
            if servo:
                self.cam_target = p['grasp_pitch_deg']
                reached = pitch_fresh and abs(math.radians(p['grasp_pitch_deg']) - self.pitch) <= p['grasp_pitch_tol']
            if reached:
                self.claw = max(p['claw_init_deg'], p['claw_close_deg'])
                if self.close_time is None:
                    self.close_time = now
                if now - self.close_time >= p['claw_settle_sec']:
                    discard = (self.action.clearance if self.action is not None and
                               self.action.first_grasp_id else p['discard_enabled'])
                    goal = self.action_api.goal
                    if goal is not None and goal.operation in ('grasp', 'approach_grasp'):
                        discard = goal.discard_after
                    if discard:
                        self.start_discard()
                    else:
                        self.state = 'done'
                        self.dbg.event('state', state='done')
                        self.finish_first(now)
            else:
                self.close_time = None
        elif self.state == 'discard':
            if servo:
                self.cam_target = math.degrees(p['pitch_home'])
            lw, rw, self.claw = self.discard_task.step(now)
            if self.discard_task.done:
                self.state = 'done'
                self.dbg.event('state', state='done')
                self.finish_first(now)
        elif self.state == 'done' and servo:
            # Grasp finished; command the camera back to its initial (home) pitch.
            self.cam_target = math.degrees(p['pitch_home'])
        self.cam_target = max(p['cam_min_deg'], min(p['cam_max_deg'], self.cam_target))
        return lw * p['wheel_speed_scale'], rw * p['wheel_speed_scale'], self.claw, self.claw, self.cam_target

    def tick(self):
        wheels = self.action_api.before_tick()
        lw, rw, lg, rg, cp = self.compute(self.get_clock().now().nanoseconds * 1e-9)
        if wheels is not None:
            lw, rw = (v * self.settings['wheel_speed_scale'] for v in wheels)
        frame = encode_command(self.seq, lw, rw, lg, rg, cp)
        self.pub.publish(String(data=frame))
        self.dbg.event('tx_command', every=0.5, state=self.state, frame=frame, pitch=self.pitch)
        self.seq = (self.seq + 1) % 65536

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
            self.pub.publish(String(data=encode_command(self.seq, 0., 0., self.claw, self.claw, cam)))
            self.seq = (self.seq + 1) % 65536
            time.sleep(period)


def main():
    run(Actuator)
