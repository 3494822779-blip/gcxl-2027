import math
import pytest
from block_perception.discard import DiscardTask


def settings(**kw):
    return dict(discard_distance_m=.3, discard_speed_mps=.1,
                discard_turn_speed_mps=.17, max_wheel_speed=.5,
                min_turn_speed=.1, track_width=.2, claw_settle_sec=.5,
                claw_init_deg=0., claw_max_deg=90., **kw)


def test_complete_sequence_and_stop():
    task = DiscardTask(settings(), 0.)
    t = 0.
    for phase, duration, wheels, claw in [
        ('turn_ccw', math.pi * .2 / (4 * .17), (-.17, .17), 0.),
        ('open', .5, (0., 0.), 90.),
        ('backward', 3., (-.1, -.1), 90.),
        ('open_again', .5, (0., 0.), 90.),
        ('forward', 3., (.1, .1), 90.),
        ('turn_cw', math.pi * .2 / (4 * .17), (.17, -.17), 90.),
    ]:
        assert task.phase == phase
        assert task.step(t) == pytest.approx((*wheels, claw))
        assert task.step(t + duration - .001) == pytest.approx((*wheels, claw))
        assert task.step(t + duration + .001) == (0., 0., claw)
        t += duration + .051
    assert task.done
    assert task.step(t + 100) == (0., 0., 90.)


def test_limits_snapshot_and_zero_distance():
    p = settings()
    p.update(max_wheel_speed=.05, discard_distance_m=0.)
    task = DiscardTask(p, 20.)
    p['discard_distance_m'] = 10.
    assert task.step(0.) == (-.05, .05, 20.)
    assert task.steps[0][1] == pytest.approx(math.pi * .2 / (4 * .05))
    assert task.steps[2][1] == task.steps[4][1] == 0.
    task.index, task.started = 2, None
    assert task.step(10.) == (0., 0., 90.)


@pytest.mark.parametrize('key,value', [('discard_distance_m', -1.),
    ('discard_distance_m', float('nan')), ('discard_speed_mps', 0.),
    ('discard_turn_speed_mps', -1.), ('claw_max_deg', 80.)])
def test_invalid_settings(key, value):
    p = settings()
    p[key] = value
    with pytest.raises(ValueError):
        DiscardTask(p, 0.)
