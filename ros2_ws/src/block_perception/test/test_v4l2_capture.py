import errno
from unittest.mock import Mock

import pytest

from block_perception import v4l2_capture as v


def bare_capture():
    capture = v.V4L2Capture.__new__(v.V4L2Capture)
    capture.fd = 10
    capture.buffers = [bytearray(b'jpeg-data')]
    capture.streaming = False
    capture.sequence = None
    capture.sequence_gaps = capture.error_frames = 0
    capture.defaults = {}
    return capture


def test_copy_and_requeue_before_consumer_runs(monkeypatch):
    capture = bare_capture()
    capture.sequence = 4
    monkeypatch.setattr(v.select, 'select', lambda *args: ([10], [], []))
    calls = []

    def ioctl(cmd, buf):
        calls.append(cmd)
        if cmd == v.DQBUF:
            buf.index, buf.bytesused, buf.sequence = 0, 4, 7
        if cmd == v.QBUF:
            capture.buffers[0][:4] = b'xxxx'

    capture._ioctl = ioctl
    assert capture.read() == b'jpeg'
    assert calls == [v.DQBUF, v.QBUF]
    assert capture.sequence_gaps == 2


@pytest.mark.parametrize('flags,size', [(0x40, 4), (0, 0), (0, 100)])
def test_bad_frame_always_requeues(monkeypatch, flags, size):
    capture = bare_capture()
    monkeypatch.setattr(v.select, 'select', lambda *args: ([10], [], []))
    calls = []

    def ioctl(cmd, buf):
        calls.append(cmd)
        if cmd == v.DQBUF:
            buf.flags, buf.bytesused = flags, size

    capture._ioctl = ioctl
    if size == 100:
        with pytest.raises(RuntimeError):
            capture.read()
    else:
        assert capture.read() is None
    assert calls == [v.DQBUF, v.QBUF]


def test_timeout_does_not_dequeue(monkeypatch):
    capture = bare_capture()
    capture._ioctl = Mock()
    monkeypatch.setattr(v.select, 'select', lambda *args: ([], [], []))
    assert capture.read() is None
    capture._ioctl.assert_not_called()


def test_transient_dequeue(monkeypatch):
    capture = bare_capture()
    monkeypatch.setattr(v.select, 'select', lambda *args: ([10], [], []))
    capture._ioctl = Mock(side_effect=BlockingIOError(errno.EAGAIN, 'try again'))
    assert capture.read() is None


def test_disconnect_release_is_idempotent(monkeypatch):
    capture = bare_capture()
    capture.streaming = True
    buffer = Mock()
    capture.buffers = [buffer]
    capture._ioctl = Mock(side_effect=OSError(errno.ENODEV, 'unplugged'))
    close = Mock()
    monkeypatch.setattr(v.os, 'close', close)
    capture.release()
    capture.release()
    buffer.close.assert_called_once()
    close.assert_called_once_with(10)
    assert capture.fd == -1


def test_auto_default_before_dependent_controls_and_readback():
    capture = bare_capture()
    auto, manual, brightness, button = 0x0098090C, 0x0098091A, 0x00980900, 0x00980922
    ids = sorted([auto, manual, brightness, button])
    writes = []

    def ioctl(cmd, ctrl):
        if cmd == v.QUERYCTRL:
            if ctrl.id & 0x80000000:
                previous = ctrl.id & 0x7FFFFFFF
                candidates = [x for x in ids if x > previous]
                if not candidates:
                    raise OSError(errno.EINVAL, 'end')
                ctrl.id = candidates[0]
            ctrl.type = 4 if ctrl.id == button else 1
            ctrl.name = str(ctrl.id).encode()
            ctrl.default_value = 1 if ctrl.id == auto else 0
            ctrl.flags = 16 if ctrl.id == manual and writes else 0
        elif cmd == v.S_CTRL:
            writes.append((ctrl.id, ctrl.value))
        elif cmd == v.G_CTRL:
            ctrl.value = 1 if ctrl.id == auto else 0

    capture._ioctl = ioctl
    capture.restore_defaults()
    assert writes == [(auto, 1), (brightness, 0)]
    assert len(capture.defaults) == 2


def test_default_write_failure_names_control():
    capture = bare_capture()

    def ioctl(cmd, ctrl):
        if cmd == v.QUERYCTRL:
            if ctrl.id == (0x00980900 | 0x80000000):
                raise OSError(errno.EINVAL, 'end')
            ctrl.id, ctrl.type, ctrl.name = 0x00980900, 1, b'Brightness'
        elif cmd == v.S_CTRL:
            raise OSError(errno.EIO, 'failed')

    capture._ioctl = ioctl
    with pytest.raises(RuntimeError, match='Brightness'):
        capture.restore_defaults()


def test_negotiation_failure_closes_fd(monkeypatch):
    monkeypatch.setattr(v.os, 'open', lambda *args: 10)
    close = Mock()
    monkeypatch.setattr(v.os, 'close', close)

    def ioctl(self, cmd, fmt):
        assert cmd == v.S_FMT
        fmt.fmt.pix.width = 640

    monkeypatch.setattr(v.V4L2Capture, '_ioctl', ioctl)
    with pytest.raises(RuntimeError, match='negotiated'):
        v.V4L2Capture('/dev/video0', 1280, 720, 60)
    close.assert_called_once_with(10)
