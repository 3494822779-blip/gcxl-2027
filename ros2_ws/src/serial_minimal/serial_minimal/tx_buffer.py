"""Bounded nonblocking line writer. Never queue a history of motion commands."""
import math
import time


class TxBuffer:
    def __init__(self, write, timeout=0.1, clock=time.monotonic):
        if not math.isfinite(timeout) or timeout <= 0:
            raise ValueError('write timeout must be positive and finite')
        self.write, self.timeout, self.clock = write, timeout, clock
        self.pending = b''
        self.latest = None
        self.started = self.latest_at = None
        self.last_complete = None
        self.resync = False

    def enqueue(self, data):
        self.latest, self.latest_at = data, self.clock()

    def fail(self):
        self.pending, self.latest = b'', None
        self.last_complete = None
        self.resync = True

    def flush(self):
        now = self.clock()
        try:
            if not self.pending and self.latest is not None:
                if now - self.latest_at >= self.timeout:
                    raise TimeoutError('queued serial command expired')
                self.pending = (b'\n' if self.resync else b'') + self.latest
                self.latest = None
                self.started = self.latest_at
            if not self.pending:
                return False
            if now - self.started >= self.timeout:
                raise TimeoutError('serial frame write timed out')
            try:
                count = self.write(self.pending)
            except BlockingIOError:
                return False
            if count <= 0 or count > len(self.pending):
                raise OSError('serial write made no progress')
            self.pending = self.pending[count:]
            if not self.pending:
                self.resync = False
                self.last_complete = now
                return True
            return False
        except OSError:
            self.fail()
            raise
