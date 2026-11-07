#!/usr/bin/env python3
"""Move the camera pitch servo between -90 and +90 degrees to check wiring and
the angle sign (protocol: docs/serial_protocol.md).

Keep the camera linkage clear. The claws and wheels remain stopped.
Standalone (no ROS). Stop the ROS serial_node first: only one process can use the port.

Sends $CMD frames at 20 Hz with the wheels and claws stopped and the camera at
the requested angle, then ends by sending home frames (camera back to 0 deg), also on Ctrl+C
or an error. --keep holds the last angles instead of going home.

Angles in degrees, same convention as docs/serial_protocol.md:
  cp: camera pitch, [-90, 90], positive = up, 0 = home
(Limits must match gripper/actuator in config/perception.yaml.)

  scripts/servo_test.py --port /dev/ttyACM0 --cp 90 --seconds 3            # camera up
  scripts/servo_test.py --port /dev/ttyACM0 --cp -90 --seconds 3           # camera down
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
    print('Only the camera pitch servo will move. Ctrl+C stops.')
    print('First CMD:', encode(0, 0., 0., 0., 0., a.cp))
    if not a.yes and input('Press Enter to start (anything else + Enter to abort): ').strip():
        sys.exit('aborted')

    try:
        fd = open_port(a.port, a.baud)
    except OSError as e:
        sys.exit(f'FAIL: cannot open {a.port}: {e} (port busy, wrong name or no permission?)')
    tx = Sender(fd)
    try:
        if a.loop:
            n = 0
            while True:
                n += 1
                print(f'cycle {n}: target')
                tx.hold(0., 0., a.cp, a.seconds)
                print(f'cycle {n}: home')
                tx.hold(0., 0., 0., a.seconds)
        tx.hold(0., 0., a.cp, a.seconds)
        print('done')
    except KeyboardInterrupt:
        print('\ninterrupted')
    finally:
        end_lg, end_rg, end_cp = (0., 0., a.cp) if a.keep else (0., 0., 0.)
        tx.stop(end_lg, end_rg, end_cp)
        os.close(fd)
        print(f'stop frames sent' + (f' (holding camera cp={end_cp:.1f})' if a.keep
              else ' (servos back to 0 deg)'))


if __name__ == '__main__':
    main()
