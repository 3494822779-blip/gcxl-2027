"""Single-owner ROS Action API. Timer-driven execution never blocks ROS callbacks.

All actuator writes still pass through Actuator.tick and its serial encoder.
"""
import math
import time

from rclpy.action import ActionClient, ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.task import Future
from rclpy.qos import QoSProfile, DurabilityPolicy
from std_msgs.msg import Bool
from block_interfaces.action import RobotTask
from block_interfaces.msg import TargetTask
TARGET_OPERATIONS = {'target_grasp', 'approach_grasp', 'first_grasp'}


class ActionAPI:
    OPERATIONS = {'align', 'target_grasp', 'approach_grasp', 'grasp', 'first_grasp',
                  'move', 'turn', 'claw', 'camera', 'stop'}

    def __init__(self, node):
        self.node = node
        self.reserved = False
        self.handle = None
        self.future = None
        self.goal = None
        self.completed = False
        self.first_id = 0
        self.last_first_id = 0
        self.target_request = node.create_publisher(TargetTask, 'task/target_grasp',
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
        self.reset = node.create_publisher(Bool, 'target/reset', 10)
        self.client = ActionClient(node, RobotTask, 'robot/execute')
        self.server = ActionServer(node, RobotTask, 'robot/execute',
                                   execute_callback=self.execute,
                                   goal_callback=self.accept,
                                   cancel_callback=self.cancel,
                                   callback_group=ReentrantCallbackGroup())

    def submit(self, operation, **kwargs):
        """Compatibility and autonomous entry points also use real ROS Action goals."""
        future = self.client.send_goal_async(RobotTask.Goal(operation=operation, **kwargs))
        def accepted(response):
            handle = response.result()
            if not handle.accepted:
                self.node.get_logger().warning(f'{operation} action rejected (busy or invalid)')
                return
            def completed(response):
                result = response.result().result
                self.node.get_logger().info(f'{operation}: {result.message}')
            handle.get_result_async().add_done_callback(completed)
        future.add_done_callback(accepted)
        return future

    def validate(self, goal):
        p = self.node.settings
        if goal.operation not in self.OPERATIONS:
            raise ValueError('unknown operation')
        if not all(math.isfinite(v) for v in (goal.timeout_sec, goal.distance_m,
                                              goal.angle_deg, goal.speed_mps)):
            raise ValueError('goal values must be finite')
        if goal.timeout_sec < 0 or goal.speed_mps < 0:
            raise ValueError('timeout/speed must be nonnegative')
        if goal.discard_after:
            raise ValueError('discard_after is disabled')
        if goal.operation in TARGET_OPERATIONS and (not goal.has_target_class or goal.target_class_id < 0):
            raise ValueError('target grasp requires an explicit nonnegative target_class_id')
        if goal.operation == 'claw' and not p['claw_min_deg'] <= goal.angle_deg <= p['claw_max_deg']:
            raise ValueError('claw angle outside limits')
        if goal.operation == 'camera' and (not p['camera_servo'] or
                not p['cam_min_deg'] <= goal.angle_deg <= p['cam_max_deg']):
            raise ValueError('camera servo disabled or angle outside limits')

    def accept(self, goal):
        try:
            self.validate(goal)
        except ValueError as exc:
            self.node.get_logger().warning(f'Action rejected: {exc}')
            return GoalResponse.REJECT
        # Reserve during acceptance, not execute: two simultaneous goals cannot both win.
        if self.reserved or (self.node.state == 'grasp' and goal.operation != 'stop'):
            return GoalResponse.REJECT
        if self.node.auto_timer is not None:
            self.node.auto_timer.cancel()
        self.reserved = True
        return GoalResponse.ACCEPT

    def cancel(self, handle):
        return CancelResponse.ACCEPT

    async def execute(self, handle):
        self.handle, self.goal = handle, handle.request
        self.future = Future()
        self.started = time.monotonic()
        self.completed = False
        self.motion_started = None
        n, g = self.node, self.goal
        self.timeout = g.timeout_sec or (180. if g.operation in TARGET_OPERATIONS else 60.)
        self.first_id = 0
        self.hold()
        n.action = None
        n.close_time = None
        n.reset_sweep()
        if g.operation in TARGET_OPERATIONS:
            self.first_id = max(1, self.last_first_id + 1, n.get_clock().now().nanoseconds)
            self.last_first_id = self.first_id
        n.memory.begin_task(g.operation, self.first_id)
        if g.operation in TARGET_OPERATIONS:
            n.memory.task['target_class_id'] = g.target_class_id
        n.publish_memory()
        try:
            self.validate(g)
            if g.operation == 'align' or g.operation in TARGET_OPERATIONS:
                n.state = 'track'
                if g.operation in TARGET_OPERATIONS:
                    self.target_request.publish(TargetTask(task_id=self.first_id,
                        target_class_id=g.target_class_id, active=True))
                else:
                    self.reset.publish(Bool(data=True))
            elif g.operation == 'grasp':
                n.begin_grasp()
            elif g.operation in ('move', 'turn'):
                speed = g.speed_mps or n.settings['discard_speed_mps']
                if g.operation == 'turn':
                    speed = max(g.speed_mps or n.settings['discard_turn_speed_mps'],
                                n.settings['min_turn_speed'])
                self.speed = min(speed, n.settings['max_wheel_speed'])
                self.duration = (abs(g.distance_m) / self.speed if g.operation == 'move' else
                                 abs(math.radians(g.angle_deg)) * n.settings['track_width'] / (2 * self.speed))
            elif g.operation == 'claw':
                if (n.left_claw, n.right_claw) != (g.angle_deg, g.angle_deg):
                    n.memory.uncertain_hold('manual_claw_command')
                n.claw = g.angle_deg
            elif g.operation == 'camera':
                n.cam_target = g.angle_deg
        except ValueError as exc:
            self.finish(False, str(exc))
        result = await self.future
        self.handle = self.goal = self.future = None
        self.reserved = False
        return result

    def hold(self):
        n = self.node
        n.state = 'hold'
        n.cmd = n.grip = None
        n.resume_after = n.get_clock().now().nanoseconds * 1e-9

    def finish(self, success, message, canceled=False, status=None):
        if self.future is None or self.future.done():
            return
        n = self.node
        track = n.action.track_id if n.action is not None else (n.cmd.track_id if n.cmd else 0)
        phase = self.current_phase()
        n.memory.finish_task(status or ('completed' if success else 'canceled' if canceled else 'aborted'),
                             phase, message)
        n.publish_memory()
        self.hold()
        if success and n.settings['camera_servo'] and (self.goal.operation == 'grasp' or self.goal.operation in TARGET_OPERATIONS):
            n.cam_target = math.degrees(n.settings['pitch_home'])
        if self.goal.operation in TARGET_OPERATIONS:
            self.target_request.publish(TargetTask(task_id=self.first_id, active=False))
        if canceled:
            self.handle.canceled()
        elif success:
            self.handle.succeed()
        else:
            self.handle.abort()
        self.future.set_result(RobotTask.Result(success=success, message=message, track_id=track))

    def current_phase(self):
        n = self.node
        if n.state == 'grasp':
            return 'claw_close' if n.close_time is not None else 'grasp_pitch'
        if n.state == 'track':
            now = n.get_clock().now().nanoseconds * 1e-9
            if self.goal is not None and self.goal.operation in TARGET_OPERATIONS:
                return n.grasp_blocker(now) or 'grasp_ready'
            if n.cmd is not None and (not n.fresh(n.cmd, now) or not n.cmd.valid):
                return 'stale_or_invalid_target'
            return (n.cmd.reason if n.cmd is not None else '') or 'waiting_for_target'
        if n.state == 'hold' and self.goal is not None:
            return self.goal.operation
        return n.state

    def before_tick(self):
        """Returns an optional direct wheel command in m/s; other phases use compute()."""
        if self.handle is None or self.future.done():
            return None
        n, g = self.node, self.goal
        elapsed = time.monotonic() - self.started
        if self.handle.is_cancel_requested:
            self.finish(False, 'canceled; wheels stopped, servo targets held', canceled=True)
            return None
        if elapsed >= self.timeout:
            self.finish(False, 'timeout; wheels stopped, servo targets held', status='timeout')
            return None
        phase = self.current_phase()
        n.memory.phase(phase)
        source = n.cmd if n.state == 'track' else n.action
        self.handle.publish_feedback(RobotTask.Feedback(
            phase=phase, elapsed_sec=elapsed,
            track_id=source.track_id if source is not None else 0,
            clearance=bool(n.sweep is not None and not n.sweep.done and n.state == 'track')))
        motion_elapsed = 0.
        if g.operation in ('move', 'turn', 'claw'):
            if self.motion_started is None:
                self.motion_started = time.monotonic()
            motion_elapsed = time.monotonic() - self.motion_started
        done = self.completed
        if g.operation == 'stop':
            done = True
        elif g.operation in ('move', 'turn'):
            done = motion_elapsed >= self.duration
            if not done:
                sign = math.copysign(1., g.distance_m if g.operation == 'move' else g.angle_deg)
                speed = self.speed * sign
                return (speed, speed) if g.operation == 'move' else (-speed, speed)
        elif g.operation == 'claw':
            done = motion_elapsed >= n.settings['claw_settle_sec']
        elif g.operation == 'camera':
            now = n.get_clock().now().nanoseconds * 1e-9
            done = (n.pitch_time is not None and 0 <= now - n.pitch_time <= n.settings['pitch_timeout']
                    and abs(n.pitch - math.radians(g.angle_deg)) <= n.settings['grasp_pitch_tol'])
        elif g.operation == 'align':
            now = n.get_clock().now().nanoseconds * 1e-9
            done = n.fresh(n.cmd, now) and n.cmd.valid and not n.cmd.predicted and n.cmd.aligned
        elif g.operation == 'grasp' or g.operation in TARGET_OPERATIONS:
            done = n.state == 'done'
        if done:
            self.finish(True, 'completed (timed motion is not physical grasp/odometry confirmation)')
        return None

    def allows_command(self, msg):
        if not self.reserved or self.goal is None:
            return False
        op = self.goal.operation
        return ((op == 'align' and msg.first_grasp_id == 0) or
                (op in TARGET_OPERATIONS and msg.first_grasp_id == self.first_id
                 and msg.class_id == self.goal.target_class_id and not msg.clearance))

    def allows_grasp(self):
        return self.goal is not None and self.goal.operation in TARGET_OPERATIONS
