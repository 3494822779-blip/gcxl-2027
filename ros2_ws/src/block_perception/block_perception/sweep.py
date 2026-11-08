"""ROS-independent, single-use sweep timing; progress counts valid control time only."""
import math


class Sweep:
    def __init__(self, side, overlap, near, settings):
        self.side = side
        self.elapsed = 0.
        self.duration = settings['sweep_duration_sec']
        self.tau = settings['sweep_tau_sec']
        self.angle = settings['sweep_claw_deg']
        weight = settings['sweep_weight'] * (settings['sweep_near_multiplier'] if near else 1.)
        self.bias = side * min(1., weight) * settings['sweep_max_yaw_rate'] if overlap > settings['sweep_overlap'] else 0.

    @property
    def done(self):
        return self.elapsed >= self.duration

    def advance(self, dt):
        self.elapsed = min(self.duration, self.elapsed + max(0., dt))

    def yaw(self):
        return 0. if self.done else self.bias * math.exp(-self.elapsed / self.tau)
