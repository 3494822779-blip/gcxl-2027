#!/usr/bin/env python3
"""Serial send/receive check for the MCU link (protocol: docs/serial_protocol.md).

Standalone (no ROS). Stop the ROS serial_node first: only one process can use the port.

Sends $CMD frames at 20 Hz (default: wheels stopped, claws at the initial angle, camera
speed 0, so nothing moves) and counts the $FB feedback frames coming back.

  scripts/serial_check.py --port /dev/ttyUSB0            # with the MCU connected
  scripts/serial_check.py --port /dev/ttyUSB0 --loopback # TX and RX wired together, no MCU

Exit code 0 = pass, 1 = fail.
"""
import argparse
import os
import select
import sys
import termios
import time
import tty

BAUD = {9600: termios.B9600, 19200: termios.B19200, 38400: termios.B38400,
        57600: termios.B57600, 115200: termios.B115200}


def checksum(body):  # same as block_perception.actuator.checksum
    value = 0
    for ch in body.encode():
        value ^= ch
    return f'{value:02X}'


def encode(seq, lw, rw, lg, rg, cp):
    body = f'CMD,{seq},{lw:.3f},{rw:.3f},{lg:.1f},{rg:.1f},{cp:.3f}'
    return f'${body}*{checksum(body)}'


def parse(line):
    """Return (kind, fields) for a valid frame, ('bad', reason) otherwise."""
    line = line.strip()
    if not line.startswith('$') or '*' not in line:
        return 'bad', 'not a frame'
    body, _, tail = line[1:].partition('*')
    if tail.upper() != checksum(body):
        return 'bad', 'checksum'
    fields = body.split(',')
    try:
        if fields[0] == 'FB' and len(fields) == 3:
            return 'FB', (int(fields[1]), float(fields[2]))
        if fields[0] == 'CMD' and len(fields) == 7:
            return 'CMD', (int(fields[1]),) + tuple(float(f) for f in fields[2:])
    except ValueError:
        pass
    return 'bad', 'fields'


def open_port(path, baud):
    if baud not in BAUD:
        sys.exit(f'unsupported baud {baud}; choose from {sorted(BAUD)}')
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    tty.setraw(fd)
    attrs = termios.tcgetattr(fd)
    attrs[2] |= termios.CLOCAL | termios.CREAD
    attrs[4] = attrs[5] = BAUD[baud]
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)
    return fd


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', default='/dev/ttyUSB0')
    ap.add_argument('--baud', type=int, default=115200)
    ap.add_argument('--duration', type=float, default=5., help='seconds to run (default 5)')
    ap.add_argument('--rate', type=float, default=20., help='CMD frames per second (default 20)')
    ap.add_argument('--loopback', action='store_true', help='expect our own CMD frames back (TX wired to RX)')
    ap.add_argument('--lw', type=float, default=0.); ap.add_argument('--rw', type=float, default=0.)
    ap.add_argument('--lg', type=float, default=20.); ap.add_argument('--rg', type=float, default=20.)
    ap.add_argument('--cp', type=float, default=0., help='camera pitch ANGLE in degrees (0 = home)')
    a = ap.parse_args()
    if any(v for v in (a.lw, a.rw)) or a.cp:
        print('WARNING: non-zero wheel command or camera angle; the robot will move. Ctrl+C stops.')
    print(f'note: camera servo is commanded to {a.cp:.1f} deg (0 = home)')

    try:
        fd = open_port(a.port, a.baud)
    except OSError as e:
        sys.exit(f'FAIL: cannot open {a.port}: {e} (port busy, wrong name or no permission? try the dialout group)')

    sent = errors = 0
    kinds = {'FB': 0, 'CMD': 0, 'bad': 0}
    bad_samples, last_fb, fb_seqs, pitches = [], None, [], []
    buf, rx_bytes, rx_sample = b'', 0, b''
    t0 = time.monotonic()
    next_tx = t0
    try:
        while time.monotonic() - t0 < a.duration:
            now = time.monotonic()
            if now >= next_tx:
                try:
                    os.write(fd, encode(sent % 65536, a.lw, a.rw, a.lg, a.rg, a.cp).encode() + b'\n')
                    sent += 1
                except OSError as e:
                    errors += 1
                    print(f'write error: {e}')
                next_tx += 1 / a.rate
            ready, _, _ = select.select([fd], [], [], max(0., min(next_tx - time.monotonic(), .02)))
            if ready:
                try:
                    chunk = os.read(fd, 4096)
                except BlockingIOError:
                    chunk = b''
                rx_bytes += len(chunk)
                if len(rx_sample) < 120:
                    rx_sample += chunk[:120 - len(rx_sample)]
                buf += chunk
                *lines, buf = buf.split(b'\n')
                buf = buf[-512:]
                for raw in lines:
                    text = raw.decode(errors='replace').strip()
                    if not text:
                        continue
                    kind, data = parse(text)
                    kinds[kind] += 1
                    if kind == 'FB':
                        fb_seqs.append(data[0]); pitches.append(data[1]); last_fb = data
                    elif kind == 'bad' and len(bad_samples) < 3:
                        bad_samples.append((text[:60], data))
    except KeyboardInterrupt:
        pass
    finally:
        os.close(fd)

    elapsed = time.monotonic() - t0
    print(f'port {a.port} @ {a.baud}, {elapsed:.1f}s')
    print(f'sent  : {sent} CMD frames ({sent/elapsed:.1f}/s), write errors {errors}')
    print(f'raw rx: {rx_bytes} bytes' + (f', first bytes {rx_sample!r}' if rx_bytes else ''))
    print(f'recv  : FB {kinds["FB"]}, CMD(echo) {kinds["CMD"]}, bad {kinds["bad"]}')
    for text, why in bad_samples:
        print(f'  bad frame ({why}): {text!r}')
    if pitches:
        gaps = sum(1 for p, q in zip(fb_seqs, fb_seqs[1:]) if (q - p) % 65536 != 1)
        print(f'FB    : {kinds["FB"]/elapsed:.1f}/s, pitch {min(pitches):.1f}..{max(pitches):.1f} deg, '
              f'last {last_fb[1]:.1f}, seq gaps {gaps}')

    problems = []
    if errors or not sent:
        problems.append('TX failed')
    if a.loopback:
        if kinds['CMD'] < 0.9 * sent - 2:
            problems.append(f'loopback echoed {kinds["CMD"]}/{sent} frames: check TX-RX wire / baud')
    else:
        if kinds['FB'] == 0 and rx_bytes == 0:
            problems.append('no bytes received at all: check wiring (TX/RX swapped?), baud, common GND, '
                            'whether the MCU firmware sends $FB (USB-CDC boards may reset when the port opens)')
        elif kinds['FB'] == 0:
            problems.append(f'received {rx_bytes} bytes but no valid $FB line: compare the bytes above with '
                            'docs/serial_protocol.md (frames must end with \\n)')
        elif kinds['FB'] / elapsed < 15:
            problems.append(f'FB rate {kinds["FB"]/elapsed:.1f}/s < 15/s (spec: >= 20/s)')
    if kinds['bad']:
        problems.append(f'{kinds["bad"]} corrupted/invalid frames: check baud, line noise, checksum implementation')
    if problems:
        print('FAIL: ' + '; '.join(problems))
        return 1
    print('PASS')
    return 0


if __name__ == '__main__':
    sys.exit(main())
