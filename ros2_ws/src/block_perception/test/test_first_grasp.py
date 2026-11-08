from types import SimpleNamespace as Box
import pytest
from block_perception.first_grasp import candidates, obstacle_for
from block_perception.sweep import Sweep


def box(i, cls, x=100., y=200., w=80., h=40., observed=True):
    return Box(track_id=i, class_id=cls, x=x, y=y, width=w, height=h,
               confidence=.9, observed=observed)


def test_explicit_class_and_nearest_real_target():
    a, b = box(1, 2), box(2, 2, y=300.)
    assert candidates([a, b, box(3, 0, y=400.), box(4, 2, y=500., observed=False)], 2) == [b, a]
    assert candidates([a, b], 0) == []
    assert candidates([box(1, 2, w=0.), box(2, 2, x=float('nan'))], 2) == []


def test_foreground_only_largest_horizontal_overlap_and_center_tie():
    target = box(1, 0)
    assert obstacle_for(target, [target, box(2, 1, y=20.)]) is None
    assert obstacle_for(target, [box(2, 1, x=180., y=300.)]) is None
    small, large = box(2, 1, x=160., y=400.), box(3, 1, x=110., y=300.)
    b, side, fraction, near = obstacle_for(target, [small, large])
    assert b == large and side == 1 and fraction == pytest.approx(70/80) and not near
    b, side, fraction, near = obstacle_for(target, [box(4, 1, y=220.)])
    assert side == -1 and fraction == 1. and near
    assert obstacle_for(target, [box(4, 1, y=220., observed=False)]) is None


def settings():
    return dict(sweep_duration_sec=1., sweep_tau_sec=.33, sweep_claw_deg=-30.,
                sweep_weight=.2, sweep_near_multiplier=2., sweep_max_yaw_rate=1., sweep_overlap=.5)


def test_exponential_decay_side_near_and_exact_threshold():
    a = Sweep(-1, .51, False, settings())
    assert a.yaw() == -.2
    a.advance(.33)
    assert a.yaw() == pytest.approx(-.2 / __import__('math').e)
    a.advance(.67)
    assert a.done and a.yaw() == 0.
    assert Sweep(1, .8, True, settings()).yaw() == .4
    assert Sweep(-1, .5, True, settings()).yaw() == 0.
