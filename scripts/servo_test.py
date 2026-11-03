#!/usr/bin/env python3
"""Move the three servos (left claw, right claw, camera pitch), to check the wiring and
the angle sign (protocol: docs/serial_protocol.md).

THE CLAWS WILL MOVE. Keep fingers clear of the gripper.
Standalone (no ROS). Stop the ROS serial_node first: only one process can use the port.

Sends $CMD frames at 20 Hz with the wheels stopped (lw = rw = 0) and the three servos at
the given angles, then ends by sending home frames (servos back to 0 deg), also on Ctrl+C
or an error. --keep holds the last angles instead of going home.

Angles in degrees, same convention as docs/serial_protocol.md:
  lg: left claw, [0, 90], larger = more open
  rg: right claw, [0, 90], larger = more open
  cp: camera pitch, [-15, 30], positive = up, 0 = home
(Limits must match gripper/actuator in config/perception.yaml.)

  scripts/servo_test.py --port /dev/ttyACM0 --lg 60 --rg 60 --seconds 3   # both claws to 60 deg
  scripts/servo_test.py --port /dev/ttyACM0 --cp 30 --seconds 3           # camera up
  scripts/servo_test.py --port /dev/ttyACM0 --pattern open                # single preset
  scripts/servo_test.py --port /dev/ttyACM0 --lg 90 --keep                # hold at 90 deg after exit

--pattern runs a single preset: open (claws 90), close (claws 0), cam-up (cp 30),
cam-down (cp -15).
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from serial_check import encode, open_port  # noqa: E402

CLAW_MIN, CLAW_MAX = 0., 90.   # claw angle range (deg), see perception.yaml gripper/actuator
CAM_MIN, CAM_MAX = -15., 30.   # camera pitch range (deg)
RATE = 20.                     # CMD frames per second, same as the actuator node

PATTERNS = {
    'open': (90., 90., 0.),      # both claws fully open
    'close': (0., 0., 0.),       # both claws closed (init position)
    'cam-up': (0., 0., 30.),     # camera pitch up
    'cam-down': (0., 0., -15.),  # camera pitch down (grasp view)
}


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
    ap.add_argument('--lg', type=float, default=0., help=f'left claw angle in degrees [{CLAW_MIN:g}, {CLAW_MAX:g}]')
    ap.add_argument('--rg', type=float, default=0., help=f'right claw angle in degrees [{CLAW_MIN:g}, {CLAW_MAX:g}]')
    ap.add_argument('--cp', type=float, default=0., help=f'camera pitch angle in degrees [{CAM_MIN:g}, {CAM_MAX:g}]')
    ap.add_argument('--seconds', type=float, default=2., help='how long to hold the angles (default 2)')
    ap.add_argument('--pattern', choices=sorted(PATTERNS), help='run this preset instead of --lg/--rg/--cp')
    ap.add_argument('--keep', action='store_true', help='hold the last angles instead of going home at the end')
    ap.add_argument('--yes', action='store_true', help='skip the confirmation prompt')
    a = ap.parse_args()

    if a.pattern:
        a.lg, a.rg, a.cp = PATTERNS[a.pattern]
    if not CLAW_MIN <= a.lg <= CLAW_MAX:
        sys.exit(f'--lg must be in [{CLAW_MIN:g}, {CLAW_MAX:g}]')
    if not CLAW_MIN <= a.rg <= CLAW_MAX:
        sys.exit(f'--rg must be in [{CLAW_MIN:g}, {CLAW_MAX:g}]')
    if not CAM_MIN <= a.cp <= CAM_MAX:
        sys.exit(f'--cp must be in [{CAM_MIN:g}, {CAM_MAX:g}]')
    if not 0 < a.seconds <= 10:
        sys.exit('--seconds must be in (0, 10]')

    print(f'port {a.port} @ {a.baud}, lg={a.lg:.1f} rg={a.rg:.1f} cp={a.cp:.1f} deg, {a.seconds:g}s'
          + (f' (pattern {a.pattern})' if a.pattern else ''))
    print('The claws/camera servo will move. Keep fingers clear. Ctrl+C stops.')
    if not a.yes and input('Press Enter to start (anything else + Enter to abort): ').strip():
        sys.exit('aborted')

    try:
        fd = open_port(a.port, a.baud)
    except OSError as e:
        sys.exit(f'FAIL: cannot open {a.port}: {e} (port busy, wrong name or no permission?)')
    tx = Sender(fd)
    try:
        tx.hold(a.lg, a.rg, a.cp, a.seconds)
        print('done')
    except KeyboardInterrupt:
        print('\ninterrupted')
    finally:
        end_lg, end_rg, end_cp = (a.lg, a.rg, a.cp) if a.keep else (0., 0., 0.)
        tx.stop(end_lg, end_rg, end_cp)
        os.close(fd)
        print(f'stop frames sent' + (f' (holding lg={end_lg:.1f} rg={end_rg:.1f} cp={end_cp:.1f})' if a.keep
              else ' (servos back to 0 deg)'))


if __name__ == '__main__':
    main()
