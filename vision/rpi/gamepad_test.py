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
print(f"Axes: {js.get_numaxes()}   Buttons: {js.get_numbuttons()}   Hats (D-pad): {js.get_numhats()}")
print("\nMove the LEFT STICK, press every button and the D-pad. Note the numbers. Ctrl+C to quit.\n")

# Resting values: PlayStation triggers sit at -1.0 when released, so report
# changes from rest instead of anything non-zero.
pygame.event.pump()
time.sleep(0.2)
pygame.event.pump()
rest = [js.get_axis(a) for a in range(js.get_numaxes())]

while True:
    pygame.event.pump()
    parts = []
    for a in range(js.get_numaxes()):
        v = js.get_axis(a)
        if abs(v - rest[a]) > 0.25:
            parts.append(f"axis{a}={v:+.2f}")
    for b in range(js.get_numbuttons()):
        if js.get_button(b):
            parts.append(f"BUTTON {b}")
    for h in range(js.get_numhats()):
        if js.get_hat(h) != (0, 0):
            parts.append(f"hat{h}={js.get_hat(h)}")
    if parts:
        print("   " + "   ".join(parts))
    time.sleep(0.1)
