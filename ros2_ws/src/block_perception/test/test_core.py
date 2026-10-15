from types import SimpleNamespace
import numpy as np
import pytest
from block_perception.core import Lock, CVFilter, rate, validate


def box(i, bottom):
    return SimpleNamespace(track_id=i, y=bottom-20, height=20, confidence=0.9, observed=True)


def test_lock_bottom_edge_then_pitch_order_change_and_loss():
    lock = Lock()
    assert lock.select([box(1, 100), box(2, 140)]).track_id == 2
    assert lock.select([box(1, 180), box(2, 80)]).track_id == 2
    assert lock.select([box(1, 180)]) is None
    assert lock.id == 2
    assert lock.select([box(2, 80)]).track_id == 2


def test_filter_prediction_noise_and_ordering():
    rng = np.random.default_rng(8)
    kf = CVFilter()
    raw_error, smooth_error = [], []
    for i in range(100):
        t = i / 30
        truth = np.array([100 + 8*t, 200-3*t])
        measured = truth + rng.normal(0, 4, 2)
        xy = kf.step(t, measured, 2., 16., 100.)
        if i > 20:
            raw_error.append(np.sum((measured-truth)**2))
            smooth_error.append(np.sum((xy-truth)**2))
    assert np.mean(smooth_error) < np.mean(raw_error) / 2
    previous = kf.state.copy()
    predicted = kf.step(3.5, None, 2., 16., 100.)
    assert predicted[0] > previous[0]
    assert kf.last_seen == 99/30
    assert kf.step(3., (0, 0), 2., 16., 100.) is None
    assert np.linalg.eigvalsh(kf.cov).min() > 0


def test_deadzone_saturation_and_validation():
    assert rate(0.02, .04, -.5, .3) == 0
    assert rate(1., .04, -.5, .3) == -.3
    assert rate(-1., .04, -.5, .3) == .3
    for params in [dict(measurement_noise=0), dict(pitch_min=1., pitch_max=0.),
                   dict(deadzone_x=1.), dict(process_noise=float('nan'))]:
        with pytest.raises(ValueError):
            validate(params)
