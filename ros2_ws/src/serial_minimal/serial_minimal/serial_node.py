#!/usr/bin/env python3
import os
import signal
import termios
import threading
import time
import tty

import rclpy
from rclpy.node import Node
from rclpy.executors import SingleThreadedExecutor
from rclpy.signals import SignalHandlerOptions
from std_msgs.msg import String


BAUD = {9600: termios.B9600, 19200: termios.B19200, 38400: termios.B38400,
        57600: termios.B57600, 115200: termios.B115200}


class SerialNode(Node):
    def __init__(self):
        super().__init__('serial_node')
        self.declare_parameter('port', '/dev/ttyUSB0')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('poll_period', 0.01)
        self.declare_parameter('shutdown_grace', 0.5)
        self.port = self.get_parameter('port').value
        baud = int(self.get_parameter('baud').value)
        if baud not in BAUD:
            raise ValueError(f'Unsupported baud: {baud}')
        self.fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        tty.setraw(self.fd)
        attrs = termios.tcgetattr(self.fd)
        attrs[4] = attrs[5] = BAUD[baud]
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        self.rx_pub = self.create_publisher(String, 'serial_rx', 10)
        self.tx_sub = self.create_subscription(String, 'serial_tx', self.write, 10)
        self.timer = self.create_timer(float(self.get_parameter('poll_period').value), self.read)
        self.get_logger().info(f'Opened {self.port} @ {baud} baud')

    def write(self, msg):
        try:
            os.write(self.fd, msg.data.encode() + b'\n')
        except OSError as exc:
            self.get_logger().error(f'Write failed: {exc}')

    def read(self):
        try:
            data = os.read(self.fd, 4096)
        except BlockingIOError:
            return
        except OSError as exc:
            self.get_logger().error(f'Read failed: {exc}')
            return
        if data:
            msg = String()
            msg.data = data.decode(errors='replace')
            self.rx_pub.publish(msg)

    def drain(self):
        """Keep writing what other nodes still publish, so their last frames (the actuator's stop
        frames) reach the MCU before the port closes."""
        grace = float(self.get_parameter('shutdown_grace').value)
        deadline = time.monotonic() + max(grace, 0.)
        while time.monotonic() < deadline and rclpy.ok():
            yield

    def destroy_node(self):
        if getattr(self, 'fd', None) is not None:
            os.close(self.fd)
            self.fd = None
        super().destroy_node()


STOP = threading.Event()


def main(args=None):
    # Handle SIGINT/SIGTERM here instead of letting rclpy invalidate the context first: the port
    # stays open for shutdown_grace seconds so the actuator's stop frames are still written.
    rclpy.init(args=args, signal_handler_options=SignalHandlerOptions.NO)
    for sig in (signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, lambda *_: STOP.set())
    node = SerialNode()
    executor = SingleThreadedExecutor()
    executor.add_node(node)
    try:
        while rclpy.ok() and not STOP.is_set():
            executor.spin_once(timeout_sec=0.1)
        for _ in node.drain():
            executor.spin_once(timeout_sec=0.02)
    finally:
        # launch forwards SIGINT again during teardown; rclpy.shutdown() would restore the default.
        for sig in (signal.SIGINT, signal.SIGTERM):
            signal.signal(sig, signal.SIG_IGN)
        executor.shutdown()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
