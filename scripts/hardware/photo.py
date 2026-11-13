#!/usr/bin/env python3
"""拍照工具：用 perception.yaml 里的相机参数拍照（独立脚本，不需要 ROS）。

相机参数（source / width / height / fps）默认从
ros2_ws/src/block_perception/config/perception.yaml 的 camera 小节读取，
采集流水线与 block_perception 的 camera 节点完全相同（GStreamer JPEG 解码），
所以拍到的画面与感知节点看到的一致。拍照时不要同时跑感知 launch
（同一台相机不能被两个进程打开）。

  scripts/photo.py                            # 预览窗口，空格/s 拍照，q 退出
  scripts/photo.py --shots 20 --interval 0.5  # 自动连拍 20 张，间隔 0.5 s（interval 0 = 每帧都拍）
  scripts/photo.py --shots 5 --no-preview     # 无窗口（SSH）连拍
  scripts/photo.py --width 1280 --height 720  # 覆盖 yaml 里的分辨率
  scripts/photo.py --guides                   # 叠加对准参考线（画面中线 + 下边缘目标带），
                                              # 参考线会一起存进照片；bottom_target/deadzone_y
                                              # 取自 yaml 的 alignment 小节，可用于标定
  scripts/photo.py --yaml 别的.yaml --out /tmp/p   # 换参数文件、输出目录

照片存为 <out>/photo_YYYYmmdd_HHMMSS_mmm.jpg（默认 ./photos）。
"""
import argparse
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import yaml
except ImportError:
    yaml = None

import cv2

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_YAML = REPO_ROOT / 'ros2_ws/src/block_perception/config/perception.yaml'


def load_params(path):
    """Read all sections from the perception yaml (camera is required)."""
    if yaml is None:
        sys.exit('FAIL: PyYAML not installed (pip install pyyaml)')
    if not path.is_file():
        sys.exit(f'FAIL: config not found: {path}')
    data = yaml.safe_load(path.read_text(encoding='utf-8'))
    try:
        data['camera']['ros__parameters']
    except (KeyError, TypeError):
        sys.exit(f'FAIL: no camera section in {path}')
    return data


def open_capture(source, width, height, fps):
    """Same pipeline as block_perception's camera node."""
    is_device = isinstance(source, str) and source.startswith('/dev/video')
    has_gst = 'GStreamer:                   YES' in cv2.getBuildInformation()
    if is_device and has_gst:
        pipeline = (f'v4l2src device={source} ! '
                    f'image/jpeg,width={width},height={height},framerate={int(fps)}/1 ! '
                    'jpegdec ! videoconvert ! video/x-raw,format=BGR ! '
                    'appsink drop=true max-buffers=1 sync=false')
        cap = cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)
    elif is_device:
        cap = cv2.VideoCapture(source, cv2.CAP_V4L2)
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        cap.set(cv2.CAP_PROP_FPS, fps)
    else:
        cap = cv2.VideoCapture(source)
    if not cap.isOpened():
        sys.exit(f'FAIL: cannot open camera/video source: {source} '
                 f'(busy, wrong name or no permission?)')
    return cap


def alignment_band(params):
    """bottom_target / deadzone_y from the alignment section, for --guides."""
    align = params.get('alignment', {}).get('ros__parameters', {})
    return float(align.get('bottom_target', 0.9)), float(align.get('deadzone_y', 0.2))


