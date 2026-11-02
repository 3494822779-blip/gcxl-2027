"""ROS-independent state and math; all times in seconds, pitch in radians."""
import math
import numpy as np


class Lock:
    def __init__(self):
        self.id = None

    def select(self, boxes):
        candidates = [b for b in boxes if b.observed]
        if self.id is None and candidates:
            self.id = max(candidates, key=lambda b: (b.y + b.height, b.confidence, -b.track_id)).track_id
        return next((b for b in candidates if b.track_id == self.id), None)


class CVFilter:
    def __init__(self):
        self.state = None
        self.time = None
        self.last_seen = None

    def step(self, t, measurement, q, r, initial_variance):
        if self.time is not None and t <= self.time:
            return None  # No rewinding from duplicate/out-of-order messages.
        if self.state is None:
            if measurement is None:
                return None
            self.state = np.array([*measurement, 0., 0.])
            self.cov = np.eye(4) * initial_variance
        else:
            dt = t - self.time
            f = np.eye(4)
            f[0, 2] = f[1, 3] = dt
            g = np.array([[dt*dt/2, 0], [0, dt*dt/2], [dt, 0], [0, dt]])
            self.state = f @ self.state
            self.cov = f @ self.cov @ f.T + q * g @ g.T
            if measurement is not None:
                h = np.eye(2, 4)
                noise = np.eye(2) * r
                k = np.linalg.solve(h @ self.cov @ h.T + noise, h @ self.cov).T
                self.state += k @ (np.asarray(measurement) - h @ self.state)
                a = np.eye(4) - k @ h
                self.cov = a @ self.cov @ a.T + k @ noise @ k.T
        self.time = t
        if measurement is not None:
            self.last_seen = t
        return self.state[:2].copy()


def rate(error, deadzone, gain, limit):
    return 0. if abs(error) <= deadzone else max(-limit, min(limit, gain * error))


def validate(values):
    for name, value in values.items():
        if isinstance(value, (float, int)) and not isinstance(value, bool) and not math.isfinite(value):
            raise ValueError(f'{name} must be finite')
    for name in ['process_noise', 'measurement_noise', 'initial_variance', 'prediction_timeout',
                 'input_timeout', 'pitch_timeout', 'max_yaw_rate', 'max_pitch_rate', 'hold_seconds']:
        if name in values and values[name] <= 0:
            raise ValueError(f'{name} must be positive')
    for name in ['deadzone_x', 'deadzone_y']:
        if name in values and not 0 <= values[name] < 1:
            raise ValueError(f'{name} must be in [0, 1)')
    if 'pitch_min' in values and values['pitch_min'] > values['pitch_max']:
        raise ValueError('pitch_min must not exceed pitch_max')
    for name in ['track_width', 'max_wheel_speed', 'pitch_home_tol',
                 'claw_settle_sec', 'mm_per_px', 'link_length']:
        if name in values and values[name] <= 0:
            raise ValueError(f'{name} must be positive')
    for name in ['forward_speed', 'forward_max_error_x', 'claw_margin_mm']:
        if name in values and values[name] < 0:
            raise ValueError(f'{name} must not be negative')
    if 'bottom_target' in values and not 0 < values['bottom_target'] <= 1:
        raise ValueError('bottom_target must be in (0, 1]')
    if 'cam_min_deg' in values and not values['cam_min_deg'] < values['cam_max_deg']:
        raise ValueError('cam_min_deg must be below cam_max_deg')
    # Claws open outward from the initial angle only; never below it.
    if 'claw_init_deg' in values and values['claw_init_deg'] < 0:
        raise ValueError('claw_init_deg must not be negative')
    if 'claw_max_deg' in values and values['claw_max_deg'] < values['claw_init_deg']:
        raise ValueError('claw_max_deg must not be below claw_init_deg')
    if 'claw_close_deg' in values and not values['claw_init_deg'] <= values['claw_close_deg'] <= values['claw_max_deg']:
        raise ValueError('claw_close_deg must be within [claw_init_deg, claw_max_deg]')
