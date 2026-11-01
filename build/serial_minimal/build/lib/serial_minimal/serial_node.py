#!/usr/bin/env python3
import os
import termios
import tty

import rclpy
from rclpy.node import Node
from std_msgs.msg import String


BAUD = {9600: termios.B9600, 19200: termios.B19200, 38400: termios.B38400,
        57600: termios.B57600, 115200: termios.B115200}


class SerialNode(Node):
    def __init__(self):
        super().__init__('serial_node')
        self.declare_parameter('port', '/dev/ttyUSB0')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('poll_period', 0.01)
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

    def destroy_node(self):
        if getattr(self, 'fd', None) is not None:
            os.close(self.fd)
            self.fd = None
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = SerialNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
