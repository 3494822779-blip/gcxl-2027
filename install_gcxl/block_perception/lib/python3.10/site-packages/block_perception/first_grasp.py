"""Image-space candidate selection for the first class-0 grasp."""
import math


def candidates(boxes, overlap=0.0):
    """Return (action box, clearance) pairs, direct class-0 grasps first.

    Exclusion strip: target left/right, image top to target top. Overlap is
    intersection area / class-1 box area (not IoU). Foreground is estimated
    using a larger bottom coordinate and positive horizontal overlap.
    """
    boxes = [b for b in boxes if b.width > 0 and b.height > 0 and
             all(math.isfinite(v) for v in (b.x, b.y, b.width, b.height))]
    result = []
    for target in boxes:
        if target.class_id != 0 or not target.observed or target.y <= 0:
            continue
        def horizontal(b):
            return max(0., min(target.x + target.width, b.x + b.width) - max(target.x, b.x))
        unsafe = any(b.class_id == 1 and
                     horizontal(b) * max(0., min(target.y, b.y + b.height) - max(0., b.y))
                     / (b.width * b.height) > overlap for b in boxes)
        if unsafe:
            continue
        front = [b for b in boxes if b.track_id != target.track_id and horizontal(b) > 0 and
                 b.y + b.height > target.y + target.height]
        if front:
            nearest = max(front, key=lambda b: (b.y + b.height, b.confidence, -b.track_id))
            if nearest.observed:
                result.append((nearest, True))
        else:
            result.append((target, False))
    return sorted(result, key=lambda item: (item[1], -(item[0].y + item[0].height),
                                           -item[0].confidence, item[0].track_id))
