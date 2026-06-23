"""
Canonical ASL feature extraction — imported by BOTH training and inference
so the two can never drift apart.

Key property: features are SCALE-INVARIANT. Every measurement is divided by the
hand's own size (wrist -> middle-finger MCP distance), so a sign produces the
same numbers whether the hand fills the frame (dataset) or is small (webcam).
"""

import math

DISTANCE_PAIRS = [
    (4, 8), (4, 12), (4, 16), (4, 20),
    (8, 12), (12, 16), (16, 20),
    (4, 0), (8, 0), (12, 0), (16, 0), (20, 0),
]


def _dist(a, b):
    return math.sqrt((a.x - b.x) ** 2 + (a.y - b.y) ** 2 + (a.z - b.z) ** 2)


def landmarks_to_features(hand):
    """hand: list of 21 MediaPipe landmarks. Returns scale-invariant feature row."""
    wrist = hand[0]
    mid_mcp = hand[9]

    # Hand-size reference — used to normalize everything
    scale = _dist(wrist, mid_mcp)
    if scale < 1e-6:
        scale = 1e-6

    row = []
    for lm in hand:
        row.extend([
            (lm.x - wrist.x) / scale,
            (lm.y - wrist.y) / scale,
            (lm.z - wrist.z) / scale,
        ])

    for a, b in DISTANCE_PAIRS:
        row.append(_dist(hand[a], hand[b]) / scale)

    return row
