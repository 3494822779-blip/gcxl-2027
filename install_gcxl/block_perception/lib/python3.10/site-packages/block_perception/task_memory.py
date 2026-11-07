"""Decision memory: command outcomes, never inferred physical grasp confirmation.

No ROS or filesystem dependency. History belongs to this process session only.
Restarting creates an empty history and unknown physical state.
"""
import copy
from datetime import datetime, timezone
import uuid


def timestamp():
    return datetime.now(timezone.utc).isoformat()


class TaskMemory:
    def __init__(self):
        self.session_id = uuid.uuid4().hex
        self.data = {'version': 1, 'tasks': [], 'grasps': [], 'discards': []}
        self.task = self.grasp = self.discard = None
        self.held = {'state': 'unknown', 'grasp_id': None, 'physical_confirmed': False}
        self.revision = 0

    def _changed(self):
        self.revision += 1

    def _record(self, kind, **fields):
        record = dict(id=uuid.uuid4().hex, session_id=self.session_id,
                      started_at=timestamp(), status='running', **fields)
        self.data[kind].append(record)
        return record

    def begin_task(self, operation, first_grasp_id=0):
        self.task = self._record('tasks', operation=operation, first_grasp_id=first_grasp_id,
                                 phase='starting', first_formal_completed=False, clearance_count=0)
        self.grasp = self.discard = None
        self._changed()

    def begin_grasp(self, target=None, left_deg=0., right_deg=0.):
        if self.grasp is not None and self.grasp['status'] == 'running':
            return self.grasp['id']
        self.uncertain_hold('new_grasp')
        first_id = target.first_grasp_id if target is not None else 0
        purpose = ('clearance' if target.clearance else 'first_formal') if first_id else 'normal'
        self.grasp = self._record('grasps', task_id=self.task['id'] if self.task else None,
                                  first_grasp_id=first_id, purpose=purpose,
                                  track_id=target.track_id if target is not None else None,
                                  class_id=target.class_id if target is not None else None,
                                  left_close_deg=left_deg, right_close_deg=right_deg,
                                  physical_confirmed=False, phase='grasp_pitch', disposition='pending')
        self._changed()
        return self.grasp['id']

    def phase(self, phase):
        changed = False
        for record in (self.task, self.discard if self.discard and self.discard['status'] == 'running' else self.grasp):
            if record is not None and record['status'] == 'running' and record['phase'] != phase:
                record['phase'] = phase
                changed = True
        if changed:
            self._changed()

    def complete_grasp(self):
        if self.grasp is None or self.grasp['status'] != 'running':
            return
        self.grasp.update(status='completed', finished_at=timestamp(), disposition='commanded_hold')
        self.held = dict(state='commanded_hold', grasp_id=self.grasp['id'], physical_confirmed=False)
        if self.task and self.grasp['purpose'] == 'first_formal':
            self.task['first_formal_completed'] = True
        self._changed()

    def begin_discard(self):
        if self.discard is not None and self.discard['status'] == 'running':
            return
        linked = self.held['grasp_id'] if self.held['state'] == 'commanded_hold' else None
        self.discard = self._record('discards', task_id=self.task['id'] if self.task else None,
                                    grasp_id=linked, phase='turn_ccw', physical_confirmed=False)
        for grasp in self.data['grasps']:
            if grasp['id'] == linked:
                grasp['disposition'] = 'discard_running'
        self._changed()

    def complete_discard(self):
        if self.discard is None or self.discard['status'] != 'running':
            return
        self.discard.update(status='completed', finished_at=timestamp())
        for grasp in self.data['grasps']:
            if grasp['id'] == self.discard['grasp_id']:
                grasp['disposition'] = 'discard_completed'
                if self.task and grasp['task_id'] == self.task['id'] and grasp['purpose'] == 'clearance':
                    self.task['clearance_count'] += 1
        self.held = dict(state='release_command_completed', grasp_id=None, physical_confirmed=False)
        self._changed()

    def uncertain_hold(self, reason):
        if self.held['state'] == 'unknown':
            return
        linked = self.held['grasp_id']
        for grasp in self.data['grasps']:
            if grasp['id'] == linked:
                grasp['disposition'] = 'unknown'
        self.held = dict(state='unknown', grasp_id=linked, reason=reason, physical_confirmed=False)
        self._changed()

    def finish_task(self, status, phase, reason=''):
        if self.task is None or self.task['status'] != 'running':
            return
        self.task.update(status=status, phase=phase, reason=reason, finished_at=timestamp())
        interrupted_motion = False
        for record in (self.grasp, self.discard):
            if record is not None and record['status'] == 'running':
                record.update(status=status if status != 'completed' else 'interrupted',
                              phase=phase, reason=reason, finished_at=timestamp())
                interrupted_motion = True
        if self.discard is not None and self.discard['status'] != 'completed':
            for grasp in self.data['grasps']:
                if grasp['id'] == self.discard['grasp_id']:
                    grasp['disposition'] = 'discard_interrupted'
        if interrupted_motion:
            self.held = dict(state='unknown', grasp_id=self.grasp['id'] if self.grasp else self.held['grasp_id'],
                             physical_confirmed=False)
        self._changed()

    def decision_context(self):
        return copy.deepcopy(dict(version=1, session_id=self.session_id, revision=self.revision,
                                  task=self.task, held=self.held,
                                  latest_grasp=self.data['grasps'][-1] if self.data['grasps'] else None,
                                  latest_discard=self.data['discards'][-1] if self.data['discards'] else None,
                                  unresolved_tasks=[t for t in self.data['tasks'] if t['status'] in
                                                    ('canceled', 'timeout', 'aborted', 'interrupted')]))

    def snapshot(self):
        return dict(context=self.decision_context(), history=copy.deepcopy(self.data))
