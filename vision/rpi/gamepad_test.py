"""
Standalone Xbox-controller test — run on the Pi.
Prints which axes move and which buttons you press, so we can map the controller
before wiring it into the robot.

    ~/philo312/bin/python gamepad_test.py
"""

import os
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")  # no display window needed
import time
import pygame

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    raise SystemExit("No controller found. Plug the Xbox controller into the Pi (USB cable) and retry.")

js = pygame.joystick.Joystick(0)
js.init()
print(f"Controller: {js.get_name()}")
print(f"Axes: {js.get_numaxes()}   Buttons: {js.get_numbuttons()}")
print("\nMove the LEFT STICK and press buttons. Note the numbers. Ctrl+C to quit.\n")

while True:
    pygame.event.pump()
    parts = []
    for a in range(js.get_numaxes()):
        v = js.get_axis(a)
        if abs(v) > 0.25:
            parts.append(f"axis{a}={v:+.2f}")
    for b in range(js.get_numbuttons()):
        if js.get_button(b):
            parts.append(f"BUTTON {b}")
    if parts:
        print("   " + "   ".join(parts))
    time.sleep(0.1)
