"""Single-plane MJPEG capture using the Linux V4L2 streaming API directly."""
import ctypes as C
import errno
import fcntl
from fractions import Fraction
import mmap
import os
import select


U32, S32 = C.c_uint32, C.c_int32


class PixelFormat(C.Structure):
    _fields_ = [(n, U32) for n in ('width', 'height', 'pixelformat', 'field',
                'bytesperline', 'sizeimage', 'colorspace', 'priv', 'flags',
                'ycbcr_enc', 'quantization', 'xfer_func')]


class FormatData(C.Union):
    _fields_ = [('pix', PixelFormat), ('raw', C.c_uint8 * 200), ('alignment', C.c_void_p)]


class Format(C.Structure):
    _fields_ = [('type', U32), ('fmt', FormatData)]


class StreamParm(C.Structure):
    _fields_ = [('type', U32), ('capability', U32), ('capturemode', U32),
                ('numerator', U32), ('denominator', U32), ('extendedmode', U32),
                ('readbuffers', U32), ('reserved', C.c_uint8 * 176)]


class RequestBuffers(C.Structure):
    _fields_ = [('count', U32), ('type', U32), ('memory', U32),
                ('capabilities', U32), ('flags', C.c_uint8), ('reserved', C.c_uint8 * 3)]


class Timeval(C.Structure):
    _fields_ = [('sec', C.c_long), ('usec', C.c_long)]


class BufferMemory(C.Union):
    _fields_ = [('offset', U32), ('userptr', C.c_ulong), ('planes', C.c_void_p), ('fd', S32)]


class Buffer(C.Structure):
    _fields_ = [('index', U32), ('type', U32), ('bytesused', U32), ('flags', U32),
                ('field', U32), ('timestamp', Timeval), ('timecode', C.c_uint8 * 16),
                ('sequence', U32), ('memory', U32), ('m', BufferMemory),
                ('length', U32), ('reserved2', U32), ('request_fd', S32)]


class QueryControl(C.Structure):
    _fields_ = [('id', U32), ('type', U32), ('name', C.c_char * 32),
                ('minimum', S32), ('maximum', S32), ('step', S32),
                ('default_value', S32), ('flags', U32), ('reserved', U32 * 2)]


class Control(C.Structure):
    _fields_ = [('id', U32), ('value', S32)]


def request(number, struct, direction=3):
    return (direction << 30) | (C.sizeof(struct) << 16) | (ord('V') << 8) | number


S_FMT = request(5, Format)
REQBUFS = request(8, RequestBuffers)
QUERYBUF = request(9, Buffer)
QBUF = request(15, Buffer)
DQBUF = request(17, Buffer)
STREAMON = request(18, U32, 1)
STREAMOFF = request(19, U32, 1)
S_PARM = request(22, StreamParm)
G_CTRL = request(27, Control)
S_CTRL = request(28, Control)
QUERYCTRL = request(36, QueryControl)
MJPG = int.from_bytes(b'MJPG', 'little')
AUTO_CONTROLS = {0x0098090C, 0x00980912, 0x00980919, 0x009A0901, 0x009A090C, 0x009A0914}


