"""
Xbox controller -> differential drive, for Philo manual mode.
Mapping (from gamepad_test.py): left stick X = axis0, left stick Y = axis1,
A button = 0. Headless-friendly (dummy SDL video driver).
"""

import os
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")  # no window needed
import pygame

DEADZONE      = 0.15
MAX_SPEED     = 200    # matches the robot's command range
TOGGLE_BUTTON = 0      # A


class Gamepad:
    def __init__(self):
        pygame.init()
        pygame.joystick.init()
        if pygame.joystick.get_count() == 0:
            raise RuntimeError("No controller found — plug the Xbox controller into the Pi (USB).")
        self.js = pygame.joystick.Joystick(0)
        self.js.init()
        self._prev_toggle = False
        print(f"Controller connected: {self.js.get_name()}")

    def _axis(self, i):
        v = self.js.get_axis(i)
        return 0.0 if abs(v) < DEADZONE else v

    def poll(self):
        """Return (left, right, toggle_pressed). Arcade drive from the left stick."""
        pygame.event.pump()
        steer    = self._axis(0)        # left stick X  (left -, right +)
        throttle = -self._axis(1)       # left stick Y  (up = forward)

        left  = max(-1.0, min(1.0, throttle + steer))
        right = max(-1.0, min(1.0, throttle - steer))
        left  = int(left  * MAX_SPEED)
        right = int(right * MAX_SPEED)

        pressed = bool(self.js.get_button(TOGGLE_BUTTON))
        toggle  = pressed and not self._prev_toggle
        self._prev_toggle = pressed
        return left, right, toggle
