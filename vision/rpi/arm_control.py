"""
Game-controller arm controls, shared by drive.py and main.py --manual.

  D-pad left/right   base      (turn the arm)
  D-pad up/down      shoulder  (main arm forward/back)
  RB / LB            elbow     (front arm up/down)
  Y                  close gripper (grab)
  X                  open gripper (release)
  B / Circle         auto-grab on/off (closes by itself when something is in the jaws)
  Start / Menu       home pose

A is left alone (it toggles AUTO/MANUAL in main.py); View brakes in drive.py.
Hold a D-pad direction or bumper to keep a joint moving. The ESP32 clamps every
joint to its measured safe range; the same limits are mirrored here so a held
button doesn't keep counting past the end.
"""

LIMITS  = {0: (10, 170), 1: (20, 120), 2: (45, 155), 3: (50, 140)}   # keep in sync with arm.cpp
HOME    = {0: 90, 1: 20, 2: 135}
OPEN    = 50
GRIP    = 140
AUTO_CH = 99          # "SERVO 99 <deg>" = auto-grab on at <deg>, 0 = off (see arm.h)

BTN_B, BTN_X, BTN_Y, BTN_LB, BTN_RB, BTN_START = 1, 2, 3, 4, 5, 7      # Xbox layout in SDL


class ArmPad:
    def __init__(self, js, send_servo, grip=GRIP, deg_per_s=60):
        self.js = js
        self.send = send_servo               # send(ch, deg)
        self.grip = grip
        self.rate = deg_per_s
        self.pos = {ch: float(d) for ch, d in HOME.items()}
        self.auto = False
        self._prev = {}

    def _pressed(self, b):
        """True once per press (rising edge)."""
        down = self.js.get_numbuttons() > b and bool(self.js.get_button(b))
        edge = down and not self._prev.get(b, False)
        self._prev[b] = down
        return edge

    def _held(self, b):
        return self.js.get_numbuttons() > b and bool(self.js.get_button(b))

    def home(self):
        for ch, deg in HOME.items():
            self.pos[ch] = float(deg)
            self.send(ch, deg)

    def set_auto(self, on):
        self.auto = on
        self.send(AUTO_CH, self.grip if on else 0)
        print(f"\nAuto-grab {'ON (grip ' + str(self.grip) + ')' if on else 'OFF'}")

    def poll(self, dt):
        """Call every loop with the seconds since the last call (after pygame events are pumped)."""
        if self._pressed(BTN_Y):
            self.send(3, self.grip)
        if self._pressed(BTN_X):
            self.send(3, OPEN)
        if self._pressed(BTN_B):
            self.set_auto(not self.auto)
        if self._pressed(BTN_START):
            self.home()

        hx, hy = self.js.get_hat(0) if self.js.get_numhats() > 0 else (0, 0)
        step = self.rate * dt
        self._jog(0, -hx * step)                                   # D-pad right = turn right
        self._jog(1, hy * step)                                    # D-pad up = shoulder forward
        self._jog(2, (self._held(BTN_RB) - self._held(BTN_LB)) * step)

    def _jog(self, ch, delta):
        if not delta:
            return
        lo, hi = LIMITS[ch]
        old = int(round(self.pos[ch]))
        self.pos[ch] = max(lo, min(hi, self.pos[ch] + delta))
        new = int(round(self.pos[ch]))
        if new != old:
            self.send(ch, new)
