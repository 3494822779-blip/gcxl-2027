#!/usr/bin/env python3
"""Send -30 degrees to the left claw, then to the right claw.

Standalone (no ROS). Stop serial_node before using the same port.
Each stage sends CMD frames at 20 Hz for --seconds (default 2):
  1. left = -30 deg, right = 0 deg
  2. left = -30 deg, right = -30 deg
Wheels stay stopped; camera uses --cp (default 0 deg).
Afterwards all servos return to 0 deg, including on Ctrl+C or errors.
--keep holds the most recently commanded angles instead.
--loop repeats the sequence with a home stage between cycles.

  scripts/servo_test.py --port /dev/ttyACM0 --seconds 2 --keep
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from serial_check import encode, open_port  # noqa: E402

CAM_MIN, CAM_MAX = -90., 90.   # camera pitch range (deg)
RATE = 20.                     # CMD frames per second, same as the actuator node


class Sender:
    def __init__(self, fd):
        self.fd, self.seq = fd, 0

    def send(self, lg, rg, cp):
        os.write(self.fd, encode(self.seq % 65536, 0., 0., lg, rg, cp).encode() + b'\n')
        self.seq += 1

    def hold(self, lg, rg, cp, seconds):
        end = time.monotonic() + seconds
        nxt = time.monotonic()
        while time.monotonic() < end:
            self.send(lg, rg, cp)
            nxt += 1 / RATE
            time.sleep(max(0., nxt - time.monotonic()))

    def stop(self, lg, rg, cp, frames=10):
        for _ in range(frames):
            self.send(lg, rg, cp)
            time.sleep(1 / RATE)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', default='/dev/ttyACM0')
    ap.add_argument('--baud', type=int, default=115200)
    ap.add_argument('--cp', type=float, default=0., help=f'camera pitch angle in degrees [{CAM_MIN:g}, {CAM_MAX:g}]')
    ap.add_argument('--seconds', type=float, default=2., help='how long to hold the angles (default 2)')
    ap.add_argument('--keep', action='store_true', help='hold the last angles instead of going home at the end')
    ap.add_argument('--loop', action='store_true',
                    help='repeat forever: hold the angles for --seconds, then home (0 deg) for --seconds; Ctrl+C stops')
    ap.add_argument('--yes',action='store_true', help='skip the confirmation prompt')
    a = ap.parse_args()

    if not CAM_MIN <= a.cp <= CAM_MAX:
        sys.exit(f'--cp must be in [{CAM_MIN:g}, {CAM_MAX:g}]')
    if not 0 < a.seconds <= 10:
        sys.exit('--seconds must be in (0, 10]')

    print(f'port {a.port} @ {a.baud}, camera cp={a.cp:.1f} deg, {a.seconds:g}s')
    print('Left claw -30 deg first, then right claw -30 deg. Ctrl+C stops.')
    print('First CMD:', encode(0, 0., 0., -30., 0., a.cp))
    if not a.yes and input('Press Enter to start (anything else + Enter to abort): ').strip():
        sys.exit('aborted')

    try:
        fd = open_port(a.port, a.baud)
    except OSError as e:
        sys.exit(f'FAIL: cannot open {a.port}: {e} (port busy, wrong name or no permission?)')
    tx = Sender(fd)
    last_angles = (0., 0., 0.)
    try:
        n = 0
        while True:
            n += 1
            print(f'cycle {n}: left claw -30 deg')
            last_angles = (-30., 0., a.cp)
            tx.hold(*last_angles, a.seconds)
            print(f'cycle {n}: right claw -30 deg (left stays -30 deg)')
            last_angles = (-30., -30., a.cp)
            tx.hold(*last_angles, a.seconds)
            if not a.loop:
                break
            print(f'cycle {n}: home')
            last_angles = (0., 0., 0.)
            tx.hold(*last_angles, a.seconds)
        print('done')
    except KeyboardInterrupt:
        print('\ninterrupted')
    finally:
        end_lg, end_rg, end_cp = last_angles if a.keep else (0., 0., 0.)
        tx.stop(end_lg, end_rg, end_cp)
        os.close(fd)
        print(f'stop frames sent' + (f' (holding lg={end_lg:.1f}, rg={end_rg:.1f}, cp={end_cp:.1f})' if a.keep
              else ' (servos back to 0 deg)'))


if __name__ == '__main__':
    main()
