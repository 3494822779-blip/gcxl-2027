from types import SimpleNamespace
from block_perception.task_memory import TaskMemory


def target(track=7, cls=0, first=0, clearance=False):
    return SimpleNamespace(track_id=track, class_id=cls, first_grasp_id=first, clearance=clearance)


def test_grasp_dedup_unknown_identity_and_snapshot_isolation():
    m = TaskMemory()
    m.begin_task('grasp')
    identity = m.begin_grasp(None, 0., 17.)
    assert m.begin_grasp() == identity
    m.complete_grasp()
    revision = m.revision
    m.complete_grasp()
    assert m.revision == revision
    assert len(m.data['grasps']) == 1
    assert m.grasp['class_id'] is None and m.grasp['track_id'] is None
    assert m.grasp['right_close_deg'] == 17.
    assert not m.held['physical_confirmed']
    view = m.snapshot()
    view['history']['grasps'][0]['status'] = 'broken'
    assert m.grasp['status'] == 'completed'


def test_clearance_then_formal_with_distinct_history_and_progress():
    m = TaskMemory()
    m.begin_task('first_grasp', 123)
    m.begin_grasp(target(cls=2, first=123, clearance=True))
    m.complete_grasp()
    assert not m.task['first_formal_completed']
    m.begin_discard()
    m.complete_discard()
    m.complete_discard()
    assert m.task['clearance_count'] == 1
    assert not m.task['first_formal_completed']
    m.begin_grasp(target(track=8, cls=0, first=123))
    m.complete_grasp()
    m.finish_task('completed', 'done')
    assert m.task['first_formal_completed']
    assert [g['purpose'] for g in m.data['grasps']] == ['clearance', 'first_formal']
    assert [g['disposition'] for g in m.data['grasps']] == ['discard_completed', 'commanded_hold']
    m.begin_task('first_grasp', 124)
    assert not m.task['first_formal_completed']  # progress belongs to a task


def test_standalone_discard_links_prior_grasp_and_keeps_completed_grasp_on_cancel():
    m = TaskMemory()
    m.begin_task('grasp')
    m.begin_grasp(target())
    m.complete_grasp()
    grasp = m.grasp
    m.finish_task('completed', 'done')
    m.begin_task('discard')
    m.begin_discard()
    assert m.discard['grasp_id'] == grasp['id']
    m.phase('backward')
    m.finish_task('canceled', 'backward', 'user canceled')
    assert m.discard['status'] == 'canceled'
    assert grasp['status'] == 'completed'
    assert grasp['disposition'] == 'discard_interrupted'
    assert m.held['state'] == 'unknown'
    assert m.decision_context()['unresolved_tasks'][0]['phase'] == 'backward'


def test_timeout_during_grasp_is_not_success_and_is_idempotent():
    m = TaskMemory()
    m.begin_task('first_grasp', 5)
    m.begin_grasp(target(first=5))
    m.phase('claw_close')
    m.finish_task('timeout', 'claw_close')
    m.finish_task('completed', 'done')
    assert not m.task['first_formal_completed']
    assert m.grasp['status'] == m.task['status'] == 'timeout'
    assert m.held['state'] == 'unknown'


def test_manual_open_breaks_association_and_restart_forgets_everything():
    m = TaskMemory()
    m.begin_task('grasp')
    m.begin_grasp(target())
    m.complete_grasp()
    m.finish_task('completed', 'done')
    m.uncertain_hold('manual_claw_command')
    m.begin_task('discard')
    m.begin_discard()
    assert m.discard['grasp_id'] is None  # do not fabricate a disposal of the old object
    restarted = TaskMemory()
    assert restarted.session_id != m.session_id
    assert restarted.data['tasks'] == restarted.data['grasps'] == restarted.data['discards'] == []
    assert restarted.held['state'] == 'unknown'
