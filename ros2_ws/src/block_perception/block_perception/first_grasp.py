"""Geometry for explicit-class target selection and single-obstacle sweeping."""
import math


def valid_boxes(boxes):
    return [b for b in boxes if b.observed and b.width > 0 and b.height > 0
            and all(math.isfinite(v) for v in (b.x, b.y, b.width, b.height, b.confidence))]


def candidates(boxes, target_class):
    return sorted((b for b in valid_boxes(boxes) if b.class_id == target_class),
                  key=lambda b: (-(b.y + b.height), -b.confidence, b.track_id))


def obstacle_for(target, boxes):
    """Return (box, side, horizontal fraction of target, actual area overlap)."""
    front = []
    for b in valid_boxes(boxes):
        width = min(target.x + target.width, b.x + b.width) - max(target.x, b.x)
        if b.track_id == target.track_id or width <= 0 or b.y + b.height <= target.y + target.height:
            continue
        front.append((width, b))
    if not front:
        return None
    width, b = max(front, key=lambda item: (item[0], item[1].confidence, -item[1].track_id))
    side = -1 if b.x + b.width / 2 <= target.x + target.width / 2 else 1
    near = min(target.y + target.height, b.y + b.height) > max(target.y, b.y)
    return b, side, width / target.width, near
