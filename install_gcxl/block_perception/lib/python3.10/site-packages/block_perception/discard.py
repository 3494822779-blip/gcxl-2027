"""Reusable timed discard task; no ROS or serial dependencies.

step(now_seconds) returns left/right wheel speeds (m/s) and claw angle (degrees).
Call periodically until done. Configuration is snapshotted at construction.
When entry angles differ, right_claw holds the independent right target; step()
retains its original three-value API with the left angle in the last position.
"""
import math


class DiscardTask:
    def __init__(self, settings, claw, right_claw=None):
        p = settings
        for key in ('discard_speed_mps', 'discard_turn_speed_mps', 'max_wheel_speed',
                    'track_width', 'claw_settle_sec'):
            if not math.isfinite(p[key]) or p[key] <= 0:
                raise ValueError(f'{key} must be finite and positive')
        if not math.isfinite(p['discard_distance_m']) or p['discard_distance_m'] < 0:
            raise ValueError('discard_distance_m must be finite and nonnegative')
        if not p['claw_init_deg'] <= 90. <= p['claw_max_deg']:
            raise ValueError('discard requires 90 degrees within claw limits')
        speed = min(p['discard_speed_mps'], p['max_wheel_speed'])
        turn = min(max(p['discard_turn_speed_mps'], p['min_turn_speed']), p['max_wheel_speed'])
        turn_sec = math.pi * p['track_width'] / (4 * turn)
        move_sec = p['discard_distance_m'] / speed
        settle = p['claw_settle_sec']
        self.steps = [
            ('turn_ccw', turn_sec, -turn, turn, claw),
            ('open', settle, 0., 0., 90.),
            ('backward', move_sec, -speed, -speed, 90.),
            ('open_again', settle, 0., 0., 90.),
            ('forward', move_sec, speed, speed, 90.),
            ('turn_cw', turn_sec, turn, -turn, 90.),
        ]
        self.index = 0
        self.started = None
        self.claw = claw
        self.right_claw = claw if right_claw is None else right_claw

    @property
    def done(self):
        return self.index == len(self.steps)

    @property
    def phase(self):
        return 'done' if self.done else self.steps[self.index][0]

    def step(self, now):
        if self.done:
            return 0., 0., self.claw
        _, duration, lw, rw, claw = self.steps[self.index]
        if self.started is not None and now - self.started >= duration:
            self.index += 1
            self.started = None
            # Stop between phases. Start timing only when the next command is emitted.
            return 0., 0., self.claw
        if self.started is None:
            self.started = now
        self.claw = claw
        if self.index > 0:  # The first turn must preserve both entry angles.
            self.right_claw = claw
        return (lw, rw, claw) if duration > 0 else (0., 0., claw)
