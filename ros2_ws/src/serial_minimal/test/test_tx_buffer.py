from types import SimpleNamespace
import pytest
from serial_minimal.tx_buffer import TxBuffer


def test_partial_writes_finish_a_line_and_skip_superseded_commands():
    output = bytearray()
    def write(data):
        count = min(3, len(data))
        output.extend(data[:count])
        return count
    tx = TxBuffer(write)
    tx.enqueue(b'first\n')
    assert not tx.flush()
    tx.enqueue(b'superseded\n')
    tx.enqueue(b'stop\n')
    for _ in range(5):
        tx.flush()
    assert output == b'first\nstop\n'


def test_blocked_write_times_out_and_resynchronizes_before_recovery():
    clock = SimpleNamespace(t=1.)
    output = bytearray()
    def blocked(data):
        raise BlockingIOError()
    tx = TxBuffer(blocked, clock=lambda: clock.t)
    tx.enqueue(b'move\n')
    assert not tx.flush()
    clock.t += .2
    with pytest.raises(TimeoutError):
        tx.flush()
    assert not tx.pending and tx.latest is None and tx.last_complete is None
    tx.write = lambda data: output.extend(data) or len(data)
    tx.enqueue(b'stop\n')
    assert tx.flush()
    assert output == b'\nstop\n'


def test_old_waiting_command_is_not_sent():
    clock = SimpleNamespace(t=1.)
    output = []
    tx = TxBuffer(lambda data: output.append(data) or len(data), clock=lambda: clock.t)
    tx.enqueue(b'move\n')
    clock.t += .2
    with pytest.raises(TimeoutError):
        tx.flush()
    assert not output


@pytest.mark.parametrize('returned', [0, -1, 999])
def test_invalid_write_count_is_a_link_fault(returned):
    tx = TxBuffer(lambda data: returned)
    tx.enqueue(b'stop\n')
    with pytest.raises(OSError):
        tx.flush()
    assert tx.last_complete is None and tx.resync
