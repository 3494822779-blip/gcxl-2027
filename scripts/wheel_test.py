#!/usr/bin/env python3
"""Make the wheels move, to check the wiring and the sign of lw/rw (protocol: docs/serial_protocol.md).

THE ROBOT WILL MOVE. Lift the drive wheels off the ground or clear a free area first.
Standalone (no ROS). Stop the ROS serial_node first: only one process can use the port.

Steps (each for --seconds, with a 1 s stop in between). lw/rw > 0 means the wheel turns FORWARD:
  1 left wheel forward only      2 right wheel forward only
  3 both wheels forward          4 both wheels backward
Use --pattern to run a single step: left, right, forward, backward, spin-left, spin-right.

--raw N sends the wheel value N directly (the MCU's own unit, no m/s conversion) instead of
--speed. It bypasses the +/-1.0 range of docs/serial_protocol.md, so use it only for this test.

Claws are held at --lg/--rg (default 0 deg) and the camera angle at --cp (default 0 deg).
The script always ends by sending stop frames, also on Ctrl+C or an error.

  scripts/wheel_test.py --port /dev/ttyACM0                      # all four steps, 0.1 m/s, 2 s each
  scripts/wheel_test.py --port /dev/ttyACM0 --pattern forward --speed 0.2 --seconds 3
  scripts/wheel_test.py --port /dev/ttyACM0 --pattern forward --raw 500 --seconds 2   # lw = rw = 500
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from serial_check import encode, open_port  # noqa: E402

MAX_SPEED = 0.5   # m/s hard cap for this test
MAX_RAW = 500.    # hard cap for --raw (MCU unit)
RATE = 20.        # CMD frames per second, same as the actuator node

STEPS = {
    'left': ('left wheel forward only', lambda v: (v, 0.)),
    'right': ('right wheel forward only', lambda v: (0., v)),
    'forward': ('both wheels forward', lambda v: (v, v)),
    'backward': ('both wheels backward', lambda v: (-v, -v)),
    'spin-left': ('spin left in place (left back, right forward)', lambda v: (-v, v)),
    'spin-right': ('spin right in place (left forward, right back)', lambda v: (v, -v)),
}
DEFAULT_ORDER = ['left', 'right', 'forward', 'backward']


class Sender:
    def __init__(self, fd, lg, rg, cp):
        self.fd, self.lg, self.rg, self.cp, self.seq = fd, lg, rg, cp, 0

    def send(self, lw, rw):
        os.write(self.fd, encode(self.seq % 65536, lw, rw, self.lg, self.rg, self.cp).encode() + b'\n')
        self.seq += 1

    def hold(self, lw, rw, seconds):
        end = time.monotonic() + seconds
        nxt = time.monotonic()
        while time.monotonic() < end:
            self.send(lw, rw)
            nxt += 1 / RATE
            time.sleep(max(0., nxt - time.monotonic()))

    def stop(self, frames=10):
        for _ in range(frames):
            self.send(0., 0.)
            time.sleep(1 / RATE)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', default='/dev/ttyACM0')
    ap.add_argument('--baud', type=int, default=115200)
    ap.add_argument('--speed', type=float, default=0.1, help=f'wheel speed in m/s (default 0.1, max {MAX_SPEED})')
    ap.add_argument('--raw', type=float, help=f'send this wheel value as is (MCU unit, max {MAX_RAW:g}) instead of --speed')
    ap.add_argument('--seconds', type=float, default=2., help='duration of each step (default 2)')
    ap.add_argument('--pattern', choices=sorted(STEPS), help='run only this step')
    ap.add_argument('--lg', type=float, default=0., help='claw angle sent while testing, degrees')
    ap.add_argument('--rg', type=float, default=0.)
    ap.add_argument('--cp', type=float, default=0., help='camera servo angle sent while testing, degrees')
    ap.add_argument('--yes', action='store_true', help='skip the confirmation prompt')
    a = ap.parse_args()
    if a.raw is not None:
        if not 0 < a.raw <= MAX_RAW:
            sys.exit(f'--raw must be in (0, {MAX_RAW:g}]')
        level, unit = a.raw, ' (raw)'
    else:
        if not 0 < a.speed <= MAX_SPEED:
            sys.exit(f'--speed must be in (0, {MAX_SPEED}] m/s')
        level, unit = a.speed, ' m/s'
    if not 0 < a.seconds <= 10:
        sys.exit('--seconds must be in (0, 10]')

    order = [a.pattern] if a.pattern else DEFAULT_ORDER
    print(f'port {a.port} @ {a.baud}, wheel value {level:g}{unit}, {a.seconds}s per step: ' + ', '.join(order))
    print('The wheels will turn. Lift the drive wheels or clear the area. Ctrl+C stops.')
    if not a.yes and input('Press Enter to start (anything else + Enter to abort): ').strip():
        sys.exit('aborted')

    try:
        fd = open_port(a.port, a.baud)
    except OSError as e:
        sys.exit(f'FAIL: cannot open {a.port}: {e} (port busy, wrong name or no permission?)')
    tx = Sender(fd, a.lg, a.rg, a.cp)
    try:
        for i, name in enumerate(order, 1):
            text, wheels = STEPS[name]
            lw, rw = wheels(level)
            print(f'[{i}/{len(order)}] {text}: lw={lw:+.2f} rw={rw:+.2f}', flush=True)
            tx.hold(lw, rw, a.seconds)
            tx.stop(frames=int(RATE))   # 1 s stop between steps
        print('done')
    except KeyboardInterrupt:
        print('\ninterrupted')
    finally:
        tx.stop()
        os.close(fd)
        print('stop frames sent')


if __name__ == '__main__':
    main()
