from types import SimpleNamespace as Box
from block_perception.first_grasp import candidates


def box(i, cls, x=100., y=200., w=80., h=40., observed=True):
    return Box(track_id=i, class_id=cls, x=x, y=y, width=w, height=h,
               confidence=.9, observed=observed)


def test_strip_extends_up_not_down_and_edge_touch_is_allowed():
    target = box(1, 0)
    assert candidates([target, box(2, 1, y=50.)]) == []
    assert candidates([target, box(2, 1, x=180., y=50.)]) == [(target, False)]
    front = box(2, 1, y=300.)
    assert candidates([target, front]) == [(front, True)]


def test_small_overlap_threshold_and_predicted_hazards():
    target = box(1, 0)
    other = box(2, 1, x=179., y=50., observed=False)
    assert candidates([target, other]) == []
    assert candidates([target, other], .02) == [(target, False)]
    assert candidates([box(1, 0, observed=False)]) == []
    assert candidates([box(1, 0, y=0.)]) == []


def test_nearest_clearance_and_prefer_unblocked_target():
    target = box(1, 0)
    near = box(3, 2, y=350.)
    assert candidates([target, box(2, 3, y=300.), near]) == [(near, True)]
    clear = box(4, 0, x=400.)
    assert candidates([target, near, clear])[0] == (clear, False)
    near.observed = False
    assert candidates([target, near]) == []
