"""Opt-in debug event log (JSON lines), off unless BP_DEBUG is set.

  BP_DEBUG=1                 enable for all nodes
  BP_DEBUG=actuator,filter   enable for the named nodes only
  BP_DEBUG_DIR=<dir>         output directory (default ~/.ros/block_perception_debug)

Each node writes <dir>/<node>.jsonl (5 MB x 3 rotating files), one event per line:
  {"t": <ROS clock seconds>, "wall": <unix seconds>, "node": "actuator", "event": "state", ...fields}
Disabled loggers are no-ops, so call sites need no guards. Pass every=<sec> to rate-limit a
per-key event that fires in a hot callback; the call is dropped if the same event was
logged less than <sec> ago.
"""
import json
import logging
import os
import time
from logging.handlers import RotatingFileHandler


def enabled_for(name, spec=None):
    spec = os.environ.get('BP_DEBUG', '') if spec is None else spec
    spec = spec.strip().lower()
    if spec in ('', '0', 'false', 'off', 'no'):
        return False
    if spec in ('1', 'true', 'on', 'yes', 'all'):
        return True
    return name in {s.strip() for s in spec.split(',')}


class DebugLog:
    def __init__(self, node, directory=None, spec=None):
        self.node = node
        self.name = node.get_name()
        self.on = enabled_for(self.name, spec)
        self._last = {}
        self._logger = None
        if not self.on:
            return
        directory = directory or os.environ.get('BP_DEBUG_DIR') or os.path.expanduser('~/.ros/block_perception_debug')
        os.makedirs(directory, exist_ok=True)
        self._logger = logging.getLogger(f'block_perception.debug.{self.name}.{id(self)}')
        self._logger.setLevel(logging.DEBUG)
        self._logger.propagate = False
        self.path = os.path.join(directory, f'{self.name}.jsonl')
        self._handler = RotatingFileHandler(self.path, maxBytes=5_000_000, backupCount=3, encoding='utf-8')
        self._handler.setFormatter(logging.Formatter('%(message)s'))
        self._logger.addHandler(self._handler)
        node.get_logger().info(f'debug log enabled: {self.path}')

    def event(self, event, every=None, **fields):
        if not self.on:
            return
        now = time.monotonic()
        if every is not None:
            if now - self._last.get(event, -1e18) < every:
                return
            self._last[event] = now
        record = {'t': round(self.node.get_clock().now().nanoseconds * 1e-9, 4),
                  'wall': round(time.time(), 4), 'node': self.name, 'event': event}
        record.update(fields)
        try:
            self._logger.debug(json.dumps(record, ensure_ascii=False, default=str))
        except (TypeError, ValueError, OSError):
            pass    # debug logging must never break a control node

    def close(self):
        if self._logger is not None:
            self._logger.removeHandler(self._handler)
            self._handler.close()
            self.on = False
