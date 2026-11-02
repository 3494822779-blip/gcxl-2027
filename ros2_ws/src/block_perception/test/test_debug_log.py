import json
from types import SimpleNamespace
from block_perception.debug_log import DebugLog, enabled_for


class FakeNode:
    def get_name(self): return 'actuator'
    def get_clock(self): return SimpleNamespace(now=lambda: SimpleNamespace(nanoseconds=2_000_000_000))
    def get_logger(self): return SimpleNamespace(info=lambda *_: None)


def test_enabled_for():
    assert not enabled_for('a', '') and not enabled_for('a', '0')
    assert enabled_for('a', '1') and enabled_for('a', 'x, a')
    assert not enabled_for('a', 'b')


def test_disabled_is_noop(tmp_path):
    log = DebugLog(FakeNode(), str(tmp_path), spec='')
    log.event('x', v=1)
    assert list(tmp_path.iterdir()) == []


def test_writes_jsonl_and_throttles(tmp_path):
    log = DebugLog(FakeNode(), str(tmp_path), spec='1')
    log.event('state', state='grasp')
    log.event('tx', every=60., n=1)
    log.event('tx', every=60., n=2)
    log.close()
    rows = [json.loads(l) for l in (tmp_path / 'actuator.jsonl').read_text().splitlines()]
    assert [r['event'] for r in rows] == ['state', 'tx']
    assert rows[0]['t'] == 2.0 and rows[0]['state'] == 'grasp' and rows[1]['n'] == 1