def draw_guides(frame, bottom_target, deadzone_y):
    """Burn the alignment reference lines into the frame (also into saved photos)."""
    h, w = frame.shape[:2]
    green = (0, 255, 0)
    cv2.line(frame, (w // 2, 0), (w // 2, h), green, 1)          # vertical centre
    cv2.line(frame, (0, h // 2), (w, h // 2), green, 1)          # horizontal centre
    target_y = int(bottom_target * h)                            # bottom edge target
    band = int(deadzone_y * h / 2)                               # error_y = (y - target)/ (h/2)
    cv2.line(frame, (0, target_y), (w, target_y), green, 2)
    cv2.line(frame, (0, target_y - band), (w, target_y - band), green, 1)
    cv2.line(frame, (0, target_y + band), (w, target_y + band), green, 1)


class Preview:
    """Best-effort preview window; falls back to headless when no display is available."""
    def __init__(self, enabled):
        self.visible = False
        self._enabled = enabled
        self._ok = True

    def tick(self, frame):
        """Show the frame and return the pressed key (or -1; headless -> always -1)."""
        if not self._enabled or not self._ok:
            return -1
        try:
            cv2.imshow('photo', frame)
            self.visible = True
            return cv2.waitKey(1) & 0xFF
        except cv2.error as e:
            print(f'note: preview unavailable ({e}); falling back to headless')
            self._ok = False
            return -1


def save(out_dir, frame):
    name = datetime.now().strftime('photo_%Y%m%d_%H%M%S_%f')[:-3] + '.jpg'
    path = out_dir / name
    if not cv2.imwrite(str(path), frame, [cv2.IMWRITE_JPEG_QUALITY, 95]):
        print(f'FAIL: cannot write {path}')
        return False
    print(f'saved {path}')
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--yaml', default=str(DEFAULT_YAML),
                    help='parameter file (default: block_perception perception.yaml)')
    ap.add_argument('--source', help='override camera.source')
    ap.add_argument('--width', type=int, help='override camera.width')
    ap.add_argument('--height', type=int, help='override camera.height')
    ap.add_argument('--fps', type=float, help='override camera.fps')
    ap.add_argument('--out', default='photos', help='output directory (default: ./photos)')
    ap.add_argument('--shots', type=int, default=0,
                    help='auto-capture N photos then exit (0 = manual, key press)')
    ap.add_argument('--interval', type=float, default=0.,
                    help='seconds between auto shots (0 = every frame)')
    ap.add_argument('--no-preview', action='store_true', help='do not open a preview window')
    ap.add_argument('--guides', action='store_true',
                    help='burn alignment guide lines (centre + bottom band) into the photos')
    a = ap.parse_args()

    params = load_params(Path(a.yaml))
    cam = params['camera']['ros__parameters']
    source = a.source or cam.get('source', '/dev/video0')
    width = a.width or int(cam.get('width', 640))
    height = a.height or int(cam.get('height', 480))
    fps = a.fps or float(cam.get('fps', 30.0))
    if width <= 0 or height <= 0 or fps <= 0:
        sys.exit('FAIL: invalid width/height/fps')

    cap = open_capture(source, width, height, fps)
    out_dir = Path(a.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'camera: {source} {width}x{height} @ {fps} fps; output: {out_dir.resolve()}/')
    # Warm up: the first frames still have the exposure/white balance settling.
    for _ in range(10):
        cap.read()

    bottom_target = deadzone_y = None
    if a.guides:
        bottom_target, deadzone_y = alignment_band(params)
        print(f'guides: bottom_target={bottom_target}, deadzone_y={deadzone_y} (burned into photos)')

    preview = Preview(not a.no_preview)
    saved = auto_taken = 0
    next_shot = 0.0
    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print('FAIL: camera read failed or file ended')
                return 1
            if a.guides:
                draw_guides(frame, bottom_target, deadzone_y)
            key = preview.tick(frame)
            now = time.monotonic()
            if key in (ord('q'), 27):
                break
            if key in (ord(' '), ord('s'), 13):
                saved += save(out_dir, frame)
            if a.shots > 0 and auto_taken < a.shots and now >= next_shot:
                auto_taken += 1
                saved += save(out_dir, frame)
                next_shot = now + a.interval
                if auto_taken >= a.shots:
                    break
            if a.shots <= 0 and not preview.visible:
                print('FAIL: no display and --shots not set; use --shots N for headless capture')
                return 1
    except KeyboardInterrupt:
        pass
    finally:
        cap.release()
        cv2.destroyAllWindows()
    print(f'{saved} photo(s) in {out_dir}/')
    return 0


if __name__ == '__main__':
    sys.exit(main())