class V4L2Capture:
    """Own the fd and mmap buffers. Only the reader thread may read/close it."""

    def __init__(self, source, width, height, fps, log=lambda message: None):
        self.fd = -1
        self.buffers = []
        self.streaming = False
        self.sequence = None
        self.sequence_gaps = 0
        self.error_frames = 0
        self.defaults = {}
        try:
            self.fd = os.open(source, os.O_RDWR | os.O_NONBLOCK | os.O_CLOEXEC)
            fmt = Format(type=1)
            fmt.fmt.pix = PixelFormat(width=width, height=height, pixelformat=MJPG)
            self._ioctl(S_FMT, fmt)
            pix = fmt.fmt.pix
            if (pix.width, pix.height, pix.pixelformat) != (width, height, MJPG):
                raise RuntimeError(f'Camera negotiated {pix.width}x{pix.height}, '
                                   f'fourcc={pix.pixelformat:#x}; expected {width}x{height} MJPG')
            rate = Fraction(str(fps)).limit_denominator(1000000)
            parm = StreamParm(type=1, numerator=rate.denominator, denominator=rate.numerator)
            self._ioctl(S_PARM, parm)
            self.actual_fps = parm.denominator / parm.numerator if parm.numerator else 0.
            self.restore_defaults()
            log(f'V4L2 {width}x{height} MJPG, negotiated {self.actual_fps:g} fps; '
                f'default controls: {self.defaults}')
            req = RequestBuffers(count=4, type=1, memory=1)
            self._ioctl(REQBUFS, req)
            if req.count < 2:
                raise RuntimeError('V4L2 requires at least two mmap buffers')
            for index in range(req.count):
                buf = Buffer(index=index, type=1, memory=1)
                self._ioctl(QUERYBUF, buf)
                self.buffers.append(mmap.mmap(self.fd, buf.length, flags=mmap.MAP_SHARED,
                                             prot=mmap.PROT_READ | mmap.PROT_WRITE,
                                             offset=buf.m.offset))
                self._ioctl(QBUF, buf)
            self._ioctl(STREAMON, U32(1))
            self.streaming = True
        except BaseException:
            self.release()
            raise

    def _ioctl(self, command, value):
        while True:
            try:
                fcntl.ioctl(self.fd, command, value)
                return
            except OSError as exc:
                if exc.errno != errno.EINTR:
                    raise

    def restore_defaults(self):
        controls = []
        next_id = 0x80000000  # V4L2_CTRL_FLAG_NEXT_CTRL
        while True:
            query = QueryControl(id=next_id)
            try:
                self._ioctl(QUERYCTRL, query)
            except OSError as exc:
                if exc.errno == errno.EINVAL:
                    break
                raise
            next_id = query.id | 0x80000000
            # Standard user/camera controls only; never invoke buttons or relative motion.
            if query.id & 0xFFFF0000 not in (0x00980000, 0x009A0000):
                continue
            if query.id in (0x009A0904, 0x009A0905, 0x009A0908, 0x009A0909,
                            0x009A090E, 0x009A090F, 0x009A0920, 0x009A0921):
                continue
            if query.type in (1, 2, 3, 9):
                controls.append(query)
        controls.sort(key=lambda q: q.id not in AUTO_CONTROLS)
        for original in controls:
            query = QueryControl(id=original.id)
            self._ioctl(QUERYCTRL, query)  # Auto modes can change inactive flags.
            if query.flags & (1 | 2 | 4 | 16 | 64 | 128):
                continue  # disabled, grabbed, read-only, inactive, write-only, volatile
            name = query.name.decode(errors='replace')
            try:
                self._ioctl(S_CTRL, Control(id=query.id, value=query.default_value))
                value = Control(id=query.id)
                self._ioctl(G_CTRL, value)
                if value.value != query.default_value:
                    raise RuntimeError(f'readback {value.value}, expected {query.default_value}')
            except (OSError, RuntimeError) as exc:
                raise RuntimeError(f'Cannot restore default control {name}: {exc}') from exc
            self.defaults[name] = value.value

    def read(self, timeout=0.5):
        """Return owned JPEG bytes, or None on timeout/transient/error frame."""
        if not select.select([self.fd], [], [], timeout)[0]:
            return None
        buf = Buffer(type=1, memory=1)
        try:
            self._ioctl(DQBUF, buf)
        except OSError as exc:
            if exc.errno == errno.EAGAIN:
                return None
            raise
        try:
            if self.sequence is not None:
                delta = (buf.sequence - self.sequence) & 0xFFFFFFFF
                if 1 < delta < 0x80000000:
                    self.sequence_gaps += delta - 1
            self.sequence = buf.sequence
            if buf.flags & 0x40 or not buf.bytesused:
                self.error_frames += 1
                return None
            if buf.index >= len(self.buffers) or buf.bytesused > len(self.buffers[buf.index]):
                raise RuntimeError('Invalid V4L2 buffer index/length')
            return self.buffers[buf.index][:buf.bytesused]
        finally:
            self._ioctl(QBUF, buf)

    def release(self):
        if self.fd < 0:
            return
        try:
            if self.streaming:
                try:
                    self._ioctl(STREAMOFF, U32(1))
                except OSError:
                    pass  # Disconnected devices still need local resources released.
        finally:
            self.streaming = False
            for buffer in self.buffers:
                buffer.close()
            self.buffers.clear()
            os.close(self.fd)
            self.fd = -1
