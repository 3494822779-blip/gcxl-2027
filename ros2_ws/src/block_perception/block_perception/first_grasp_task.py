"""Independent first-grasp task node; owns target selection until completion.

Start via /robot/execute first_grasp; task/first_grasp is a compatibility client. Never run alongside the normal selector
on the same target/raw topic. Low-level alignment and actuation are shared.
"""
from rclpy.action import ActionClient
from block_interfaces.action import RobotTask
from rclpy.qos import qos_profile_sensor_data
from std_msgs.msg import Bool, UInt64
from block_interfaces.msg import Frame, Target, AlignmentCommand
from .nodes import ConfigNode, run, seconds
from .core import Lock
from .first_grasp import candidates as first_candidates


class FirstGraspTask(ConfigNode):
    def __init__(self):
        super().__init__('first_grasp')
        self.configure(dict(first_grasp_overlap=0.0))
        self.lock = Lock()
        self.last_stamp = None
        self.first_id = 0
        self.first_busy = None
        self.first_done = False
        self.clearance = False
        self.pub = self.create_publisher(Target, 'target/raw', 10)
        self.create_subscription(Frame, 'tracks', self.frame, qos_profile_sensor_data)
        self.action_client = ActionClient(self, RobotTask, 'robot/execute')
        self.create_subscription(UInt64, 'task/first_grasp_cancel', self.cancel_action, 10)
        self.create_subscription(UInt64, 'task/first_grasp_id', self.start_action, 10)
        self.create_subscription(Bool, 'task/first_grasp', self.start_first, 10)
        self.create_subscription(AlignmentCommand, 'alignment/command', self.first_command, 10)
        self.create_subscription(AlignmentCommand, 'task/grasp_finished', self.first_finished, 10)

    def start_action(self, msg):
        if not msg.data:
            return
        self.first_id = msg.data
        self.first_done = False
        self.first_busy = None
        self.reset(Bool(data=True))
        self.last_stamp = self.first_id * 1e-9

    def cancel_action(self, msg):
        if msg.data != self.first_id:
            return
        self.first_done = True
        self.first_busy = None
        self.reset(Bool(data=True))

    def start_first(self, msg):
        # Compatibility trigger; the Action server owns admission and completion.
        if msg.data:
            self.action_client.send_goal_async(RobotTask.Goal(operation='first_grasp'))

    def first_command(self, msg):
        if (self.first_id and not self.first_done and msg.first_grasp_id == self.first_id
                and msg.grasp_request and msg.track_id == self.lock.id):
            self.first_busy = (msg.track_id, msg.clearance)

    def first_finished(self, msg):
        if (msg.first_grasp_id != self.first_id or self.first_busy is None or
                self.first_busy != (msg.track_id, msg.clearance)):
            return
        self.first_busy = None
        self.first_done = not msg.clearance
        self.reset(Bool(data=True))
        # Require images captured after completion, not queued pre-discard frames.
        self.last_stamp = seconds(msg.header.stamp)

    def reset(self, msg):
        if msg.data and self.first_busy is None:
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
        prev_id = self.lock.id
        if not self.first_id or self.first_busy is not None or self.first_done:
            self.pub.publish(Target(header=msg.image.header))
            return
        choices = first_candidates(msg.boxes, self.settings['first_grasp_overlap'])
        chosen = next((item for item in choices if item[0].track_id == self.lock.id
                       and item[1] == self.clearance), choices[0] if choices else None)
        self.lock.id = chosen[0].track_id if chosen else None
        b, self.clearance = chosen if chosen else (None, False)
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
            out.first_grasp_id, out.clearance = self.first_id, self.clearance
            out.class_id = b.class_id
            out.raw_x = out.x = b.x + b.width / 2
            out.raw_y = out.y = b.y + b.height  # bottom edge, not centre
            out.last_observed = msg.image.header.stamp
        self.pub.publish(out)


def main():
    run(FirstGraspTask)
