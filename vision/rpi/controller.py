class PhiloController:
    MAX_SPEED = 200   # tune to your ESP32 PWM range
    MIN_SPEED = 60    # minimum to overcome motor stiction
    GESTURE_FRAMES = 5
    DEAD_ZONE = 0.1   # fraction of frame width, no steering in this band

    # Bounding box area thresholds at imgsz=320 — bigger = closer
    NEAR_AREA = 12000
    FAR_AREA  = 3000

    def __init__(self):
        self._stop_count   = 0
        self._resume_count = 0
        self._gesture_stop = False

    def update_gesture(self, kpt_ys):
        """
        kpt_ys: (lw_y, rw_y, ls_y, rs_y) from tracker, or None.
        In image coords Y increases downward, so wrist_y < shoulder_y means raised.
        Returns True if gesture-stopped.
        """
        if kpt_ys is None:
            self._stop_count = self._resume_count = 0
            return self._gesture_stop

        lw_y, rw_y, ls_y, rs_y = kpt_ys
        raised = (lw_y < ls_y) or (rw_y < rs_y)

        if raised:
            self._stop_count   += 1
            self._resume_count  = 0
        else:
            self._resume_count += 1
            self._stop_count    = 0

        if self._stop_count   >= self.GESTURE_FRAMES:
            self._gesture_stop = True
        if self._resume_count >= self.GESTURE_FRAMES:
            self._gesture_stop = False

        return self._gesture_stop

    def compute_cmd(self, bbox, frame_w):
        """
        Returns (left, right, flags).
        bbox: [x1, y1, x2, y2] or None.
        """
        if bbox is None:
            return 0, 0, 0

        x1, y1, x2, y2 = bbox
        cx   = (x1 + x2) / 2
        area = (x2 - x1) * (y2 - y1)

        # Speed: further away → faster, closer → slower
        t = max(0.0, min(1.0, (area - self.FAR_AREA) / (self.NEAR_AREA - self.FAR_AREA)))
        base = int(self.MAX_SPEED - t * (self.MAX_SPEED - self.MIN_SPEED))

        # Steering: offset from -1 (full left) to +1 (full right)
        offset = (cx - frame_w / 2) / (frame_w / 2)

        if abs(offset) < self.DEAD_ZONE:
            return base, base, 0
        elif offset > 0:                         # person right → turn right
            return base, int(base * (1 - abs(offset))), 0
        else:                                    # person left → turn left
            return int(base * (1 - abs(offset))), base, 0
